#include "u8g2_hal.h"
#include "main.h"

// 微秒级延时（72MHz主频）
static void delay_us(uint32_t us)
{
    uint32_t cycles = us * 72 / 4;
    while (cycles--);
}

// I2C 软件时序：发送一个字节
static void i2c_write_byte(uint8_t byte)
{
    for (int i = 7; i >= 0; i--)
    {
        SCL_LOW();
        delay_us(2);
        if (byte & (1 << i))
            SDA_HIGH();
        else
            SDA_LOW();
        delay_us(2);
        SCL_HIGH();
        delay_us(4);
    }
    // 第9个时钟：ACK（我们忽略ACK，直接拉低SCL继续）
    SCL_LOW();
    SDA_HIGH();   // 释放SDA，让从机可以拉低
    delay_us(2);
    SCL_HIGH();
    delay_us(4);
    SCL_LOW();
}

// I2C START 信号：SCL高时SDA下降沿
static void i2c_start(void)
{
    SDA_HIGH();
    SCL_HIGH();
    delay_us(4);
    SDA_LOW();
    delay_us(4);
    SCL_LOW();
}

// I2C STOP 信号：SCL高时SDA上升沿
static void i2c_stop(void)
{
    SDA_LOW();
    SCL_LOW();
    delay_us(2);
    SCL_HIGH();
    delay_us(4);
    SDA_HIGH();
    delay_us(4);
}

/*
 * u8g2 HAL 回调函数
 *
 * u8g2 底层通过这个回调和硬件通信
 * msg 参数告诉我们当前需要执行什么操作
 */
uint8_t u8g2_gpio_and_delay_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    switch (msg)
    {
        // u8g2 请求初始化GPIO和延时资源
        case U8X8_MSG_GPIO_AND_DELAY_INIT:
            // CubeMX已经在MX_GPIO_Init()里初始化好了
            // 拉高SDA和SCL，I2C空闲状态
            SDA_HIGH();
            SCL_HIGH();
            break;

        // u8g2 请求微秒延时
        case U8X8_MSG_DELAY_MILLI:
            HAL_Delay(arg_int);
            break;

        case U8X8_MSG_DELAY_10MICRO:
            delay_us(10 * arg_int);
            break;

        case U8X8_MSG_DELAY_100NANO:
            // 72MHz下约1个NOP足够，这里简单处理
            __NOP();
            break;

        // u8g2 请求发送一包I2C数据
        // arg_ptr 指向数据缓冲区，arg_int 是字节数
        case U8X8_MSG_BYTE_SEND:
        {
            uint8_t *data = (uint8_t *)arg_ptr;
            while (arg_int--)
            {
                i2c_write_byte(*data++);
            }
            break;
        }

        // u8g2 告诉我们I2C设备地址
        // 这里发送 START + 地址字节
        case U8X8_MSG_BYTE_SET_DC:
            break;

        case U8X8_MSG_BYTE_START_TRANSFER:
        {
            // 发送START信号
            i2c_start();
            // 发送从机地址（u8x8_GetI2CAddress返回已左移1位的地址）
            i2c_write_byte(u8x8_GetI2CAddress(u8x8));
            break;
        }

        case U8X8_MSG_BYTE_END_TRANSFER:
            // 发送STOP信号
            i2c_stop();
            break;

        default:
            return 0;
    }
    return 1;
}

/*
 * 初始化 u8g2
 * 在 main.c 里声明 u8g2_t u8g2 后调用此函数
 */
void u8g2_Init(u8g2_t *u8g2)
{
    // 调试：手动扫描I2C地址，用逻辑分析仪或直接试两个常见地址
    // SSD1306 只有两种地址：0x3C(0x78) 或 0x3D(0x7A)
    // 先强制设置为 0x78 试一次
    
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(
        u8g2,
        U8G2_R0,
        u8g2_gpio_and_delay_cb,
        u8g2_gpio_and_delay_cb
    );

    // 强制设置I2C地址为 0x78（即0x3C左移1位）
    u8x8_SetI2CAddress(&u8g2->u8x8, 0x78);

    u8g2_InitDisplay(u8g2);
    u8g2_SetPowerSave(u8g2, 0);
}