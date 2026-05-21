/**
  ******************************************************************************
  * @file    buzzer.c
  * @brief   无源蜂鸣器软件 PWM 驱动（PA7）
  *
  * 通过软件翻转 GPIO 产生方波，驱动无源蜂鸣器发声。
  * 每发出一个方波周期检查一次按键，支持中途打断。
  ******************************************************************************
  */
#include "buzzer.h"
#include "delay.h"

/* 按键引脚：PB1，低电平有效（内部上拉） */
#define KEY_GPIO_PORT   GPIOB
#define KEY_GPIO_PIN    GPIO_PIN_1

/* 外部标志：1 = 检测到按键，在 main 中清零 */
volatile uint8_t g_key_pressed = 0;

/**
  * @brief  初始化蜂鸣器引脚（PA7 推挽输出）
  */
void Buzzer_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* 初始低电平，蜂鸣器静音 */
    HAL_GPIO_WritePin(BUZZER_GPIO_PORT, BUZZER_GPIO_PIN, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin   = BUZZER_GPIO_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;   /* 高速，确保高频翻转 */
    HAL_GPIO_Init(BUZZER_GPIO_PORT, &GPIO_InitStruct);
}

/**
  * @brief  发出指定频率和时长的音调，期间检测按键
  * @param  freq_hz     : 音调频率（Hz），0 = 休止符
  * @param  duration_ms : 持续时间（毫秒）
  * @note   如果按键被按下，g_key_pressed 置 1，函数立即返回
  */
void Buzzer_Tone(uint32_t freq_hz, uint32_t duration_ms)
{
    if (freq_hz == 0)
    {
        /* 休止符：静音等待，同时检测按键 */
        HAL_GPIO_WritePin(BUZZER_GPIO_PORT, BUZZER_GPIO_PIN, GPIO_PIN_RESET);
        uint32_t start = HAL_GetTick();
        while ((HAL_GetTick() - start) < duration_ms)
        {
            if (HAL_GPIO_ReadPin(KEY_GPIO_PORT, KEY_GPIO_PIN) == GPIO_PIN_RESET)
            {
                Delay_ms(20); /* 消抖 */
                if (HAL_GPIO_ReadPin(KEY_GPIO_PORT, KEY_GPIO_PIN) == GPIO_PIN_RESET)
                {
                    g_key_pressed = 1;
                    /* 等待按键释放 */
                    while (HAL_GPIO_ReadPin(KEY_GPIO_PORT, KEY_GPIO_PIN) == GPIO_PIN_RESET);
                    return;
                }
            }
        }
        return;
    }

    /* 半周期时间（微秒）= 500000 / freq */
    uint32_t half_us = 500000U / freq_hz;

    /* 总共需要翻转的次数 = duration_ms * 1000 / half_us */
    uint32_t total_toggles = (duration_ms * 1000U) / half_us;

    for (uint32_t i = 0; i < total_toggles; i++)
    {
        HAL_GPIO_TogglePin(BUZZER_GPIO_PORT, BUZZER_GPIO_PIN);
        Delay_us(half_us);

        /* 每个半周期检测一次按键（低电平有效） */
        if ((i & 0x3F) == 0)  /* 每 64 次才读一次 GPIO，减少影响音质 */
        {
            if (HAL_GPIO_ReadPin(KEY_GPIO_PORT, KEY_GPIO_PIN) == GPIO_PIN_RESET)
            {
                Delay_ms(20); /* 消抖 */
                if (HAL_GPIO_ReadPin(KEY_GPIO_PORT, KEY_GPIO_PIN) == GPIO_PIN_RESET)
                {
                    g_key_pressed = 1;
                    Buzzer_Stop();
                    /* 等待按键释放 */
                    while (HAL_GPIO_ReadPin(KEY_GPIO_PORT, KEY_GPIO_PIN) == GPIO_PIN_RESET);
                    return;
                }
            }
        }
    }

    Buzzer_Stop();
}

/**
  * @brief  关闭蜂鸣器（输出低电平）
  */
void Buzzer_Stop(void)
{
    HAL_GPIO_WritePin(BUZZER_GPIO_PORT, BUZZER_GPIO_PIN, GPIO_PIN_RESET);
}
