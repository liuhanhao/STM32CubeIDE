# MQTT — 消息队列遥测传输 API 文档（ESP-IDF）

## 📋 模块概述

MQTT（Message Queuing Telemetry Transport）是轻量级发布/订阅消息协议，ESP-IDF 内置 MQTT 客户端：
- 支持 MQTT 3.1、3.1.1、5.0
- 支持 QoS 0/1/2
- 支持 TLS/SSL 加密连接
- 自动重连
- 遗嘱消息（Last Will）

---

## 🗺️ 使用场景速查

| 场景 | 说明 |
|------|------|
| IoT 设备上报数据 | 发布传感器数据到 Broker |
| 远程控制设备 | 订阅控制主题，接收命令 |
| 设备状态监控 | 遗嘱消息检测设备离线 |
| 多设备协同 | 通过 Broker 转发消息 |

**MQTT vs HTTP：**

| 特性 | MQTT | HTTP |
|------|------|------|
| 协议开销 | 极小（2 字节头）| 较大 |
| 连接方式 | 长连接，实时推送 | 短连接，轮询 |
| 双向通信 | ✅ 天然支持 | ❌ 需要轮询或 WebSocket |
| 适用场景 | IoT 实时数据、控制 | REST API、文件下载 |

---

## 📚 相关文件

- **头文件**: `mqtt_client.h`

---

## 🔧 数据类型

### esp_mqtt_client_config_t — 客户端配置

```c
typedef struct {
    struct {
        const char *uri;          // Broker URI（如 "mqtt://broker.hivemq.com"）
        const char *host;         // 主机名（与 uri 二选一）
        uint32_t port;            // 端口（默认 1883，TLS 8883）
        const char *username;     // 用户名
        const char *password;     // 密码
        const char *client_id;    // 客户端 ID（唯一）
    } broker;
    struct {
        const char *certificate;  // TLS 证书
        bool skip_cert_common_name_check; // 跳过证书验证
    } credentials;
    struct {
        int keepalive;            // 心跳间隔（秒，默认 120）
        bool disable_auto_reconnect; // 禁用自动重连
        const char *lwt_topic;    // 遗嘱主题
        const char *lwt_msg;      // 遗嘱消息
        int lwt_qos;              // 遗嘱 QoS
        int lwt_retain;           // 遗嘱保留
    } session;
} esp_mqtt_client_config_t;
```

---

### MQTT 事件类型

| 事件 | 说明 |
|------|------|
| `MQTT_EVENT_CONNECTED` | 连接到 Broker |
| `MQTT_EVENT_DISCONNECTED` | 断开连接 |
| `MQTT_EVENT_SUBSCRIBED` | 订阅成功 |
| `MQTT_EVENT_UNSUBSCRIBED` | 取消订阅成功 |
| `MQTT_EVENT_PUBLISHED` | 发布成功（QoS 1/2）|
| `MQTT_EVENT_DATA` | 收到订阅消息 |
| `MQTT_EVENT_ERROR` | 错误 |

---

## 🎯 核心函数

### esp_mqtt_client_init()

创建 MQTT 客户端。

```c
esp_mqtt_client_handle_t esp_mqtt_client_init(const esp_mqtt_client_config_t *config);
```

---

### esp_mqtt_client_register_event()

注册事件回调。

```c
esp_err_t esp_mqtt_client_register_event(esp_mqtt_client_handle_t client,
                                          esp_mqtt_event_id_t event,
                                          esp_event_handler_t event_handler,
                                          void *event_handler_arg);
```

---

### esp_mqtt_client_start()

启动客户端（开始连接）。

```c
esp_err_t esp_mqtt_client_start(esp_mqtt_client_handle_t client);
```

---

### esp_mqtt_client_stop()

停止客户端。

```c
esp_err_t esp_mqtt_client_stop(esp_mqtt_client_handle_t client);
```

---

### esp_mqtt_client_publish()

发布消息。

```c
int esp_mqtt_client_publish(esp_mqtt_client_handle_t client,
                             const char *topic,
                             const char *data,
                             int len,
                             int qos,
                             int retain);
```

**参数：**
- `topic`: 主题字符串
- `data`: 消息内容
- `len`: 消息长度（0 = 自动计算字符串长度）
- `qos`: 服务质量（0/1/2）
- `retain`: 保留消息（0/1）

**返回值：** 消息 ID（失败返回 -1）

---

### esp_mqtt_client_subscribe()

订阅主题。

```c
int esp_mqtt_client_subscribe(esp_mqtt_client_handle_t client,
                               const char *topic,
                               int qos);
```

---

### esp_mqtt_client_unsubscribe()

取消订阅。

```c
int esp_mqtt_client_unsubscribe(esp_mqtt_client_handle_t client,
                                 const char *topic);
```

---

## 💡 完整应用示例

### 示例1：基础 MQTT 发布与订阅

