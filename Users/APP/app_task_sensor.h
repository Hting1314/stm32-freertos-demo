#ifndef APP_TASK_SENSOR_H
#define APP_TASK_SENSOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cmsis_os.h"

/* DHT11 传感器任务入口 */
void StartSensorTask(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* APP_TASK_SENSOR_H */
