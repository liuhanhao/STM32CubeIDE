# esp_timer — 软件定时器 API 文档（ESP-IDF）

## 📋 模块概述

esp_timer 是基于系统时钟的高分辨率软件定时器，提供：
- 单次（One-shot）和周期（Periodic）两种模式
- 微秒级参数设置（实际精度约 50μs）
- 回调在专用任务中执行（非 ISR，可调用普通 API）
- 获取系统运行时间（微秒精度，64 位不溢出）

---

## 🗺️ 使用场景速查

| 场景 | 推荐 |
|------|------|
| LVGL Tick 源（2ms 周期）| esp_timer ✅ |
| 周期性传感器读取（100ms）| esp_timer ✅ |
| 超时检测（单次）| esp_timer ✅ |
| 精确微秒级定时（<100μs）| GPTimer ✅ |
| 脉宽测量 | GPTimer ✅ |
| 步进电机脉冲 | GPTimer ✅ |

**esp_timer vs GPTimer vs FreeRTOS xTimerCreate：**

| 特性 | esp_timer | GPTimer | FreeRTOS xTimer |
|------|-----------|---------|-----------------|
| 精度 | ~50μs | 硬件级 μs | Tick 精度（1ms）|
| 回调上下文 | esp_timer 任务 | ISR | Timer 任务 |
| 可调用阻塞 API | ❌（会影响其他定时器）| ❌ | ✅ |
| 最小周期 | ~50μs | 1μs | 1 Tick（1ms）|
| 适用场景 | LVGL、周期任务 | 精确定时 | 普通周期任务 |

---

## 📚 相关文件

- **头文件**: `esp_timer.h`

---

## 🔧 数据类型

### esp_timer_create_args_t — 定时器创建参数

```c
typedef struct {
    esp_timer_cb_t callback;              // 回调函数（void func(void *arg)）
    void *arg;                            // 传给回调的参数
    esp_timer_dispatch_t dispatch_method; // 执行方式（ESP_TIMER_TASK，默认）
    const char *name;                     // 定时器名称（调试用）
    bool skip_unhandled_events;           // 跳过积压的未处理事件
} esp_timer_create_args_t;
```

**dispatch_method 选项：**

| 值 | 说明 |
|----|------|
| `ESP_TIMER_TASK` | 在 esp_timer 专用任务中执行（默认，推荐）|
| `ESP_TIMER_ISR` | 在 ISR 中执行（精度更高，但限制更多）|

---

## 🎯 核心函数

### esp_timer_create()

创建定时器（不启动）。

```c
esp_err_t esp_timer_create(const esp_timer_create_args_t *create_args,
                            esp_timer_handle_t *out_handle);
```

---

### esp_timer_start_periodic()

启动周期定时器。

```c
esp_err_t esp_timer_start_periodic(esp_timer_handle_t timer, uint64_t period_us);
```

**参数：** `period_us` — 周期，单位微秒

---

### esp_timer_start_once()

启动单次定时器（触发一次后自动停止）。

```c
esp_err_t esp_timer_start_once(esp_timer_handle_t timer, uint64_t timeout_us);
```

---

### esp_timer_restart()

重启定时器（修改周期或重置倒计时）。

```c
esp_err_t esp_timer_restart(esp_timer_handle_t timer, uint64_t timeout_us);
```

---

### esp_timer_stop()

停止定时器（不删除）。

```c
esp_err_t esp_timer_stop(esp_timer_handle_t timer);
```

---

### esp_timer_delete()

删除定时器，释放资源（必须先停止）。

```c
esp_err_t esp_timer_delete(esp_timer_handle_t timer);
```

---

### esp_timer_get_time()

获取系统启动后的微秒数（64 位，永不溢出）。

```c
int64_t esp_timer_get_time(void);
```

**使用示例：**
```c
int64_t start = esp_timer_get_time();
/* ... 执行某些操作 ... */
int64_t elapsed_us = esp_timer_get_time() - start;
ESP_LOGI(TAG, "耗时: %lld μs", elapsed_us);
```

---

### esp_timer_is_active()

检查定时器是否正在运行。

```c
bool esp_timer_is_active(esp_timer_handle_t timer);
```

---

### esp_timer_get_next_alarm()

获取下次触发的时间（微秒，相对于系统启动）。

```c
int64_t esp_timer_get_next_alarm(void);
```

---

## 💡 完整应用示例

### 示例1：LVGL Tick 定时器（最常用）

```c
#include "esp_timer.h"
#include "lvgl.h"

static void lvgl_tick_cb(void *arg)
{
    lv_tick_inc(2);  // 每 2ms 增加 2
}

void lvgl_tick_init(void)
{
    esp_timer_handle_t timer;
    const esp_timer_create_args_t args = {
        .callback = lvgl_tick_cb,
        .name     = "lvgl_tick",
    };
    ESP_ERROR_CHECK(esp_timer_create(&args, &timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(timer, 2000));  // 2000μs = 2ms
}
```

---

### 示例2：周期性传感器读取

