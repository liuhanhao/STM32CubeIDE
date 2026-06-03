/**
 * @file    app_globals.h
 * @brief   全局共享数据结构定义，供各模块 include 使用
 */

#ifndef APP_GLOBALS_H
#define APP_GLOBALS_H

#include <stdint.h>

/* ------------------------------------------------------------------ */
/*  WiFi 连接状态                                                       */
/* ------------------------------------------------------------------ */
typedef enum {
    WIFI_DISCONNECTED = 0,  /* 未连接 */
    WIFI_CONNECTED    = 1   /* 已连接 */
} WiFiStatus_t;

/* ------------------------------------------------------------------ */
/*  天气数据                                                            */
/* ------------------------------------------------------------------ */
#define WEATHER_TEXT_LEN  17    /* 最长 16 字符 + '\0' */

typedef struct {
    char    text[WEATHER_TEXT_LEN]; /* 天气状况文本，异常时为 "No Data" */
    int8_t  tempMax;                /* 最高温度，单位 °C */
    int8_t  tempMin;                /* 最低温度，单位 °C */
    uint16_t icon;                  /* 图标码 */
    uint8_t  humidity;              /* 湿度，单位 % */
    uint8_t  precip;                /* 降水概率，单位 % (0~100) */
    uint8_t  wind_scale;            /* 风力等级 */
    char     wind_dir[8];           /* 风向，如 "南"、"无持续" */
    uint8_t valid;                  /* 0 = 无效/未获取，1 = 有效 */
} WeatherData_t;

/* ------------------------------------------------------------------ */
/*  DHT11 温湿度数据                                                    */
/* ------------------------------------------------------------------ */
typedef struct {
    int8_t  temperature;  /* 温度整数部分，单位 °C */
    uint8_t humidity;     /* 湿度整数部分，单位 % */
    uint8_t valid;        /* 0 = 无效，1 = 有效 */
} DHT11Data_t;

/* ------------------------------------------------------------------ */
/*  显示页面枚举                                                        */
/* ------------------------------------------------------------------ */
typedef enum {
    PAGE_CLOCK        = 0,   /* 时钟页面 */
    PAGE_WEATHER      = 1,   /* 天气页面（图标+描述+高低温） */
    PAGE_WEATHER_DETAIL = 2, /* 天气详情（降水+风力+湿度） */
    PAGE_DHT11        = 3,   /* 温湿度页面 */
    PAGE_COUNT        = 4
} DisplayPage_t;

#endif /* APP_GLOBALS_H */
