# NVS 非易失性存储模块 API 文档（ESP-IDF）

## 📋 模块概述

NVS（Non-Volatile Storage）是 ESP-IDF 提供的键值对存储系统，数据保存在 Flash 中，断电不丢失：
- 键值对存储（key-value），支持多种数据类型
- 命名空间隔离（不同模块的数据互不干扰）
- 磨损均衡（自动分散写入，延长 Flash 寿命）
- 支持加密存储（NVS Encryption）
- 比直接操作 Flash 简单得多，是保存配置参数的首选方案

---

## 🗺️ 使用场景速查

| 场景 | 推荐方案 |
|------|----------|
| 保存 WiFi 密码、设备配置 | NVS（首选） |
| 保存用户设置（亮度、音量） | NVS |
| 设备首次启动标志 | NVS（存一个 uint8 标志位） |
| 运行计数、统计数据 | NVS（注意写入次数限制） |
| 大量数据（>几KB） | SPIFFS 或 LittleFS |
| 频繁写入（每秒多次） | 外部 EEPROM 或 SPIFFS |

**NVS vs 其他存储方案**

| 方案 | 容量 | 写入次数 | 适用 |
|------|------|---------|------|
| NVS | 受分区大小限制（通常 16KB~64KB） | ~10,000 次/页 | 配置参数，偶发写入 |
| SPIFFS | 受分区大小限制 | 较高 | 文件存储，中等频率写入 |
| 外部 EEPROM | 256B~64KB | ~1,000,000 次 | 频繁写入的小数据 |
| 外部 SPI Flash | 1MB~128MB | ~100,000 次 | 大量数据 |

---

## 📚 相关文件

- **头文件**: `nvs_flash.h`（初始化）
- **头文件**: `nvs.h`（读写操作）

---

## 🔧 数据类型

### nvs_handle_t

NVS 操作句柄（打开命名空间后获得）。

```c
typedef uint32_t nvs_handle_t;
```

---

### nvs_open_mode_t — 打开模式

```c
typedef enum {
    NVS_READONLY  = 0,  // 只读
    NVS_READWRITE = 1,  // 读写
} nvs_open_mode_t;
```

---

## 🎯 核心函数

### 初始化

#### nvs_flash_init()

初始化默认 NVS 分区（必须在使用 NVS 前调用一次）。

```c
esp_err_t nvs_flash_init(void);
```

**返回值：**
- `ESP_OK`: 成功
- `ESP_ERR_NVS_NO_FREE_PAGES`: NVS 分区无可用页（需要擦除）
- `ESP_ERR_NVS_NEW_VERSION_FOUND`: NVS 版本不匹配（需要擦除）

**标准初始化模式（处理擦除情况）：**
```c
esp_err_t ret = nvs_flash_init();
if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
}
ESP_ERROR_CHECK(ret);
```

---

#### nvs_flash_erase()

擦除默认 NVS 分区（清空所有数据）。

```c
esp_err_t nvs_flash_erase(void);
```

> ⚠️ 擦除后所有 NVS 数据丢失，谨慎使用。

---

### 命名空间操作

#### nvs_open()

打开命名空间，获取操作句柄。

```c
esp_err_t nvs_open(const char *name, nvs_open_mode_t open_mode, nvs_handle_t *out_handle);
```

**参数：**
- `name`: 命名空间名称（最长 15 字符）
- `open_mode`: `NVS_READONLY` 或 `NVS_READWRITE`
- `out_handle`: 输出句柄

---

#### nvs_close()

关闭命名空间句柄（释放资源，不影响数据）。

```c
void nvs_close(nvs_handle_t c_handle);
```

---

#### nvs_commit()

将写入操作提交到 Flash（写入后必须调用）。

```c
esp_err_t nvs_commit(nvs_handle_t c_handle);
```

---

#### nvs_erase_key()

删除指定键。

```c
esp_err_t nvs_erase_key(nvs_handle_t c_handle, const char *key);
```

---

#### nvs_erase_all()

清空命名空间内所有键值对。

```c
esp_err_t nvs_erase_all(nvs_handle_t c_handle);
```

---

### 写入函数

