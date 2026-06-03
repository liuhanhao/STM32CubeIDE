/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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

/*
 * ============================================================
 *  硬件接线说明  —  OLED 天气钟（STM32F103C8T6 核心板）
 * ============================================================
 *
 * 【电源说明】
 *   所有模块均使用 3.3V 供电（STM32 核心板的 3.3V 引脚）。
 *   ESP01S 在发送数据时峰值电流可达 300mA，建议单独由
 *   AMS1117-3.3V 或 LDO 模块供电，共地即可。
 *
 * ============================================================
 *  一、OLED 屏幕（SSD1306，IIC，128×64）
 * ============================================================
 *
 *   OLED 引脚        STM32 引脚
 *   ───────────────────────────
 *   VCC         →   3.3V
 *   GND         →   GND
 *   SCL         →   PB8   （软件 I2C 时钟）
 *   SDA         →   PB9   （软件 I2C 数据）
 *
 *   注：使用软件模拟 I2C，PB8/PB9 配置为开漏输出。
 *       模块通常已内置 4.7kΩ 上拉电阻，无需外接。
 *
 * ============================================================
 *  二、ESP01S（WiFi 模块，AT 固件）
 * ============================================================
 *
 *   ESP01S 引脚     STM32 引脚
 *   ───────────────────────────
 *   VCC         →   3.3V（建议独立 LDO，峰值电流约 300mA）
 *   GND         →   GND
 *   TXD         →   PA10  （USART1_RX）
 *   RXD         →   PA9   （USART1_TX）
 *   CH_PD/EN    →   3.3V  （必须拉高，否则模块不工作）
 *   RST         →   3.3V  （正常工作时拉高；悬空也可）
 *   GPIO0       →   悬空  （正常运行模式；烧录固件时接 GND）
 *   GPIO2       →   悬空
 *
 *   注：ESP01S 的 RXD 引脚为 3.3V 逻辑，STM32 TX 输出 3.3V，
 *       可直接连接，无需电平转换。
 *       波特率：115200 bps，8N1。
 *
 * ============================================================
 *  三、DHT11（温湿度传感器模块，含上拉电阻）
 * ============================================================
 *
 *   DHT11 模块引脚  STM32 引脚
 *   ───────────────────────────
 *   VCC (+)     →   3.3V
 *   GND (-)     →   GND
 *   DATA (OUT)  →   PA0   （单总线数据，可在 dht11.h 中修改）
 *
 *   注：使用的是带 PCB 的三引脚模块，模块已内置上拉电阻，
 *       DATA 引脚无需再外接上拉，STM32 侧配置为浮空输入。
 *       若使用四引脚裸传感器，需在 DATA 与 VCC 之间
 *       串联一个 4.7kΩ 上拉电阻。
 *
 * ============================================================
 *  四、RTC 备用电源（可选，断电保持时间）
 * ============================================================
 *
 *   CR2032 纽扣电池正极  →  STM32 VBAT 引脚
 *   CR2032 纽扣电池负极  →  GND
 *
 *   注：接入 VBAT 后，断电时 RTC 仍可由电池维持计时。
 *       不接则每次上电后时间从 NTP 同步，或从默认值
 *       2000-01-01 00:00:00 开始。
 *
 * ============================================================
 *  五、引脚占用汇总
 * ============================================================
 *
 *   引脚   功能
 *   ─────────────────────────────────────────
 *   PA0    DHT11 数据总线（单总线）
 *   PA9    USART1_TX → ESP01S RXD
 *   PA10   USART1_RX → ESP01S TXD
 *   PB8    OLED SCL（软件 I2C）
 *   PB9    OLED SDA（软件 I2C）
 *   VBAT   RTC 备用电源（可接 CR2032）
 *   3.3V   OLED VCC / DHT11 VCC / ESP01S VCC（建议独立）
 *   GND    所有模块共地
 *
 * ============================================================
 *  六、烧录前必须修改的配置项
 * ============================================================
 *
 *   1. WiFi 凭据（本文件 main() 函数内）：
 *        AT_SendWait("AT+CWJAP=\"YOUR_SSID\",\"YOUR_PASSWORD\"", ...)
 *      将 YOUR_SSID 和 YOUR_PASSWORD 替换为实际 WiFi 名称和密码。
 *
 *   2. 和风天气 API 配置（Core/Src/weather_client.c 顶部）：
 *        #define WEATHER_CITY_ID   "101010100"       // 城市 ID
 *        #define WEATHER_API_KEY   "YOUR_API_KEY_HERE" // API Key
 *      城市 ID 查询：https://weather.qweather.com/
 *      API Key 申请：https://dev.qweather.com/
 *
 *   3. DHT11 数据引脚（Core/Inc/dht11.h）：
 *        #define DHT11_GPIO_PORT  GPIOA
 *        #define DHT11_GPIO_PIN   GPIO_PIN_0
 *      如需更换引脚，在此修改即可。
 *
 * ============================================================
 */
#include "main.h"
#include "OLED.h"
#include "app_globals.h"
#include "dht11.h"
#include "at_controller.h"
#include "ntp_client.h"
#include "weather_client.h"
#include "display_manager.h"

/* ------------------------------------------------------------------ */
/*  全局变量定义                                                         */
/* ------------------------------------------------------------------ */
RTC_HandleTypeDef        hrtc;
UART_HandleTypeDef       huart1;
volatile WiFiStatus_t    g_wifi_status = WIFI_DISCONNECTED;
DHT11Data_t              g_dht11_data  = {0};
/* g_weather_data 在 weather_client.c 中定义，此处通过 weather_client.h 引入 extern 声明 */

