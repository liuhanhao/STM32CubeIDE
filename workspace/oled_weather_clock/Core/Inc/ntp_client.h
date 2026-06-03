/**
 * @file    ntp_client.h
 * @brief   NTP 时间同步客户端接口
 *          使用 ESP8266 AT 固件内置 SNTP（AT+CIPSNTPCFG / AT+CIPSNTPTIME?）
 *          固件版本 1.7.x 及以上支持，时区 UTC+8，无需手动 UDP。
 */

#ifndef NTP_CLIENT_H
#define NTP_CLIENT_H

#include <stdint.h>

/**
 * @brief  执行一次完整的 NTP 时间同步流程（最多重试 3 次）
 *         成功时将 UTC+8 时间写入 STM32 RTC。
 * @return 1 = 同步成功；0 = 全部重试失败（RTC 保持原有值）
 */
uint8_t NTP_Sync(void);

#endif /* NTP_CLIENT_H */
