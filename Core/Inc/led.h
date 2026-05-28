#ifndef __LED_H
#define __LED_H

#include "main.h"

#define LED_ON  1
#define LED_OFF 0

/* LED当前状态（全局变量，供OLED显示使用） */
extern uint8_t Led_Status;

/**
 * @brief  初始化LED引脚（PC13，推挽输出）
 */
void Led_Init(void);

/**
 * @brief  设置LED亮灭
 * @param  status: LED_ON(1) 点亮, LED_OFF(0) 熄灭
 * @note   PC13低电平点亮（板载LED）
 */
void Led_Set(_Bool status);

#endif /* __LED_H */
