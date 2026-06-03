/**
  ******************************************************************************
  * @file    delay.c
  * @brief   基于 SysTick 的延时函数实现
  *
  * HAL 已将 SysTick 配置为 1ms 中断、向下计数。
  * 系统时钟 72MHz，SysTick LOAD 值 = 71999，即每个 tick = 1/72 us。
  *
  * Delay_us 通过直接读取 SysTick->VAL 计数器实现，不依赖中断，
  * 精度可达单微秒级别。
  ******************************************************************************
  */
#include "delay.h"

/* 每微秒对应的 SysTick 计数个数 = 系统时钟(Hz) / 1000000 */
#define TICKS_PER_US  (SystemCoreClock / 1000000U)

/**
  * @brief  微秒级延时
  * @param  us: 延时时间（微秒），最大值约 4294967295 us
  * @note   通过轮询 SysTick->VAL 实现，关中断期间仍可用
  */
void Delay_us(uint32_t us)
{
    uint32_t ticks = us * TICKS_PER_US;
    uint32_t reload = SysTick->LOAD;
    uint32_t start  = SysTick->VAL;

    while (ticks > 0)
    {
        uint32_t now = SysTick->VAL;

        /* 处理计数器从 0 回绕到 LOAD 的情况 */
        uint32_t elapsed = (start >= now) ? (start - now) : (reload - now + start);

        if (elapsed >= ticks)
        {
            break;
        }
        ticks -= elapsed;
        start  = now;
    }
}

/**
  * @brief  毫秒级延时
  * @param  ms: 延时时间（毫秒）
  */
void Delay_ms(uint32_t ms)
{
    while (ms--)
    {
        Delay_us(1000);
    }
}

/**
  * @brief  秒级延时
  * @param  s: 延时时间（秒）
  */
void Delay_s(uint32_t s)
{
    while (s--)
    {
        Delay_ms(1000);
    }
}
