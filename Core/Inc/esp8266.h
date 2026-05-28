#ifndef __ESP8266_H
#define __ESP8266_H

#include "stm32f1xx_hal.h"
#include <string.h>
#include <stdio.h>

#define ESP_UART          huart2
#define ESP_RX_BUF_SIZE   1024

// ===== 修改为你自己的配置 =====
#define WIFI_SSID         
#define WIFI_PASS         

// OneNet MQTT 配置（新版OneNet物联网平台）
#define ONENET_HOST       
#define ONENET_PORT       
#define ONENET_PRODUCT_ID 
#define ONENET_DEVICE_ID  
#define ONENET_TOKEN      

// MQTT Topics
#define TOPIC_PUB       "$sys/" ONENET_PRODUCT_ID "/" ONENET_DEVICE_ID "/thing/property/post"
#define TOPIC_SUB       "$sys/" ONENET_PRODUCT_ID "/" ONENET_DEVICE_ID "/thing/property/set"
#define TOPIC_SET_REPLY "$sys/" ONENET_PRODUCT_ID "/" ONENET_DEVICE_ID "/thing/property/set_reply"

extern UART_HandleTypeDef ESP_UART;

uint8_t ESP8266_Init(void);
uint8_t ESP8266_MQTTConnect(void);
uint8_t ESP8266_PublishData(uint8_t temp, uint8_t humi);
void    ESP8266_UART_RxCallback(void);
uint8_t ESP8266_CheckLEDCmd(uint8_t *ledState);
const char* ESP8266_GetResponse(void);
uint8_t ESP8266_QueryVersion(void);

#endif