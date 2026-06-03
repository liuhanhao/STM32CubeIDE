# Deep Sleep — 深度睡眠 API 文档（ESP-IDF）

## 📋 模块概述

ESP32-S3 提供多种低功耗模式，ESP-IDF 通过 `esp_sleep` API 管理：

| 模式 | 功耗 | 唤醒后 | 适用场景 |
|------|------|--------|---------|
| Light Sleep | ~0.8mA | 继续执行 | 短暂等待，需要快速响应 |
| Deep Sleep | ~10μA | 重新启动（类似复位）| 长时间休眠，极低功耗 |
| Hibernation | ~2.5μA | 重新启动 | 最低功耗，仅 RTC 定时器唤醒 |

> **Deep Sleep 特点：** 唤醒后代码从 `app_main` 重新执行，
> 普通变量丢失，只有 RTC 内存中的数据保留。

---

## 🗺️ 使用场景速查

| 场景 | 唤醒源 |
|------|--------|
| 定时采集数据（每 5 分钟）| RTC 定时器 |
| 按键唤醒（低功耗设备）| EXT0/EXT1（GPIO）|
| 触摸唤醒 | Touch Pad |
| ULP 协处理器唤醒 | ULP |
| 多种条件任一满足 | 多唤醒源组合 |

---

## 📚 相关文件

- **头文件**: `esp_sleep.h`

---

## 一、唤醒源配置

### RTC 定时器唤醒

```c
#include "esp_sleep.h"

/* 设置 30 秒后唤醒 */
esp_sleep_enable_timer_wakeup(30 * 1000000ULL);  // 单位：微秒
```

---

### GPIO 唤醒（EXT0 — 单引脚）

```c
/* 当 GPIO0 为低电平时唤醒 */
esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);  // 0=低电平，1=高电平
```

> **注意：** EXT0 只支持 RTC GPIO（GPIO0~GPIO21）。

---

### GPIO 唤醒（EXT1 — 多引脚）

```c
/* 当 GPIO0 或 GPIO2 任一为低电平时唤醒 */
uint64_t pin_mask = (1ULL << GPIO_NUM_0) | (1ULL << GPIO_NUM_2);
esp_sleep_enable_ext1_wakeup(pin_mask, ESP_EXT1_WAKEUP_ANY_LOW);
/* ESP_EXT1_WAKEUP_ANY_LOW：任一引脚为低电平
 * ESP_EXT1_WAKEUP_ALL_HIGH：所有引脚为高电平 */
```

---

### 触摸唤醒

```c
/* 启用触摸唤醒（需先配置触摸传感器）*/
esp_sleep_enable_touchpad_wakeup();
```

---

## 二、进入睡眠

### esp_deep_sleep_start()

进入深度睡眠（不返回，唤醒后重新启动）。

```c
esp_deep_sleep_start();
```

---

### esp_light_sleep_start()

进入浅睡眠（唤醒后从此处继续执行）。

```c
esp_err_t ret = esp_light_sleep_start();
/* 唤醒后继续执行 */
ESP_LOGI(TAG, "从浅睡眠唤醒");
```

---

## 三、唤醒原因查询

### esp_sleep_get_wakeup_cause()

获取上次唤醒原因（在 `app_main` 开头调用）。

```c
esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
switch (cause) {
    case ESP_SLEEP_WAKEUP_TIMER:
        ESP_LOGI(TAG, "定时器唤醒");
        break;
    case ESP_SLEEP_WAKEUP_EXT0:
        ESP_LOGI(TAG, "EXT0 GPIO 唤醒");
        break;
    case ESP_SLEEP_WAKEUP_EXT1:
        ESP_LOGI(TAG, "EXT1 GPIO 唤醒");
        uint64_t pins = esp_sleep_get_ext1_wakeup_status();
        ESP_LOGI(TAG, "唤醒引脚掩码: 0x%llX", pins);
        break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
        ESP_LOGI(TAG, "触摸唤醒");
        break;
    case ESP_SLEEP_WAKEUP_UNDEFINED:
        ESP_LOGI(TAG, "首次上电或复位");
        break;
    default:
        ESP_LOGI(TAG, "其他唤醒原因: %d", cause);
        break;
}
```

---

## 四、RTC 内存（深度睡眠保持数据）

深度睡眠期间，普通 RAM 断电，只有 RTC 内存保持数据。

```c
/* 使用 RTC_DATA_ATTR 修饰符将变量放入 RTC 内存 */
RTC_DATA_ATTR static int boot_count = 0;
RTC_DATA_ATTR static float last_temperature = 0.0f;

void app_main(void)
{
    boot_count++;
    ESP_LOGI(TAG, "第 %d 次启动", boot_count);
    /* boot_count 在深度睡眠后保持，不会重置为 0 */
}
```

**RTC 内存限制：**
- ESP32-S3 RTC 慢速内存：8KB
- RTC 快速内存：8KB（可在 ULP 中使用）

---

## 💡 完整应用示例

### 示例1：定时采集传感器数据（每 5 分钟）

