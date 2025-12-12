#include "app_error.h"
#include "bsp_uart.h"     // LOG_ERROR / LOG_INFO
#include <stdio.h>
#include <string.h>

/* 模块名字符串（用于打印可读） */
static const char *AppModuleToStr(AppModule_t mod)
{
    switch (mod)
    {
        case APP_MOD_SYS:    return "SYS";
        case APP_MOD_SENSOR: return "SENSOR";
        case APP_MOD_DHT11:  return "DHT11";
        case APP_MOD_QUEUE:  return "QUEUE";
        case APP_MOD_UART:   return "UART";
        case APP_MOD_CMD:    return "CMD";
        case APP_MOD_KEY:    return "KEY";
        default:             return "UNKNOWN";
    }
}

/* 错误码字符串（用于打印可读）
 * 注意：最小版本不需要覆盖所有码，但建议把常用的写全。
 */
static const char *AppErrorToStr(AppError_t code)
{
    switch (code)
    {
        case APP_OK:                   return "OK";
        case APP_E_SENSOR_FAIL:        return "SENSOR_FAIL";

        case APP_E_DHT11_RESP1_TIMEOUT:return "DHT11_RESP1_TIMEOUT";
        case APP_E_DHT11_RESP2_TIMEOUT:return "DHT11_RESP2_TIMEOUT";
        case APP_E_DHT11_RESP3_TIMEOUT:return "DHT11_RESP3_TIMEOUT";
        case APP_E_DHT11_BIT_TIMEOUT:  return "DHT11_BIT_TIMEOUT";
        case APP_E_DHT11_CHECKSUM_ERR: return "DHT11_CHECKSUM_ERR";

        case APP_E_QUEUE_TIMEOUT:      return "QUEUE_TIMEOUT";
        case APP_E_QUEUE_FULL:         return "QUEUE_FULL";

        case APP_E_UART_RX_FAIL:       return "UART_RX_FAIL";
        case APP_E_CMD_UNKNOWN:        return "CMD_UNKNOWN";

        default:                       return "ERR_UNKNOWN";
    }
}

void APP_ErrorReport(AppModule_t mod,
                     AppError_t code,
                     int32_t detail,
                     const char *file,
                     int32_t line,
                     const char *func,
                     const char *msg_fmt, ...)
{
    /* 基础错误头：带“错误链路”定位信息（file:line func） */
    if (msg_fmt == NULL)
    {
        LOG_ERROR("[%s] code=%s(%d) detail=%ld at %s:%ld %s()\r\n",
                  AppModuleToStr(mod),
                  AppErrorToStr(code), (int)code,
                  (long)detail,
                  file, (long)line, func);
        return;
    }

    /* 有补充说明：先拼一个 msg */
    char msg[128];
    va_list args;
    va_start(args, msg_fmt);
    (void)vsnprintf(msg, sizeof(msg), msg_fmt, args);
    va_end(args);

    LOG_ERROR("[%s] code=%s(%d) detail=%ld at %s:%ld %s() | %s\r\n",
              AppModuleToStr(mod),
              AppErrorToStr(code), (int)code,
              (long)detail,
              file, (long)line, func,
              msg);
}
