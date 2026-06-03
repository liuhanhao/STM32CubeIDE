# IDF Core — 系统核心 API 文档（ESP-IDF）

## 📋 模块概述

ESP-IDF 系统核心提供项目运行的基础设施，包括：
- 系统初始化与启动流程
- 日志系统（ESP_LOG）
- 芯片信息与 Flash 信息查询
- 堆内存管理
- 系统重启与复位原因
- FreeRTOS 任务工具函数

---

## 📚 相关文件

- **头文件**: `esp_system.h`、`esp_log.h`、`esp_chip_info.h`、`esp_flash.h`
- **头文件**: `esp_heap_caps.h`、`freertos/FreeRTOS.h`、`freertos/task.h`

---

## 一、启动入口

### app_main()

ESP-IDF 应用程序入口（相当于 `main()`）。

```c
void app_main(void)
{
    /* 所有初始化和任务创建在此进行 */
    /* app_main 本身运行在一个 FreeRTOS 任务中（优先级 1，栈 8KB）*/
    /* app_main 返回后该任务被删除，其他任务继续运行 */
}
```

**注意：** `app_main` 不是 `main()`，不需要返回值，也不需要 `while(1)`。
如果需要持续运行，在 `app_main` 中创建 FreeRTOS 任务。

---

## 二、日志系统（ESP_LOG）

### 日志级别

| 级别 | 宏 | 说明 |
|------|-----|------|
| Error | `ESP_LOGE` | 严重错误，系统可能无法继续 |
| Warning | `ESP_LOGW` | 警告，可能有问题 |
| Info | `ESP_LOGI` | 一般信息（默认显示）|
| Debug | `ESP_LOGD` | 调试信息（默认不显示）|
| Verbose | `ESP_LOGV` | 详细信息（默认不显示）|

### 日志宏

```c
#include "esp_log.h"

static const char *TAG = "MY_MODULE";  // 模块标签

ESP_LOGE(TAG, "错误: %d", error_code);
ESP_LOGW(TAG, "警告: %s", message);
ESP_LOGI(TAG, "信息: 温度=%.1f°C", temp);
ESP_LOGD(TAG, "调试: 寄存器值=0x%08X", reg_val);
ESP_LOGV(TAG, "详细: 循环计数=%d", count);
```

**输出格式：**
```
I (1234) MY_MODULE: 信息: 温度=25.6°C
^  ^      ^          ^
|  |      |          消息内容
|  |      模块标签
|  启动后毫秒数
日志级别
```

### 运行时修改日志级别

```c
/* 设置特定标签的日志级别 */
esp_log_level_set("MY_MODULE", ESP_LOG_DEBUG);

/* 设置所有模块的日志级别 */
esp_log_level_set("*", ESP_LOG_INFO);

/* 完全关闭某模块日志 */
esp_log_level_set("MY_MODULE", ESP_LOG_NONE);
```

### 编译时日志级别

在 `sdkconfig.defaults` 中设置：
```ini
CONFIG_LOG_DEFAULT_LEVEL_INFO=y   # 默认 Info 级别
CONFIG_LOG_MAXIMUM_LEVEL_DEBUG=y  # 最高允许 Debug 级别
```

### 十六进制数据打印

```c
/* 打印字节数组（十六进制格式）*/
uint8_t data[] = {0x01, 0x02, 0x03, 0xFF};
ESP_LOG_BUFFER_HEX(TAG, data, sizeof(data));
/* 输出: I (xxx) TAG: 01 02 03 ff */

/* 带 ASCII 的十六进制打印 */
ESP_LOG_BUFFER_HEXDUMP(TAG, data, sizeof(data), ESP_LOG_INFO);
```

---

## 三、错误处理

### ESP_ERROR_CHECK()

检查返回值，失败时打印错误信息并 abort。

```c
ESP_ERROR_CHECK(esp_wifi_init(&cfg));
/* 等价于：
 * esp_err_t ret = esp_wifi_init(&cfg);
 * if (ret != ESP_OK) {
 *     ESP_LOGE("ESP_ERROR_CHECK", "Failed: %s", esp_err_to_name(ret));
 *     abort();
 * }
 */
```

### ESP_ERROR_CHECK_WITHOUT_ABORT()

检查返回值，失败时打印错误但不 abort。

```c
ESP_ERROR_CHECK_WITHOUT_ABORT(some_function());
```

### esp_err_to_name()

将错误码转换为可读字符串。

```c
esp_err_t ret = some_function();
if (ret != ESP_OK) {
    ESP_LOGE(TAG, "失败: %s (0x%x)", esp_err_to_name(ret), ret);
}
```

**常用错误码：**

