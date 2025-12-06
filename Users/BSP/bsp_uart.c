#include "bsp_uart.h"
#include "usart.h"      // huart1 在这里声明
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* 可以根据需要调整缓冲区大小 */
#define BSP_UART_PRINTF_BUF_SIZE   128

/* 简单的本地缓冲区（非线程安全版本）
 * 后面如果想做线程安全，可以引入 mutex。
 */
static char uart_printf_buf[BSP_UART_PRINTF_BUF_SIZE];

void BSP_UART_Init(void)
{
    /* 这里默认假设：
     * - MX_USART1_UART_Init() 已经被 main.c 调用过
     * - 波特率等在 CubeMX 里配好即可
     *
     * 如果有需要，可以在这里加：
     *   - setvbuf(stdout, NULL, _IONBF, 0);
     *   - 或者和 libc printf 绑定等
     *
     * 目前先留空，实现上是一个“语义上的初始化点”，
     * 便于 APP 在 main.c 里按顺序统一调用 BSP 初始化。
     */
}

/* 基础发送：阻塞式发送一段数据 */
void BSP_UART_Send(const uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0)
        return;

    /* 这里使用 HAL 阻塞发送，timeout 设置为 HAL_MAX_DELAY。
     * 对当前这个“小系统 + 短 log” 来说足够了。
     */
    HAL_UART_Transmit(&huart1, (uint8_t *)data, len, HAL_MAX_DELAY);
}

/* 发送单字节 */
void BSP_UART_PutChar(uint8_t ch)
{
    BSP_UART_Send(&ch, 1);
}

/* printf 风格输出：格式化到本地缓冲，然后通过 UART 发送。
 * 注意：
 * - 这是一个“简单版”实现，不带互斥保护。
 * - 多任务并发调用时，字符串可能交织。后面可以再引入 mutex 来升级。
 */
int uart_printf(const char *fmt, ...)
{
    if (fmt == NULL)
        return 0;

    va_list args;
    int     len;

    va_start(args, fmt);
    len = vsnprintf(uart_printf_buf, BSP_UART_PRINTF_BUF_SIZE, fmt, args);
    va_end(args);

    if (len <= 0)
        return 0;

    /* 截断保护：如果输出长度超过缓冲区，则只发前 N-1 个字符 */
    if (len >= BSP_UART_PRINTF_BUF_SIZE)
    {
        len = BSP_UART_PRINTF_BUF_SIZE - 1;
    }

    BSP_UART_Send((uint8_t *)uart_printf_buf, (uint16_t)len);

    return len;
}
