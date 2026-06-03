/**
 * @file    ntp_client.c
 * @brief   NTP 时间同步客户端实现
 *          使用 ESP8266 AT 固件内置 SNTP 功能（AT+CIPSNTPCFG / AT+CIPSNTPTIME?）
 *          无需手动建立 UDP 连接，固件版本 1.7.x 及以上均支持。
 */

#include "ntp_client.h"
#include "at_controller.h"
#include "main.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "OLED.h"

/* ------------------------------------------------------------------ */
/*  外部句柄                                                             */
/* ------------------------------------------------------------------ */
extern RTC_HandleTypeDef hrtc;

/* ------------------------------------------------------------------ */
/*  内部宏                                                               */
/* ------------------------------------------------------------------ */
#define IS_LEAP_YEAR(y) \
    (((y) % 4 == 0 && (y) % 100 != 0) || ((y) % 400 == 0))

static const uint8_t s_days_in_month[12] = {
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

/* ------------------------------------------------------------------ */
/*  线性化缓冲区快照                                                     */
/* ------------------------------------------------------------------ */
static char s_snap[AT_RX_BUF_SIZE + 1];

static void make_snapshot(void)
{
    AT_Linearize(s_snap, AT_RX_BUF_SIZE + 1);
}

/* ------------------------------------------------------------------ */
/*  ntp_parse_timestamp                                                  */
/* ------------------------------------------------------------------ */
static void ntp_parse_timestamp(uint32_t unix_sec,
                                RTC_TimeTypeDef *t,
                                RTC_DateTypeDef *d)
{
    /* 已是 UTC+8，直接拆分 */
    t->Seconds = (uint8_t)(unix_sec % 60UL);
    t->Minutes = (uint8_t)((unix_sec / 60UL) % 60UL);
    t->Hours   = (uint8_t)((unix_sec / 3600UL) % 24UL);

    uint32_t days = unix_sec / 86400UL;
    uint16_t year = 1970U;
    while (1)
    {
        uint32_t diy = IS_LEAP_YEAR(year) ? 366UL : 365UL;
        if (days < diy) break;
        days -= diy;
        year++;
    }

    uint8_t month = 1U;
    while (month <= 12U)
    {
        uint8_t dim = s_days_in_month[month - 1U];
        if (month == 2U && IS_LEAP_YEAR(year)) dim = 29U;
        if (days < (uint32_t)dim) break;
        days -= (uint32_t)dim;
        month++;
    }

    d->Year    = (uint8_t)(year - 2000U);
    d->Month   = month;
    d->Date    = (uint8_t)(days + 1U);
    d->WeekDay = RTC_WEEKDAY_MONDAY;
}

/* ------------------------------------------------------------------ */
/*  月份名称转数字                                                        */
/* ------------------------------------------------------------------ */
static uint8_t month_name_to_num(const char *mon)
{
    /* AT+CIPSNTPTIME? 返回格式：Thu May 28 14:30:00 2026 */
    static const char *names[12] = {
        "Jan","Feb","Mar","Apr","May","Jun",
        "Jul","Aug","Sep","Oct","Nov","Dec"
    };
    for (uint8_t i = 0; i < 12U; i++)
    {
        if (strncmp(mon, names[i], 3) == 0) return (uint8_t)(i + 1U);
    }
    return 1U;
}

/* ------------------------------------------------------------------ */
/*  ntp_attempt                                                          */
/* ------------------------------------------------------------------ */
static uint8_t ntp_attempt(void)
{
    /* 步骤1：配置 SNTP，时区 +8，使用阿里云 NTP 服务器 */
    /* AT+CIPSNTPCFG=<enable>,<timezone>,"<server1>","<server2>" */
    if (!AT_SendWait("AT+CIPSNTPCFG=1,8,\"ntp.aliyun.com\",\"cn.ntp.org.cn\"",
                     "OK", 3000U))
    {
        OLED_ShowString(1, 1, "SNTP cfg fail");
        HAL_Delay(2000U);
        return 0U;
    }

    /* 步骤2：等待 SNTP 同步完成（最多 10 秒轮询） */
    /* AT+CIPSNTPTIME? 在同步前返回 "Thu Jan  1 00:00:00 1970"
     * 同步后返回实际时间，年份 >= 2020 才算有效 */
    uint8_t synced = 0U;
    uint32_t t0 = HAL_GetTick();
    while (HAL_GetTick() - t0 < 10000U)
    {
        HAL_Delay(1000U);
        AT_ClearRxBuf();
        AT_SendWait("AT+CIPSNTPTIME?", "OK", 2000U);
        /* 响应格式：+CIPSNTPTIME:Thu May 28 14:30:00 2026\r\nOK */
        if (AT_Contains("+CIPSNTPTIME:") &&
            !AT_Contains("1970"))
        {
            /* 立刻保存快照，AT_Contains 刚完成线性化 */
            strncpy(s_snap, AT_GetSnapshot(), AT_RX_BUF_SIZE);
            s_snap[AT_RX_BUF_SIZE] = '\0';
            synced = 1U;
            break;
        }
    }

    if (!synced)
    {
        OLED_ShowString(1, 1, "SNTP no sync");
        HAL_Delay(2000U);
        return 0U;
    }

    /* 步骤3：解析时间字符串（快照已在轮询时保存） */
    /* 找到 "+CIPSNTPTIME:" 后面的时间字符串 */
    const char *p = strstr(s_snap, "+CIPSNTPTIME:");
    if (p == NULL)
    {
        OLED_ShowString(1, 1, "SNTP parse err");
        HAL_Delay(2000U);
        return 0U;
    }
    p += strlen("+CIPSNTPTIME:");

    /* 格式：Thu May 28 14:30:00 2026
     *        0   4   8  11       20  */
    /* 跳过星期（3字符 + 空格） */
    if (strlen(p) < 24U)
    {
        OLED_ShowString(1, 1, "SNTP str short");
        HAL_Delay(2000U);
        return 0U;
    }

    char mon_str[4]  = {0};
    uint8_t  day     = 0U;
    uint8_t  hour    = 0U;
    uint8_t  minute  = 0U;
    uint8_t  second  = 0U;
    uint16_t year    = 0U;

    /* 解析：跳过 "Thu " (4字节) 得到 "May 28 14:30:00 2026" */
    const char *ts = p + 4U;
    /* mon(3) + ' ' + day(1~2) + ' ' + HH:MM:SS + ' ' + YYYY */
    strncpy(mon_str, ts, 3);
    mon_str[3] = '\0';

    /* sscanf 在嵌入式环境可用，解析剩余字段 */
    int d_day = 0, d_hour = 0, d_min = 0, d_sec = 0, d_year = 0;
    sscanf(ts + 4, "%d %d:%d:%d %d",
           &d_day, &d_hour, &d_min, &d_sec, &d_year);

    day    = (uint8_t)d_day;
    hour   = (uint8_t)d_hour;
    minute = (uint8_t)d_min;
    second = (uint8_t)d_sec;
    year   = (uint16_t)d_year;

    /* 基本有效性校验 */
    if (year < 2020U || year > 2099U)
    {
        char dbg[17];
        snprintf(dbg, sizeof(dbg), "yr err:%u", year);
        OLED_ShowString(1, 1, dbg);
        HAL_Delay(2000U);
        return 0U;
    }

    /* 步骤4：写入 RTC */
    RTC_TimeTypeDef rtc_time = {0};
    RTC_DateTypeDef rtc_date = {0};

    rtc_time.Hours   = hour;
    rtc_time.Minutes = minute;
    rtc_time.Seconds = second;
    rtc_date.Year    = (uint8_t)(year - 2000U);
    rtc_date.Month   = month_name_to_num(mon_str);
    rtc_date.Date    = day;
    rtc_date.WeekDay = RTC_WEEKDAY_MONDAY;

    HAL_RTC_SetTime(&hrtc, &rtc_time, RTC_FORMAT_BIN);
    HAL_RTC_SetDate(&hrtc, &rtc_date, RTC_FORMAT_BIN);

    /* 调试：显示同步结果 */
    {
        char dbg[17];
        snprintf(dbg, sizeof(dbg), "%02u:%02u:%02u", hour, minute, second);
        OLED_ShowString(1, 1, "NTP OK:");
        OLED_ShowString(2, 1, dbg);
        HAL_Delay(2000U);
    }

    return 1U;
}

/* ------------------------------------------------------------------ */
/*  NTP_Sync                                                             */
/* ------------------------------------------------------------------ */
uint8_t NTP_Sync(void)
{
    for (uint8_t retry = 0U; retry < 3U; retry++)
    {
        if (ntp_attempt()) return 1U;
        HAL_Delay(2000U);
    }
    return 0U;
}
