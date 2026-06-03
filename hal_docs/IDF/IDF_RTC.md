# RTC — 实时时钟 API 文档（ESP-IDF）

## 📋 模块概述

ESP32-S3 内置 RTC 模块，配合 ESP-IDF 的时间管理 API 提供：
- 系统时间获取与设置（Unix 时间戳）
- SNTP 网络时间同步
- 时区配置
- RTC 内存（深度睡眠保持数据）
- 定时唤醒（配合深度睡眠）

> **注意：** ESP32 没有独立的 RTC 晶振引脚（不像 STM32 有 LSE），
> 断电后时间丢失。需要持久时间必须通过 SNTP 同步或外接 RTC 芯片（DS3231）。

---

## 🗺️ 使用场景速查

| 场景 | 方案 |
|------|------|
| 获取当前时间（有网络）| SNTP 同步 |
| 数据记录时间戳 | SNTP + `gettimeofday` |
| 定时唤醒（深度睡眠）| RTC 定时器 + `esp_sleep_enable_timer_wakeup` |
| 断电保持时间 | 外接 DS3231（I2C）|
| 时间格式化显示 | `localtime` + `strftime` |

---

## 📚 相关文件

- **头文件**: `esp_sntp.h`（SNTP 同步）
- **头文件**: `time.h`（标准 C 时间函数）
- **头文件**: `sys/time.h`（`gettimeofday`）

---

## 一、系统时间操作

### 获取当前时间

```c
#include <time.h>
#include <sys/time.h>

/* 方法1：获取 Unix 时间戳（秒）*/
time_t now;
time(&now);
ESP_LOGI(TAG, "Unix 时间戳: %lld", (long long)now);

/* 方法2：获取高精度时间（秒 + 微秒）*/
struct timeval tv;
gettimeofday(&tv, NULL);
ESP_LOGI(TAG, "时间: %lld.%06ld", (long long)tv.tv_sec, tv.tv_usec);

/* 方法3：转换为本地时间结构体 */
struct tm timeinfo;
localtime_r(&now, &timeinfo);
ESP_LOGI(TAG, "时间: %04d-%02d-%02d %02d:%02d:%02d",
         timeinfo.tm_year + 1900,
         timeinfo.tm_mon + 1,
         timeinfo.tm_mday,
         timeinfo.tm_hour,
         timeinfo.tm_min,
         timeinfo.tm_sec);

/* 方法4：格式化字符串 */
char time_str[64];
strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &timeinfo);
ESP_LOGI(TAG, "时间: %s", time_str);
```

---

### 手动设置时间

```c
#include <sys/time.h>

/* 设置为 2024-01-01 00:00:00 */
struct tm t = {
    .tm_year = 2024 - 1900,  // 年份从 1900 开始
    .tm_mon  = 0,             // 月份从 0 开始（0=1月）
    .tm_mday = 1,
    .tm_hour = 0,
    .tm_min  = 0,
    .tm_sec  = 0,
};
time_t timestamp = mktime(&t);

struct timeval tv = {
    .tv_sec  = timestamp,
    .tv_usec = 0,
};
settimeofday(&tv, NULL);
```

---

### 时区配置

```c
#include <time.h>

/* 设置时区（必须在获取本地时间前设置）*/
setenv("TZ", "CST-8", 1);   // 中国标准时间（UTC+8）
tzset();

/* 常用时区字符串 */
// "CST-8"          中国标准时间 UTC+8
// "UTC0"           UTC
// "EST5EDT"        美国东部时间
// "JST-9"          日本标准时间 UTC+9
// "CET-1CEST"      中欧时间（含夏令时）
```

---

## 二、SNTP 网络时间同步

### esp_sntp_init()（旧 API）/ esp_netif_sntp_init()（新 API）

```c
#include "esp_sntp.h"

/* 旧 API（IDF 4.x / 5.x 均可用）*/
void sntp_sync_init(void)
{
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_setservername(1, "ntp.aliyun.com");  // 国内备用
    esp_sntp_init();
}
```

---

### 等待时间同步完成

```c
void wait_for_sntp_sync(void)
{
    ESP_LOGI(TAG, "等待 SNTP 同步...");
    int retry = 0;
    const int max_retry = 15;

    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && retry < max_retry) {
        ESP_LOGI(TAG, "等待中... (%d/%d)", ++retry, max_retry);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
        ESP_LOGI(TAG, "SNTP 同步成功");
    } else {
        ESP_LOGW(TAG, "SNTP 同步超时");
    }
}
```

---

### 同步回调

```c
/* 注册同步完成回调 */
static void sntp_sync_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "时间已同步: %lld", (long long)tv->tv_sec);
}

esp_sntp_set_time_sync_notification_cb(sntp_sync_cb);
```

---

## 💡 完整应用示例

### 示例1：WiFi 连接后同步时间并显示

