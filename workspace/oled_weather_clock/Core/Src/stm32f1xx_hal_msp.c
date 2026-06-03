/**
  ******************************************************************************
  * @file         stm32f1xx_hal_msp.c
  * @brief        HAL MSP 初始化与反初始化
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

/**
  * @brief  全局 MSP 初始化
  */
void HAL_MspInit(void)
{
  __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();

  /* 禁用 JTAG，启用 SWD */
  __HAL_AFIO_REMAP_SWJ_NOJTAG();
}

/**
  * @brief  RTC MSP 初始化 —— 启用 LSE 并将其作为 RTC 时钟源
  * @param  hrtc  RTC 句柄指针
  */
void HAL_RTC_MspInit(RTC_HandleTypeDef *hrtc)
{
  if (hrtc->Instance == RTC)
  {
    /* 使能 PWR 和 BKP 时钟，以访问备份域 */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_RCC_BKP_CLK_ENABLE();

    /* 解除备份域写保护 */
    HAL_PWR_EnableBkUpAccess();

    /* 启动 LSE 振荡器 */
    __HAL_RCC_LSE_CONFIG(RCC_LSE_ON);

    /* 等待 LSE 就绪（最长约 5 秒） */
    uint32_t tickstart = HAL_GetTick();
    while (__HAL_RCC_GET_FLAG(RCC_FLAG_LSERDY) == RESET)
    {
      if ((HAL_GetTick() - tickstart) > 5000U)
      {
        /* LSE 启动失败，回退到 LSI */
        __HAL_RCC_LSE_CONFIG(RCC_LSE_OFF);
        __HAL_RCC_LSI_ENABLE();
        while (__HAL_RCC_GET_FLAG(RCC_FLAG_LSIRDY) == RESET) {}
        __HAL_RCC_RTC_CONFIG(RCC_RTCCLKSOURCE_LSI);
        __HAL_RCC_RTC_ENABLE();
        return;
      }
    }

    /* 选择 LSE 作为 RTC 时钟源并使能 RTC */
    __HAL_RCC_RTC_CONFIG(RCC_RTCCLKSOURCE_LSE);
    __HAL_RCC_RTC_ENABLE();
  }
}

/**
  * @brief  RTC MSP 反初始化
  * @param  hrtc  RTC 句柄指针
  */
void HAL_RTC_MspDeInit(RTC_HandleTypeDef *hrtc)
{
  if (hrtc->Instance == RTC)
  {
    __HAL_RCC_RTC_DISABLE();
  }
}

/**
  * @brief  UART MSP 初始化 —— 配置 USART1 GPIO（PA9/PA10）及 NVIC
  * @param  huart  UART 句柄指针
  */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    /* 使能 USART1 和 GPIOA 时钟 */
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* PA9  → USART1_TX（复用推挽输出） */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = GPIO_PIN_9;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PA10 → USART1_RX（浮空输入） */
    GPIO_InitStruct.Pin  = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* 配置 USART1 中断优先级并使能 */
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  }
}

/**
  * @brief  UART MSP 反初始化
  * @param  huart  UART 句柄指针
  */
void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    __HAL_RCC_USART1_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9 | GPIO_PIN_10);
    HAL_NVIC_DisableIRQ(USART1_IRQn);
  }
}
