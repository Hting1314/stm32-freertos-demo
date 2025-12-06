/*
 * app_task_key.h
 *
 * Description: 按键/模式控制任务的对外接口
 */
 
#ifndef __APP_TASK_KEY_H__
#define __APP_TASK_KEY_H__

#ifdef __cplusplus
extern "C" {
#endif
	
#include "cmsis_os.h"

/**
 * @brief  FreeRTOS 任务入口函数
 * @param  argument: 任务参数 (FreeRTOS 标准接口)
 */
void StartTask_Key(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* APP_TASK_KEY_H */
