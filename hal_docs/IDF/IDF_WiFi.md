# WiFi 模块 API 文档（ESP-IDF）

## 📋 模块概述

ESP-IDF 的 WiFi 驱动提供完整的 802.11 b/g/n 无线连接能力，包括：
- Station 模式（连接到路由器）
- AP 模式（创建热点）
- Station + AP 共存模式
- WPA/WPA2/WPA3 安全认证
- 事件驱动架构（连接、断开、获取 IP 等事件）
- 自动重连机制

---

## 🗺️ 使用场景速查

| 场景 | 模式 |
|------|------|
| 连接家庭/办公 WiFi，访问互联网 | Station 模式 |
| 设备配网（手机直连设备） | AP 模式 |
| 既能上网又能被手机直连 | Station + AP 共存 |
| 智能家居设备 | Station 模式 + NVS 保存凭据 |
| 无路由器的点对点通信 | AP + Station 或 ESP-NOW |

---

## 📚 相关文件

- **头文件**: `esp_wifi.h`（WiFi 驱动）
- **头文件**: `esp_event.h`（事件循环）
- **头文件**: `esp_netif.h`（网络接口）
- **头文件**: `lwip/inet.h`（IP 地址工具）

---

## 🔧 核心概念

### 事件驱动架构

ESP-IDF WiFi 使用事件循环（Event Loop）通知应用层状态变化：

```
WiFi 驱动 → 事件循环 → 用户回调函数
```

**常用 WiFi 事件：**

| 事件 | 说明 |
|------|------|
| `WIFI_EVENT_STA_START` | Station 模式启动 |
| `WIFI_EVENT_STA_CONNECTED` | 连接到 AP（未获取 IP）|
| `WIFI_EVENT_STA_DISCONNECTED` | 断开连接 |
| `IP_EVENT_STA_GOT_IP` | 获取到 IP 地址（可以通信了）|
| `WIFI_EVENT_AP_START` | AP 模式启动 |
| `WIFI_EVENT_AP_STACONNECTED` | 有设备连接到本机 AP |
| `WIFI_EVENT_AP_STADISCONNECTED` | 设备从本机 AP 断开 |
| `WIFI_EVENT_SCAN_DONE` | WiFi 扫描完成 |

---

## 🎯 核心函数

### 系统初始化（必须步骤）

```c
/* 1. 初始化 NVS（WiFi 驱动需要 NVS 存储校准数据）*/
nvs_flash_init();

/* 2. 初始化网络接口 */
esp_netif_init();

/* 3. 创建默认事件循环 */
esp_event_loop_create_default();

/* 4. 创建默认 Station 网络接口 */
esp_netif_create_default_wifi_sta();

/* 5. 初始化 WiFi 驱动 */
wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
esp_wifi_init(&cfg);
```

---

#### esp_wifi_set_mode()

设置 WiFi 工作模式。

```c
esp_err_t esp_wifi_set_mode(wifi_mode_t mode);
```

| 模式 | 说明 |
|------|------|
| `WIFI_MODE_STA` | Station 模式 |
| `WIFI_MODE_AP` | AP 模式 |
| `WIFI_MODE_APSTA` | Station + AP 共存 |
| `WIFI_MODE_NULL` | 关闭 WiFi |

---

#### esp_wifi_set_config()

配置 WiFi 参数（SSID、密码等）。

```c
esp_err_t esp_wifi_set_config(wifi_interface_t interface, wifi_config_t *conf);
```

**Station 配置：**
```c
wifi_config_t sta_config = {
    .sta = {
        .ssid     = "MyWiFi",
        .password = "MyPassword",
        .threshold.authmode = WIFI_AUTH_WPA2_PSK,  // 最低安全要求
        .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,          // WPA3 支持
    },
};
esp_wifi_set_config(WIFI_IF_STA, &sta_config);
```

**AP 配置：**
```c
wifi_config_t ap_config = {
    .ap = {
        .ssid           = "ESP32-AP",
        .ssid_len       = strlen("ESP32-AP"),
        .channel        = 1,
        .password       = "12345678",
        .max_connection = 4,
        .authmode       = WIFI_AUTH_WPA2_PSK,
    },
};
esp_wifi_set_config(WIFI_IF_AP, &ap_config);
```

