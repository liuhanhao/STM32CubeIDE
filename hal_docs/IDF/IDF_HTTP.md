# HTTP — 网络通信 API 文档（ESP-IDF）

## 📋 模块概述

ESP-IDF 提供完整的 HTTP 客户端和服务端支持：
- **HTTP 客户端**（`esp_http_client`）：向服务器发送请求，获取数据
- **HTTP 服务端**（`esp_http_server`）：在 ESP32 上运行 Web 服务器

---

## 🗺️ 使用场景速查

| 场景 | 方案 |
|------|------|
| 从云端获取数据（天气、API）| HTTP 客户端 GET |
| 上传传感器数据到服务器 | HTTP 客户端 POST |
| OTA 固件升级 | HTTP 客户端 + esp_ota |
| 手机配网（AP 模式）| HTTP 服务端 |
| 设备 Web 控制界面 | HTTP 服务端 + SPIFFS 静态文件 |
| REST API 接口 | HTTP 服务端 |

---

## 📚 相关文件

- **客户端头文件**: `esp_http_client.h`
- **服务端头文件**: `esp_http_server.h`

---

## 一、HTTP 客户端

### 核心函数

#### esp_http_client_init()

创建 HTTP 客户端实例。

```c
esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t *config);
```

**配置结构体（常用字段）：**
```c
typedef struct {
    const char *url;                    // 请求 URL
    const char *host;                   // 主机名（与 url 二选一）
    const char *path;                   // 路径
    int port;                           // 端口（默认 80/443）
    esp_http_client_method_t method;    // 请求方法
    int timeout_ms;                     // 超时（ms）
    bool disable_auto_redirect;         // 禁用自动重定向
    int max_redirection_count;          // 最大重定向次数
    esp_http_client_event_handle_t event_handler; // 事件回调
    void *user_data;                    // 用户数据
    bool is_async;                      // 异步模式
    const char *cert_pem;               // HTTPS 证书（PEM 格式）
    bool skip_cert_common_name_check;   // 跳过证书验证（调试用）
} esp_http_client_config_t;
```

---

#### esp_http_client_perform()

执行 HTTP 请求（阻塞直到完成）。

```c
esp_err_t esp_http_client_perform(esp_http_client_handle_t client);
```

---

#### esp_http_client_cleanup()

释放客户端资源。

```c
esp_err_t esp_http_client_cleanup(esp_http_client_handle_t client);
```

---

### 完整示例

#### 示例1：GET 请求（获取数据）

```c
#include "esp_http_client.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "HTTP";

/* 响应数据缓冲区 */
static char response_buf[4096];
static int  response_len = 0;

/* 事件回调 */
static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            /* 接收响应数据 */
            if (!esp_http_client_is_chunked_response(evt->client)) {
                int copy_len = evt->data_len;
                if (response_len + copy_len < sizeof(response_buf) - 1) {
                    memcpy(response_buf + response_len, evt->data, copy_len);
                    response_len += copy_len;
                }
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            response_buf[response_len] = '\0';
            ESP_LOGI(TAG, "响应: %s", response_buf);
            response_len = 0;
            break;
        case HTTP_EVENT_ERROR:
            ESP_LOGE(TAG, "HTTP 错误");
            break;
        default:
            break;
    }
    return ESP_OK;
}

void http_get_example(void)
{
    esp_http_client_config_t cfg = {
        .url           = "http://httpbin.org/get",
        .event_handler = http_event_handler,
        .timeout_ms    = 5000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        int length = esp_http_client_get_content_length(client);
        ESP_LOGI(TAG, "状态码: %d, 内容长度: %d", status, length);
    } else {
        ESP_LOGE(TAG, "请求失败: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}
```

---

#### 示例2：POST 请求（上传数据）

```c
void http_post_json(const char *url, const char *json_body)
{
    esp_http_client_config_t cfg = {
        .url        = url,
        .method     = HTTP_METHOD_POST,
        .timeout_ms = 5000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);

    /* 设置请求头和请求体 */
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_body, strlen(json_body));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "POST 成功，状态码: %d",
                 esp_http_client_get_status_code(client));
    } else {
        ESP_LOGE(TAG, "POST 失败: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}

/* 使用示例 */
void upload_sensor_data(float temp, float humi)
{
    char json[128];
    snprintf(json, sizeof(json),
             "{\"temperature\":%.1f,\"humidity\":%.1f}", temp, humi);
    http_post_json("http://your-server.com/api/data", json);
}
```

---

#### 示例3：HTTPS 请求

```c
/* 需要服务器的根证书（PEM 格式）*/
extern const char server_cert_pem_start[] asm("_binary_server_cert_pem_start");
extern const char server_cert_pem_end[]   asm("_binary_server_cert_pem_end");

void https_get_example(void)
{
    esp_http_client_config_t cfg = {
        .url      = "https://api.example.com/data",
        .cert_pem = server_cert_pem_start,  // 服务器证书
        /* 调试时可跳过证书验证（生产环境不要用）*/
        /* .skip_cert_common_name_check = true, */
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
}
```

