#include "app_task_uart.h"
#include "app_types.h"     // CmdType / CMD_TOGGLE
#include "cmsis_os.h"
#include "bsp_uart.h"      // uart_printf()
#include "usart.h"         // huart1
#include <string.h>
#include <stdio.h>
#include "app_error.h"

/* ============================================================ */
/* 1. 私有变量定义区 (放在文件顶部)                             */
/* ============================================================ */

/* 这是一个“乒乓球”，中断接住了就扔进队列，然后准备接下一个 */
static uint8_t rx_byte_buffer;


/* 队列由 freertos.c 创建，这里仅声明 */
extern osMessageQueueId_t queueHeartbeatHandle;     // 心跳队列（uint32_t）
extern osMessageQueueId_t queueCmdHandle;  // 命令队列（CmdType）
extern osMutexId_t uartMutexHandle;  // 声明互斥锁
extern osMessageQueueId_t queueUartByteHandle;


/* ============================================================ */
/* 2. 中断回调函数 (HAL 库会自动调用这里)                       */
/* ============================================================ */

/* * 放在这里最安全！CubeMX 不会碰这个文件。
 * 当 UART1 收到数据产生中断时，HAL 库会查找有没有这个函数，有就执行。
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    /* 过滤：只处理 USART1 的数据 */
    if (huart->Instance == USART1)
    {
        /* A. 把收到的字节塞入队列 
         * timeout = 0: 中断里不能等待，塞不进去就丢弃（通常队列够大不会丢）
         */
        osMessageQueuePut(queueUartByteHandle, &rx_byte_buffer, 0, 0);

        /* B. 【关键】重新开启中断，准备接收下一个字节 */
        HAL_UART_Receive_IT(&huart1, &rx_byte_buffer, 1);
    }
}


/* 心跳 / 日志生产者任务 */
void StartPrintTask(void *argument)
{
	
    uint32_t heartbeat = 0;
    (void)argument;	
    for (;;)
    {
        LOG_INFO("[PRINT] heartbeat=%lu\r\n", heartbeat);

        /* 向队列发送数据（供 LED 任务消费） */
        osMessageQueuePut(queueHeartbeatHandle, &heartbeat, 0, 0);

        heartbeat++;

        vTaskDelay(pdMS_TO_TICKS(1000));   // 延时 1s
    }
}


/* ============================================================ */
/* 3. 任务函数 (StartCmdTask)                                  */
/* ============================================================ */

void StartCmdTask(void *argument)
{
	
    (void)argument;
    uint8_t ch;	
    char    buf[16];
    uint8_t idx = 0;

		/* 1. 初始化 */
    memset(buf, 0, sizeof(buf));
		
		/* 清除可能存在的旧错误标志 */
		__HAL_UART_CLEAR_OREFLAG(&huart1);
		
		/* ------------------------------------------------------- */
    /* A. 启动中断接收 (推倒第一块多米诺骨牌)                    */
    /* ------------------------------------------------------- */
    /* 任务一开始运行，就告诉硬件：“准备好接收，放到 rx_byte_buffer” */
	
    HAL_UART_Receive_IT(&huart1, &rx_byte_buffer, 1);
    
    LOG_INFO("[CMD] Interrupt Mode Ready.\r\n");

    for (;;)
    {
        /* 后续可以考虑改成中断 + RingBuffer 更工程化。
			
					 --- 修改点 1：把 HAL_MAX_DELAY 改为 0 ---
         * 0 表示：尝试读取 1 字节，如果寄存器里有数据就读走返回 HAL_OK；
         * 如果没数据，立刻返回 HAL_TIMEOUT，绝对不等待。
			
			     --- 修改点 2：把 HAL_MAX_DELAY 改为 2 ---
			   * 2 表示： 2ms 超时，0ms超时太快了；
			   *  
         */
			
				/* --------------------------------------------------- */
        /* B. 从队列拿数据 (阻塞等待，不占 CPU)——已经改为中断   */
        /* --------------------------------------------------- */
			
        if (osMessageQueueGet(queueUartByteHandle, &ch, NULL, osWaitForever) == osOK)
        {
						/* C. 拼凑字符串逻辑  */
            if (ch == '\r' || ch == '\n')
            {
								/* 收到回车换行，说明一句话结束了 */
                if (idx > 0)
                {
                    buf[idx] = '\0';  // 结束字符串
									
									/* 此时再打印收到的完整字符串，调试查看 */
                    LOG_INFO("[CMD] Recv Frame: [%s]\r\n", buf);

                    if (strcmp(buf, "toggle") == 0)
                    {
                        CmdType cmd = CMD_TOGGLE;
												/* 发送给 LED 任务 */
                        osMessageQueuePut(queueCmdHandle, &cmd, 0, 0);
												LOG_INFO("[CMD] Action: LED TOGGLE.\r\n");
                    }
//                    else
//                    {
//                        uart_printf("[CMD] Unknown Cmd.\r\n");
//                    }

                    /* 重置缓冲区 */
                    idx = 0;
                    memset(buf, 0, sizeof(buf));
                }
            }
            else
            {
								/* 正常字符，存入 buffer */
                if (idx < sizeof(buf) - 1)
                {
                    buf[idx++] = (char)ch;
                }
                else
                {
                    /* 溢出保护：简单粗暴地重置 */
                    idx = 0;
                    memset(buf, 0, sizeof(buf));
//                    LOG_ERROR("[CMD] Buffer Overflow, Reset.\r\n");
										APP_ERR(APP_MOD_QUEUE, APP_E_QUEUE_FULL, 0, "queueCmd put failed");
                }
            }
        }
//				else
//				{
//						
//						/* 4. 没收到数据时的处理 */
//            
//            /* 必加：检查并清除 ORE 错误，防止串口死锁 */
//            if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_ORE))
//            {
//                __HAL_UART_CLEAR_OREFLAG(&huart1);
//            }
//						
//					/* === 修改点 2：没收到数据时，让出 CPU ===
//             * 这里非常关键！
//             * osDelay(10) 会让当前任务进入“阻塞”状态 10ms。
//             * 在这 10ms 里，FreeRTOS 调度器会把 CPU 交给 SensorTask 或 LedTask。
//             * 10ms 的轮询间隔对于人类输入来说是感觉不到延迟的。
//             */
//					osDelay(10);
//				}
    }
}
