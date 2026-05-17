/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : 舵机按键控制程序（HAL TIM PWM）
  *                   PA0 -> TIM2_CH1 PWM 输出（舵机信号线）
  *                   PB1 -> 按键输入（另一端接 GND，内部上拉）
  *                   每按一次旋转 30°，超过 180° 回到 0°
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
#include "delay.h"

/* TIM2 句柄 */
TIM_HandleTypeDef htim2;

/* 当前舵机角度，范围 0~180，步进 30 */
static uint16_t servo_angle = 0;

/**
  * @brief  将角度转换为 TIM2 比较值（脉宽 us）
  * @note   标准舵机：0° -> 500us，180° -> 2500us
  *         PSC=71, ARR=19999 时 1 tick = 1us
  */
static uint32_t angle_to_compare(uint16_t angle)
{
    return 500U + (uint32_t)angle * 2000U / 180U;
}

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);

/**
  * @brief  应用程序入口
  * @retval int
  */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM2_Init();

    /* 舵机复位到 0°，启动 PWM */
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, angle_to_compare(0));
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);

    while (1)
    {
        /* 检测按键按下（PB1 低电平有效） */
        if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_RESET)
        {
            Delay_ms(20); /* 软件消抖 */

            if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_RESET)
            {
                /* 等待按键释放，避免重复触发 */
                while (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_RESET);
                Delay_ms(20); /* 释放消抖 */

                /* 步进 +30°，超过 180° 回到 0° */
                servo_angle += 30;
                if (servo_angle > 180)
                {
                    servo_angle = 0;
                }

                /* 更新 PWM 脉宽 */
                __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, angle_to_compare(servo_angle));
            }
        }
    }
}

/**
  * @brief  系统时钟配置
  * @retval None
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType  = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState        = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue  = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState        = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState    = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource   = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL      = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief  GPIO 初始化
  * @retval None
  */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 使能 GPIOB、GPIOD 时钟（GPIOA 由 HAL_TIM_PWM_MspInit 中使能） */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    /* PB1：按键输入，内部上拉（按键另一端接 GND） */
    GPIO_InitStruct.Pin  = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/**
  * @brief  TIM2 PWM 初始化
  * @note   系统时钟 72MHz，APB1 Timer 时钟 = 72MHz
  *         PSC=71  -> 定时器时钟 1MHz（1 tick = 1us）
  *         ARR=19999 -> 周期 20ms（50Hz，标准舵机信号）
  * @retval None
  */
static void MX_TIM2_Init(void)
{
    TIM_OC_InitTypeDef sConfigOC = {0};

    htim2.Instance               = TIM2;
    htim2.Init.Prescaler         = 71;
    htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim2.Init.Period            = 19999;
    htim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
    {
        Error_Handler();
    }

    sConfigOC.OCMode     = TIM_OCMODE_PWM1;
    sConfigOC.Pulse      = angle_to_compare(0); /* 初始脉宽 500us（0°） */
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief  错误处理函数
  * @retval None
  */
void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}

#ifdef USE_FULL_ASSERT
/**
  * @brief  断言失败时报告文件名和行号
  * @param  file: 源文件名指针
  * @param  line: 错误所在行号
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
