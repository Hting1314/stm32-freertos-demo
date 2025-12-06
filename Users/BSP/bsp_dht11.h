#ifndef BSP_DHT11_H
#define BSP_DHT11_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>

/* 若之后想支持多个传感器，可以在这里扩展结构体，目前简单用 humi/temp 两个字节 */
typedef struct
{
    uint8_t humi;   // 湿度整数部分
    uint8_t temp;   // 温度整数部分
} BSP_DHT11_Data_t;

/* DHT11 BSP 初始化：
 * - 调用底层 DHT11 驱动的初始化（比如 DWT_Delay_Init + 引脚模式）
 * - 一般在系统启动阶段调用一次即可
 */
void BSP_DHT11_Init(void);

/* 读取一次 DHT11 的温湿度：
 * - humi / temp 为输出参数（整数部分）
 * - 返回 HAL_OK 表示成功，其它表示失败（可统一用 HAL_ERROR）
 */
HAL_StatusTypeDef BSP_DHT11_Read(uint8_t *humi, uint8_t *temp);

/* 可选：如果想一次拿结构体 */
HAL_StatusTypeDef BSP_DHT11_ReadData(BSP_DHT11_Data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* BSP_DHT11_H */