---

#### esp_wifi_start()

启动 WiFi（根据模式开始工作）。

```c
esp_err_t esp_wifi_start(void);
```

---

#### esp_wifi_connect()

发起连接（Station 模式，在 `WIFI_EVENT_STA_START` 事件后调用）。

```c
esp_err_t esp_wifi_connect(void);
```

---

#### esp_wifi_disconnect()

断开当前连接。

```c
esp_err_t esp_wifi_disconnect(void);
```

---

#### esp_wifi_stop()

停止 WiFi。

```c
esp_err_t esp_wifi_stop(void);
```

---

#### esp_wifi_scan_start()

扫描附近 WiFi 热点。

```c
esp_err_t esp_wifi_scan_start(const wifi_scan_config_t *config, bool block);
```

---

#### esp_wifi_scan_get_ap_records()

获取扫描结果。

```c
esp_err_t esp_wifi_scan_get_ap_records(uint16_t *number, wifi_ap_record_t *ap_records);
```

---

### 事件注册

#### esp_event_handler_instance_register()

注册事件回调。

```c
esp_err_t esp_event_handler_instance_register(
    esp_event_base_t event_base,
    int32_t event_id,
    esp_event_handler_t event_handler,
    void *event_handler_arg,
    esp_event_handler_instance_t *instance);
```

**常用写法：**
```c
/* 注册所有 WiFi 事件 */
esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                     &wifi_event_handler, NULL, NULL);

/* 注册 IP 获取事件 */
esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                     &wifi_event_handler, NULL, NULL);
```

---

## 💡 完整应用示例

### 示例1：Station 模式连接 WiFi（带重连）

```c
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "freertos/event_groups.h"

#define WIFI_SSID      "MyWiFi"
#define WIFI_PASSWORD  "MyPassword"
#define MAX_RETRY      5

static const char *TAG = "WIFI";
static int retry_count = 0;

/* 事件组：用于通知主任务 WiFi 状态 */
static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t *disc = (wifi_event_sta_disconnected_t *)event_data;
        ESP_LOGW(TAG, "断开连接，原因: %d", disc->reason);

        if (retry_count < MAX_RETRY) {
            esp_wifi_connect();
            retry_count++;
            ESP_LOGI(TAG, "重连中... (%d/%d)", retry_count, MAX_RETRY);
        } else {
            xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGE(TAG, "连接失败，已达最大重试次数");
        }

    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "获取 IP: " IPSTR, IP2STR(&event->ip_info.ip));
        retry_count = 0;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    wifi_event_group = xEventGroupCreate();

    /* 系统初始化 */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* 注册事件 */
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                         &wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                         &wifi_event_handler, NULL, NULL);

    /* 配置 WiFi */
    wifi_config_t wifi_config = {
        .sta = {
            .ssid     = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "正在连接 WiFi: %s", WIFI_SSID);

    /* 等待连接结果（最多 10 秒）*/
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                            pdFALSE, pdFALSE,
                                            pdMS_TO_TICKS(10000));

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "WiFi 连接成功");
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "WiFi 连接失败");
    } else {
        ESP_LOGE(TAG, "WiFi 连接超时");
    }
}

void app_main(void)
{
    /* 初始化 NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init_sta();
}
```

---

### 示例2：AP 模式（创建热点）

```c
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"

#define AP_SSID     "ESP32-Config"
#define AP_PASSWORD "12345678"
#define AP_CHANNEL  1
#define AP_MAX_CONN 4

static const char *TAG = "WIFI_AP";

static void ap_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "设备连接: " MACSTR ", AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "设备断开: " MACSTR, MAC2STR(event->mac));
    }
}

void wifi_init_ap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                         &ap_event_handler, NULL, NULL);

    wifi_config_t ap_config = {
        .ap = {
            .ssid           = AP_SSID,
            .ssid_len       = strlen(AP_SSID),
            .channel        = AP_CHANNEL,
            .password       = AP_PASSWORD,
            .max_connection = AP_MAX_CONN,
            .authmode       = WIFI_AUTH_WPA2_PSK,
        },
    };

    /* 无密码时设置为开放模式 */
    if (strlen(AP_PASSWORD) == 0) {
        ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "AP 已启动: SSID=%s, 密码=%s", AP_SSID, AP_PASSWORD);
}
```

