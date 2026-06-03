#ifndef __PULSE_H
#define __PULSE_H

#include "stm32f1xx_hal.h"

/**
 * @brief 初始化脉冲计数器
 *        PB0 配置为上升沿外部中断（EXTI0）
 *        SU-03T A25(引脚13) → STM32 PB0
 */
void Pulse_Init(void);

/**
 * @brief 主循环中调用，检测是否有完整命令到达
 *        原理：收到最后一个脉冲后 300ms 内无新脉冲，则认为命令结束
 * @return 0 = 无新命令；1~10 = 脉冲个数（对应命令编号）
 */
uint8_t Pulse_GetCmd(void);

/* 供 stm32f1xx_it.c 中 EXTI0_IRQHandler 调用 */
void EXTI0_IRQ_Process(void);

#endif /* __PULSE_H */