```c
#include "esp_sleep.h"
#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "DEEP_SLEEP";

/* RTC 内存中的持久数据 */
RTC_DATA_ATTR static uint32_t sample_count = 0;
RTC_DATA_ATTR static float    last_temp    = 0.0f;

#define SLEEP_DURATION_US  (5 * 60 * 1000000ULL)  // 5 分钟

void app_main(void)
{
    /* 检查唤醒原因 */
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

    if (cause == ESP_SLEEP_WAKEUP_TIMER) {
        ESP_LOGI(TAG, "定时唤醒，第 %lu 次采集", ++sample_count);
    } else {
        ESP_LOGI(TAG, "首次启动");
        sample_count = 0;
    }

    /* 初始化 NVS */
    nvs_flash_init();

    /* 读取传感器 */
    float temperature = read_temperature();  // 用户实现
    float humidity    = read_humidity();
    last_temp = temperature;

    ESP_LOGI(TAG, "温度=%.1f°C 湿度=%.1f%%", temperature, humidity);

    /* 上传数据（如果有 WiFi）*/
    /* upload_data(temperature, humidity); */

    /* 配置定时唤醒并进入深度睡眠 */
    ESP_LOGI(TAG, "进入深度睡眠，5 分钟后唤醒...");
    esp_sleep_enable_timer_wakeup(SLEEP_DURATION_US);
    esp_deep_sleep_start();
    /* 不会执行到这里 */
}
```

---

### 示例2：按键唤醒（低功耗设备）

```c
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define WAKEUP_PIN  GPIO_NUM_0

static const char *TAG = "WAKEUP";

void app_main(void)
{
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

    if (cause == ESP_SLEEP_WAKEUP_EXT0) {
        ESP_LOGI(TAG, "按键唤醒");
        /* 执行按键触发的操作 */
        handle_button_press();
    } else {
        ESP_LOGI(TAG, "首次启动，初始化设备...");
        device_init();
    }

    /* 处理完成后进入深度睡眠，等待下次按键 */
    ESP_LOGI(TAG, "进入深度睡眠，等待按键...");

    /* 配置 GPIO0 低电平唤醒 */
    esp_sleep_enable_ext0_wakeup(WAKEUP_PIN, 0);

    /* 睡眠前配置引脚上拉（保持确定电平）*/
    rtc_gpio_pullup_en(WAKEUP_PIN);
    rtc_gpio_pulldown_dis(WAKEUP_PIN);

    esp_deep_sleep_start();
}
```

---

### 示例3：浅睡眠（快速唤醒）

```c
#include "esp_sleep.h"
#include "driver/gpio.h"

void light_sleep_demo(void)
{
    ESP_LOGI(TAG, "进入浅睡眠...");

    /* 配置唤醒源 */
    gpio_wakeup_enable(GPIO_NUM_0, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();
    esp_sleep_enable_timer_wakeup(10 * 1000000ULL);  // 10 秒超时

    /* 进入浅睡眠（会返回）*/
    esp_light_sleep_start();

    /* 唤醒后继续执行 */
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    if (cause == ESP_SLEEP_WAKEUP_GPIO) {
        ESP_LOGI(TAG, "GPIO 唤醒");
    } else if (cause == ESP_SLEEP_WAKEUP_TIMER) {
        ESP_LOGI(TAG, "定时器超时唤醒");
    }
}
```

---

### 示例4：多唤醒源组合

```c
void configure_multiple_wakeup(void)
{
    /* 同时配置多个唤醒源，任一满足即唤醒 */

    /* 1. 定时器（30 分钟）*/
    esp_sleep_enable_timer_wakeup(30 * 60 * 1000000ULL);

    /* 2. 按键（GPIO0 低电平）*/
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);

    /* 3. 触摸（如果有触摸屏）*/
    /* esp_sleep_enable_touchpad_wakeup(); */

    ESP_LOGI(TAG, "进入深度睡眠（定时器/按键均可唤醒）");
    esp_deep_sleep_start();
}
```

---

## ⚠️ 注意事项

### 1. 深度睡眠后代码重新执行

深度睡眠唤醒 ≠ 从睡眠处继续，而是重新执行 `app_main`。
必须在 `app_main` 开头检查 `esp_sleep_get_wakeup_cause()` 判断是否是唤醒启动。

### 2. RTC 内存大小有限

RTC 慢速内存只有 8KB，不要存放大数组。
大量数据用 NVS 或 Flash 存储。

### 3. 睡眠前关闭外设

进入深度睡眠前，应关闭 WiFi、蓝牙等外设，否则功耗不会降低：
```c
esp_wifi_stop();
esp_wifi_deinit();
esp_deep_sleep_start();
```

### 4. GPIO 状态在深度睡眠中

深度睡眠期间，普通 GPIO 状态不保持（浮空）。
需要保持电平的引脚使用 RTC GPIO 并调用 `rtc_gpio_hold_en()`：
```c
rtc_gpio_hold_en(GPIO_NUM_4);  // 保持当前电平
```

### 5. EXT0/EXT1 只支持 RTC GPIO

GPIO0~GPIO21 是 RTC GPIO，可用于深度睡眠唤醒。
GPIO22 以上不支持 EXT0/EXT1 唤醒。

---

## 📖 相关文档

- [IDF_RTC.md](IDF_RTC.md) — 实时时钟
- [IDF_GPIO.md](IDF_GPIO.md) — GPIO 配置
- [IDF_NVS.md](IDF_NVS.md) — NVS 存储（保存睡眠前数据）
- [README.md](README.md) — 模块导航
