/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */



#include "main.h"
#include "app_types.h"
#include "app_task_led.h"
#include "app_task_uart.h"
#include "app_task_sensor.h"
#include "app_task_key.h"



/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */



//任务句柄
osThreadId_t defaultTaskHandle;
osThreadId_t LedTaskHandle;
osThreadId_t PrintTaskHandle;
osThreadId_t CmdTaskHandle;
osThreadId_t SensorTaskHandle;
osThreadId_t KeyTaskHandle;



//队列句柄
osMessageQueueId_t queueHeartbeatHandle;
osMessageQueueId_t queueCmdHandle;
osMessageQueueId_t queueKeyHandle;



/* 任务属性定义 -------------------------------------------------------------*/
/* DefaultTask */
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* LedTask */
const osThreadAttr_t LedTask_attributes = {
  .name       = "LedTask",
  .stack_size = 128 * 4,
  .priority   = (osPriority_t) osPriorityNormal,
};

/* PrintTask */
const osThreadAttr_t PrintTask_attributes = {
  .name       = "PrintTask",
  .stack_size = 128 * 4,
  .priority   = (osPriority_t) osPriorityNormal,
};

/* CmdTask */
const osThreadAttr_t CmdTask_attributes = {
  .name       = "CmdTask",
  .stack_size = 256 * 4,                      // 命令解析稍微给大一点栈
  .priority   = (osPriority_t) osPriorityNormal,
};

/* SensorTask */
const osThreadAttr_t SensorTask_attributes = {
  .name       = "SensorTask",
  .stack_size = 512 * 4,
  .priority   = (osPriority_t) osPriorityBelowNormal,
};

/* KeyTask */
const osThreadAttr_t KeyTask_attributes = {
  .name       = "KeyTask",
  .stack_size = 128 * 4,
  .priority   = (osPriority_t) osPriorityNormal,
};



/* 队列属性定义 -------------------------------------------------------------*/
/* 心跳队列：uint32_t，长度 8 */
const osMessageQueueAttr_t queueHeartbeat_attributes = {
  .name = "queueHeartbeat"
};

/* 命令队列：CmdType，长度 4 */
const osMessageQueueAttr_t queueCmd_attributes = {
  .name = "queueCmd"
};

/* 按键事件队列：uint8_t，长度 8 */
const osMessageQueueAttr_t queueKey_attributes = {
  .name = "queueKey"
};



/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */



void StartLedTask(void *argument);              
void StartPrintTask(void *argument);            
void StartCmdTask(void *argument);              
void StartSensorTask(void *argument);           
void StartKeyTask(void *argument);      



/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
	
	
	
 /* 心跳队列：供 PrintTask 发送 heartbeat，LedTask 消费 */
  queueHeartbeatHandle = osMessageQueueNew(8, sizeof(uint32_t), &queueHeartbeat_attributes);

  /* 命令队列：供 CmdTask 发送 CmdType，LedTask 消费 */
  queueCmdHandle = osMessageQueueNew(4, sizeof(CmdType), &queueCmd_attributes);
	
	/*  按键事件队列：PF6 EXTI 回调 → KeyTask */
  queueKeyHandle = osMessageQueueNew(8, sizeof(uint8_t), &queueKey_attributes);
	
	
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
//  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */



 /* LED 任务：消费 queueHeartbeatHandle + queueCmdHandle */
  LedTaskHandle = osThreadNew(StartLedTask, NULL, &LedTask_attributes);

  /* Print 任务：定时发送 heartbeat 到 queueHeartbeatHandle */
  PrintTaskHandle = osThreadNew(StartPrintTask, NULL, &PrintTask_attributes);

  /* CMD 任务：阻塞读 UART，解析命令并发 CmdType 到 queueCmdHandle */
  CmdTaskHandle = osThreadNew(StartCmdTask, NULL, &CmdTask_attributes);

  /* Sensor 任务：周期性读取 DHT11，并通过串口打印 */
  SensorTaskHandle = osThreadNew(StartSensorTask, NULL, &SensorTask_attributes);

  /* Key 任务：从某个队列拿“模式变化” */
  KeyTaskHandle = osThreadNew(StartTask_Key, NULL, &KeyTask_attributes);
	

	
	
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */



/* USER CODE END Application */