| 错误码 | 值 | 说明 |
|--------|-----|------|
| `ESP_OK` | 0 | 成功 |
| `ESP_FAIL` | -1 | 通用失败 |
| `ESP_ERR_NO_MEM` | 0x101 | 内存不足 |
| `ESP_ERR_INVALID_ARG` | 0x102 | 参数无效 |
| `ESP_ERR_INVALID_STATE` | 0x103 | 状态无效 |
| `ESP_ERR_NOT_FOUND` | 0x105 | 未找到 |
| `ESP_ERR_NOT_SUPPORTED` | 0x106 | 不支持 |
| `ESP_ERR_TIMEOUT` | 0x107 | 超时 |

---

## 四、芯片与系统信息

### esp_chip_info()

获取芯片信息。

```c
#include "esp_chip_info.h"

esp_chip_info_t chip;
esp_chip_info(&chip);

ESP_LOGI(TAG, "芯片型号: %d", chip.model);
ESP_LOGI(TAG, "核心数: %d", chip.cores);
ESP_LOGI(TAG, "版本: %d", chip.revision);
ESP_LOGI(TAG, "功能: WiFi=%d, BT=%d, BLE=%d",
         (chip.features & CHIP_FEATURE_WIFI_BGN) != 0,
         (chip.features & CHIP_FEATURE_BT) != 0,
         (chip.features & CHIP_FEATURE_BLE) != 0);
```

**chip.model 值：**

| 值 | 芯片 |
|----|------|
| `CHIP_ESP32` | ESP32 |
| `CHIP_ESP32S2` | ESP32-S2 |
| `CHIP_ESP32S3` | ESP32-S3 |
| `CHIP_ESP32C3` | ESP32-C3 |

---

### esp_flash_get_size()

获取 Flash 大小。

```c
#include "esp_flash.h"

uint32_t flash_size = 0;
esp_flash_get_size(NULL, &flash_size);
ESP_LOGI(TAG, "Flash: %lu MB", flash_size / (1024 * 1024));
```

---

### esp_psram_is_initialized() / esp_psram_get_size()

检查 PSRAM 状态。

```c
#include "esp_psram.h"

if (esp_psram_is_initialized()) {
    size_t psram_size = esp_psram_get_size();
    ESP_LOGI(TAG, "PSRAM: %u MB", (unsigned)(psram_size / (1024 * 1024)));
} else {
    ESP_LOGW(TAG, "PSRAM 未初始化");
}
```

---

### esp_get_idf_version()

获取 ESP-IDF 版本字符串。

```c
ESP_LOGI(TAG, "IDF 版本: %s", esp_get_idf_version());
/* 输出: IDF 版本: v5.1.2 */
```

---

## 五、堆内存管理

### 标准内存分配

```c
#include <stdlib.h>

void *ptr = malloc(1024);
if (ptr == NULL) {
    ESP_LOGE(TAG, "内存分配失败");
    return;
}
free(ptr);
```

### heap_caps_malloc() — 指定内存类型分配

```c
#include "esp_heap_caps.h"

/* 从内部 SRAM 分配（速度最快）*/
void *ptr = heap_caps_malloc(1024, MALLOC_CAP_INTERNAL);

/* 从 PSRAM 分配（容量大）*/
void *ptr = heap_caps_malloc(1024 * 1024, MALLOC_CAP_SPIRAM);

/* 从 DMA 可访问内存分配（SPI/UART DMA 必须）*/
void *ptr = heap_caps_malloc(1024, MALLOC_CAP_DMA);

/* 32 位对齐内存 */
void *ptr = heap_caps_malloc(1024, MALLOC_CAP_32BIT);

free(ptr);  // 释放方式相同
```

**内存类型标志：**

| 标志 | 说明 |
|------|------|
| `MALLOC_CAP_INTERNAL` | 内部 SRAM |
| `MALLOC_CAP_SPIRAM` | PSRAM（外部 SPI RAM）|
| `MALLOC_CAP_DMA` | DMA 可访问（内部 SRAM）|
| `MALLOC_CAP_32BIT` | 32 位对齐 |
| `MALLOC_CAP_DEFAULT` | 默认（优先内部）|

### 查询可用内存

```c
/* 内部堆可用内存 */
ESP_LOGI(TAG, "内部堆可用: %lu bytes", esp_get_free_heap_size());

/* 内部堆最小剩余（历史最低值，用于检测内存泄漏）*/
ESP_LOGI(TAG, "内部堆最小: %lu bytes", esp_get_minimum_free_heap_size());

/* 指定类型的可用内存 */
size_t psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
ESP_LOGI(TAG, "PSRAM 可用: %u bytes", psram_free);

/* 最大可分配的连续块 */
size_t max_block = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
ESP_LOGI(TAG, "最大连续块: %u bytes", max_block);
```

