#ifndef APP_ERROR_H
#define APP_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdarg.h>

/* ====== 错误模块来源（谁报的错）====== */
typedef enum
{
    APP_MOD_SYS    = 0,
    APP_MOD_SENSOR = 1,
    APP_MOD_DHT11  = 2,
    APP_MOD_QUEUE  = 3,
    APP_MOD_UART   = 4,
    APP_MOD_CMD    = 5,
    APP_MOD_KEY    = 6,
} AppModule_t;

/* ====== 统一错误码（错在哪）====== */
typedef enum
{
    APP_OK = 0,

    /* --- Sensor / DHT11 --- */
    APP_E_SENSOR_FAIL          = 1000,
    APP_E_DHT11_RESP1_TIMEOUT  = 1101,
    APP_E_DHT11_RESP2_TIMEOUT  = 1102,
    APP_E_DHT11_RESP3_TIMEOUT  = 1103,
    APP_E_DHT11_BIT_TIMEOUT    = 1110,
    APP_E_DHT11_CHECKSUM_ERR   = 1120,

    /* --- Queue --- */
    APP_E_QUEUE_TIMEOUT        = 2001,
    APP_E_QUEUE_FULL           = 2002,

    /* --- UART / CMD --- */
    APP_E_UART_RX_FAIL         = 3001,
    APP_E_CMD_UNKNOWN          = 3101,

} AppError_t;

/* ====== 错误上报入口 ======
 * - mod: 模块来源
 * - code: 错误码
 * - detail: 额外数值（如 bit index、timeout_us、queue name hash 等）
 * - msg: 可选补充说明（printf风格）
 */
void APP_ErrorReport(AppModule_t mod,
                     AppError_t code,
                     int32_t detail,
                     const char *file,
                     int32_t line,
                     const char *func,
                     const char *msg_fmt, ...);

/* ====== 宏：自动带 file/line/func ====== */
#define APP_ERR(mod, code, detail, fmt, ...) \
    APP_ErrorReport((mod), (code), (detail), __FILE__, __LINE__, __func__, (fmt), ##__VA_ARGS__)

#define APP_ERR0(mod, code) \
    APP_ErrorReport((mod), (code), 0, __FILE__, __LINE__, __func__, NULL)

#ifdef __cplusplus
}
#endif

#endif /* APP_ERROR_H */