---

## 二、HTTP 服务端

### 核心函数

#### httpd_start()

启动 HTTP 服务器。

```c
esp_err_t httpd_start(httpd_handle_t *handle, const httpd_config_t *config);
```

---

#### httpd_register_uri_handler()

注册 URI 处理函数。

```c
esp_err_t httpd_register_uri_handler(httpd_handle_t handle,
                                      const httpd_uri_t *uri_handler);
```

---

#### httpd_resp_send()

发送响应。

```c
esp_err_t httpd_resp_send(httpd_req_t *req, const char *buf, ssize_t buf_len);
```

---

### 完整示例

#### 示例4：简单 Web 服务器

```c
#include "esp_http_server.h"
#include "esp_log.h"

static const char *TAG = "HTTP_SERVER";
static httpd_handle_t server = NULL;

/* 根路径处理 */
static esp_err_t root_handler(httpd_req_t *req)
{
    const char *html =
        "<!DOCTYPE html><html><body>"
        "<h1>ESP32-S3 Web Server</h1>"
        "<p>温度: <span id='temp'>--</span>°C</p>"
        "<button onclick=\"fetch('/led/toggle')\">切换 LED</button>"
        "</body></html>";

    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_send(req, html, strlen(html));
    return ESP_OK;
}

/* LED 控制接口 */
static esp_err_t led_toggle_handler(httpd_req_t *req)
{
    static bool led_state = false;
    led_state = !led_state;
    gpio_set_level(GPIO_NUM_4, led_state);

    char resp[64];
    snprintf(resp, sizeof(resp), "{\"led\":%s}", led_state ? "true" : "false");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, strlen(resp));
    return ESP_OK;
}

/* 传感器数据接口 */
static esp_err_t sensor_handler(httpd_req_t *req)
{
    float temp = 25.6f;  // 实际读取传感器
    float humi = 60.0f;

    char json[128];
    snprintf(json, sizeof(json),
             "{\"temperature\":%.1f,\"humidity\":%.1f}", temp, humi);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, strlen(json));
    return ESP_OK;
}

/* POST 接口（接收数据）*/
static esp_err_t config_post_handler(httpd_req_t *req)
{
    char buf[256];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "接收失败");
        return ESP_FAIL;
    }
    buf[ret] = '\0';
    ESP_LOGI(TAG, "收到 POST 数据: %s", buf);

    httpd_resp_send(req, "OK", 2);
    return ESP_OK;
}

httpd_handle_t start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "服务器启动失败");
        return NULL;
    }

    /* 注册路由 */
    httpd_uri_t uri_root = {
        .uri     = "/",
        .method  = HTTP_GET,
        .handler = root_handler,
    };
    httpd_register_uri_handler(server, &uri_root);

    httpd_uri_t uri_led = {
        .uri     = "/led/toggle",
        .method  = HTTP_GET,
        .handler = led_toggle_handler,
    };
    httpd_register_uri_handler(server, &uri_led);

    httpd_uri_t uri_sensor = {
        .uri     = "/api/sensor",
        .method  = HTTP_GET,
        .handler = sensor_handler,
    };
    httpd_register_uri_handler(server, &uri_sensor);

    httpd_uri_t uri_config = {
        .uri     = "/api/config",
        .method  = HTTP_POST,
        .handler = config_post_handler,
    };
    httpd_register_uri_handler(server, &uri_config);

    ESP_LOGI(TAG, "Web 服务器已启动");
    return server;
}
```

---

## ⚠️ 注意事项

### 1. HTTP 客户端需要 WiFi 连接

必须在 WiFi 连接并获取 IP 后才能使用 HTTP 客户端。

### 2. HTTPS 证书

HTTPS 请求需要服务器根证书，可以：
- 将 PEM 文件放入 `main/` 目录，在 `CMakeLists.txt` 中用 `target_add_binary_data` 嵌入
- 使用 `skip_cert_common_name_check = true` 跳过验证（仅调试）

### 3. 响应数据缓冲

HTTP 客户端的响应数据通过事件回调分块接收，需要自己拼接完整响应。

### 4. 服务端并发

`HTTPD_DEFAULT_CONFIG()` 默认支持 7 个并发连接，可通过 `config.max_open_sockets` 调整。

---

## 📖 相关文档

- [IDF_WiFi.md](IDF_WiFi.md) — WiFi 连接
- [IDF_MQTT.md](IDF_MQTT.md) — MQTT 协议
- [IDF_SPIFFS.md](IDF_SPIFFS.md) — 文件系统（静态网页文件）
- [README.md](README.md) — 模块导航
