/**
 * @file    weather_client.c
 * @brief   天气数据获取客户端 —— 对接心知天气 HTTP API
 *
 * 接口：http://api.seniverse.com/v3/weather/daily.json
 *       ?key=KEY&location=CITY&language=zh-Hans&unit=c&start=0&days=1
 *
 * 响应示例：
 *   {"results":[{"daily":[{"text_day":"阴","high":"36","low":"27"}]}]}
 */

#include "weather_client.h"
#include "at_controller.h"
#include "app_globals.h"
#include "main.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "OLED.h"

/* ------------------------------------------------------------------ */
/*  用户配置                                                             */
/* ------------------------------------------------------------------ */
#define WEATHER_API_KEY   "Szr8uu3XG63tDTyY3"
#define WEATHER_LOCATION  "foshan"
#define WEATHER_HOST      "api.seniverse.com"
#define WEATHER_PORT      80

/* ------------------------------------------------------------------ */
/*  外部引用                                                             */
/* ------------------------------------------------------------------ */
extern volatile WiFiStatus_t g_wifi_status;
extern UART_HandleTypeDef    huart1;

/* ------------------------------------------------------------------ */
/*  全局天气数据缓存                                                     */
/* ------------------------------------------------------------------ */
WeatherData_t g_weather_data = {0};

/* ------------------------------------------------------------------ */
/*  内部缓冲区                                                           */
/* ------------------------------------------------------------------ */
static char s_resp_snap[AT_RX_BUF_SIZE + 1];
/* ------------------------------------------------------------------ */
/*  json_find_str                                                        */
/* ------------------------------------------------------------------ */
static uint8_t json_find_str(const char *json, const char *key,
                              char *out_buf, uint16_t max_len)
{
    char pattern[48];
    const char *p;
    uint16_t i = 0;

    snprintf(pattern, sizeof(pattern), "\"%s\":\"", key);
    p = strstr(json, pattern);
    if (p == NULL) { out_buf[0] = '\0'; return 0U; }
    p += strlen(pattern);
    while (*p != '\0' && *p != '"' && i < (uint16_t)(max_len - 1U))
        out_buf[i++] = *p++;
    out_buf[i] = '\0';
    return (i > 0U) ? 1U : 0U;
}

/* ------------------------------------------------------------------ */
/*  weather_make_snapshot 已废弃，改用 AT_GetSnapshot()               */
/* ------------------------------------------------------------------ */

