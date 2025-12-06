#ifndef __BSP_KEY_H__
#define __BSP_KEY_H__

#include "main.h"

void Key_Init(void);             //初始化按键
void EXTI0_IRQHandler(void);     //按键外部中断处理


#endif