```c
#include "esp_sntp.h"
#include "esp_log.h"
#include <time.h>
#include <sys/time.h>

static const char *TAG = "RTC";

void time_sync_init(void)
{
    /* 设置时区（中国）*/
    setenv("TZ", "CST-8", 1);
    tzset();

    /* 配置 SNTP */
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "ntp.aliyun.com");
    esp_sntp_setservername(1, "pool.ntp.org");
    esp_sntp_init();

    /* 等待同步（最多 30 秒）*/
    int retry = 0;
    while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED && retry < 15) {
        vTaskDelay(pdMS_TO_TICKS(2000));
        retry++;
    }
}

void print_current_time(void)
{
    time_t now;
    struct tm timeinfo;
    char buf[64];

    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(buf, sizeof(buf), "%Y年%m月%d日 %H:%M:%S", &timeinfo);
    ESP_LOGI(TAG, "当前时间: %s", buf);
}

void app_main(void)
{
    /* WiFi 连接（略）*/
    wifi_init_sta();

    /* 时间同步 */
    time_sync_init();
    print_current_time();

    /* 每秒打印时间 */
    while (1) {
        print_current_time();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

---

### 示例2：数据记录时间戳

```c
typedef struct {
    time_t    timestamp;
    float     temperature;
    float     humidity;
} sensor_record_t;

sensor_record_t create_record(float temp, float humi)
{
    sensor_record_t rec;
    time(&rec.timestamp);
    rec.temperature = temp;
    rec.humidity    = humi;
    return rec;
}

void print_record(const sensor_record_t *rec)
{
    struct tm t;
    char buf[32];
    localtime_r(&rec->timestamp, &t);
    strftime(buf, sizeof(buf), "%H:%M:%S", &t);
    ESP_LOGI(TAG, "[%s] 温度=%.1f°C 湿度=%.1f%%",
             buf, rec->temperature, rec->humidity);
}
```

---

### 示例3：外接 DS3231 RTC 芯片（断电保持）

```c
/* DS3231 通过 I2C 连接，地址 0x68 */
#include "driver/i2c_master.h"

#define DS3231_ADDR  0x68

/* DS3231 寄存器 */
#define REG_SECONDS  0x00
#define REG_MINUTES  0x01
#define REG_HOURS    0x02
#define REG_DAY      0x03
#define REG_DATE     0x04
#define REG_MONTH    0x05
#define REG_YEAR     0x06

static i2c_master_dev_handle_t ds3231_handle;

/* BCD 转十进制 */
static uint8_t bcd2dec(uint8_t bcd) { return (bcd >> 4) * 10 + (bcd & 0x0F); }
static uint8_t dec2bcd(uint8_t dec) { return ((dec / 10) << 4) | (dec % 10); }

void ds3231_read_time(struct tm *t)
{
    uint8_t reg = REG_SECONDS;
    uint8_t data[7];
    i2c_master_transmit_receive(ds3231_handle, &reg, 1, data, 7, 100);

    t->tm_sec  = bcd2dec(data[0] & 0x7F);
    t->tm_min  = bcd2dec(data[1]);
    t->tm_hour = bcd2dec(data[2] & 0x3F);
    t->tm_mday = bcd2dec(data[4]);
    t->tm_mon  = bcd2dec(data[5] & 0x1F) - 1;
    t->tm_year = bcd2dec(data[6]) + 100;  // 2000 年基准
}

void ds3231_set_system_time(void)
{
    struct tm t;
    ds3231_read_time(&t);
    time_t timestamp = mktime(&t);
    struct timeval tv = { .tv_sec = timestamp };
    settimeofday(&tv, NULL);
    ESP_LOGI(TAG, "从 DS3231 同步时间成功");
}
```

---

## ⚠️ 注意事项

### 1. ESP32 断电时间丢失

ESP32 没有外部 RTC 晶振，断电后时间从 1970-01-01 00:00:00 重新开始。
解决方案：
- 有网络：每次启动后 SNTP 同步
- 无网络：外接 DS3231 等 RTC 芯片

### 2. SNTP 需要 WiFi 连接

必须在 WiFi 连接并获取 IP 后才能启动 SNTP 同步。

### 3. 时区必须在 localtime 前设置

```c
setenv("TZ", "CST-8", 1);
tzset();
/* 之后调用 localtime_r 才会返回正确的本地时间 */
```

### 4. tm_year 和 tm_mon 的偏移

```c
timeinfo.tm_year + 1900  // 实际年份
timeinfo.tm_mon  + 1     // 实际月份（1~12）
```

---

## 📖 相关文档

- [IDF_WiFi.md](IDF_WiFi.md) — WiFi 连接（SNTP 前提）
- [IDF_DeepSleep.md](IDF_DeepSleep.md) — 深度睡眠（RTC 定时唤醒）
- [IDF_NVS.md](IDF_NVS.md) — NVS 存储
- [README.md](README.md) — 模块导航
