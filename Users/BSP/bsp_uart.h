#ifndef BSP_UART_H
#define BSP_UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>

/* 若之后想支持多串口，可以在这里加枚举，现在先只管 USART1 */
typedef enum
{
    BSP_UART_PORT_1 = 0,
} BSP_UartPort_t;



/* 初始化 BSP UART
 * - 默认假定 MX_USART1_UART_Init() 已经在 main.c 中调用过
 * - 这里主要做一些缓冲区 / printf 相关的小设置（如果需要）
 */

void BSP_UART_Init(void);

/* 发送一串原始数据（阻塞式） */
void BSP_UART_Send(const uint8_t *data, uint16_t len);

/* 发送一个字节（阻塞式） */
void BSP_UART_PutChar(uint8_t ch);

/* printf 风格输出，供 APP 层直接使用 */
int uart_printf(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* BSP_UART_H */
