#ifndef __DHT11_H__
#define __DHT11_H__

#include "main.h"

#define DHT11_PORT GPIOB
#define DHT11_PIN  GPIO_PIN_11

void DHT11_Init(void);
void DWT_Delay_Init(void);
void DWT_Delay_us(uint32_t us);
HAL_StatusTypeDef DHT11_ReadRaw(uint8_t data[5]);

#endif