| 函数 | 类型 | 说明 |
|------|------|------|
| `nvs_set_i8` | int8_t | 写入有符号 8 位整数 |
| `nvs_set_u8` | uint8_t | 写入无符号 8 位整数 |
| `nvs_set_i16` | int16_t | 写入有符号 16 位整数 |
| `nvs_set_u16` | uint16_t | 写入无符号 16 位整数 |
| `nvs_set_i32` | int32_t | 写入有符号 32 位整数 |
| `nvs_set_u32` | uint32_t | 写入无符号 32 位整数 |
| `nvs_set_i64` | int64_t | 写入有符号 64 位整数 |
| `nvs_set_u64` | uint64_t | 写入无符号 64 位整数 |
| `nvs_set_str` | char* | 写入字符串（以 `\0` 结尾） |
| `nvs_set_blob` | void* | 写入任意二进制数据 |

**通用签名：**
```c
esp_err_t nvs_set_TYPE(nvs_handle_t c_handle, const char *key, TYPE value);
```

**字符串和 blob：**
```c
esp_err_t nvs_set_str(nvs_handle_t c_handle, const char *key, const char *value);
esp_err_t nvs_set_blob(nvs_handle_t c_handle, const char *key,
                        const void *value, size_t length);
```

---

### 读取函数

| 函数 | 类型 |
|------|------|
| `nvs_get_i8` | int8_t |
| `nvs_get_u8` | uint8_t |
| `nvs_get_i16` | int16_t |
| `nvs_get_u16` | uint16_t |
| `nvs_get_i32` | int32_t |
| `nvs_get_u32` | uint32_t |
| `nvs_get_i64` | int64_t |
| `nvs_get_u64` | uint64_t |
| `nvs_get_str` | char* |
| `nvs_get_blob` | void* |

**通用签名：**
```c
esp_err_t nvs_get_TYPE(nvs_handle_t c_handle, const char *key, TYPE *out_value);
```

**字符串读取（两步：先获取长度，再读取）：**
```c
/* 第一步：获取字符串长度（含 \0）*/
size_t required_size;
nvs_get_str(handle, "ssid", NULL, &required_size);

/* 第二步：分配内存并读取 */
char *ssid = malloc(required_size);
nvs_get_str(handle, "ssid", ssid, &required_size);
```

**blob 读取（同样两步）：**
```c
size_t required_size;
nvs_get_blob(handle, "config", NULL, &required_size);
void *config = malloc(required_size);
nvs_get_blob(handle, "config", config, &required_size);
```

**读取返回值：**
- `ESP_OK`: 成功
- `ESP_ERR_NVS_NOT_FOUND`: 键不存在（首次启动时常见）

---

## 💡 完整应用示例

### 示例1：保存和读取 WiFi 配置

```c
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

#define NVS_NAMESPACE "wifi_cfg"
static const char *TAG = "NVS";

/* 保存 WiFi 配置 */
esp_err_t wifi_config_save(const char *ssid, const char *password)
{
    nvs_handle_t handle;
    esp_err_t ret;

    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "打开 NVS 失败: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = nvs_set_str(handle, "ssid", ssid);
    if (ret != ESP_OK) goto cleanup;

    ret = nvs_set_str(handle, "password", password);
    if (ret != ESP_OK) goto cleanup;

    ret = nvs_commit(handle);  // 必须 commit！
    ESP_LOGI(TAG, "WiFi 配置已保存: SSID=%s", ssid);

cleanup:
    nvs_close(handle);
    return ret;
}

/* 读取 WiFi 配置 */
esp_err_t wifi_config_load(char *ssid, size_t ssid_len,
                            char *password, size_t pwd_len)
{
    nvs_handle_t handle;
    esp_err_t ret;

    ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "NVS 命名空间不存在（首次启动）");
        return ret;
    }

    ret = nvs_get_str(handle, "ssid", ssid, &ssid_len);
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "未找到 SSID，使用默认配置");
        nvs_close(handle);
        return ret;
    }

    nvs_get_str(handle, "password", password, &pwd_len);
    ESP_LOGI(TAG, "读取 WiFi 配置: SSID=%s", ssid);

    nvs_close(handle);
    return ESP_OK;
}

void app_main(void)
{
    /* 初始化 NVS（标准模式）*/
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* 尝试读取配置 */
    char ssid[32] = {0};
    char password[64] = {0};

    if (wifi_config_load(ssid, sizeof(ssid), password, sizeof(password)) != ESP_OK) {
        /* 首次启动，保存默认配置 */
        wifi_config_save("MyWiFi", "MyPassword123");
    } else {
        ESP_LOGI(TAG, "使用已保存的 WiFi: %s", ssid);
    }
}
```

