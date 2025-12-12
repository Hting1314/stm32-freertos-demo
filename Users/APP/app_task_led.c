#include "app_task_led.h"
#include "app_types.h"     // 定义 CmdType / CMD_TOGGLE
#include "cmsis_os.h"
#include "bsp_uart.h"      // uart_printf()
#include "bsp_led.h"
#include "main.h"          // HAL_GPIO_* / GPIOG 定义，暂时还没完全 BSP 化

/* 由 freertos.c 中创建，这里只声明 */
extern osMessageQueueId_t queueHeartbeatHandle;     // 心跳队列（uint32_t）
extern osMessageQueueId_t queueCmdHandle;  // 命令队列（CmdType）

void StartLedTask(void *argument)
{
    uint32_t recvValue = 0;
    CmdType  cmd;
    uint8_t  ledEnabled = 1;   // 1: 正常闪烁, 0: 关闭闪烁

    (void)argument;

    for (;;)
    {
        /* 1) 先非阻塞检查是否有命令到达（用于切换 LED 模式） */
        if (osMessageQueueGet(queueCmdHandle, &cmd, NULL, 0) == osOK)
        {
            if (cmd == CMD_TOGGLE)
            {
                ledEnabled = !ledEnabled;

                LOG_INFO("[LED] mode changed: %s\r\n",
                            ledEnabled ? "ON" : "OFF");

                if (!ledEnabled)
                {
                    /* 关闭模式时，确保灯灭（PG13 置高或置低视你的硬件而定） */
                    BSP_LED_Run_Off();
                }
            }
        }

        /* 2) 再从心跳队列里拿数据（带 100ms 超时），用来驱动闪烁节奏 */
        if (osMessageQueueGet(queueHeartbeatHandle,
                              &recvValue,
                              NULL,
                              pdMS_TO_TICKS(100)) == osOK)
        {
            if (ledEnabled)
            {
                BSP_LED_Run_Toggle();
                LOG_INFO("[LED] heartbeat=%lu (toggled)\r\n", recvValue);
            }
            else
            {
                LOG_INFO("[LED] heartbeat=%lu (ignored, mode OFF)\r\n",
                            recvValue);
            }
        }

        /* 如果 100ms 内没有收到心跳消息，也没关系，下一轮继续循环 */
    }
}
