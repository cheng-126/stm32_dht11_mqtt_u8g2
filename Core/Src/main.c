/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "led.h"
#include <stdio.h>
#include "u8g2_hal.h"
#include "dht11.h"
#include "esp8266.h"
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
u8g2_t u8g2;
DHT11_Data_t dht11_data;
uint32_t lastUpload = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/* USER CODE BEGIN 0 */
static void u8g2_DrawUint8(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, uint8_t val)
{
    char buf[4];
    sprintf(buf, "%d", val);
    u8g2_DrawStr(u8g2, x, y, buf);
}

static void oled_refresh(u8g2_t *u8g2, DHT11_Data_t *data, uint8_t led_status)
{
    char temp_buf[4];
    char humi_buf[4];
    sprintf(temp_buf, "%d", data->temperature);
    sprintf(humi_buf, "%d", data->humidity);

    u8g2_ClearBuffer(u8g2);
    u8g2_SetFont(u8g2, u8g2_font_ncenB08_tr);
    u8g2_DrawStr(u8g2, 4,  14, "Temp:");
    u8g2_DrawStr(u8g2, 40, 14, temp_buf);
    u8g2_DrawStr(u8g2, 64, 14, "C");
    u8g2_DrawStr(u8g2, 4,  34, "Humi:");
    u8g2_DrawStr(u8g2, 40, 34, humi_buf);
    u8g2_DrawStr(u8g2, 64, 34, "%");
    u8g2_DrawStr(u8g2, 4,  54, "LED:");
    u8g2_DrawStr(u8g2, 32, 54, led_status ? "ON " : "OFF");
    u8g2_SendBuffer(u8g2);
}
/* USER CODE END 0 */

int main(void)
{
  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  /* USER CODE END SysInit */

  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_USART2_UART_Init();

  /* USER CODE BEGIN 2 */
  Led_Init();
  __HAL_UART_ENABLE_IT(&huart2, UART_IT_RXNE);

  // 初始化OLED
  u8g2_Init(&u8g2);
  u8g2_ClearBuffer(&u8g2);
  u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
  u8g2_DrawStr(&u8g2, 4, 14, "Initializing...");
  u8g2_SendBuffer(&u8g2);

  // 初始化ESP8266
  u8g2_ClearBuffer(&u8g2);
  u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
  u8g2_DrawStr(&u8g2, 4, 14, "Connecting WiFi");
  u8g2_SendBuffer(&u8g2);
  while(ESP8266_Init()) {
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
    u8g2_DrawStr(&u8g2, 4, 14, "WiFi Fail,Retry");
    u8g2_SendBuffer(&u8g2);
    HAL_Delay(3000);
  }

  // 连接MQTT
  u8g2_ClearBuffer(&u8g2);
  u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
  u8g2_DrawStr(&u8g2, 4, 14, "MQTT Connecting");
  u8g2_SendBuffer(&u8g2);
  uint8_t mqttErr;
  while((mqttErr = ESP8266_MQTTConnect()) != 0) {
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
    u8g2_DrawStr(&u8g2,  4, 14, "MQTT Fail,Retry");
    u8g2_DrawStr(&u8g2,  4, 28, "ErrCode:");
    u8g2_DrawUint8(&u8g2, 68, 28, mqttErr);
    u8g2_SendBuffer(&u8g2);
    HAL_Delay(5000);
  }

  u8g2_ClearBuffer(&u8g2);
  u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
  u8g2_DrawStr(&u8g2, 4, 14, "Hardware Init OK");
  u8g2_SendBuffer(&u8g2);
  HAL_Delay(1500);

  DHT11_Read(&dht11_data);
  /* USER CODE END 2 */

  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
    /* USER CODE BEGIN 3 */

    if(HAL_GetTick() - lastUpload > 5000) {
      lastUpload = HAL_GetTick();

      uint8_t ret = DHT11_Read(&dht11_data);
      if(ret == 0) {
        oled_refresh(&u8g2, &dht11_data, Led_Status);
        ESP8266_PublishData(dht11_data.temperature, dht11_data.humidity);
      } else {
        u8g2_ClearBuffer(&u8g2);
        u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
        u8g2_DrawStr(&u8g2,  4, 14, "DHT11 Err:");
        u8g2_DrawUint8(&u8g2, 68, 14, ret);
        u8g2_DrawStr(&u8g2,  4, 34, "Retrying...");
        u8g2_SendBuffer(&u8g2);
      }
    }

    uint8_t newLed;
    if(ESP8266_CheckLEDCmd(&newLed)) {
      Led_Set(newLed);
      oled_refresh(&u8g2, &dht11_data, Led_Status);
    }

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
