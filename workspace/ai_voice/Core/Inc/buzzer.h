#ifndef __BUZZER_H
#define __BUZZER_H

#include "stm32f1xx_hal.h"

/**
 * @brief 初始化蜂鸣器（PA0 → TIM2_CH1 PWM 输出）
 */
void Buzzer_Init(void);

/**
 * @brief 设置蜂鸣器发声频率
 * @param freq_hz  频率（Hz），传 0 表示静音
 */
void Buzzer_SetFreq(uint32_t freq_hz);

#endif /* __BUZZER_H */
