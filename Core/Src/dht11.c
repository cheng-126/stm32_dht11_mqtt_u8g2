#include "dht11.h"

extern TIM_HandleTypeDef htim2; // CubeMX生成

/* 微秒延时（基于TIM2） */
static void delay_us(uint32_t us) {
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    HAL_TIM_Base_Start(&htim2);
    while(__HAL_TIM_GET_COUNTER(&htim2) < us);
    HAL_TIM_Base_Stop(&htim2);
}

/* 切换PA0为输出模式 */
static void DHT11_SetOutput(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = DHT11_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_GPIO, &GPIO_InitStruct);
}

/* 切换PA0为输入模式（上拉） */
static void DHT11_SetInput(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin  = DHT11_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DHT11_GPIO, &GPIO_InitStruct);
}

/* 带超时的等待引脚电平变化，超时返回1，成功返回0 */
static uint8_t DHT11_WaitLevel(GPIO_PinState level, uint16_t timeout_us) {
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    HAL_TIM_Base_Start(&htim2);
    while(HAL_GPIO_ReadPin(DHT11_GPIO, DHT11_PIN) != level) {
        if(__HAL_TIM_GET_COUNTER(&htim2) > timeout_us) {
            HAL_TIM_Base_Stop(&htim2);
            return 1; // 超时
        }
    }
    HAL_TIM_Base_Stop(&htim2);
    return 0; // 成功
}

/* 读取一个字节 */
static uint8_t DHT11_ReadByte(void) {
    uint8_t byte = 0;
    for(int i = 0; i < 8; i++) {
        /* 1. 等待低电平结束（每个bit前DHT11会拉低~50us） */
        if(DHT11_WaitLevel(GPIO_PIN_SET, 100)) return 0;

        /* 2. 测量高电平持续时间：26-28us=0, 70us=1 */
        __HAL_TIM_SET_COUNTER(&htim2, 0);
        HAL_TIM_Base_Start(&htim2);

        /* 等待高电平结束 */
        if(DHT11_WaitLevel(GPIO_PIN_RESET, 100)) {
            HAL_TIM_Base_Stop(&htim2);
            return 0;
        }

        uint16_t pulse = __HAL_TIM_GET_COUNTER(&htim2);

        byte <<= 1;
        if(pulse > 40) {
            byte |= 0x01;  // 高电平>40us 判定为"1"
        }
    }
    return byte;
}

uint8_t DHT11_Read(DHT11_Data_t *data) {
    uint8_t buf[5] = {0};

    /*
     * 关闭全局中断，防止USART2等中断打断DHT11微秒级时序
     * DHT11整个通信过程约4-5ms，影响极小
     */
    __disable_irq();

    // 主机发送起始信号：拉低>18ms，然后拉高20-40us
    DHT11_SetOutput();
    HAL_GPIO_WritePin(DHT11_GPIO, DHT11_PIN, GPIO_PIN_RESET);

    /* 需要延时>18ms，但__disable_irq后HAL_Delay不工作，用TIM延时代替 */
    delay_us(20000);  // 20ms

    HAL_GPIO_WritePin(DHT11_GPIO, DHT11_PIN, GPIO_PIN_SET);
    delay_us(30);

    // 切换为输入模式，等待DHT11响应
    DHT11_SetInput();

    // DHT11响应：先拉低80us，再拉高80us
    if(DHT11_WaitLevel(GPIO_PIN_RESET, 100)) {  // 等待DHT11拉低
        __enable_irq();
        return 1; // 无响应
    }
    if(DHT11_WaitLevel(GPIO_PIN_SET, 100)) {     // 等待低电平结束（~80us）
        __enable_irq();
        return 2;
    }
    if(DHT11_WaitLevel(GPIO_PIN_RESET, 100)) {   // 等待高电平结束（~80us）
        __enable_irq();
        return 3;
    }

    // 读取5字节数据（湿度整数+小数 + 温度整数+小数 + 校验和）
    for(int i = 0; i < 5; i++) {
        buf[i] = DHT11_ReadByte();
    }

    // 恢复中断
    __enable_irq();

    // 校验和验证
    if(buf[4] != (uint8_t)(buf[0]+buf[1]+buf[2]+buf[3])) return 4;

    data->humidity    = buf[0];
    data->temperature = buf[2];
    return 0; // 成功
}