/* ------------------------------------------------------------------ */
/*  函数前向声明                                                         */
/* ------------------------------------------------------------------ */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_RTC_Init(void);
static void MX_USART1_UART_Init(void);

/**
  * @brief  应用程序入口
  * @retval int
  */
int main(void)
{
    /* 硬件初始化 */
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_RTC_Init();
    MX_USART1_UART_Init();

    /* 外设初始化 */
    OLED_Init();
    Display_ShowBoot("Booting...");   /* 需求 1.2 */
    HAL_Delay(800);

    AT_Init();       /* 初始化 AT 缓冲区，开启 RXNE 中断 */
    DHT11_Init();    /* 初始化 DHT11 引脚 */

    /* ---- ESP01S 版本号 1.7.4.0 上电等待 + AT 握手 ---- */
    /* ESP01S 上电后需要约 1~2 秒完成自检，期间会输出启动日志 */
    HAL_Delay(2000);

    /* 发送 AT 确认模块就绪，最多重试 5 次 */
    {
        uint8_t at_ok = 0;
        for (uint8_t i = 0; i < 5; i++)
        {
            if (AT_SendWait("AT", "OK", 1000))
            {
                at_ok = 1;
                break;
            }
            HAL_Delay(500);
        }
        if (!at_ok)
        {
            /* 模块无响应：检查接线（TX/RX 是否接反、CH_PD 是否接 3.3V） */
            Display_ShowBoot("ESP No Resp");
            HAL_Delay(3000);
            /* 继续尝试，不阻塞 */
        }
    }

    /* 设置 Station 模式（防止模块处于 AP 或 AP+STA 模式） */
    AT_SendWait("AT+CWMODE=1", "OK", 2000);
    HAL_Delay(500);

    /* ---- WiFi 连接 ---- */
    Display_ShowBoot("WiFi...");
    /* ===== 用户需修改这里的 SSID 和 PASSWORD ===== */
    /* 等待 WIFI GOT IP，确保 IP 地址已分配，才能进行网络请求 */
    if (!AT_SendWait("AT+CWJAP=\"TP-LINK_1202\",\"liu88567878\"", "WIFI GOT IP", 20000))
    {
        /* WiFi 连接失败 */
        Display_ShowBoot("WiFi Fail");
        HAL_Delay(2000);
        /* g_wifi_status 保持 WIFI_DISCONNECTED */
    }
    else
    {
        Display_ShowBoot("WiFi OK");
        /* 等待网络栈完全就绪，避免 AT+CIPSTART 过早发送失败 */
        HAL_Delay(1000);
        g_wifi_status = WIFI_CONNECTED;

        /* ---- NTP 时间同步 ---- */
        Display_ShowBoot("NTP...");
        if (NTP_Sync())
        {
            Display_ShowBoot("NTP OK");
        }
        else
        {
            Display_ShowBoot("NTP Fail");
        }
        HAL_Delay(800);

        /* ---- 天气数据获取 ---- */
        Display_ShowBoot("Weather...");
        if (Weather_Fetch())
        {
            Display_ShowBoot("Weather OK");
        }
        else
        {
            Display_ShowBoot("No Data");
        }
        HAL_Delay(800);
    }

    Display_Init();         /* 进入分页显示 */

    /* 主循环时间片计数 */
    uint32_t dht11_tick   = 0;
    uint32_t weather_tick = HAL_GetTick();  /* 初始化为当前时刻，避免启动后立即重复请求 */
    uint8_t  dht11_fail_count = 0;

    while (1)
    {
        /* DHT11 每 2 秒采集一次（需求 5.1） */
        if (HAL_GetTick() - dht11_tick >= 2000U)
        {
            dht11_tick = HAL_GetTick();
            if (DHT11_Read(&g_dht11_data))
            {
                dht11_fail_count = 0;          /* 读取成功，重置失败计数 */
            }
            else
            {
                if (++dht11_fail_count >= 3U)  /* 连续 3 次失败（需求 5.3） */
                {
                    g_dht11_data.valid = 0;
                }
            }
        }

        /* 天气每 30 分钟刷新一次（需求 4.4） */
        if (g_wifi_status == WIFI_CONNECTED &&
            HAL_GetTick() - weather_tick >= 1800000UL)
        {
            weather_tick = HAL_GetTick();
            Weather_Fetch();
        }

        /* 检查并刷新分页显示（需求 6.1） */
        Display_Update();
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

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
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
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
}

/**
  * @brief  RTC 初始化，使用 LSE 32.768kHz，设置默认时间 2000-01-01 00:00:00
  * @retval None
  */
static void MX_RTC_Init(void)
{
  hrtc.Instance        = RTC;
  hrtc.Init.AsynchPrediv = RTC_AUTO_1_SECOND;
  hrtc.Init.OutPut     = RTC_OUTPUTSOURCE_NONE;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    /* 初始化失败时不阻塞，让上层处理 */
    return;
  }

  /* 设置默认时间 2000-01-01 00:00:00 */
  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};
  sTime.Hours   = 0; sTime.Minutes = 0; sTime.Seconds = 0;
  sDate.Year    = 0; sDate.Month   = 1; sDate.Date    = 1;
  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
  HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
  HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
}

/**
  * @brief  USART1 初始化，115200/8N1，开启 RXNE 接收中断
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{
  huart1.Instance          = USART1;
  huart1.Init.BaudRate     = 115200;
  huart1.Init.WordLength   = UART_WORDLENGTH_8B;
  huart1.Init.StopBits     = UART_STOPBITS_1;
  huart1.Init.Parity       = UART_PARITY_NONE;
  huart1.Init.Mode         = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  HAL_UART_Init(&huart1);
  __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);
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