---

### 示例3：WiFi 扫描

```c
#include "esp_wifi.h"
#include "esp_log.h"

static const char *TAG = "WIFI_SCAN";

void wifi_scan(void)
{
    /* 初始化（略，同 Station 模式）*/

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    /* 开始扫描（阻塞直到完成）*/
    wifi_scan_config_t scan_cfg = {
        .ssid        = NULL,  // NULL = 扫描所有
        .bssid       = NULL,
        .channel     = 0,     // 0 = 所有信道
        .show_hidden = true,  // 显示隐藏 SSID
    };
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_cfg, true));

    /* 获取结果 */
    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);
    ESP_LOGI(TAG, "发现 %d 个 WiFi 热点", ap_count);

    wifi_ap_record_t *ap_list = malloc(ap_count * sizeof(wifi_ap_record_t));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_list));

    for (int i = 0; i < ap_count; i++) {
        ESP_LOGI(TAG, "[%2d] SSID: %-32s | RSSI: %3d dBm | 信道: %2d",
                 i + 1,
                 ap_list[i].ssid,
                 ap_list[i].rssi,
                 ap_list[i].primary);
    }

    free(ap_list);
}
```

---

### 示例4：从 NVS 读取 WiFi 凭据

```c
/* 结合 NVS 实现配网后自动连接 */

#include "nvs.h"
#include "esp_wifi.h"

bool load_wifi_credentials(char *ssid, char *password)
{
    nvs_handle_t h;
    if (nvs_open("wifi", NVS_READONLY, &h) != ESP_OK) return false;

    size_t ssid_len = 32, pwd_len = 64;
    bool ok = (nvs_get_str(h, "ssid", ssid, &ssid_len) == ESP_OK) &&
              (nvs_get_str(h, "password", password, &pwd_len) == ESP_OK);
    nvs_close(h);
    return ok;
}

void save_wifi_credentials(const char *ssid, const char *password)
{
    nvs_handle_t h;
    nvs_open("wifi", NVS_READWRITE, &h);
    nvs_set_str(h, "ssid", ssid);
    nvs_set_str(h, "password", password);
    nvs_commit(h);
    nvs_close(h);
}
```

---

## ⚠️ 注意事项

### 1. ADC2 与 WiFi 冲突

WiFi 启用后 ADC2 不可用。需要模拟采集的项目只使用 ADC1（GPIO1~GPIO10）。

### 2. 初始化顺序

必须按以下顺序初始化：
1. `nvs_flash_init()`
2. `esp_netif_init()`
3. `esp_event_loop_create_default()`
4. `esp_netif_create_default_wifi_sta()` 或 `_ap()`
5. `esp_wifi_init()`
6. 注册事件
7. `esp_wifi_set_mode()` + `esp_wifi_set_config()`
8. `esp_wifi_start()`

### 3. 事件回调中不要阻塞

事件回调在事件循环任务中执行，不能调用阻塞 API（如 `vTaskDelay`）。
需要阻塞操作时，通过队列或事件组通知其他任务。

### 4. 断线重连

`WIFI_EVENT_STA_DISCONNECTED` 事件中调用 `esp_wifi_connect()` 实现自动重连。
注意限制重试次数，避免无限重连消耗电量。

### 5. 信号强度

`wifi_ap_record_t.rssi` 单位为 dBm，参考值：
- > -50 dBm：信号极好
- -50 ~ -70 dBm：信号良好
- -70 ~ -80 dBm：信号一般
- < -80 dBm：信号差，可能断线

### 6. 功耗优化

```c
/* 启用 WiFi 省电模式（降低功耗，略微增加延迟）*/
esp_wifi_set_ps(WIFI_PS_MIN_MODEM);  // 最小调制解调器省电
esp_wifi_set_ps(WIFI_PS_MAX_MODEM);  // 最大省电（延迟更高）
esp_wifi_set_ps(WIFI_PS_NONE);       // 关闭省电（默认，延迟最低）
```

---

## 📖 相关文档

- [IDF_NVS.md](IDF_NVS.md) — NVS 存储（保存 WiFi 凭据）
- [README.md](README.md) — 模块导航
