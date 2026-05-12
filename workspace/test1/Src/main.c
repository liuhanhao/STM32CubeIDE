/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : 主程序
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 ******************************************************************************
 */

#include "main.h"

/* 私有函数原型 */
void SystemClock_Config(void);

/**
  * @brief  主程序入口
  * @retval int
  */
int main(void)
{
  /* 复位所有外设,初始化 Flash 接口和 Systick */
  HAL_Init();

  /* 配置系统时钟 */
  SystemClock_Config();

  /* 初始化所有配置的外设 */
  /* 在这里添加你的外设初始化代码 */

  /* 无限循环 */
  while (1)
  {
    /* 用户代码 */
  }
}

/**
  * @brief 系统时钟配置
  *        配置系统时钟为 72MHz
  *        使用外部 8MHz 晶振
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** 初始化 RCC 振荡器
   * HSE: 8MHz 外部晶振
   * PLL: HSE * 9 = 72MHz
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** 初始化 CPU, AHB 和 APB 总线时钟
   * SYSCLK: 72MHz
   * HCLK:   72MHz (AHB)
   * PCLK1:  36MHz (APB1, 最大 36MHz)
   * PCLK2:  72MHz (APB2, 最大 72MHz)
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

/**
  * @brief  错误处理函数
  * @retval None
  */
void Error_Handler(void)
{
  /* 用户可以在这里添加自己的错误处理代码 */
  __disable_irq();
  while (1)
  {
  }
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  断言失败时的报告函数
  * @param  file: 源文件名指针
  * @param  line: 断言错误的行号
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* 用户可以添加自己的实现来报告文件名和行号 */
  /* 例如: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
}
#endif
