#ifndef __BUZZER_H
#define __BUZZER_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

/**
  * @brief  无源蜂鸣器驱动（软件 PWM，PA7）
  *
  * 原理：通过 GPIO 翻转 + 微秒延时模拟 PWM 方波。
  * 频率由半周期时间决定：half_period_us = 500000 / freq
  */

/* 蜂鸣器引脚定义
 * 注意：PB8/PB9 已被 OLED I2C 占用，蜂鸣器使用 PA7 */
#define BUZZER_GPIO_PORT   GPIOA
#define BUZZER_GPIO_PIN    GPIO_PIN_7

void Buzzer_Init(void);
void Buzzer_Tone(uint32_t freq_hz, uint32_t duration_ms);
void Buzzer_Stop(void);

#endif /* __BUZZER_H */
