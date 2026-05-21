/**
  ******************************************************************************
  * @file    main.c
  * @brief   八音盒主程序（含 OLED 显示）
  *
  * 功能：
  *   - OLED 实时显示当前播放的歌曲编号和名称
  *   - 顺序播放 NUM_SONGS 首歌，一首播完自动切换下一首
  *   - 按下 PB1 按键立即跳到下一首
  *   - 播完最后一首后循环回第一首
  *
  * 接线：
  *   - 无源蜂鸣器：正极(VCC) → 3.3V，负极(GND) → GND，信号(I/O) → PA7
  *   - 按键一端        → PB1，另一端 → GND（内部上拉）
  *   - OLED SCL        → PB8
  *   - OLED SDA        → PB9
  *   - OLED VCC        → 3.3V，GND → GND
  ******************************************************************************
  */
#include "main.h"
#include "delay.h"
#include "buzzer.h"
#include "songs.h"
#include "OLED.h"

/* 按键检测标志，在 buzzer.c 中被置位 */
extern volatile uint8_t g_key_pressed;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);

/**
  * @brief  刷新 OLED 显示当前歌曲信息
  * @param  song_idx  当前歌曲索引（0 起）
  */
static void Display_SongInfo(uint8_t song_idx)
{
    /* 第 1 行：显示"Now Playing:" */
    OLED_ShowString(1, 1, "Now Playing:");

    /* 第 2 行：显示歌曲编号，例如 "Song: 1 / 2" */
    OLED_ShowString(2, 1, "Song:   /  ");
    OLED_ShowNum(2, 7, song_idx + 1, 1);    /* 当前编号（1 起）*/
    OLED_ShowNum(2, 9, NUM_SONGS,    1);    /* 总数 */

    /* 第 3 行：清空后显示歌曲名称 */
    OLED_ShowString(3, 1, "                ");  /* 先清行 */
    OLED_ShowString(3, 1, (char *)song_names[song_idx]);

    /* 第 4 行：显示操作提示 */
    OLED_ShowString(4, 1, "KEY:next song");
}

/**
  * @brief  应用程序入口
  */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    /* 初始化 OLED（内部会初始化 PB8/PB9） */
    OLED_Init();

    /* 初始化蜂鸣器（PA7） */
    Buzzer_Init();

    uint8_t current_song = 0;   /* 当前播放的歌曲索引 */

    /* 显示第一首歌曲信息 */
    Display_SongInfo(current_song);

    while (1)
    {
        const Note   *song = song_list[current_song];
        uint16_t      len  = song_list_len[current_song];

        /* 逐音符播放 */
        for (uint16_t i = 0; i < len; i++)
        {
            g_key_pressed = 0;

            Buzzer_Tone(song[i].freq, song[i].duration);

            /* 音符间隙 */
            if (g_key_pressed == 0)
            {
                Delay_ms(30);
            }

            /* 按键被按下，立即退出本首歌 */
            if (g_key_pressed)
            {
                g_key_pressed = 0;
                break;
            }
        }

        Buzzer_Stop();

        /* 播完或跳过：切换到下一首 */
        current_song = (current_song + 1) % NUM_SONGS;

        /* 更新 OLED 显示 */
        Display_SongInfo(current_song);

        /* 歌曲间隙 0.5 秒 */
        Delay_ms(500);
    }
}

/**
  * @brief  系统时钟配置（HSE 8MHz × PLL 9 = 72MHz）
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
    RCC_OscInitStruct.PLL.PLLMUL     = RCC_PLL_MUL9;
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
  *         PB1  - 按键，输入上拉（低电平有效）
  *         PA7  - 蜂鸣器，由 Buzzer_Init() 初始化
  *         PB8  - OLED SCL，由 OLED_Init() 初始化
  *         PB9  - OLED SDA，由 OLED_Init() 初始化
  */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    /* PB1：按键，上拉输入，低电平触发 */
    GPIO_InitStruct.Pin  = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/**
  * @brief  错误处理
  */
void Error_Handler(void)
{
    __disable_irq();
    while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif
