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
#include "main.h"
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

/* ================== 1. 任务句柄 ================== */
osThreadId_t LedTaskHandle;
osThreadId_t PrintTaskHandle;
osThreadId_t CmdTaskHandle;
osThreadId_t KeyTaskHandle;
osThreadId_t SensorTaskHandle;

/* ================== 2. 任务属性 (Stack & Priority) ================== */

/* LED 任务：包含 printf，栈设为 512 */
const osThreadAttr_t LedTask_attributes = {
  .name = "LedTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* 打印心跳任务：如果不怎么用 printf，256 也够，稳妥起见给 256 */
const osThreadAttr_t PrintTask_attributes = {
  .name = "PrintTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* 命令任务：包含 printf 且处理字符串，栈设为 512 */
const osThreadAttr_t CmdTask_attributes = {
  .name = "CmdTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal, 
};

/* 传感器任务：【高优先级】保护时序，且包含 printf，栈设为 512 */
const osThreadAttr_t SensorTask_attributes = {
  .name = "SensorTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal, // <--- 关键！高于 CmdTask
};

/* 按键任务 */
const osThreadAttr_t KeyTask_attributes = {
  .name = "KeyTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};


/* ================== 3.队列句柄与属性定义 ================== */

/* 1. 命令队列 (Cmd) */
osMessageQueueId_t queueCmdHandle;
const osMessageQueueAttr_t queueCmd_attributes = {
  .name = "CmdQueue"
};

/* 2. 心跳队列 (Heartbeat) */
osMessageQueueId_t queueHeartbeatHandle;
const osMessageQueueAttr_t queueHeartbeat_attributes = {
  .name = "HeartbeatQueue"
};

/* 3. 按键队列 (Key) */
osMessageQueueId_t queueKeyHandle;
const osMessageQueueAttr_t queueKey_attributes = {
  .name = "KeyQueue"
};

/* 4. 串口字节流队列 (UART Byte) */
/* 这个队列通常不需要太复杂的属性，但为了统一风格也加上 */
osMessageQueueId_t queueUartByteHandle;
const osMessageQueueAttr_t queueUartByte_attributes = {
  .name = "UartByteQueue"
};


/* ================== 4. 互斥锁句柄 ================== */
osMutexId_t uartMutexHandle;

/* 定义互斥锁属性 */
const osMutexAttr_t uartMutex_attributes = {
	.name = "UartMutex"
};

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
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

	
	/* 1. 创建互斥锁 (最先创建) */
  uartMutexHandle = osMutexNew(&uartMutex_attributes);

  /* 2. 创建队列 (带属性版本) */

  /* Cmd 队列: 深度 5 (够存几个并发命令), 元素类型 CmdType */
  queueCmdHandle = osMessageQueueNew(5, sizeof(CmdType), &queueCmd_attributes);

  /* Heartbeat 队列: 深度 8 (您指定的), 元素类型 uint32_t */
  queueHeartbeatHandle = osMessageQueueNew(8, sizeof(uint32_t), &queueHeartbeat_attributes);

  /* Key 队列: 深度 5, 元素类型 uint8_t */
  queueKeyHandle = osMessageQueueNew(5, sizeof(uint8_t), &queueKey_attributes);

  /* UART Byte 队列: 深度 128 (作为缓冲池，必须大), 元素类型 uint8_t */
  queueUartByteHandle = osMessageQueueNew(128, sizeof(uint8_t), &queueUartByte_attributes);
	
	
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
		
		
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

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

