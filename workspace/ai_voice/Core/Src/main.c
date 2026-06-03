/**
 ******************************************************************************
 * @file     main.c
 * @brief    语音控制蜂鸣器音乐播放器
 *
 * 硬件接线：
 *
 *  【无源蜂鸣器】
 *   STM32 PA0        → 蜂鸣器正极（TIM2_CH1 PWM）
 *   蜂鸣器负极        → GND
 *
 *  【SU-03T 语音模块】
 *   SU-03T VCC(引脚1)  → 手机充电头 5V（Micro USB 供电）
 *   SU-03T GND(引脚2)  → STM32 任意 GND（两板共地，必接）
 *   SU-03T A25(引脚13) → STM32 PB0（脉冲信号输入）
 *   其余引脚            → 悬空（不接）
 *
 *  【OLED（软件 I2C）】
 *   OLED SCL         → STM32 PB8
 *   OLED SDA         → STM32 PB9
 *   OLED VCC         → 3.3V
 *   OLED GND         → GND
 *
 * SU-03T 平台配置说明（smartpi.cn）：
 *   唤醒词：你好小智 / 小智精灵
 *   引脚：A25，控制方式「端口输出」，动作「输出脉冲」，周期 50ms
 *
 *   命令词         → 脉冲次数
 *   播放欢乐颂     →  1
 *   播放小星星     →  2
 *   播放马里奥     →  3
 *   播放两只老虎   →  4
 *   播放音乐       →  5
 *   暂停           →  6
 *   继续           →  7
 *   停止音乐       →  8
 *   下一首         →  9
 *   上一首         → 10
 *
 * 识别原理：
 *   SU-03T 识别命令后在 A25 输出 N 个脉冲，STM32 PB0 外部中断计数，
 *   最后一个脉冲结束 500ms 内无新脉冲则判定命令完成，根据脉冲数执行对应操作。
 ******************************************************************************
 */

#include "main.h"
#include "OLED.h"
#include "buzzer.h"
#include "pulse.h"
#include "player.h"
#include "songs.h"

/* 函数声明 */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void UI_Update(void);

/* ------------------------------------------------------------------ */
/*  脉冲命令编号定义（与 SU-03T 平台配置脉冲次数对应）                        */
/* ------------------------------------------------------------------ */
#define CMD_PLAY_ODE      1    /* 播放欢乐颂   → 1 个脉冲 */
#define CMD_PLAY_STAR     2    /* 播放小星星   → 2 个脉冲 */
#define CMD_PLAY_MARIO    3    /* 播放马里奥   → 3 个脉冲 */
#define CMD_PLAY_TIGERS   4    /* 播放两只老虎 → 4 个脉冲 */
#define CMD_PLAY          5    /* 播放音乐     → 5 个脉冲 */
#define CMD_PAUSE         6    /* 暂停         → 6 个脉冲 */
#define CMD_RESUME        7    /* 继续         → 7 个脉冲 */
#define CMD_STOP          8    /* 停止音乐     → 8 个脉冲 */
#define CMD_NEXT          9    /* 下一首       → 9 个脉冲 */
#define CMD_PREV          10   /* 上一首       → 10 个脉冲 */

/* ------------------------------------------------------------------ */
/*  主程序入口                                                            */
/* ------------------------------------------------------------------ */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    /* 外设初始化 */
    OLED_Init();
    Buzzer_Init();
    Pulse_Init();
    Player_Init();

    /* 开机画面 */
    OLED_Clear();
    OLED_ShowString(1, 1, "Voice  Player");
    OLED_ShowString(2, 1, "Say: ni hao");
    OLED_ShowString(3, 1, "xiao zhi");
    OLED_ShowString(4, 1, "to wake up~");
    HAL_Delay(1500);

    /* 显示待机界面 */
    UI_Update();

    /* 主循环 */
    while (1)
    {
        /* 推进播放器（非阻塞） */
        Player_Tick();

        /* 检查是否有完整脉冲命令 */
        uint8_t cmd = Pulse_GetCmd();
        if (cmd > 0)
        {
            switch (cmd)
            {
                case CMD_PLAY_ODE:
                    Player_Play(SONG_ODE_TO_JOY);
                    break;
                case CMD_PLAY_STAR:
                    Player_Play(SONG_LITTLE_STAR);
                    break;
                case CMD_PLAY_MARIO:
                    Player_Play(SONG_MARIO);
                    break;
                case CMD_PLAY_TIGERS:
                    Player_Play(SONG_TWO_TIGERS);
                    break;
                case CMD_PLAY:
                    /* 若暂停中则继续，否则重新播放当前歌曲 */
                    if (Player_GetState() == PLAYER_PAUSED) {
                        Player_Resume();
                    } else {
                        Player_Play(Player_GetSongIdx());
                    }
                    break;
                case CMD_PAUSE:
                    Player_Pause();
                    break;
                case CMD_RESUME:
                    Player_Resume();
                    break;
                case CMD_STOP:
                    Player_Stop();
                    break;
                case CMD_NEXT:
                    Player_Next();
                    break;
                case CMD_PREV:
                    Player_Prev();
                    break;
                default:
                    /* 未知脉冲数，忽略 */
                    break;
            }

            UI_Update();
        }

        /* 播放结束时自动刷新界面 */
        static PlayerState s_last_state = PLAYER_STOPPED;
        PlayerState cur_state = Player_GetState();
        if (cur_state != s_last_state) {
            s_last_state = cur_state;
            UI_Update();
        }
    }
}

/* ------------------------------------------------------------------ */
/*  OLED 界面刷新                                                         */
/* ------------------------------------------------------------------ */
static void UI_Update(void)
{
    OLED_Clear();

    PlayerState state    = Player_GetState();
    uint8_t     song_idx = Player_GetSongIdx();

    /* Line 1：歌曲名 */
    OLED_ShowString(1, 1, (char *)g_songs[song_idx].name);

    /* Line 2：播放状态 */
    switch (state) {
        case PLAYER_PLAYING:
            OLED_ShowString(2, 1, ">> Playing...");
            break;
        case PLAYER_PAUSED:
            OLED_ShowString(2, 1, "|| Paused");
            break;
        case PLAYER_STOPPED:
        default:
            OLED_ShowString(2, 1, "[] Stopped");
            break;
    }

    /* Line 3：歌曲序号 */
    OLED_ShowString(3, 1, "Song ");
    OLED_ShowNum(3, 6, song_idx + 1, 1);
    OLED_ShowString(3, 7, "/");
    OLED_ShowNum(3, 8, SONG_COUNT, 1);

    /* Line 4：操作提示 */
    if (state == PLAYER_PLAYING) {
        OLED_ShowString(4, 1, "Say: pause/next");
    } else if (state == PLAYER_PAUSED) {
        OLED_ShowString(4, 1, "Say: jixu/stop");
    } else {
        OLED_ShowString(4, 1, "Say song name");
    }
}

/* ------------------------------------------------------------------ */
/*  系统时钟配置（72MHz，HSE 8MHz + PLL×9）                               */
/* ------------------------------------------------------------------ */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState            = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue      = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL          = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}

/* ------------------------------------------------------------------ */
/*  GPIO 时钟使能                                                         */
/* ------------------------------------------------------------------ */
static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
}

/* ------------------------------------------------------------------ */
/*  错误处理                                                              */
/* ------------------------------------------------------------------ */
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
