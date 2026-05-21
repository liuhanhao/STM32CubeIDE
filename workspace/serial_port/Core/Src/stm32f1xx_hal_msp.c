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

/* USART1 RX 使用 DMA1 通道5 */
static DMA_HandleTypeDef hdma_usart1_rx;

/**
  * @brief  USART MSP 初始化 —— 由 HAL_UART_Init() 自动调用
  */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  if (huart->Instance == USART1)
  {
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* PA9 —— USART1_TX：复用推挽输出 */
    GPIO_InitStruct.Pin   = GPIO_PIN_9;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PA10 —— USART1_RX：浮空输入 */
    GPIO_InitStruct.Pin  = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* DMA1 通道5 —— USART1_RX */
    hdma_usart1_rx.Instance                 = DMA1_Channel5;
    hdma_usart1_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hdma_usart1_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_usart1_rx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_usart1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_usart1_rx.Init.Mode                = DMA_NORMAL;   /* 普通模式，IDLE后手动重启 */
    hdma_usart1_rx.Init.Priority            = DMA_PRIORITY_LOW;
    HAL_DMA_Init(&hdma_usart1_rx);

    /* 关联 DMA 句柄到 UART */
    __HAL_LINKDMA(huart, hdmarx, hdma_usart1_rx);

    /* DMA1 通道5 中断 */
    HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);

    /* USART1 全局中断（用于 IDLE） */
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  }
}

/**
  * @brief  USART MSP 反初始化
  */
void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    __HAL_RCC_USART1_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9 | GPIO_PIN_10);
    HAL_DMA_DeInit(huart->hdmarx);
    HAL_NVIC_DisableIRQ(USART1_IRQn);
    HAL_NVIC_DisableIRQ(DMA1_Channel5_IRQn);
  }
}
