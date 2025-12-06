#ifndef APP_TASK_LED_H
#define APP_TASK_LED_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cmsis_os.h"

/* LED 任务入口函数 */
void StartLedTask(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* APP_TASK_LED_H */