---

### 示例2：保存结构体（blob）

```c
#include "nvs_flash.h"
#include "nvs.h"

typedef struct {
    uint8_t  brightness;   // 亮度 0~100
    uint8_t  volume;       // 音量 0~100
    uint32_t boot_count;   // 启动次数
    char     device_name[32]; // 设备名称
} device_config_t;

#define NVS_NS  "device"
#define NVS_KEY "config"

esp_err_t config_save(const device_config_t *cfg)
{
    nvs_handle_t h;
    nvs_open(NVS_NS, NVS_READWRITE, &h);
    esp_err_t ret = nvs_set_blob(h, NVS_KEY, cfg, sizeof(device_config_t));
    if (ret == ESP_OK) nvs_commit(h);
    nvs_close(h);
    return ret;
}

esp_err_t config_load(device_config_t *cfg)
{
    nvs_handle_t h;
    nvs_open(NVS_NS, NVS_READONLY, &h);
    size_t size = sizeof(device_config_t);
    esp_err_t ret = nvs_get_blob(h, NVS_KEY, cfg, &size);
    nvs_close(h);
    return ret;
}

void app_main(void)
{
    /* NVS 初始化（略）*/

    device_config_t cfg;
    if (config_load(&cfg) != ESP_OK) {
        /* 首次启动，使用默认值 */
        cfg.brightness = 80;
        cfg.volume     = 50;
        cfg.boot_count = 0;
        strncpy(cfg.device_name, "ESP32-Device", sizeof(cfg.device_name));
    }

    cfg.boot_count++;
    ESP_LOGI("NVS", "第 %lu 次启动", cfg.boot_count);

    config_save(&cfg);
}
```

---

### 示例3：首次启动检测

```c
/* 经典用法：用 NVS 标志位判断是否首次启动 */

bool is_first_boot(void)
{
    nvs_handle_t h;
    uint8_t flag = 0;

    esp_err_t ret = nvs_open("system", NVS_READONLY, &h);
    if (ret != ESP_OK) return true;  // 命名空间不存在 = 首次启动

    ret = nvs_get_u8(h, "initialized", &flag);
    nvs_close(h);

    return (ret == ESP_ERR_NVS_NOT_FOUND || flag == 0);
}

void mark_initialized(void)
{
    nvs_handle_t h;
    nvs_open("system", NVS_READWRITE, &h);
    nvs_set_u8(h, "initialized", 1);
    nvs_commit(h);
    nvs_close(h);
}
```

---

## ⚠️ 注意事项

### 1. 必须调用 nvs_commit()

写入操作（`nvs_set_*`）只是写入内存缓存，必须调用 `nvs_commit()` 才会真正写入 Flash。
忘记 commit 是最常见的 bug。

### 2. 键名长度限制

键名（key）最长 **15 个字符**（不含 `\0`），超过会被截断或报错。

### 3. 命名空间长度限制

命名空间名称最长 **15 个字符**。

### 4. 写入次数

NVS 使用磨损均衡，但底层 Flash 每页约 10,000 次擦写寿命。
频繁写入（每分钟多次）的数据不适合用 NVS，考虑外部 EEPROM。

### 5. 线程安全

NVS 操作是线程安全的，多个任务可以同时打开不同命名空间。
但同一命名空间的句柄不建议跨任务共享。

### 6. 分区配置

默认 NVS 分区在 `partitions.csv` 中定义：
```csv
nvs, data, nvs, 0x9000, 0x5000   # 20KB NVS 分区
```
如需更大 NVS 空间，修改分区表中的大小。

---

## 📖 相关文档

- [IDF_WiFi.md](IDF_WiFi.md) — WiFi 连接（NVS 保存凭据）
- [README.md](README.md) — 模块导航
