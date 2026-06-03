/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
#include "main.h"
#include "OLED.h"
#include "dht11.h"
#include "delay.h"

void SystemClock_Config(void);
static void MX_GPIO_Init(void);

/**
  * @brief  应用程序入口
  * @retval int
  */
int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();

  /* 外设初始化 */
  OLED_Init();
  DHT11_Init();

  /* 显示固定标题 */
  OLED_ShowString(1, 1, "DHT11 Monitor");
  OLED_ShowString(2, 1, "Humi:   %  RH");
  OLED_ShowString(3, 1, "Temp:   C       ");

  DHT11_Data_t sensor;

  while (1)
  {
    /* 每 2 秒读取一次（DHT11 采样率 ≤ 1Hz，建议间隔 ≥ 2s）*/
	Delay_ms(2000);

    if (DHT11_Read(&sensor) == DHT11_OK)
    {
      /* 湿度：显示在第 2 行第 7 列，2 位整数 */
      OLED_ShowNum(2, 7, sensor.humi_int, 2);

      /* 温度：显示在第 3 行第 7 列，2 位整数，后跟舒适度标签
         < 22°C       → Cool 凉爽
         22°C ~ 29°C  → Nice 舒适
         >= 30°C      → Hot  闷热 */
      OLED_ShowNum(3, 7, sensor.temp_int, 2);
      if (sensor.temp_int < DHT11_TEMP_COOL_MAX)
      {
        OLED_ShowString(3, 10, " Cool  ");
      }
      else if (sensor.temp_int < DHT11_TEMP_HOT_MIN)
      {
        OLED_ShowString(3, 10, " Nice  ");
      }
      else
      {
        OLED_ShowString(3, 10, " Hot   ");
      }

      /* 第 4 行根据湿度显示舒适度：
         < 40%       → Dry     干燥
         40% ~ 60%   → Comfort 舒适
         > 60%       → Humid   潮湿 */
      if (sensor.humi_int < DHT11_HUMI_DRY_MAX)
      {
        OLED_ShowString(4, 1, "Feel: Dry       ");
      }
      else if (sensor.humi_int <= DHT11_HUMI_COMFORT_MAX)
      {
        OLED_ShowString(4, 1, "Feel: Comfort   ");
      }
      else
      {
        OLED_ShowString(4, 1, "Feel: Humid     ");
      }
    }
    else
    {
      OLED_ShowString(4, 1, "Feel: ERROR     ");
    }
  }
}

/**
  * @brief  系统时钟配置
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

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

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  GPIO 初始化
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
}

/**
  * @brief  错误处理函数
  * @retval None
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
/**
  * @brief  断言失败时报告文件名和行号
  * @param  file: 源文件名指针
  * @param  line: 错误所在行号
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