```c
#include "mqtt_client.h"
#include "esp_log.h"

static const char *TAG = "MQTT";
static esp_mqtt_client_handle_t mqtt_client = NULL;

/* 事件回调 */
static void mqtt_event_handler(void *arg, esp_event_base_t base,
                                int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "已连接到 Broker");
            /* 连接后订阅主题 */
            esp_mqtt_client_subscribe(mqtt_client, "esp32/control", 1);
            /* 发布上线消息 */
            esp_mqtt_client_publish(mqtt_client, "esp32/status",
                                    "online", 0, 1, 1);  // retain=1
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "已断开连接，等待自动重连...");
            break;

        case MQTT_EVENT_DATA:
            /* 收到订阅消息 */
            ESP_LOGI(TAG, "收到消息:");
            ESP_LOGI(TAG, "  主题: %.*s", event->topic_len, event->topic);
            ESP_LOGI(TAG, "  内容: %.*s", event->data_len, event->data);

            /* 处理控制命令 */
            if (strncmp(event->topic, "esp32/control", event->topic_len) == 0) {
                if (strncmp(event->data, "led_on", event->data_len) == 0) {
                    gpio_set_level(GPIO_NUM_4, 1);
                } else if (strncmp(event->data, "led_off", event->data_len) == 0) {
                    gpio_set_level(GPIO_NUM_4, 0);
                }
            }
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT 错误");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                ESP_LOGE(TAG, "TCP 错误: %d", event->error_handle->esp_tls_last_esp_err);
            }
            break;

        default:
            break;
    }
}

void mqtt_init(void)
{
    esp_mqtt_client_config_t cfg = {
        .broker = {
            .uri = "mqtt://broker.hivemq.com",  // 公共测试 Broker
        },
        .credentials = {
            /* .username = "user", */
            /* .password = "pass", */
        },
        .session = {
            .keepalive   = 60,
            .lwt_topic   = "esp32/status",
            .lwt_msg     = "offline",
            .lwt_qos     = 1,
            .lwt_retain  = 1,
        },
    };

    mqtt_client = esp_mqtt_client_init(&cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID,
                                    mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
    ESP_LOGI(TAG, "MQTT 客户端已启动");
}
```

---

### 示例2：定时上报传感器数据

```c
static void sensor_report_task(void *arg)
{
    char payload[128];
    int msg_id;

    while (1) {
        float temp = read_temperature();
        float humi = read_humidity();

        /* 发布 JSON 格式数据 */
        snprintf(payload, sizeof(payload),
                 "{\"temperature\":%.1f,\"humidity\":%.1f,\"ts\":%lld}",
                 temp, humi, (long long)esp_timer_get_time() / 1000000);

        msg_id = esp_mqtt_client_publish(mqtt_client,
                                          "esp32/sensor/data",
                                          payload, 0,
                                          1,   // QoS 1
                                          0);  // 不保留
        if (msg_id >= 0) {
            ESP_LOGI(TAG, "数据已发布 (msg_id=%d): %s", msg_id, payload);
        } else {
            ESP_LOGW(TAG, "发布失败（可能未连接）");
        }

        vTaskDelay(pdMS_TO_TICKS(30000));  // 每 30 秒上报
    }
}

void start_sensor_report(void)
{
    xTaskCreate(sensor_report_task, "sensor_report", 4096, NULL, 5, NULL);
}
```

---

### 示例3：连接阿里云 IoT（MQTT over TLS）

```c
/* 阿里云 IoT 连接参数 */
#define ALIYUN_PRODUCT_KEY   "your_product_key"
#define ALIYUN_DEVICE_NAME   "your_device_name"
#define ALIYUN_DEVICE_SECRET "your_device_secret"
#define ALIYUN_REGION        "cn-shanghai"

/* 阿里云 MQTT 连接信息（需要 HMAC-SHA1 签名，此处简化）*/
void mqtt_aliyun_init(void)
{
    char uri[128];
    snprintf(uri, sizeof(uri),
             "mqtts://%s.iot-as-mqtt.%s.aliyuncs.com:8883",
             ALIYUN_PRODUCT_KEY, ALIYUN_REGION);

    esp_mqtt_client_config_t cfg = {
        .broker = {
            .uri = uri,
            .verification = {
                .skip_cert_common_name_check = true,  // 简化处理
            },
        },
        .credentials = {
            .client_id = ALIYUN_DEVICE_NAME "&" ALIYUN_PRODUCT_KEY,
            .username  = ALIYUN_DEVICE_NAME "&" ALIYUN_PRODUCT_KEY,
            /* password 需要 HMAC-SHA1 签名，此处省略 */
        },
    };

    mqtt_client = esp_mqtt_client_init(&cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID,
                                    mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}
```

---

## ⚠️ 注意事项

### 1. 必须在 WiFi 连接后启动

MQTT 客户端需要网络连接，在 `IP_EVENT_STA_GOT_IP` 事件后启动。

### 2. QoS 级别选择

| QoS | 说明 | 适用 |
|-----|------|------|
| 0 | 最多一次，不确认 | 传感器数据（允许丢失）|
| 1 | 至少一次，可能重复 | 控制命令（推荐）|
| 2 | 恰好一次，最可靠 | 支付、关键操作 |

### 3. 主题通配符

```
esp32/+/data    # + 匹配单级（如 esp32/sensor/data）
esp32/#         # # 匹配多级（如 esp32/a/b/c）
```

### 4. 客户端 ID 唯一性

同一 Broker 上，相同 Client ID 的新连接会踢掉旧连接。
多设备时确保 Client ID 唯一（可用 MAC 地址）：
```c
uint8_t mac[6];
esp_wifi_get_mac(WIFI_IF_STA, mac);
char client_id[32];
snprintf(client_id, sizeof(client_id),
         "esp32_%02x%02x%02x", mac[3], mac[4], mac[5]);
```

---

## 📖 相关文档

- [IDF_WiFi.md](IDF_WiFi.md) — WiFi 连接
- [IDF_HTTP.md](IDF_HTTP.md) — HTTP 通信
- [README.md](README.md) — 模块导航
