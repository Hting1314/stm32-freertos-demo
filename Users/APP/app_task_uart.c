#include "app_task_uart.h"
#include "app_types.h"     // CmdType / CMD_TOGGLE
#include "cmsis_os.h"
#include "bsp_uart.h"      // uart_printf()
#include "usart.h"         // huart1
#include <string.h>

/* 队列由 freertos.c 创建，这里仅声明 */
extern osMessageQueueId_t queueHeartbeatHandle;     // 心跳队列（uint32_t）
extern osMessageQueueId_t queueCmdHandle;  // 命令队列（CmdType）


/* 心跳 / 日志生产者任务 */
void StartPrintTask(void *argument)
{
    uint32_t heartbeat = 0;
    (void)argument;

    for (;;)
    {
        uart_printf("[PRINT] heartbeat=%lu\r\n", heartbeat);

        /* 向队列发送数据（供 LED 任务消费） */
        osMessageQueuePut(queueHeartbeatHandle, &heartbeat, 0, 0);

        heartbeat++;

        vTaskDelay(pdMS_TO_TICKS(1000));   // 延时 1s
    }
}


/* 命令解析任务：阻塞式读 UART，解析 "toggle" 命令 */
void StartCmdTask(void *argument)
{
    (void)argument;

    uint8_t ch;
    char    buf[16];
    uint8_t idx = 0;

    memset(buf, 0, sizeof(buf));

    uart_printf("[CMD] ready. Type 'toggle' + Enter.\r\n");

    for (;;)
    {
        /* 阻塞接收 1 字节。
         * 注意：这是 HAL 阻塞式 API，在 RTOS 里虽然能用，
         * 但后续可以考虑改成中断 + RingBuffer 更工程化。
         */
        if (HAL_UART_Receive(&huart1, &ch, 1, HAL_MAX_DELAY) == HAL_OK)
        {
            if (ch == '\r' || ch == '\n')
            {
                if (idx > 0)
                {
                    buf[idx] = '\0';  // 结束字符串

                    uart_printf("[CMD] recv: %s\r\n", buf);

                    if (strcmp(buf, "toggle") == 0)
                    {
                        CmdType cmd = CMD_TOGGLE;
                        osMessageQueuePut(queueCmdHandle, &cmd, 0, 0);
                        uart_printf("[CMD] send CMD_TOGGLE\r\n");
                    }
                    else
                    {
                        uart_printf("[CMD] unknown cmd\r\n");
                    }

                    /* 重置缓冲区 */
                    idx = 0;
                    memset(buf, 0, sizeof(buf));
                }
            }
            else
            {
                if (idx < sizeof(buf) - 1)
                {
                    buf[idx++] = (char)ch;
                }
                else
                {
                    /* 溢出保护：简单粗暴地重置 */
                    idx = 0;
                    memset(buf, 0, sizeof(buf));
                    uart_printf("[CMD] buffer overflow, reset.\r\n");
                }
            }
        }
    }
}