```c
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "SENSOR_TIMER";
static esp_timer_handle_t sensor_timer;

typedef struct {
    float temperature;
    float humidity;
} sensor_data_t;

static sensor_data_t g_sensor_data;

static void sensor_read_cb(void *arg)
{
    /* 在 esp_timer 任务中执行，可以调用普通 API */
    /* 注意：不要在这里做耗时操作（会影响其他定时器精度）*/
    g_sensor_data.temperature = 25.6f;  // 实际读取传感器
    g_sensor_data.humidity    = 60.0f;
    ESP_LOGD(TAG, "温度=%.1f°C 湿度=%.1f%%",
             g_sensor_data.temperature, g_sensor_data.humidity);
}

void sensor_timer_init(void)
{
    const esp_timer_create_args_t args = {
        .callback = sensor_read_cb,
        .name     = "sensor_read",
    };
    ESP_ERROR_CHECK(esp_timer_create(&args, &sensor_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(sensor_timer, 500000));  // 500ms
    ESP_LOGI(TAG, "传感器定时器已启动（500ms 周期）");
}
```

---

### 示例3：超时检测（单次）

```c
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "TIMEOUT";
static esp_timer_handle_t timeout_timer;
static volatile bool timeout_occurred = false;

static void timeout_cb(void *arg)
{
    timeout_occurred = true;
    ESP_LOGW(TAG, "操作超时！");
}

void timeout_init(void)
{
    const esp_timer_create_args_t args = {
        .callback = timeout_cb,
        .name     = "timeout",
    };
    esp_timer_create(&args, &timeout_timer);
}

/* 开始超时计时（5秒）*/
void timeout_start(void)
{
    timeout_occurred = false;
    esp_timer_start_once(timeout_timer, 5000000);  // 5s
}

/* 取消超时 */
void timeout_cancel(void)
{
    if (esp_timer_is_active(timeout_timer)) {
        esp_timer_stop(timeout_timer);
    }
}

/* 使用示例 */
void do_operation_with_timeout(void)
{
    timeout_start();

    /* 执行操作... */
    bool success = wait_for_response();  // 等待响应

    if (success) {
        timeout_cancel();
        ESP_LOGI(TAG, "操作成功");
    } else if (timeout_occurred) {
        ESP_LOGE(TAG, "操作超时");
    }
}
```

---

### 示例4：性能计时

```c
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "PERF";

/* 测量函数执行时间 */
#define MEASURE_TIME(name, code) do { \
    int64_t _start = esp_timer_get_time(); \
    code; \
    int64_t _elapsed = esp_timer_get_time() - _start; \
    ESP_LOGI(TAG, name " 耗时: %lld μs", _elapsed); \
} while(0)

void performance_test(void)
{
    MEASURE_TIME("矩阵运算", {
        /* 执行矩阵运算 */
        float result = 0;
        for (int i = 0; i < 10000; i++) result += i * 0.001f;
        (void)result;
    });

    MEASURE_TIME("Flash 写入", {
        /* 执行 Flash 写入 */
    });
}
```

---

### 示例5：动态修改周期

```c
#include "esp_timer.h"

static esp_timer_handle_t blink_timer;
static bool led_state = false;

static void blink_cb(void *arg)
{
    led_state = !led_state;
    gpio_set_level(GPIO_NUM_4, led_state);
}

void blink_init(void)
{
    const esp_timer_create_args_t args = {
        .callback = blink_cb,
        .name     = "blink",
    };
    esp_timer_create(&args, &blink_timer);
    esp_timer_start_periodic(blink_timer, 500000);  // 初始 500ms
}

/* 修改闪烁频率 */
void blink_set_period_ms(uint32_t period_ms)
{
    /* restart 可以在运行中修改周期 */
    esp_timer_restart(blink_timer, (uint64_t)period_ms * 1000);
}
```

---

## ⚠️ 注意事项

### 1. 回调中不要做耗时操作

esp_timer 的所有回调共用一个任务，耗时操作会延迟其他定时器：
```c
/* 错误：在回调中做耗时操作 */
static void bad_cb(void *arg) {
    vTaskDelay(pdMS_TO_TICKS(100));  // ❌ 会阻塞其他定时器
    HAL_heavy_operation();           // ❌
}

/* 正确：通过队列通知其他任务 */
static void good_cb(void *arg) {
    xQueueSendFromISR(queue, &data, NULL);  // ✅ 快速返回
}
```

### 2. 删除前必须停止

```c
esp_timer_stop(timer);    // 先停止
esp_timer_delete(timer);  // 再删除
```

### 3. 精度说明

esp_timer 的实际精度约 50μs，受系统负载影响。
需要精确微秒级定时时使用 GPTimer。

### 4. skip_unhandled_events

如果回调执行时间超过定时器周期，会积压事件。
设置 `skip_unhandled_events = true` 可以跳过积压事件，避免回调连续触发。

---

## 📖 相关文档

- [IDF_GPTimer.md](IDF_GPTimer.md) — 硬件定时器（精确定时）
- [IDF_LEDC.md](IDF_LEDC.md) — PWM 输出
- [README.md](README.md) — 模块导航
