#ifndef __DHT11_H
#define __DHT11_H

#include "stm32f1xx_hal.h"

// PA0
#define DHT11_GPIO    GPIOA
#define DHT11_PIN     GPIO_PIN_0

typedef struct {
    uint8_t humidity;
    uint8_t temperature;
} DHT11_Data_t;

uint8_t DHT11_Read(DHT11_Data_t *data);

#endif