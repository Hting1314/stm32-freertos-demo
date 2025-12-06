#ifndef __MAIN_APP_H
#define __MAIN_APP_H

#include "main.h"
#include "cmsis_os.h"
#include "bsp_dht11.h"  // APP 层包含 BSP 层的头文件，这样任务里就能调用 DHT11

// 1. 队列句柄声明
extern osMessageQueueId_t queueCmdHandle;
extern osMessageQueueId_t queueHeartbeatHandle;
extern osMessageQueueId_t queueKeyHandle;

// 2. 命令定义
typedef enum {
CMD_NONE = 0, 
CMD_TOGGLE = 1 
} CmdType;

// 3. 公共函数
void uart_printf(const char *fmt, ...);
void App_Queue_Init(void); // 队列初始化函数声明

#endif