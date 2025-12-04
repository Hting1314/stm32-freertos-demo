#include "dht11.h"

/* 内部使用的状态枚举 */
typedef enum
{
    DHT11_OK = 0,
    DHT11_TIMEOUT,
    DHT11_CHECKSUM_ERR
} DHT11_Status_t;

typedef struct
{
    uint8_t humi_int;
    uint8_t humi_dec;
    uint8_t temp_int;
    uint8_t temp_dec;
} DHT11_Frame_t;

#include "dht11.h"

/* --------- GPIO 输入/输出封装 --------- */

static void DHT11_Pin_Output(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
		__HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitStruct.Pin   = DHT11_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;          // ????
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}

static void DHT11_Pin_Input(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin  = DHT11_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;           // 再加一份内部上拉保险

    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}

/* --------- 软延时：粗略微秒级 --------- */

static void DHT11_DelayUs(uint32_t us)
{
    while (us--)
    {
        /* 这个循环次数和主频有关，但 DHT11 容忍度很高，近似即可 */
        for (volatile uint32_t i = 0; i < 20; i++)
        {
            __NOP();
        }
    }
}

/* 等待引脚变为指定状态，最多 timeout_us 微秒；超时返回1，正常返回0 */
static uint8_t DHT11_WaitForState(GPIO_PinState state, uint32_t timeout_us)
{
    while (timeout_us--)
    {
        if (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == state)
        {
            return 0;   // OK
        }
        DHT11_DelayUs(1);
    }
    return 1;           // TIMEOUT
}

/* --------- 对外：初始化 --------- */

void DHT11_Init(void)
{
		
//		uart_printf("[DHT] Initializing DHT11...\r\n");
	
    /* 默认配置为输出并拉高，空闲状态为高 */
    DHT11_Pin_Output();
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET);
//		uart_printf("[DHT] Pin set to OUTPUT, GPIO_PIN_SET\r\n");

    /* 上电后给 DHT11 一点稳定时间 */
    HAL_Delay(2000);
	
		 /* 切换为输入，等待 DHT11 响应 */
//    DHT11_Pin_Input();
//    uart_printf("[DHT] Pin set to INPUT\r\n");

//    /* 确认引脚是否拉低 */
//    if (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_RESET)
//    {
//        uart_printf("[DHT] DHT11 has pulled the line low successfully.\r\n");
//    }
//    else
//    {
//        uart_printf("[DHT] WARNING: DHT11 did not pull the line low.\r\n");
//    }
}

/* --------- 内部：读一帧原始数据 --------- */

static DHT11_Status_t DHT11_ReadFrame(DHT11_Frame_t *data)
{
    uint8_t raw[5] = {0};
    uint8_t i, j;
		
//		uart_printf("[DHT] Sending start signal...\r\n");
		
    /* 1. 发送起始信号：拉低 ≥18ms */
    DHT11_Pin_Output();
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_RESET);
    HAL_Delay(18);
		
//		uart_printf("[DHT] Start signal sent, wait for response...\r\n");

    /* 2. 拉高 20~40us */
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET);
    DHT11_DelayUs(30);
		
//		uart_printf("[DHT] Waiting for DHT11 response...\r\n");

    /* 3. 切换为输入，等待 DHT11 响应：
     *    80us 低电平 + 80us 高电平 + 再次拉低以开始发送数据
     */
    DHT11_Pin_Input();
//		uart_printf("[DHT] Pin set to INPUT\r\n");

    if (DHT11_WaitForState(GPIO_PIN_RESET, 500))   // 等待第一个低电平
		{
//				uart_printf("[DHT] resp1 low timeout\r\n");
        return DHT11_TIMEOUT;
		}
		
//		uart_printf("[DHT] Received LOW response, waiting for HIGH...\r\n");

    if (DHT11_WaitForState(GPIO_PIN_SET, 500))			// 等待变高
		{
//				uart_printf("[DHT] resp1 high timeout\r\n");
        return DHT11_TIMEOUT;
		}
		
//		uart_printf("[DHT] Received HIGH response, starting bit read...\r\n");

    if (DHT11_WaitForState(GPIO_PIN_RESET, 500))   // 再次拉低，准备发送数据位
        return DHT11_TIMEOUT;

    /* 4. 接收 40 bit 数据 */

    for (i = 0; i < 5; i++)
    {
        for (j = 0; j < 8; j++)
        {
            /* 等待本 bit 的高电平开始（前面的 50us 低电平结束） */
            if (DHT11_WaitForState(GPIO_PIN_SET, 500))
                return DHT11_TIMEOUT;

            /* 高电平开始后，延时约 40us 到采样点 */
            DHT11_DelayUs(40);

            if (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET)
            {
                /* 若此时仍为高电平，认为是逻辑1 */
                raw[i] |= (1 << (7 - j));

                /* 再等这一位的高电平结束（回到低） */
                if (DHT11_WaitForState(GPIO_PIN_RESET, 200))
                    return DHT11_TIMEOUT;
            }
            else
            {
                /* 若此时已是低电平，认为是逻辑0，直接进入下一位 */
            }
        }
    }

    /* 5. 校验和 */
    uint8_t sum = raw[0] + raw[1] + raw[2] + raw[3];
    if (sum != raw[4])
    {
//			uart_printf("[DHT] checksum err: sum=%d, raw4=%d\r\n", sum, raw[4]);
        return DHT11_CHECKSUM_ERR;
    }

    /* 6. 填充数据结构 */
    data->humi_int = raw[0];
    data->humi_dec = raw[1];
    data->temp_int = raw[2];
    data->temp_dec = raw[3];

    return DHT11_OK;
}

/* --------- 对外：只给整数温湿度 --------- */

HAL_StatusTypeDef DHT11_Read(uint8_t *humidity, uint8_t *temperature)
{
    DHT11_Frame_t frame;
    DHT11_Status_t st = DHT11_ReadFrame(&frame);

    if (st != DHT11_OK)
    {
        // uart_printf("[DHT] read error, status=%d\r\n", st);
        return HAL_ERROR;
    }

    if (humidity)
    {
        *humidity = frame.humi_int;
    }
    if (temperature)
    {
        *temperature = frame.temp_int;
    }

    return HAL_OK;
}
