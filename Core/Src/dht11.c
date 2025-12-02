#include "dht11.h"

/*	GPIO 模式切换封装	*/
static void DHT11_GPIO_Output(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};        
	GPIO_InitStruct.Pin = DHT11_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}

static void DHT11_GPIO_Input(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = DHT11_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}

static inline void DHT11_WriteLow(void)
{
	HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_RESET);
}

static inline void DHT11_WriteHigh(void)
{
	HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET);
}

static inline GPIO_PinState DHT11_ReadPin(void)
{
	return HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN);
}

/*	微秒延时实现		*/
void DWT_Delay_Init(void)
{
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
	DWT->CYCCNT = 0;
}

void DWT_Delay_us(uint32_t us)
{
	uint32_t cycles = (SystemCoreClock / 1000000U) * us;
	uint32_t start = DWT->CYCCNT;
	
	while ((DWT->CYCCNT - start) < cycles)
	{
		/* busy wait */
	}
}

/*	初始化DHT11	*/
void DHT11_Init(void)
{
	DWT_Delay_Init();
	
	/*	将引脚配置为输出并拉高（空闲为高）	*/
	DHT11_GPIO_Output();
	DHT11_WriteHigh();
	
	HAL_Delay(1000);   //上电稳定期
}

/* 读取原始数据的函数 */
HAL_StatusTypeDef DHT11_ReadRaw(uint8_t data[5])
{
	/* 
     * 这里暂时只写注释，下一天我们来按时序写代码。
     *
     * DHT11 时序（简化理解）：
     *
     * 1. MCU 发送起始信号：
     *    - 拉低总线至少 18 ms
     *    - 然后拉高 20~40 us
     *    - 切换为输入模式，准备接收 DHT11 响应
     *
     * 2. DHT11 响应：
     *    - 拉低约 80 us
     *    - 再拉高约 80 us
     *
     * 3. 数据传输（40 bit = 5 字节）：
     *    - 每一 bit 由一个固定低电平 + 可变高电平组成：
     *      * 先低电平约 50 us
     *      * 然后：
     *          - 高电平 ~26~28 us 代表 0
     *          - 高电平 ~70 us    代表 1
     *
     * 4. 数据格式（8bit * 5）：
     *    Byte0: 湿度整数部分
     *    Byte1: 湿度小数部分（通常为 0）
     *    Byte2: 温度整数部分
     *    Byte3: 温度小数部分（通常为 0）
     *    Byte4: 校验和 = Byte0 + Byte1 + Byte2 + Byte3
     *
     * 下一步我们会实现：
     *   - 发送起始信号
     *   - 接收响应脉冲
     *   - 按脉宽区分 0/1，拼成 5 字节
     *   - 校验和验证
     */
		 
	(void)data;
	return HAL_ERROR;
}
