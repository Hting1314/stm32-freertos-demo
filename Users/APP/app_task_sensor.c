#include "app_task_sensor.h"
#include "cmsis_os.h"
#include "bsp_uart.h"   // uart_printf()
#include "bsp_dht11.h"      // DHT11_Read()，后面可以 BSP 化成 bsp_dht11.h

void StartSensorTask(void *argument)
{
	
		LOG_INFO("[SENSOR] task start\r\n");
	
    (void)argument;

    uint8_t humi = 0;
    uint8_t temp = 0;
    HAL_StatusTypeDef res;

    /* 给系统和 DHT11 一点时间上电稳定 */
    osDelay(2000);

    for (;;)
    {
        res = BSP_DHT11_Read(&humi, &temp);

        if (res == HAL_OK)
        {
            LOG_INFO("DHT11 OK:T = %d C, H = %d %%\r\n", temp, humi);
        }
        else
        {
            LOG_ERROR("DHT11 ERROR:read failed\r\n");
        }

        /* 遵守 DHT11 最小采样周期（>= 1s），这里用 2s 更保险 */
        osDelay(2000);
    }
}
