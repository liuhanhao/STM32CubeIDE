/**
 * @file    display_manager.h
 * @brief   OLED 分页显示管理器接口
 *          维护页面切换状态机，按固定时间间隔依次切换
 *          时钟页（PAGE_CLOCK）→ 天气页（PAGE_WEATHER）→ 温湿度页（PAGE_DHT11）循环。
 */

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <stdint.h>
#include "app_globals.h"    /* DisplayPage_t */

/* ------------------------------------------------------------------ */
/*  页面切换参数                                                        */
/* ------------------------------------------------------------------ */

/** 每个页面停留时长（毫秒），达到后自动切换到下一页 */
#define PAGE_SWITCH_INTERVAL_MS   5000U

/* ------------------------------------------------------------------ */
/*  函数声明                                                            */
/* ------------------------------------------------------------------ */

/**
 * @brief  初始化显示管理器
 *         清空屏幕，将当前页面设置为 PAGE_CLOCK，记录初始时间戳。
 */
void Display_Init(void);

/**
 * @brief  刷新显示（在主循环中持续调用）
 *         检查距上次切换是否已满 PAGE_SWITCH_INTERVAL_MS，
 *         若是则切换到下一页并重绘；否则仅刷新当前页内容（如秒数跳变）。
 */
void Display_Update(void);

/**
 * @brief  在 OLED 上显示启动提示信息（初始化阶段使用）
 * @param  msg  要显示的提示字符串（最多 16 个可见字符）
 */
void Display_ShowBoot(const char *msg);

#endif /* DISPLAY_MANAGER_H */
