#ifndef __DHT11_H__
#define __DHT11_H__

#include "main.h"
#include "bsp_uart.h"

#define DHT11_PORT GPIOB
#define DHT11_PIN  GPIO_PIN_11

/* 对外接口：初始化 + 读数据（只给整数部分） */
void DHT11_Init(void);

/* 高层封装：直接返回整数温湿度，成功返回 HAL_OK，失败 HAL_ERROR */
HAL_StatusTypeDef DHT11_Read(uint8_t *humidity, uint8_t *temperature);

#endif
