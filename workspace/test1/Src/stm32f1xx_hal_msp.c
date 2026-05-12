/**
  ******************************************************************************
  * @file    stm32f1xx_hal_msp.c
  * @brief   HAL 库的 MSP（MCU Support Package）配置文件，用于配置外设的底层初始化
  ******************************************************************************
  */

#include "main.h"

/**
  * @brief 初始化全局 MSP
  */
void HAL_MspInit(void)
{
  __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();

  /* 系统中断初始化 */
  /* MemoryManagement_IRQn 中断配置 */
  HAL_NVIC_SetPriority(MemoryManagement_IRQn, 0, 0);
  /* BusFault_IRQn 中断配置 */
  HAL_NVIC_SetPriority(BusFault_IRQn, 0, 0);
  /* UsageFault_IRQn 中断配置 */
  HAL_NVIC_SetPriority(UsageFault_IRQn, 0, 0);
  /* SVCall_IRQn 中断配置 */
  HAL_NVIC_SetPriority(SVCall_IRQn, 0, 0);
  /* DebugMonitor_IRQn 中断配置 */
  HAL_NVIC_SetPriority(DebugMonitor_IRQn, 0, 0);
  /* PendSV_IRQn 中断配置 */
  HAL_NVIC_SetPriority(PendSV_IRQn, 0, 0);
  /* SysTick_IRQn 中断配置 */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}