---

## 六、系统重启与复位

### esp_restart()

软件重启。

```c
#include "esp_system.h"

ESP_LOGI(TAG, "系统将在 3 秒后重启...");
vTaskDelay(pdMS_TO_TICKS(3000));
esp_restart();
```

### esp_reset_reason()

获取上次复位原因。

```c
esp_reset_reason_t reason = esp_reset_reason();
switch (reason) {
    case ESP_RST_POWERON:   ESP_LOGI(TAG, "上电复位"); break;
    case ESP_RST_SW:        ESP_LOGI(TAG, "软件复位"); break;
    case ESP_RST_PANIC:     ESP_LOGE(TAG, "异常崩溃复位"); break;
    case ESP_RST_INT_WDT:   ESP_LOGE(TAG, "中断看门狗复位"); break;
    case ESP_RST_TASK_WDT:  ESP_LOGE(TAG, "任务看门狗复位"); break;
    case ESP_RST_WDT:       ESP_LOGE(TAG, "其他看门狗复位"); break;
    case ESP_RST_DEEPSLEEP: ESP_LOGI(TAG, "深度睡眠唤醒"); break;
    case ESP_RST_BROWNOUT:  ESP_LOGE(TAG, "欠压复位"); break;
    default:                ESP_LOGW(TAG, "未知复位原因: %d", reason); break;
}
```

---

## 七、时间与延时

### vTaskDelay()

FreeRTOS 任务延时（让出 CPU）。

```c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

vTaskDelay(pdMS_TO_TICKS(1000));  // 延时 1000ms
vTaskDelay(pdMS_TO_TICKS(100));   // 延时 100ms
```

### esp_timer_get_time()

获取系统启动后的微秒数（64 位，不溢出）。

```c
#include "esp_timer.h"

int64_t now_us = esp_timer_get_time();
ESP_LOGI(TAG, "运行时间: %lld ms", now_us / 1000);
```

### xTaskGetTickCount()

获取 FreeRTOS 系统节拍数。

```c
TickType_t ticks = xTaskGetTickCount();
uint32_t ms = ticks * portTICK_PERIOD_MS;
```

---

## 八、完整启动示例

```c
#include "esp_log.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_psram.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "nvs_flash.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    /* 打印复位原因 */
    esp_reset_reason_t reason = esp_reset_reason();
    ESP_LOGI(TAG, "复位原因: %d", reason);

    /* 芯片信息 */
    esp_chip_info_t chip;
    esp_chip_info(&chip);
    ESP_LOGI(TAG, "芯片: ESP32-S3, 核心: %d, 版本: %d",
             chip.cores, chip.revision);

    /* Flash 大小 */
    uint32_t flash_size;
    esp_flash_get_size(NULL, &flash_size);
    ESP_LOGI(TAG, "Flash: %lu MB", flash_size / (1024 * 1024));

    /* PSRAM */
    if (esp_psram_is_initialized()) {
        ESP_LOGI(TAG, "PSRAM: %u MB",
                 (unsigned)(esp_psram_get_size() / (1024 * 1024)));
    }

    /* 内存状态 */
    ESP_LOGI(TAG, "内部堆可用: %lu bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "PSRAM 可用: %u bytes",
             heap_caps_get_free_size(MALLOC_CAP_SPIRAM));

    /* IDF 版本 */
    ESP_LOGI(TAG, "IDF 版本: %s", esp_get_idf_version());

    /* 初始化 NVS（大多数项目需要）*/
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "系统初始化完成");
}
```

---

## ⚠️ 注意事项

### 1. TAG 命名规范

每个源文件定义一个 `static const char *TAG`，用模块名命名，方便过滤日志：
```c
static const char *TAG = "WIFI";    // wifi.c
static const char *TAG = "SENSOR";  // sensor.c
```

### 2. 内存分配失败处理

嵌入式系统内存有限，`malloc` 可能返回 NULL，必须检查：
```c
void *buf = malloc(size);
if (!buf) {
    ESP_LOGE(TAG, "malloc(%d) 失败，可用: %lu",
             size, esp_get_free_heap_size());
    return ESP_ERR_NO_MEM;
}
```

### 3. DMA 内存要求

SPI、UART、I2C 的 DMA 传输缓冲区必须在内部 SRAM 中（`MALLOC_CAP_DMA`），
PSRAM 中的缓冲区不能直接用于 DMA。

---

## 📖 相关文档

- [IDF_NVS.md](IDF_NVS.md) — 非易失性存储
- [IDF_DeepSleep.md](IDF_DeepSleep.md) — 深度睡眠
- [README.md](README.md) — 模块导航
