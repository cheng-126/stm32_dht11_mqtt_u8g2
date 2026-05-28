#ifndef U8G2_HAL_H
#define U8G2_HAL_H

#include "u8g2.h"

// 软件I2C引脚定义
#define OLED_SDA_PORT   GPIOB
#define OLED_SDA_PIN    GPIO_PIN_0
#define OLED_SCL_PORT   GPIOB
#define OLED_SCL_PIN    GPIO_PIN_1

// 引脚操作宏
#define SDA_HIGH()  HAL_GPIO_WritePin(OLED_SDA_PORT, OLED_SDA_PIN, GPIO_PIN_SET)
#define SDA_LOW()   HAL_GPIO_WritePin(OLED_SDA_PORT, OLED_SDA_PIN, GPIO_PIN_RESET)
#define SCL_HIGH()  HAL_GPIO_WritePin(OLED_SCL_PORT, OLED_SCL_PIN, GPIO_PIN_SET)
#define SCL_LOW()   HAL_GPIO_WritePin(OLED_SCL_PORT, OLED_SCL_PIN, GPIO_PIN_RESET)

// 对外接口
void u8g2_Init(u8g2_t *u8g2);
uint8_t u8g2_gpio_and_delay_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

#endif