#include "bsp_key.h"
#include "cmsis_os.h"

extern osMessageQueueId_t queueKeyHandle;   //由APP层创建的队列，传递按键事件


/*	尝试一下不初始化，在gpio中应该已经初始化过了	*/

void EXTI0_IRQHandler(void)
{
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_6);   //处理PF6的中断
	
	// 检查按键触发的外部中断
	static uint8_t mode = 0;
	mode = !mode;
	
	//向队列发送模式切换信号
	osMessageQueuePut(queueKeyHandle, &mode, 0, 0);
}
