#include "led.h"

uint8_t Led_Status = LED_OFF;

/**
 * @brief  初始化LED引脚（PC13，推挽输出）
 * @note   STM32F103C8T6板载LED连接在PC13，低电平点亮
 */
void Led_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* 默认关闭LED（高电平=灭） */
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

    /* Configure GPIO pin : PC13 */
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    Led_Status = LED_OFF;
}

/**
 * @brief  设置LED亮灭并更新全局状态
 * @param  status: LED_ON(1) 点亮, LED_OFF(0) 熄灭
 */
void Led_Set(_Bool status)
{
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, status == LED_ON ? GPIO_PIN_RESET : GPIO_PIN_SET);
    Led_Status = status;
}
