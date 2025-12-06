#include "bsp_dht11.h"

/*===================== 硬件连接配置区域 =====================*/

/* 根据实际连线修改这里：
 * 默认 DHT11 data 脚接在 PB11
 */
#define DHT11_PORT      GPIOB
#define DHT11_PIN       GPIO_PIN_11

/*===================== 内部工具：GPIO & 延时 =====================*/

/* 切换为输出模式（推挽输出） */
static void DHT11_Pin_Output(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 确保时钟开启（即使 CubeMX 已经开过，再开一次也没问题） */
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin   = DHT11_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;          // 外部如果有上拉也没关系
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}

/* 切换为输入模式（上拉输入） */
static void DHT11_Pin_Input(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin  = DHT11_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}

/*--------------------- DWT 微秒延时 ---------------------*/

/* 适用于 Cortex-M3/M4，例如 STM32F4 */
static void DWT_Delay_Init(void)
{
    /* 使能 DWT 外设计数器 */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL       |= DWT_CTRL_CYCCNTENA_Msk;
    DWT->CYCCNT      = 0;
}

/* us 级延时 */
static void DWT_Delay_us(uint32_t us)
{
    uint32_t cycles = (SystemCoreClock / 1000000U) * us;
    uint32_t start  = DWT->CYCCNT;

    while ((DWT->CYCCNT - start) < cycles)
    {
        /* busy wait */
    }
}

/* 等待引脚进入指定电平，带超时（us）
 * 返回 0: OK
 * 返回 1: TIMEOUT
 */
static uint8_t DHT11_WaitForState(GPIO_PinState state, uint32_t timeout_us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = timeout_us * (SystemCoreClock / 1000000U);

    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) != state)
    {
        if ((DWT->CYCCNT - start) > ticks)
        {
            return 1;  // TIMEOUT
        }
    }
    return 0;          // OK
}

/*===================== 对外 API 实现 =====================*/

/* 初始化：
 * - 初始化 DWT 延时
 * - 配置数据脚为输出并拉高（空闲状态）
 * - 等待上电稳定（1~2s）
 */
void BSP_DHT11_Init(void)
{
    /* 微秒延时模块 */
    DWT_Delay_Init();

    /* 默认拉高，空闲状态为高电平 */
    DHT11_Pin_Output();
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET);

    /* DHT11 上电后建议等待至少 1s */
    HAL_Delay(1500);
}

/* 核心读取函数：严格按 DHT11 协议时序
 *
 * 时序回顾：
 * 1. MCU 起始信号：
 *    - 拉低 ≥18 ms
 *    - 然后拉高 20~40 us
 *    - 切换为输入，等待 DHT11 响应
 *
 * 2. DHT11 响应：
 *    - 80 us 低电平
 *    - 80 us 高电平
 *
 * 3. 数据传输（40 bit = 5 字节）：
 *    - 每位由 50 us 低 + (26~28 us 高 = 0, ~70 us 高 = 1) 组成
 *
 * 4. 数据格式：
 *    Byte0: 湿度整数部分
 *    Byte1: 湿度小数部分（通常为 0）
 *    Byte2: 温度整数部分
 *    Byte3: 温度小数部分（通常为 0）
 *    Byte4: 校验和 = Byte0 + Byte1 + Byte2 + Byte3
 */
HAL_StatusTypeDef BSP_DHT11_Read(uint8_t *humi, uint8_t *temp)
{
    if (humi == NULL || temp == NULL)
    {
        return HAL_ERROR;
    }

    uint8_t raw[5] = {0};
    uint8_t i, j;

    /* 1. 发送起始信号：拉低 ≥18ms */
    DHT11_Pin_Output();
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_RESET);
    HAL_Delay(18);

    /* 2. 拉高 20~40us */
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET);
    DWT_Delay_us(30);

    /* 3. 切换为输入，等待 DHT11 响应 */
    DHT11_Pin_Input();

    /* DHT11 响应：80us 低电平 + 80us 高电平 */

    if (DHT11_WaitForState(GPIO_PIN_RESET, 100))   // 等待变为低电平
    {
        return HAL_TIMEOUT;
    }

    if (DHT11_WaitForState(GPIO_PIN_SET, 100))     // 等待变为高电平
    {
        return HAL_TIMEOUT;
    }

    if (DHT11_WaitForState(GPIO_PIN_RESET, 100))   // 再次拉低，准备发送数据
    {
        return HAL_TIMEOUT;
    }

    /* 4. 开始接收 40 bit 数据 */
    for (i = 0; i < 5; i++)
    {
        for (j = 0; j < 8; j++)
        {
            /* 等待 50us 低电平结束，进入高电平 */
            if (DHT11_WaitForState(GPIO_PIN_SET, 70))
            {
                return HAL_TIMEOUT;
            }

            /* 高电平开始，延时约 40us 到中间采样点 */
            DWT_Delay_us(40);

            if (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET)
            {
                /* 此时仍为高电平 → 认为是逻辑 1 */
                raw[i] |= (uint8_t)(1U << (7U - j));

                /* 等待该位的高电平结束，回到低电平，准备下一位 */
                if (DHT11_WaitForState(GPIO_PIN_RESET, 70))
                {
                    return HAL_TIMEOUT;
                }
            }
            else
            {
                /* 此时为低电平 → 认为是逻辑 0，直接进入下一位 */
            }
        }
    }

    /* 5. 校验和 */
    uint8_t sum = raw[0] + raw[1] + raw[2] + raw[3];
    if (sum != raw[4])
    {
        return HAL_ERROR;   // 校验失败
    }

    /* 6. 输出数据（只取整数部分） */
    *humi = raw[0];
    *temp = raw[2];

    return HAL_OK;
}

HAL_StatusTypeDef BSP_DHT11_ReadData(BSP_DHT11_Data_t *data)
{
    if (data == NULL)
    {
        return HAL_ERROR;
    }

    uint8_t humi = 0;
    uint8_t temp = 0;

    HAL_StatusTypeDef ret = BSP_DHT11_Read(&humi, &temp);
    if (ret != HAL_OK)
    {
        return ret;
    }

    data->humi = humi;
    data->temp = temp;

    return HAL_OK;
}
