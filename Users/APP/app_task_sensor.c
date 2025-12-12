#include "app_task_sensor.h"
#include "cmsis_os.h"
#include "app_types.h"
#include "bsp_uart.h"   // uart_printf()
#include "bsp_dht11.h"      // DHT11_Read()，后面可以 BSP 化成 bsp_dht11.h
#include "app_error.h"


extern osMessageQueueId_t queueSensorHandle;

void StartSensorTask(void *argument)
{
	
		LOG_INFO("[SENSOR] task start\r\n");
	
    (void)argument;

    uint8_t humi = 0;
    uint8_t temp = 0;
    SensorEvt_t evt;

    /* 上电稳定期：只做一次（DHT11 需要） */
    osDelay(2000);

    LOG_INFO("[SENSOR] task start (event-driven)\r\n");

    for (;;)
    {
        /* 阻塞等待事件：软件定时器每 1s 发一次 */
        if (osMessageQueueGet(queueSensorHandle, &evt, NULL, osWaitForever) == osOK)
        {
            if (evt == SENSOR_EVT_READ)
            {
                HAL_StatusTypeDef res = BSP_DHT11_Read(&humi, &temp);

                if (res == HAL_OK)
                {
                    LOG_INFO("[SENSOR] DHT11 OK: T=%d C, H=%d %%\r\n", temp, humi);
                }
                else
                {
                    APP_ERR(APP_MOD_SENSOR, APP_E_SENSOR_FAIL, res, "BSP_DHT11_Read failed");
                }
            }
        }
    }
}
