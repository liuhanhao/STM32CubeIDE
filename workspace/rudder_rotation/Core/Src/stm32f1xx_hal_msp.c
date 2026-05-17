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
  * @brief  TIM2 PWM MSP 初始化
  * @note   HAL_TIM_PWM_Init 会自动调用此回调
  *         在此使能 TIM2 时钟，并将 PA0 配置为复用推挽输出（TIM2_CH1）
  */
void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (htim->Instance == TIM2)
    {
        /* 使能 TIM2 和 GPIOA 时钟 */
        __HAL_RCC_TIM2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /* PA0 -> TIM2_CH1，复用推挽输出 */
        GPIO_InitStruct.Pin   = GPIO_PIN_0;
        GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}
