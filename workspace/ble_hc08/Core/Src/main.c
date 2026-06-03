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
#include "ble_hc08.h"
#include <string.h>

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

  /* 初始化 OLED */
  OLED_Init();
  OLED_ShowString(1, 1, "BLE HC-08 Ready");

  /* 初始化 HC-08 蓝牙（USART1，PA9/PA10）并显示结果 */
  if (BLE_Init() == HAL_OK)
  {
      OLED_ShowString(2, 1, "UART:OK RX:     ");
  }
  else
  {
      OLED_ShowString(2, 1, "UART:FAIL       ");
  }

  /* 上电后向手机发送一条欢迎消息（BLE 连接后手机端会收到）*/
  BLE_SendString("STM32 Ready!\r\n");

  char msgBuf[BLE_RX_BUFFER_SIZE];

  while (1)
  {
    /* 超时检测：App 不带 \r\n 时自动触发消息就绪 */
    BLE_Poll();

    /* 调试：第 2 行实时更新 RX 计数（始终可见）*/
    OLED_ShowNum(2, 9, BLE_GetRxByteCount(), 5);

    /* 收到手机发来的完整消息（以回车/换行结尾，或超时）*/
    if (BLE_IsMessageReady())
    {
      BLE_GetMessage(msgBuf, sizeof(msgBuf));

      /* 在 OLED 第 1 行显示收到的消息 */
      OLED_ShowString(1, 1, "                ");
      OLED_ShowString(1, 1, msgBuf);

      /* 收到消息后回复 ok */
      BLE_SendString("ok\r\n");

      /* 清除标志，准备接收下一条 */
      BLE_ClearMessage();
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
