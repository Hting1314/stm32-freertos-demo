#include "app_task_key.h"
#include "cmsis_os.h"
#include "app_types.h"
#include "bsp_uart.h"



/* 由 freertos.c 创建，这里声明即可 */
extern osMessageQueueId_t queueKeyHandle;   // PF6 EXTI → key 事件
extern osMessageQueueId_t queueCmdHandle;   // 下发给 LED 的命令队列（CmdType）


/* 约定一个简单的按键事件类型，目前只有“短按” */
#define KEY_EVT_SHORT_PRESS   1u

void StartKeyTask(void *argument)
{
    (void)argument;

    uint8_t  key_evt = 0;
    uint32_t press_count = 0;

    for (;;)
    {
        /* 阻塞等待按键事件（来自 PF6 EXTI 回调） */
        if (osMessageQueueGet(queueKeyHandle,
                              &key_evt,
                              NULL,
                              osWaitForever) == osOK)
        {
            if (key_evt == KEY_EVT_SHORT_PRESS)
            {
                press_count++;
                LOG_INFO("[KEY] short press #%lu\r\n", press_count);

                /* 将“按键事件”翻译成高级命令：CMD_TOGGLE
                 * 交给 LED 任务（和 UART 命令公用一条 Cmd 队列）。
                 */
                CmdType cmd = CMD_TOGGLE;
                if (osMessageQueuePut(queueCmdHandle, &cmd, 0, 0) == osOK)
                {
                    LOG_INFO("[KEY] send CMD_TOGGLE to LED\r\n");
                }
                else
                {
                    LOG_ERROR("[KEY] failed to send CMD_TOGGLE\r\n");
                }
            }
            else
            {
                LOG_ERROR("[KEY] unknown event: %u\r\n", key_evt);
            }
        }
    }
}
