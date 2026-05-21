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

/* IDLE 处理函数在 main.c 中定义 */
extern void UART1_IDLE_Handler(void);

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
  * @brief  USART1 全局中断服务程序
  *         处理 DMA 接收完成 和 IDLE 空闲中断
  */
void USART1_IRQHandler(void)
{
  /* 先让 HAL 处理 DMA 相关中断 */
  HAL_UART_IRQHandler(&huart1);
  /* 再处理 IDLE 空闲中断（HAL 不自动处理） */
  UART1_IDLE_Handler();
}

/**
  * @brief  DMA1 通道5 中断（USART1_RX）
  */
void DMA1_Channel5_IRQHandler(void)
{
  HAL_DMA_IRQHandler(huart1.hdmarx);
}
