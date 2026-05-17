/**
  ******************************************************************************
  * @file    stm32f1xx_it.c
  * @brief   中断服务程序
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
#include "stm32f1xx_it.h"

/* 引用 main.c 中定义的计数变量 */
extern volatile int16_t Num;

void NMI_Handler(void)
{
  while (1) {}
}

void HardFault_Handler(void)
{
  while (1) {}
}

void MemManage_Handler(void)
{
  while (1) {}
}

void BusFault_Handler(void)
{
  while (1) {}
}

void UsageFault_Handler(void)
{
  while (1) {}
}

void SVC_Handler(void)
{
}

void DebugMon_Handler(void)
{
}

void PendSV_Handler(void)
{
}

void SysTick_Handler(void)
{
  HAL_IncTick();
}

/**
  * @brief  EXTI0 中断服务函数（PB0 = A口，下降沿触发）
  *         A 下降沿时读 B 口：
  *           B = 1 → 顺时针  → Num++
  *           B = 0 → 逆时针  → Num--
  */
void EXTI0_IRQHandler(void)
{
  if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_0))
  {
    /* 简单消抖：等待 A 口稳定 */
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_RESET)
    {
      if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_SET)
      {
        Num++;   /* 顺时针 */
      }
      else
      {
        Num--;   /* 逆时针 */
      }
    }
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_0);
  }
}
