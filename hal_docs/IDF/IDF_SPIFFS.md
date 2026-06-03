# SPIFFS / LittleFS — 文件系统 API 文档（ESP-IDF）

## 📋 模块概述

ESP-IDF 支持在 Flash 分区上挂载文件系统，提供标准 C 文件 API（`fopen`/`fread`/`fwrite`）：

| 文件系统 | 特点 | 推荐场景 |
|---------|------|---------|
| SPIFFS | 无目录结构，文件名即路径，简单 | 小量配置文件、网页文件 |
| LittleFS | 支持目录，掉电安全，性能更好 | 推荐，替代 SPIFFS |
| FAT | 支持目录，兼容 SD 卡 | SD 卡、大容量存储 |

> **推荐使用 LittleFS**，比 SPIFFS 更可靠（掉电安全），支持目录结构。

---

## 🗺️ 使用场景速查

| 场景 | 方案 |
|------|------|
| 存储 HTML/CSS/JS 网页文件 | SPIFFS 或 LittleFS |
| 存储图片、字体文件 | LittleFS |
| 存储日志文件 | LittleFS |
| 存储配置文件（JSON）| LittleFS 或 NVS |
| 大量数据（>几MB）| SD 卡 + FAT |

**文件系统 vs NVS：**

| 特性 | 文件系统 | NVS |
|------|---------|-----|
| 存储方式 | 文件（任意格式）| 键值对 |
| 容量 | 受分区大小限制（通常 1~4MB）| 通常 16~64KB |
| 写入频率 | 中等 | 低（配置参数）|
| 适用 | 文件、日志、网页 | 配置参数 |

---

## 📚 相关文件

- **SPIFFS 头文件**: `esp_spiffs.h`
- **LittleFS 头文件**: `esp_littlefs.h`（需在 `idf_component.yml` 中添加）
- **标准文件 API**: `stdio.h`、`dirent.h`

---

## 一、分区表配置

在 `partitions.csv` 中添加文件系统分区：

```csv
# Name,   Type, SubType,  Offset,   Size
nvs,      data, nvs,      0x9000,   0x5000
otadata,  data, ota,      0xe000,   0x2000
app0,     app,  ota_0,    0x10000,  0x640000
spiffs,   data, spiffs,   0x670000, 0x190000   # 1.6MB SPIFFS/LittleFS 分区
```

---

## 二、SPIFFS

### 挂载

```c
#include "esp_spiffs.h"
#include "esp_log.h"

static const char *TAG = "SPIFFS";

void spiffs_init(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path              = "/spiffs",  // 挂载点
        .partition_label        = NULL,        // NULL = 使用第一个 spiffs 分区
        .max_files              = 5,           // 最大同时打开文件数
        .format_if_mount_failed = true,        // 挂载失败时格式化
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "挂载或格式化失败");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "未找到 SPIFFS 分区");
        } else {
            ESP_LOGE(TAG, "挂载失败: %s", esp_err_to_name(ret));
        }
        return;
    }

    /* 查询分区信息 */
    size_t total = 0, used = 0;
    esp_spiffs_info(NULL, &total, &used);
    ESP_LOGI(TAG, "SPIFFS 已挂载: 总计=%d bytes, 已用=%d bytes", total, used);
}

void spiffs_deinit(void)
{
    esp_vfs_spiffs_unregister(NULL);
}
```

---

## 三、LittleFS（推荐）

### 添加依赖

在 `main/idf_component.yml` 中添加：
```yaml
dependencies:
  joltwallet/littlefs: "^1.14.8"
```

### 挂载

```c
#include "esp_littlefs.h"
#include "esp_log.h"

static const char *TAG = "LITTLEFS";

void littlefs_init(void)
{
    esp_vfs_littlefs_conf_t conf = {
        .base_path              = "/lfs",
        .partition_label        = "spiffs",  // 分区标签（与 partitions.csv 一致）
        .format_if_mount_failed = true,
        .dont_mount             = false,
    };

    esp_err_t ret = esp_vfs_littlefs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LittleFS 挂载失败: %s", esp_err_to_name(ret));
        return;
    }

    size_t total = 0, used = 0;
    esp_littlefs_info("spiffs", &total, &used);
    ESP_LOGI(TAG, "LittleFS 已挂载: 总计=%d bytes, 已用=%d bytes", total, used);
}
```

---

## 四、标准文件操作

挂载后使用标准 C 文件 API，路径以挂载点开头：

### 写文件

```c
#include <stdio.h>
#include "esp_log.h"

static const char *TAG = "FS";

void write_file(const char *path, const char *content)
{
    FILE *f = fopen(path, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "打开文件失败: %s", path);
        return;
    }
    fprintf(f, "%s", content);
    fclose(f);
    ESP_LOGI(TAG, "写入文件: %s", path);
}

/* 使用示例 */
write_file("/lfs/config.txt", "brightness=80\nvolume=50\n");
```

---

### 读文件

```c
void read_file(const char *path)
{
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "文件不存在: %s", path);
        return;
    }

    char line[128];
    while (fgets(line, sizeof(line), f)) {
        /* 去掉末尾换行 */
        line[strcspn(line, "\n")] = '\0';
        ESP_LOGI(TAG, "读取: %s", line);
    }
    fclose(f);
}
```

