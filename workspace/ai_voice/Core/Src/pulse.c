/**
 * @file    pulse.c
 * @brief   脉冲计数驱动 — PB0 外部中断，识别 SU-03T 输出的脉冲命令
 *
 * 接线：
 *   SU-03T A25(引脚13) → STM32 PB0
 *   SU-03T GND(引脚2)  → STM32 GND（共地）
 *
 * 工作原理：
 *   SU-03T 识别到命令词后，在 A25 引脚连续输出 N 个脉冲（高电平脉冲）。
 *   STM32 通过 EXTI0（PB0 上升沿）中断计数，每收到一个脉冲重置 500ms 计时器。
 *   主循环调用 Pulse_GetCmd()，若距离上一个脉冲已超过 500ms，则认为本次
 *   命令结束，返回脉冲总数，计数器清零。
 *
 * SU-03T 平台配置对应关系（A25，输出脉冲，周期 50ms，超时 500ms）：
 *   脉冲次数  命令词
 *     1       播放欢乐颂
 *     2       播放小星星
 *     3       播放马里奥
 *     4       播放两只老虎
 *     5       播放音乐
 *     6       暂停
 *     7       继续
 *     8       停止音乐
 *     9       下一首
 *    10       上一首
 */

#include "pulse.h"

/* 命令超时：最后一个脉冲结束后 500ms 无新脉冲 → 命令完成 */
#define PULSE_TIMEOUT_MS   500U

/* 内部状态（volatile，供中断与主循环共享）*/
static volatile uint8_t  s_pulse_count = 0;   /* 当前已计脉冲数 */
static volatile uint32_t s_last_tick   = 0;   /* 最后一次脉冲的 HAL tick */
static volatile uint8_t  s_has_pulse   = 0;   /* 是否收到过至少一个脉冲 */

/**
 * @brief 初始化 PB0 为上升沿外部中断
 */
void Pulse_Init(void)
{
    /* 1. 使能时钟 */
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* 2. PB0 配置为输入，内部下拉（信号默认低电平）*/
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin  = GPIO_PIN_0;
    gpio.Mode = GPIO_MODE_IT_RISING;   /* 上升沿触发中断 */
    gpio.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOB, &gpio);

    /* 3. 配置 NVIC：EXTI0，优先级 0（高于 SysTick）*/
    HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

/**
 * @brief 外部中断回调（每个上升沿触发一次）
 *        在 stm32f1xx_it.c 的 EXTI0_IRQHandler 中调用
 */
void EXTI0_IRQ_Process(void)
{
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_0)) {
        s_pulse_count++;
        s_last_tick  = HAL_GetTick();
        s_has_pulse  = 1;
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_0);
    }
}

/**
 * @brief 主循环调用，返回完整命令的脉冲数
 * @return 0 = 无完整命令；1~N = 本次命令的脉冲总数
 */
uint8_t Pulse_GetCmd(void)
{
    if (!s_has_pulse) return 0;   /* 还没收到任何脉冲 */

    uint32_t now = HAL_GetTick();

    /* 距上一个脉冲不足 500ms，继续等待 */
    if ((now - s_last_tick) < PULSE_TIMEOUT_MS) return 0;

    /* 超时，命令结束：读取脉冲数并清零 */
    uint8_t count  = s_pulse_count;
    s_pulse_count  = 0;
    s_has_pulse    = 0;
    return count;
}
