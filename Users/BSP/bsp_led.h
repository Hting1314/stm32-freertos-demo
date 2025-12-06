#ifndef BSP_LED_H
#define BSP_LED_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/* 按你的板子分配：
 * - PG13：心跳 / 运行 LED
 * - PB14：模式 LED
 */
typedef enum
{
    BSP_LED_RUN = 0,   // PG13，心跳/运行灯
    BSP_LED_MODE       // PB14，模式指示灯
} BSP_LedId_t;

/* 逻辑上的“亮/灭” */
typedef enum
{
    BSP_LED_STATE_OFF = 0,
    BSP_LED_STATE_ON
} BSP_LedState_t;

/* 初始化板上 LED：
 * - 配置 PG13 / PB14 为推挽输出
 * - 设置为“默认灭灯”
 */
void BSP_LED_Init(void);

/* 通用接口 */
void BSP_LED_Set(BSP_LedId_t led, BSP_LedState_t state);
void BSP_LED_Toggle(BSP_LedId_t led);

/* 语义封装：方便在 APP 层直接使用 */

/* Run LED（PG13，有源低：0亮1灭） */
static inline void BSP_LED_Run_On(void)      { BSP_LED_Set(BSP_LED_RUN,  BSP_LED_STATE_ON);  }
static inline void BSP_LED_Run_Off(void)     { BSP_LED_Set(BSP_LED_RUN,  BSP_LED_STATE_OFF); }
static inline void BSP_LED_Run_Toggle(void)  { BSP_LED_Toggle(BSP_LED_RUN); }

/* Mode LED（PB14，有源高：1亮0灭） */
static inline void BSP_LED_Mode_On(void)     { BSP_LED_Set(BSP_LED_MODE, BSP_LED_STATE_ON);  }
static inline void BSP_LED_Mode_Off(void)    { BSP_LED_Set(BSP_LED_MODE, BSP_LED_STATE_OFF); }
static inline void BSP_LED_Mode_Toggle(void) { BSP_LED_Toggle(BSP_LED_MODE); }

/* 可选：用一个“模式号”统一设置模式灯状态 */
void BSP_LED_SetMode(uint8_t mode);

#ifdef __cplusplus
}
#endif

#endif /* BSP_LED_H */
