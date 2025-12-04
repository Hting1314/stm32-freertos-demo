#include "bsp_uart.h"
#include "usart.h"     // 里边有 extern UART_HandleTypeDef huart1
#include <stdarg.h>
#include <string.h>

void uart_printf(const char *fmt, ...)
{
    char buf[128];          // 简单起见，长度 128 字节
    va_list args;
    int len;

    va_start(args, fmt);
    len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (len < 0) {
        return; // 格式化出错，直接返回
    }

    if (len > sizeof(buf)) {
        len = sizeof(buf); // 超长就截断
    }

    HAL_UART_Transmit(&huart1, (uint8_t *)buf, len, 100);
}