/* ------------------------------------------------------------------ */
/*  Weather_Fetch                                                        */
/* ------------------------------------------------------------------ */
uint8_t Weather_Fetch(void)
{
    char cmd_buf[128];
    char http_req[300];   /* 扩大到300，确保不截断 */
    int  http_req_len;

    /* ---- 1. 清理残留连接，确保状态干净 ------------------------------ */
    /* 关闭 SNTP 后台服务，防止占用连接资源 */
    AT_SendWait("AT+CIPSNTPCFG=0", "OK", 2000U);
    HAL_Delay(1000U);  /* 等待模块状态稳定 */
    /* 关闭可能残留的连接（ALREADY CONNECTED 问题的根源） */
    AT_SendWait("AT+CIPCLOSE", "OK", 2000U);
    HAL_Delay(500U);
    AT_SendWait("AT+CIPMUX=0", "OK", 1000U);
    HAL_Delay(300U);

    /* 确认有 IP 再继续 */
    {
        uint8_t has_ip = 0U;
        uint32_t t0 = HAL_GetTick();
        while (HAL_GetTick() - t0 < 10000U)
        {
            AT_ClearRxBuf();
            AT_SendWait("AT+CIFSR", "STAIP", 2000U);
            if (AT_Contains("STAIP") && !AT_Contains("0.0.0.0"))
            {
                has_ip = 1U;
                break;
            }
            HAL_Delay(500U);
        }
        if (!has_ip)
        {
            OLED_ShowString(1, 1, "W:no IP     ");
            HAL_Delay(3000U);
            goto fetch_fail;
        }
    }

    /* ---- 2. 建立 TCP 连接 ------------------------------------------ */
    snprintf(cmd_buf, sizeof(cmd_buf),
             "AT+CIPSTART=\"TCP\",\"%s\",%d", WEATHER_HOST, WEATHER_PORT);
    OLED_ShowString(1, 1, "W:connecting");

    /* 发送前确认模块响应正常 */
    AT_SendWait("AT", "OK", 1000U);
    HAL_Delay(100U);

    /* 发送 CIPSTART，不依赖返回值，用 CIPSTATUS 确认实际状态 */
    AT_SendWait(cmd_buf, "CONNECT", 10000U);
    HAL_Delay(500U);

    /* STATUS:3 = TCP 已连接 */
    AT_ClearRxBuf();
    AT_SendWait("AT+CIPSTATUS", "OK", 2000U);
    if (!AT_Contains("STATUS:3"))
    {
        OLED_ShowString(1, 1, "W:TCP fail  ");
        if      (AT_Contains("STATUS:2")) OLED_ShowString(2, 1, "ST2:no conn ");
        else if (AT_Contains("STATUS:4")) OLED_ShowString(2, 1, "ST4:closing ");
        else if (AT_Contains("STATUS:5")) OLED_ShowString(2, 1, "ST5:no wifi ");
        else                              OLED_ShowString(2, 1, "ST:unknown  ");
        HAL_Delay(3000U);
        goto fetch_fail;
    }
    OLED_ShowString(1, 1, "W:TCP ok    ");
    HAL_Delay(300U);

    /* ---- 3. 构造 HTTP GET 请求 -------------------------------------- */
    http_req_len = snprintf(http_req, sizeof(http_req),
        "GET /v3/weather/daily.json?key=%s&location=%s"
            "&language=zh-Hans&unit=c&start=0&days=1 HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n"
        "\r\n",
        WEATHER_API_KEY, WEATHER_LOCATION, WEATHER_HOST);

    /* ---- 4. AT+CIPSEND，等待 ">" ------------------------------------ */
    snprintf(cmd_buf, sizeof(cmd_buf), "AT+CIPSEND=%d", http_req_len);
    /* 调试：显示实际发送字节数 */
    {
        char dbg[17];
        snprintf(dbg, sizeof(dbg), "len=%d", http_req_len);
        OLED_ShowString(2, 1, dbg);
    }
    if (!AT_SendWait(cmd_buf, ">", 3000U))
    {
        OLED_ShowString(1, 1, "W:SEND fail ");
        HAL_Delay(3000U);
        AT_SendWait("AT+CIPCLOSE", "OK", 2000U);
        goto fetch_fail;
    }

    /* ---- 5. 发送 HTTP 请求报文 -------------------------------------- */
    HAL_UART_Transmit(&huart1, (uint8_t *)http_req,
                      (uint16_t)http_req_len, 3000U);

    /* ---- 6. 等待服务器响应，收到后立刻用 AT_GetSnapshot 保存数据 ----- */
    {
        uint32_t t0 = HAL_GetTick();
        uint8_t  got = 0U;
        while (HAL_GetTick() - t0 < 10000U)
        {
            /* 等 +IPD 出现说明数据开始到来 */
            if (AT_Contains("+IPD"))
            {
                got = 1U;
                break;
            }
        }
        if (!got)
        {
            OLED_ShowString(1, 1, "W:no resp   ");
            HAL_Delay(3000U);
            AT_SendWait("AT+CIPCLOSE", "OK", 2000U);
            goto fetch_fail;
        }
        /* 等待数据接收完整：连接关闭时 ESP8266 会发 CLOSED */
        t0 = HAL_GetTick();
        while (HAL_GetTick() - t0 < 5000U)
        {
            if (AT_Contains("CLOSED")) break;
        }
        /* 立刻保存快照 */
        strncpy(s_resp_snap, AT_GetSnapshot(), AT_RX_BUF_SIZE);
        s_resp_snap[AT_RX_BUF_SIZE] = '\0';
    }

    /* ---- 7. 连接已由服务器关闭，确认清理 ---------------------------- */
    AT_SendWait("AT+CIPCLOSE", "OK", 1000U);
    /* ---- 8. 检查 API 错误码 ----------------------------------------- */
    if (strstr(s_resp_snap, "\"status_code\"") != NULL)
    {
        char code_buf[16] = {0};
        json_find_str(s_resp_snap, "status_code", code_buf, sizeof(code_buf));
        OLED_ShowString(1, 1, "W:API err   ");
        OLED_ShowString(2, 1, code_buf);
        HAL_Delay(3000U);
        goto fetch_fail;
    }

    /* ---- 9. 解析 JSON ------------------------------------------------ */
    {
        char text_buf[WEATHER_TEXT_LEN] = {0};
        char high_buf[8] = {0};
        char low_buf[8]  = {0};

        uint8_t ok_text = json_find_str(s_resp_snap, "text_day",
                                        text_buf, WEATHER_TEXT_LEN);
        uint8_t ok_high = json_find_str(s_resp_snap, "high",
                                        high_buf, sizeof(high_buf));
        uint8_t ok_low  = json_find_str(s_resp_snap, "low",
                                        low_buf,  sizeof(low_buf));
        char code_day_buf[8]  = {0};
        char humidity_buf[8]  = {0};
        char precip_buf[8]    = {0};
        char wind_scale_buf[8]= {0};
        char wind_dir_buf[16] = {0};
        json_find_str(s_resp_snap, "code_day",        code_day_buf,   sizeof(code_day_buf));
        json_find_str(s_resp_snap, "humidity",        humidity_buf,   sizeof(humidity_buf));
        json_find_str(s_resp_snap, "precip",          precip_buf,     sizeof(precip_buf));
        json_find_str(s_resp_snap, "wind_scale",      wind_scale_buf, sizeof(wind_scale_buf));
        json_find_str(s_resp_snap, "wind_direction",  wind_dir_buf,   sizeof(wind_dir_buf));

        if (!ok_text || (!ok_high && !ok_low))
        {
            /* 分4段显示快照内容，每段16字节，帮助诊断 */
            uint8_t seg;
            for (seg = 0; seg < 4U; seg++)
            {
                char disp[17];
                uint16_t di = 0, ri;
                uint16_t start = (uint16_t)(seg * 16U);
                for (ri = start; ri < start + 16U && ri < AT_RX_BUF_SIZE; ri++)
                {
                    char c = s_resp_snap[ri];
                    if (c == '\0') break;
                    disp[di++] = (c >= 0x20 && c < 0x7F) ? c : '.';
                }
                disp[di] = '\0';
                if (di == 0U) break;  /* 没有更多内容了 */
                OLED_ShowString(1, 1, "snap:       ");
                OLED_ShowString(2, 1, disp);
                HAL_Delay(3000U);
            }
            OLED_ShowString(1, 1, "W:parse fail");
            HAL_Delay(3000U);
            goto fetch_fail;
        }

        /* ---- 10. 更新全局缓存 --------------------------------------- */
        strncpy(g_weather_data.text, text_buf, WEATHER_TEXT_LEN - 1U);
        g_weather_data.text[WEATHER_TEXT_LEN - 1U] = '\0';
        g_weather_data.tempMax   = (int8_t)atoi(high_buf);
        g_weather_data.tempMin   = (int8_t)atoi(low_buf);
        g_weather_data.humidity  = (uint8_t)atoi(humidity_buf);
        /* precip 是小数如 "0.85"，转成百分比整数 */
        {
            float p = (float)atof(precip_buf);
            g_weather_data.precip = (uint8_t)(p * 100.0f + 0.5f);
        }
        g_weather_data.wind_scale = (uint8_t)atoi(wind_scale_buf);
        strncpy(g_weather_data.wind_dir, wind_dir_buf, sizeof(g_weather_data.wind_dir) - 1U);
        g_weather_data.wind_dir[sizeof(g_weather_data.wind_dir) - 1U] = '\0';

        /* 心知天气 code_day 映射到图标码 */
        {
            int code = atoi(code_day_buf);
            uint16_t icon = 999U;  /* 默认未知 */
            if      (code == 0)                    icon = 100;  /* 晴 */
            else if (code == 1)                    icon = 101;  /* 晴间多云 */
            else if (code == 2 || code == 3)       icon = 104;  /* 阴/多云 */
            else if (code == 4)                    icon = 104;  /* 阴 */
            else if (code >= 5  && code <= 9)      icon = 305;  /* 小雨 */
            else if (code == 10 || code == 11)     icon = 302;  /* 雷阵雨 */
            else if (code == 12 || code == 13)     icon = 307;  /* 大雨 */
            else if (code == 14 || code == 15)     icon = 400;  /* 雪 */
            else if (code == 16 || code == 17)     icon = 404;  /* 雨夹雪 */
            else if (code == 18)                   icon = 500;  /* 雾 */
            else if (code == 19)                   icon = 502;  /* 霾 */
            else if (code == 20 || code == 21)     icon = 503;  /* 沙尘 */
            g_weather_data.icon = icon;
        }
        g_weather_data.valid   = 1U;

        {
            char dbg[17];
            snprintf(dbg, sizeof(dbg), "%s %d~%dC",
                     text_buf,
                     (int)(int8_t)g_weather_data.tempMin,
                     (int)(int8_t)g_weather_data.tempMax);
            OLED_ShowString(1, 1, "W:OK        ");
            OLED_ShowString(2, 1, dbg);
            HAL_Delay(2000U);
        }
        return 1U;
    }

fetch_fail:
    if (g_weather_data.valid == 0U)
    {
        strncpy(g_weather_data.text, "No Data", WEATHER_TEXT_LEN - 1U);
        g_weather_data.text[WEATHER_TEXT_LEN - 1U] = '\0';
    }
    return 0U;
}
