#include "bsp_led.h"

/* 这里把具体 GPIO 细节锁在 BSP 里 */
#define RUN_LED_PORT      GPIOG
#define RUN_LED_PIN       GPIO_PIN_13   // 心跳灯，有源低

#define MODE_LED_PORT     GPIOB
#define MODE_LED_PIN      GPIO_PIN_14   // 模式灯，有源高


void BSP_LED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 1. 使能时钟 */
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* 2. 配置 PG13：推挽输出，低亮高灭 */
    GPIO_InitStruct.Pin   = RUN_LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(RUN_LED_PORT, &GPIO_InitStruct);

    /* 默认灭灯：有源低 → 高电平表示 OFF */
    HAL_GPIO_WritePin(RUN_LED_PORT, RUN_LED_PIN, GPIO_PIN_SET);

    /* 3. 配置 PB14：推挽输出，高亮低灭 */
    GPIO_InitStruct.Pin   = MODE_LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(MODE_LED_PORT, &GPIO_InitStruct);

    /* 默认灭灯：有源高 → 低电平表示 OFF */
    HAL_GPIO_WritePin(MODE_LED_PORT, MODE_LED_PIN, GPIO_PIN_RESET);
}

/* 内部：逻辑 ON/OFF → 实际高低电平 */
static inline GPIO_PinState prv_led_logic_to_level(BSP_LedId_t led, BSP_LedState_t state)
{
    switch (led)
    {
        case BSP_LED_RUN:
            /* 有源低：ON→RESET, OFF→SET */
            return (state == BSP_LED_STATE_ON) ? GPIO_PIN_RESET : GPIO_PIN_SET;

        case BSP_LED_MODE:
            /* 有源高：ON→SET, OFF→RESET */
            return (state == BSP_LED_STATE_ON) ? GPIO_PIN_SET   : GPIO_PIN_RESET;

        default:
            return GPIO_PIN_RESET;
    }
}

void BSP_LED_Set(BSP_LedId_t led, BSP_LedState_t state)
{
    GPIO_TypeDef *port;
    uint16_t      pin;

    switch (led)
    {
        case BSP_LED_RUN:
            port = RUN_LED_PORT;
            pin  = RUN_LED_PIN;
            break;

        case BSP_LED_MODE:
            port = MODE_LED_PORT;
            pin  = MODE_LED_PIN;
            break;

        default:
            return;
    }

    HAL_GPIO_WritePin(port, pin, prv_led_logic_to_level(led, state));
}

void BSP_LED_Toggle(BSP_LedId_t led)
{
    GPIO_TypeDef *port;
    uint16_t      pin;

    switch (led)
    {
        case BSP_LED_RUN:
            port = RUN_LED_PORT;
            pin  = RUN_LED_PIN;
            break;

        case BSP_LED_MODE:
            port = MODE_LED_PORT;
            pin  = MODE_LED_PIN;
            break;

        default:
            return;
    }

    HAL_GPIO_TogglePin(port, pin);
}

void BSP_LED_SetMode(uint8_t mode)
{
    /* 简单版本：0=灭, 非0=亮 */
    if (mode == 0)
    {
        BSP_LED_Mode_Off();
    }
    else
    {
        BSP_LED_Mode_On();
    }
}
