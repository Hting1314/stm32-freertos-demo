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
#include "usart.h"
#include <string.h>
#include "bsp_uart.h"
#include "dht11.h"
#include <stdio.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum
{
	CMD_NONE = 0,
	CMD_TOGGLE = 1,
} CmdType;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
osThreadId_t ledTaskHandle;											//创建LED任务
const osThreadAttr_t ledTask_attributes = {
	.name = "ledTask",
	.priority = (osPriority_t) osPriorityNormal,
	.stack_size = 128*4
};

osThreadId_t printTaskHandle;                 // 打印任务
const osThreadAttr_t printTask_attributes = {
	.name = "printTask",
	.priority = (osPriority_t) osPriorityNormal,
	.stack_size = 128 * 4
};

osThreadId_t cmdTaskHandle;                     // 命令任务
const osThreadAttr_t cmdTask_attributes = {
	.name = "cmdTask",
	.priority = (osPriority_t) osPriorityNormal,
	.stack_size = 256 * 4
};

osThreadId_t sensorTaskHandle;                    // 传感器任务
const osThreadAttr_t sensorTask_attributes = {
  .name = "sensorTask",
	.priority = (osPriority_t) osPriorityNormal,
	.stack_size = 256 * 4
};

/*	新增队列句柄		*/
osMessageQueueId_t queueHandle;							 //心跳队列			
osMessageQueueId_t queueCmdHandle;           //命令队列

/* USER CODE END Variables */
/* Definitions for defaultTask */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void StartLedTask(void *argument);              //声明LED任务
void StartPrintTask(void *argument);            //声明print任务
void StartCmdTask(void *argument);              //命令任务原型
void StartSensorTask(void *argument);            //传感器任务

/* USER CODE END FunctionPrototypes */

//void StartDefaultTask(void *argument);

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
	queueHandle = osMessageQueueNew(5, sizeof(uint32_t), NULL);
	queueCmdHandle = osMessageQueueNew(5, sizeof(CmdType), NULL);
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
//  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
	ledTaskHandle = osThreadNew(StartLedTask, NULL, &ledTask_attributes);
	printTaskHandle = osThreadNew(StartPrintTask, NULL, &printTask_attributes);
	cmdTaskHandle = osThreadNew(StartCmdTask, NULL, &cmdTask_attributes);
	sensorTaskHandle = osThreadNew(StartSensorTask, NULL, &sensorTask_attributes);
	
	
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


/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void StartLedTask(void *argument)
{
    uint32_t recvValue = 0;
    CmdType cmd;
    uint8_t ledEnabled = 1;   // 1: 正常闪烁, 0: 关闭闪烁

    for(;;)
    {
        /* 1) 先非阻塞检查是否有命令到达 */
        if (osMessageQueueGet(queueCmdHandle, &cmd, NULL, 0) == osOK)
        {
            if (cmd == CMD_TOGGLE)
            {
                ledEnabled = !ledEnabled;
                uart_printf("[LED] mode changed: %s\r\n",
                            ledEnabled ? "ON" : "OFF");

                if (!ledEnabled)
                {
                    /* 关闭模式时，确保灯灭 */
                    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, GPIO_PIN_SET);
                }
            }
        }

        /* 2) 再从心跳队列里拿数据（带超时），用来驱动闪烁节奏 */
        if (osMessageQueueGet(queueHandle, &recvValue, NULL,
                              pdMS_TO_TICKS(100)) == osOK)
        {
            if (ledEnabled)
            {
                HAL_GPIO_TogglePin(GPIOG, GPIO_PIN_13);
                uart_printf("[LED] heartbeat=%lu (toggled)\r\n", recvValue);
            }
            else
            {
                uart_printf("[LED] heartbeat=%lu (ignored, mode OFF)\r\n",
                            recvValue);
            }
        }

        /* 如果 100ms 内没有收到心跳消息，也没关系，下一轮继续循环 */
    }
}

void StartPrintTask(void *argument)
{
	uint32_t heartbeat = 0;
	
	for(;;)
	{
		uart_printf("[PRINT] heartbeat=%lu\r\n",heartbeat);
		
		/* ---向队列发送数据--- */
		osMessageQueuePut(queueHandle, &heartbeat, 0 ,0 );
		
		heartbeat ++;
		
		vTaskDelay(pdMS_TO_TICKS(1000));        		//延时1s
	}
}

void StartCmdTask(void *argument)
{
    uint8_t ch;
    char buf[16];
    uint8_t idx = 0;

    memset(buf, 0, sizeof(buf));

    uart_printf("[CMD] ready. Type 'toggle' + Enter.\r\n");

    for(;;)
    {
        /* 阻塞读取 1 字节：HAL 会用到 SysTick/HAL Tick，但在 RTOS 下是可行的 */
        if (HAL_UART_Receive(&huart1, &ch, 1, HAL_MAX_DELAY) == HAL_OK)
        {
            if (ch == '\r' || ch == '\n')
            {
                if (idx > 0)
                {
                    buf[idx] = '\0';   // 结束字符串

                    uart_printf("[CMD] recv: %s\r\n", buf);

                    if (strcmp(buf, "toggle") == 0)
                    {
                        CmdType cmd = CMD_TOGGLE;
                        osMessageQueuePut(queueCmdHandle, &cmd, 0, 0);
                        uart_printf("[CMD] send CMD_TOGGLE\r\n");
                    }
                    else
                    {
                        uart_printf("[CMD] unknown cmd\r\n");
                    }

                    /* 重置缓冲 */
                    idx = 0;
                    memset(buf, 0, sizeof(buf));
                }
            }
            else
            {
                if (idx < sizeof(buf) - 1)
                {
                    buf[idx++] = (char)ch;
                }
                else
                {
                    /* 溢出保护：简单粗暴重置 */
                    idx = 0;
                    memset(buf, 0, sizeof(buf));
                    uart_printf("[CMD] buffer overflow, reset.\r\n");
                }
            }
        }
    }
}

void StartSensorTask(void *argument)
{
	(void)argument;
	uint8_t humi = 0;
	uint8_t temp = 0;
	HAL_StatusTypeDef res;
	
	osDelay(2000);   //让系统和DHT11稳定
	
	for(;;)
	{
		res = DHT11_Read(&humi, &temp);
		
		if(res == HAL_OK)
		{
			uart_printf("DHT11 OK:T = %d C, H = %d %%\r\n", temp, humi);
		}
		else
		{
			uart_printf("DHT11 ERROR:read failed\r\n");
		}
		
		/* 遵守 DHT11 最小采样周期（>=1s），这里我们用 2s */
		osDelay(2000);
	}
}


/* USER CODE END Application */

