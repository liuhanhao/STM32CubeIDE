/**
 * @file    display_manager.c
 * @brief   OLED 分页显示管理器实现
 *          维护页面切换状态机，按固定时间间隔依次切换
 *          时钟页（PAGE_CLOCK）→ 天气页（PAGE_WEATHER）→ 温湿度页（PAGE_DHT11）循环。
 */

#include <stdio.h>
#include "stm32f1xx_hal.h"
#include "display_manager.h"
#include "app_globals.h"
#include "OLED.h"
#include "weather_icons.h"

/* ------------------------------------------------------------------ */
/*  外部变量声明                                                        */
/* ------------------------------------------------------------------ */

/* RTC 句柄，在 main.c 中定义 */
extern RTC_HandleTypeDef hrtc;

/* 全局天气数据缓存，在 weather_client.c 中定义 */
extern WeatherData_t g_weather_data;

/* DHT11 温湿度数据缓存，在 main.c 中定义 */
extern DHT11Data_t g_dht11_data;

/* ------------------------------------------------------------------ */
/*  模块内部状态                                                        */
/* ------------------------------------------------------------------ */

static DisplayPage_t s_current_page = PAGE_CLOCK;
static uint32_t      s_page_tick    = 0;

/* ------------------------------------------------------------------ */
/*  内部页面绘制函数（static）                                          */
/* ------------------------------------------------------------------ */

/**
 * @brief  绘制时钟页面
 *         第1行显示 HH:MM:SS，第3行显示 20YY-MM-DD
 *         若 RTC 读取失败则显示占位符
 */
static void display_page_clock(void)
{
    RTC_TimeTypeDef t;
    RTC_DateTypeDef d;
    char buf[17];

    if (HAL_RTC_GetTime(&hrtc, &t, RTC_FORMAT_BIN) == HAL_OK &&
        HAL_RTC_GetDate(&hrtc, &d, RTC_FORMAT_BIN) == HAL_OK)
    {
        /* 第1行：HH:MM:SS */
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
                 t.Hours, t.Minutes, t.Seconds);
        OLED_ShowString(1, 1, buf);

        /* 第3行：20YY-MM-DD */
        snprintf(buf, sizeof(buf), "20%02d-%02d-%02d",
                 d.Year, d.Month, d.Date);
        OLED_ShowString(3, 1, buf);
    }
    else
    {
        OLED_ShowString(1, 1, "--:--:--");
        OLED_ShowString(3, 1, "----/--/--");
    }
}

/**
 * @brief  绘制天气页面
 *         左侧 16×16 天气图标（列 0~15，页 0~1）
 *         右侧文字：
 *           第1行 col 3~16：天气描述（最多 9 字符，避免与图标重叠）
 *           第2行 col 3~16：Hi:XXC Lo:XXC
 */
static void display_page_weather(void)
{
    char buf[17];
    WeatherIconIndex_t idx;

    /* ---- 左侧：16×16 图标（page 0, col 0） ---- */
    idx = weather_icon_index(g_weather_data.icon);
    OLED_ShowImage(0, 0, 16, 2, WEATHER_ICONS[idx]);

    /* ---- 右侧第1行（Line 1, col 3 起）：天气描述，最多 9 字符 ---- */
    /* OLED_ShowString 的 Column 参数是字符列（1~16），每字符 8px   */
    /* 图标占 16px = 2 字符列，从第 3 字符列开始，可用 14 字符      */
    /* 但为了美观，截断到 9 字符（留右边距）                         */
    snprintf(buf, sizeof(buf), "%-9.9s", g_weather_data.text);
    OLED_ShowString(1, 3, buf);

    /* ---- 右侧第2行（Line 2, col 3 起）：高低温 ---- */
    snprintf(buf, sizeof(buf), "%3d/%3dC",
             g_weather_data.tempMax, g_weather_data.tempMin);
    OLED_ShowString(2, 3, buf);
}

/**
 * @brief  绘制天气详情页面
 *         第1行：降水概率
 *         第2行：风向风力
 *         第3行：湿度
 */
static void display_page_weather_detail(void)
{
    char buf[17];

    if (!g_weather_data.valid)
    {
        OLED_ShowString(1, 1, "No Data     ");
        return;
    }

    /* 第1行：降水概率 */
    snprintf(buf, sizeof(buf), "Rain:%3d%%", g_weather_data.precip);
    OLED_ShowString(1, 1, buf);

    /* 第2行：风向+风力 */
    snprintf(buf, sizeof(buf), "Wind:%-3.3s %dL",
             g_weather_data.wind_dir[0] ? g_weather_data.wind_dir : "---",
             g_weather_data.wind_scale);
    OLED_ShowString(2, 1, buf);

    /* 第3行：湿度 */
    snprintf(buf, sizeof(buf), "Humi:%3d%%", g_weather_data.humidity);
    OLED_ShowString(3, 1, buf);
}

/**
 * @brief  绘制 DHT11 温湿度页面
 *         数据有效时显示实际值，无效时显示占位符 "--"
 */
static void display_page_dht11(void)
{
    char buf[17];

    if (g_dht11_data.valid == 1)
    {
        /* 第1行：温度 */
        snprintf(buf, sizeof(buf), "Temp: %2d C", g_dht11_data.temperature);
        OLED_ShowString(1, 1, buf);

        /* 第3行：湿度（%% 输出单个百分号） */
        snprintf(buf, sizeof(buf), "Humi: %2d %%", g_dht11_data.humidity);
        OLED_ShowString(3, 1, buf);
    }
    else
    {
        OLED_ShowString(1, 1, "Temp:  -- C");
        OLED_ShowString(3, 1, "Humi:  -- %");
    }
}

/* ------------------------------------------------------------------ */
/*  公开接口实现                                                        */
/* ------------------------------------------------------------------ */

/**
 * @brief  初始化显示管理器
 *         记录初始时间戳，设置起始页为 PAGE_CLOCK，清屏并绘制初始页面
 */
void Display_Init(void)
{
    s_page_tick    = HAL_GetTick();
    s_current_page = PAGE_CLOCK;

    OLED_Clear();
    display_page_clock();
}

/**
 * @brief  刷新显示（在主循环中持续调用）
 *         距上次切换满 PAGE_SWITCH_INTERVAL_MS 后切换到下一页并重绘
 */
void Display_Update(void)
{
    if (HAL_GetTick() - s_page_tick >= PAGE_SWITCH_INTERVAL_MS)
    {
        s_current_page = (DisplayPage_t)((s_current_page + 1) % PAGE_COUNT);
        s_page_tick    = HAL_GetTick();

        OLED_Clear();

        switch (s_current_page)
        {
            case PAGE_CLOCK:
                display_page_clock();
                break;
            case PAGE_WEATHER:
                display_page_weather();
                break;
            case PAGE_WEATHER_DETAIL:
                display_page_weather_detail();
                break;
            case PAGE_DHT11:
                display_page_dht11();
                break;
            default:
                break;
        }
    }
}

/**
 * @brief  在 OLED 上显示启动提示信息（初始化阶段使用）
 * @param  msg  要显示的提示字符串（最多 16 个可见字符）
 */
void Display_ShowBoot(const char *msg)
{
    OLED_Clear();
    OLED_ShowString(1, 1, (char *)msg);
}
