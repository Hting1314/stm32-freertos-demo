#include "bsp_uart.h"
#include "usart.h"      // huart1 在这里声明
#include "cmsis_os.h"   // <--- 新增：这里包含了 osMutexId_t 类型定义
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* * 由于没有统一的 main_app.h，我们直接在这里使用 extern 声明。
 * 这告诉编译器：uartMutexHandle 变量定义在其他文件（ 在freertos.c）中。
 */
extern osMutexId_t uartMutexHandle;

/* 可以根据需要调整缓冲区大小 */
#define BSP_UART_PRINTF_BUF_SIZE   128

/* 本地缓冲区
 * 注意：在多任务环境下，该缓冲区是“共享资源”。
 * 必须通过互斥锁保护，防止 Task A 正在写的时候 Task B 插进来覆盖数据。
 */
static char uart_printf_buf[BSP_UART_PRINTF_BUF_SIZE];


/* 内部：打印时间戳 [hh:mm:ss.mmm]，不加锁，由上层 LOG_* 包裹锁 */
static void BSP_UART_PrintTimestamp_NoLock(void)
{
	
	char ts[20];
	
	uint32_t ms          = HAL_GetTick();
	uint32_t seconds     = ms / 1000U;
	uint32_t millis      = ms % 1000U;
	uint32_t minutes     = seconds / 60U;
	uint32_t hours       = minutes / 60U;
	seconds              = seconds % 60U;
	minutes              = minutes % 60U;
	
	
	/* [hh:mm:ss.mmm] 空间够用 */
	(void)snprintf(ts, sizeof(ts),
								"[%02lu:%02lu:%02lu.%03lu]",
								hours, minutes, seconds, millis);
	
	BSP_UART_Send((uint8_t *)ts,  (uint16_t)strlen(ts));
}

/* 内部：不加锁的 vprintf 实现，供 uart_printf 和 LOG_* 调用 */
static int uart_vprintf_nolock(const char *fmt, va_list args)
{
    if (fmt == NULL)
        return 0;

    int len = vsnprintf(uart_printf_buf,
                        BSP_UART_PRINTF_BUF_SIZE,
                        fmt,
                        args);

    if (len <= 0)
        return 0;

    if (len >= BSP_UART_PRINTF_BUF_SIZE)
    {
        len = BSP_UART_PRINTF_BUF_SIZE - 1;
    }

    BSP_UART_Send((uint8_t *)uart_printf_buf, (uint16_t)len);
    return len;
}


void BSP_UART_Init(void)
{
    /* * 这里的初始化保持为空即可。
     * 真正的硬件初始化由 CubeMX 生成的 MX_USART1_UART_Init() 完成。
     * 互斥锁的初始化由 App_Queue_Init() 或 main() 完成。
     */
}

/* 基础发送：阻塞式发送一段数据 */
void BSP_UART_Send(const uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0)
        return;

    /* * 使用 HAL 阻塞发送
     * 建议：不要使用 HAL_MAX_DELAY (死等)，改用 1000ms 或其他值。
     * 这样如果串口线断了或硬件坏了，任务不会永远卡死在这里。
     */
    HAL_UART_Transmit(&huart1, (uint8_t *)data, len, 1000);
}

/* 发送单字节 */
void BSP_UART_PutChar(uint8_t ch)
{
    BSP_UART_Send(&ch, 1U);
}

/**
  * @brief  线程安全的格式化串口打印函数 (升级版)
  * @note   该函数使用了互斥锁，因此不能在中断服务函数(ISR)中调用！
  * @param  fmt: 格式化字符串
  * @retval int: 发送的字符长度，-1 表示获取锁失败
  */
int uart_printf(const char *fmt, ...)
{
	if (fmt == NULL)
        return 0;

    va_list args;
    int len = 0;

    /* 上锁，防止多任务同时写串口互相打断 */
    if (uartMutexHandle != NULL)
    {
        osMutexAcquire(uartMutexHandle, osWaitForever);
    }

    va_start(args, fmt);
    len = uart_vprintf_nolock(fmt, args);
    va_end(args);

    if (uartMutexHandle != NULL)
    {
        osMutexRelease(uartMutexHandle);
    }

    return len;
//    int len = 0;
//    
//    /* 0. 基本参数检查 */
//    if (fmt == NULL)
//    {
//        return 0;
//    }

//    /* 1. 获取互斥锁 (关键升级)
//     * 范围说明：锁必须包含 vsnprintf 和 BSP_UART_Send 全过程。
//     * 原因：uart_printf_buf 是全局共享的，如果只锁 Send，
//     * 那么在格式化期间缓冲区可能被其他任务篡改。
//     */
//    if (osMutexAcquire(uartMutexHandle, osWaitForever) == osOK)
//    {
//        va_list args;

//        /* 2. 格式化字符串到全局缓冲区 */
//        va_start(args, fmt);
//        len = vsnprintf(uart_printf_buf, BSP_UART_PRINTF_BUF_SIZE, fmt, args);
//        va_end(args);

//        /* 3. 长度与截断处理逻辑 */
//        if (len > 0)
//        {
//            /* 截断保护：如果输出长度超过缓冲区，则强制截断 */
//            if (len >= BSP_UART_PRINTF_BUF_SIZE)
//            {
//                len = BSP_UART_PRINTF_BUF_SIZE - 1;
//                /* 可选：为了防止打印乱码，可以在截断处手动加结束符，
//                 * 但 vsnprintf 通常会自动处理 null 结尾。
//                 */
//            }

//            /* 4. 通过硬件发送 */
//            BSP_UART_Send((uint8_t *)uart_printf_buf, (uint16_t)len);
//        }

//        /* 5. 释放互斥锁 (必须执行！) */
//        osMutexRelease(uartMutexHandle);
//        
//        return len;
//    }
//    else
//    {
//        /* 获取锁失败 (极少发生，除非在中断里调用了本函数) */
//        return -1;
//    }
}

/* 统一 INFO 日志：带时间戳 + [INFO] 前缀 + 线程安全 */
void LOG_INFO(const char *fmt, ...)
{
    if (fmt == NULL)
        return;

    va_list args;

    if (uartMutexHandle != NULL)
    {
        osMutexAcquire(uartMutexHandle, osWaitForever);
    }

    /* 时间戳 */
    BSP_UART_PrintTimestamp_NoLock();
    /* 前缀 */
    static const char prefix[] = "[INFO] ";
    BSP_UART_Send((uint8_t *)prefix, (uint16_t)strlen(prefix));

    /* 正文 */
    va_start(args, fmt);
    (void)uart_vprintf_nolock(fmt, args);
    va_end(args);

    if (uartMutexHandle != NULL)
    {
        osMutexRelease(uartMutexHandle);
    }
}

/* 统一 ERROR 日志：带时间戳 + [ERROR] 前缀 + 线程安全 */
void LOG_ERROR(const char *fmt, ...)
{
    if (fmt == NULL)
        return;

    va_list args;

    if (uartMutexHandle != NULL)
    {
        osMutexAcquire(uartMutexHandle, osWaitForever);
    }

    /* 时间戳 */
    BSP_UART_PrintTimestamp_NoLock();
    /* 前缀 */
    static const char prefix[] = "[ERROR] ";
    BSP_UART_Send((uint8_t *)prefix, (uint16_t)strlen(prefix));

    /* 正文 */
    va_start(args, fmt);
    (void)uart_vprintf_nolock(fmt, args);
    va_end(args);

    if (uartMutexHandle != NULL)
    {
        osMutexRelease(uartMutexHandle);
    }
}

