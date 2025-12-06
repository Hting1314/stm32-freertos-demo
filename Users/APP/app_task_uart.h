#ifndef APP_TASK_UART_H
#define APP_TASK_UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cmsis_os.h"

/* 心跳打印任务 */
void StartPrintTask(void *argument);

/* 命令解析任务：接收 UART 字符，拼成字符串并解析 "toggle" */
void StartCmdTask(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* APP_TASK_UART_H */
