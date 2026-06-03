/**
 * @file    weather_client.h
 * @brief   天气数据获取客户端接口
 *          通过 AT 指令向和风天气 API 发起 HTTP GET 请求，
 *          轻量解析 JSON 响应并更新全局天气缓存 g_weather_data。
 */

#ifndef WEATHER_CLIENT_H
#define WEATHER_CLIENT_H

#include <stdint.h>
#include "app_globals.h"    /* WeatherData_t, WEATHER_TEXT_LEN */

/* 注意：WEATHER_TEXT_LEN 已在 app_globals.h 中定义为 17，此处不重复定义 */

/* ------------------------------------------------------------------ */
/*  全局变量声明（定义在 weather_client.c 中）                          */
/* ------------------------------------------------------------------ */
extern WeatherData_t g_weather_data;

/* ------------------------------------------------------------------ */
/*  函数声明                                                            */
/* ------------------------------------------------------------------ */

/**
 * @brief  执行一次天气数据请求并解析响应
 *         成功时更新 g_weather_data（valid=1）；
 *         失败时保留旧缓存，若首次失败则填入 "No Data"。
 * @return 1 = 获取并解析成功；0 = 失败（网络错误或解析失败）
 */
uint8_t Weather_Fetch(void);

#endif /* WEATHER_CLIENT_H */
