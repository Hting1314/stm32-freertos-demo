#include "app_main.h"

/* 全局句柄定义 */
osMessageQueueId_t queueCmdHandle;
osMessageQueueId_t queueHeartbeatHandle;
osMessageQueueId_t queueKeyHandle;

extern UART_HandleTypeDef huart1;

/* 串口重定向/打印 */
void uart_printf(const char *fmt, ...)
{
    char buf[128];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (len > 0) {
        HAL_UART_Transmit(&huart1, (uint8_t *)buf, len, 100);
    }
}

/* 初始化队列 (需要在 main.c 或 freertos.c 的 Init 中调用) */
void App_Queue_Init(void)
{
    queueCmdHandle = osMessageQueueNew(5, sizeof(CmdType), NULL);
    queueHeartbeatHandle = osMessageQueueNew(5, sizeof(uint32_t), NULL); // 原 queueHandle
    queueKeyHandle = osMessageQueueNew(5, sizeof(uint8_t), NULL);        // 新增
}

/* -------------------------------------------------------------------------
   Task 1: LED 控制任务 (消费者：处理串口命令 & 心跳)
   ------------------------------------------------------------------------- */
void StartLedTask(void *argument)
{
    uint32_t recvHeartbeat = 0;
    CmdType cmd;
    uint8_t ledEnabled = 1;

    for(;;)
    {
        // 1. 检查串口命令 (CMD)
        if (osMessageQueueGet(queueCmdHandle, &cmd, NULL, 0) == osOK)
        {
            if (cmd == CMD_TOGGLE)
            {
                ledEnabled = !ledEnabled;
                uart_printf("[LED] Mode: %s\r\n", ledEnabled ? "ON" : "OFF");
            }
        }

        // 2. 检查心跳 (Heartbeat) - 阻塞 100ms 作为节奏
        // 注意：这里使用的是 queueHeartbeatHandle，而不是原来的 queueHandle
        if (osMessageQueueGet(queueHeartbeatHandle, &recvHeartbeat, NULL, pdMS_TO_TICKS(100)) == osOK)
        {
            if (ledEnabled)
            {
                // HAL_GPIO_TogglePin(GPIOG, GPIO_PIN_13); 
                uart_printf("[LED] Heartbeat val: %lu\r\n", recvHeartbeat);
            }
        }
    }
}

/* -------------------------------------------------------------------------
   Task 2: 打印与心跳发生器 (生产者：产生心跳数据)
   ------------------------------------------------------------------------- */
void StartPrintTask(void *argument)
{
    uint32_t heartbeat = 0;
    
    for(;;)
    {
        // 发送心跳到 queueHeartbeatHandle
        osMessageQueuePut(queueHeartbeatHandle, &heartbeat, 0, 0);
        
        heartbeat++;
        osDelay(1000);
    }
}

/* -------------------------------------------------------------------------
   Task 3: 串口命令接收任务 (生产者：产生 CMD)
   ------------------------------------------------------------------------- */
void StartCmdTask(void *argument)
{
    uint8_t ch;
    char buf[16] = {0};
    uint8_t idx = 0;

    uart_printf("[CMD] Ready. Send 'toggle' to switch LED mode.\r\n");

    for(;;)
    {
        // 简单阻塞读取，实际项目中建议使用中断或 DMA + 信号量通知
        if (HAL_UART_Receive(&huart1, &ch, 1, HAL_MAX_DELAY) == HAL_OK)
        {
            if (ch == '\r' || ch == '\n')
            {
                if (idx > 0)
                {
                    buf[idx] = '\0';
                    if (strcmp(buf, "toggle") == 0)
                    {
                        CmdType cmd = CMD_TOGGLE;
                        osMessageQueuePut(queueCmdHandle, &cmd, 0, 0);
                    }
                    else
                    {
                        uart_printf("[CMD] Unknown: %s\r\n", buf);
                    }
                    idx = 0;
                    memset(buf, 0, sizeof(buf));
                }
            }
            else
            {
                if (idx < sizeof(buf) - 1) buf[idx++] = ch;
                else { idx = 0; } // 防止溢出
            }
        }
    }
}

/* -------------------------------------------------------------------------
   Task 4: 温湿度传感器任务
   ------------------------------------------------------------------------- */
void StartSensorTask(void *argument)
{
    uint8_t humi = 0, temp = 0;
    
    // 初始化 DWT 计数器，这对 DHT11 驱动至关重要
    DWT_Delay_Init();
    
    osDelay(2000); // 上电等待

    for(;;)
    {
        // 调用我们修复后的 DHT11_Read (包含临界区保护)
        if(DHT11_Read(&humi, &temp) == HAL_OK)
        {
            uart_printf("[DHT11] T: %d C, H: %d %%\r\n", temp, humi);
        }
        else
        {
            uart_printf("[DHT11] Read Failed\r\n");
        }
        
        osDelay(2000);
    }
}

/* -------------------------------------------------------------------------
   Task 5: 按键处理任务 (消费者：处理按键事件)
   ------------------------------------------------------------------------- */
void StartTask_Key(void *argument)
{
    uint8_t keyMode = 0;

    for (;;)
    {
        // 从独立的 queueKeyHandle 接收数据
        // 注意：你需要有一个外部中断(ISR)或者另外一个扫描任务向 queueKeyHandle 发送数据
        if (osMessageQueueGet(queueKeyHandle, &keyMode, NULL, portMAX_DELAY) == osOK)
        {
            uart_printf("[KEY] Mode Recv: %d\r\n", keyMode);

            if (keyMode == 1)
            {
                // HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);  
            }
            else
            {
                // HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);  
            }
        }
    }
}