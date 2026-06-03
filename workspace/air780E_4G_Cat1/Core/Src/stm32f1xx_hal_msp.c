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