---

### 追加写入

```c
void append_log(const char *path, const char *message)
{
    FILE *f = fopen(path, "a");  // "a" = 追加模式
    if (f == NULL) return;

    time_t now;
    time(&now);
    fprintf(f, "[%lld] %s\n", (long long)now, message);
    fclose(f);
}
```

---

### 读写二进制文件

```c
typedef struct {
    float temperature;
    float humidity;
    time_t timestamp;
} sensor_record_t;

void save_record(const sensor_record_t *rec)
{
    FILE *f = fopen("/lfs/data.bin", "ab");  // 追加二进制
    if (!f) return;
    fwrite(rec, sizeof(sensor_record_t), 1, f);
    fclose(f);
}

int load_records(sensor_record_t *buf, int max_count)
{
    FILE *f = fopen("/lfs/data.bin", "rb");
    if (!f) return 0;
    int count = fread(buf, sizeof(sensor_record_t), max_count, f);
    fclose(f);
    return count;
}
```

---

### 目录操作（LittleFS）

```c
#include <dirent.h>
#include <sys/stat.h>

/* 列出目录内容 */
void list_dir(const char *path)
{
    DIR *dir = opendir(path);
    if (!dir) {
        ESP_LOGE(TAG, "打开目录失败: %s", path);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        ESP_LOGI(TAG, "  %s %s",
                 entry->d_type == DT_DIR ? "[DIR]" : "[FILE]",
                 entry->d_name);
    }
    closedir(dir);
}

/* 创建目录 */
void create_dir(const char *path)
{
    mkdir(path, 0777);
}

/* 删除文件 */
void delete_file(const char *path)
{
    if (remove(path) == 0) {
        ESP_LOGI(TAG, "已删除: %s", path);
    }
}

/* 检查文件是否存在 */
bool file_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

/* 获取文件大小 */
long file_size(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0) return -1;
    return st.st_size;
}
```

---

## 💡 完整应用示例

### 示例：配置文件读写（JSON 格式）

```c
#include "esp_littlefs.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "CONFIG";
#define CONFIG_PATH "/lfs/config.json"

/* 保存配置 */
void config_save(int brightness, int volume, const char *wifi_ssid)
{
    FILE *f = fopen(CONFIG_PATH, "w");
    if (!f) {
        ESP_LOGE(TAG, "无法创建配置文件");
        return;
    }
    fprintf(f, "{\n");
    fprintf(f, "  \"brightness\": %d,\n", brightness);
    fprintf(f, "  \"volume\": %d,\n", volume);
    fprintf(f, "  \"wifi_ssid\": \"%s\"\n", wifi_ssid);
    fprintf(f, "}\n");
    fclose(f);
    ESP_LOGI(TAG, "配置已保存");
}

/* 读取配置（简单解析）*/
void config_load(int *brightness, int *volume, char *wifi_ssid, size_t ssid_len)
{
    FILE *f = fopen(CONFIG_PATH, "r");
    if (!f) {
        ESP_LOGW(TAG, "配置文件不存在，使用默认值");
        *brightness = 80;
        *volume     = 50;
        strncpy(wifi_ssid, "MyWiFi", ssid_len);
        return;
    }

    char line[128];
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "  \"brightness\": %d", brightness) == 1) continue;
        if (sscanf(line, "  \"volume\": %d", volume) == 1) continue;
        /* 简单字符串解析 */
        char *ssid_start = strstr(line, "\"wifi_ssid\": \"");
        if (ssid_start) {
            ssid_start += strlen("\"wifi_ssid\": \"");
            char *ssid_end = strchr(ssid_start, '"');
            if (ssid_end) {
                size_t len = ssid_end - ssid_start;
                if (len < ssid_len) {
                    strncpy(wifi_ssid, ssid_start, len);
                    wifi_ssid[len] = '\0';
                }
            }
        }
    }
    fclose(f);
    ESP_LOGI(TAG, "配置已加载: 亮度=%d 音量=%d WiFi=%s",
             *brightness, *volume, wifi_ssid);
}
```

---

## ⚠️ 注意事项

### 1. SPIFFS 无目录支持

SPIFFS 不支持真正的目录，文件名中的 `/` 只是名称的一部分：
```c
/* SPIFFS 中这是一个文件名，不是目录 */
fopen("/spiffs/dir/file.txt", "w");  // 文件名 = "dir/file.txt"
```

### 2. LittleFS 掉电安全

LittleFS 使用日志结构，掉电不会损坏文件系统。
SPIFFS 掉电可能导致文件系统损坏，需要格式化。

### 3. 文件系统大小

文件系统分区不能太小（LittleFS 最小约 128KB），
也不要太大（占用 OTA 空间）。典型配置：1~2MB。

### 4. 同时打开文件数

`max_files` 参数限制同时打开的文件数，每个文件占用约 200 字节内存。

---

## 📖 相关文档

- [IDF_NVS.md](IDF_NVS.md) — NVS 存储（配置参数）
- [IDF_WiFi.md](IDF_WiFi.md) — WiFi（HTTP 服务器提供文件）
- [README.md](README.md) — 模块导航
