/**
  ******************************************************************************
  * @file    stm32f1xx_it.c
  * @brief   中断服务函数头文件，用于处理各种中断
  ******************************************************************************
  */

#include "main.h"
#include "stm32f1xx_it.h"

/******************************************************************************/
/*           Cortex-M3 处理器中断和异常处理函数                                  */
/******************************************************************************/

/**
  * @brief 不可屏蔽中断
  */
void NMI_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief 硬件错误中断
  */
void HardFault_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief 内存管理中断
  */
void MemManage_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief 总线错误中断
  */
void BusFault_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief 用法错误中断
  */
void UsageFault_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief SVC 中断
  */
void SVC_Handler(void)
{
}

/**
  * @brief 调试监视器中断
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief PendSV 中断
  */
void PendSV_Handler(void)
{
}

/**
  * @brief 系统滴答定时器中断
  */
void SysTick_Handler(void)
{
  HAL_IncTick();
}

/******************************************************************************/
/* STM32F1xx 外设中断处理函数                                                   */
/******************************************************************************/

/* 在这里添加你的外设中断处理函数 */
