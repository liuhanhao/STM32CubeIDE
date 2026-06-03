/**
 * @file    buzzer.c
 * @brief   无源蜂鸣器驱动 — PA0 → TIM2_CH1 PWM
 *
 * 原理：通过改变 TIM2_CH1 的 ARR（自动重装载值）来改变 PWM 频率，
 *       占空比固定 50% 驱动无源蜂鸣器发声。
 *       系统时钟 72MHz，TIM2 预分频 71，计数时钟 = 1MHz。
 *       ARR = 1,000,000 / freq_hz - 1
 */

#include "buzzer.h"

static TIM_HandleTypeDef s_tim2;

/**
 * @brief 初始化蜂鸣器
 *        上电默认 PA0 输出低电平（静音），发声时再切换到 AF_PP 模式
 */
void Buzzer_Init(void)
{
    /* 1. 使能时钟 */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_TIM2_CLK_ENABLE();

    /* 2. PA0 先配置为普通推挽输出并拉低，确保上电无电流声 */
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin   = GPIO_PIN_0;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    gpio.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);

    /* 3. TIM2 基础配置：预分频 71 → 计数时钟 1MHz，不启动输出 */
    s_tim2.Instance               = TIM2;
    s_tim2.Init.Prescaler         = 71;
    s_tim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
    s_tim2.Init.Period            = 999;
    s_tim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    s_tim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(&s_tim2);

    /* 4. CH1 PWM 配置（只配置，不启动）*/
    TIM_OC_InitTypeDef oc = {0};
    oc.OCMode     = TIM_OCMODE_PWM1;
    oc.Pulse      = 500;
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&s_tim2, &oc, TIM_CHANNEL_1);
    /* 注意：此处不调用 HAL_TIM_PWM_Start，PA0 保持低电平静音 */

    /* 初始化完成后关闭 TIM2 时钟，避免空闲时外设内部翻转产生 EMI */
    __HAL_RCC_TIM2_CLK_DISABLE();
}

/**
 * @brief 设置发声频率
 * @param freq_hz  0 = 静音，>0 = 对应频率
 */
void Buzzer_SetFreq(uint32_t freq_hz)
{
    GPIO_InitTypeDef gpio = {0};

    if (freq_hz == 0) {
        /* 静音：停止 PWM，PA0 切回普通推挽并拉低 */
        HAL_TIM_PWM_Stop(&s_tim2, TIM_CHANNEL_1);

        gpio.Pin   = GPIO_PIN_0;
        gpio.Mode  = GPIO_MODE_OUTPUT_PP;   /* 脱离 TIM 控制 */
        gpio.Speed = GPIO_SPEED_FREQ_LOW;
        gpio.Pull  = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &gpio);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);  /* 强制拉低 */

        /* 彻底关闭 TIM2 时钟，消除外设内部开关噪声对线圈的 EMI 耦合 */
        __HAL_RCC_TIM2_CLK_DISABLE();
        return;
    }

    /* 防止溢出保护：超出可用范围的频率直接忽略 */
    if (freq_hz < 20 || freq_hz > 20000) {
        return;
    }

    /* 重新使能 TIM2 时钟（寄存器内容保留，无需完整重初始化）*/
    __HAL_RCC_TIM2_CLK_ENABLE();

    /* 将 PA0 切回复用推挽模式，交还给 TIM2 控制 */
    gpio.Pin   = GPIO_PIN_0;
    gpio.Mode  = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);

    /* ARR = 1,000,000 / freq - 1 */
    uint32_t arr = (1000000UL / freq_hz) - 1;

    /* 更新 ARR 和 CCR1（50% 占空比）*/
    __HAL_TIM_SET_AUTORELOAD(&s_tim2, arr);
    __HAL_TIM_SET_COMPARE(&s_tim2, TIM_CHANNEL_1, arr / 2);

    /* 启动 PWM 输出 */
    HAL_TIM_PWM_Start(&s_tim2, TIM_CHANNEL_1);
}
