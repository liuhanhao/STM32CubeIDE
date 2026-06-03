# GPTimer — 通用定时器 API 文档（ESP-IDF）

## 📋 模块概述

GPTimer（General Purpose Timer）是 ESP32-S3 的硬件定时器，提供：
- 4 个独立 64 位计数器
- 可配置分辨率（最高 1MHz = 1μs 精度）
- 周期报警（定时中断）和绝对值报警
- 捕获功能（测量外部脉宽）
- 计数方向可选（向上/向下）

---

## 🗺️ 使用场景速查

| 场景 | 配置 |
|------|------|
| 1ms 系统心跳 | 1MHz 分辨率，1000 计数报警，自动重载 |
| 精确微秒延时 | 1MHz 分辨率，单次报警 |
| 超声波测距（脉宽测量）| 捕获模式 |
| 步进电机脉冲生成 | 周期报警，动态修改周期 |
| 时间戳记录 | `gptimer_get_raw_count` 读取计数值 |

**GPTimer vs esp_timer 选择：**

| 特性 | GPTimer | esp_timer |
|------|---------|-----------|
| 精度 | 硬件级，μs 精确 | 约 50μs 误差 |
| 回调上下文 | ISR（不能阻塞）| 普通任务（可阻塞）|
| 使用复杂度 | 较高 | 简单 |
| 适用场景 | 精确定时、脉宽测量 | LVGL Tick、周期任务 |

---

## 📚 相关文件

- **头文件**: `driver/gptimer.h`

---

## 🔧 数据类型

### gptimer_config_t — 定时器配置

```c
typedef struct {
    gptimer_clock_source_t clk_src;      // 时钟源（GPTIMER_CLK_SRC_DEFAULT）
    gptimer_count_direction_t direction; // 计数方向（GPTIMER_COUNT_UP）
    uint32_t resolution_hz;              // 分辨率（Hz），如 1000000 = 1μs/计数
    int intr_priority;                   // 中断优先级（0 = 默认）
    struct {
        uint32_t intr_shared : 1;        // 共享中断向量
    } flags;
} gptimer_config_t;
```

---

### gptimer_alarm_config_t — 报警配置

```c
typedef struct {
    uint64_t alarm_count;    // 触发报警的计数值
    uint64_t reload_count;   // 报警后重载的计数值（通常为 0）
    struct {
        uint32_t auto_reload_on_alarm : 1;  // 报警时自动重载（实现周期定时）
    } flags;
} gptimer_alarm_config_t;
```

---

### gptimer_event_callbacks_t — 事件回调

```c
typedef struct {
    gptimer_alarm_cb_t on_alarm;  // 报警回调
} gptimer_event_callbacks_t;
```

**报警回调签名：**
```c
bool IRAM_ATTR on_alarm_cb(gptimer_handle_t timer,
                            const gptimer_alarm_event_data_t *edata,
                            void *user_ctx);
/* 返回 true：需要任务切换（有高优先级任务被唤醒）
 * 返回 false：不需要任务切换 */
```

---

## 🎯 核心函数

### 初始化与销毁

#### gptimer_new_timer()

创建定时器实例。

```c
esp_err_t gptimer_new_timer(const gptimer_config_t *config,
                             gptimer_handle_t *ret_timer);
```

---

#### gptimer_del_timer()

删除定时器，释放资源。

```c
esp_err_t gptimer_del_timer(gptimer_handle_t timer);
```

---

### 事件注册

#### gptimer_register_event_callbacks()

注册报警回调（必须在 enable 之前调用）。

```c
esp_err_t gptimer_register_event_callbacks(gptimer_handle_t timer,
                                            const gptimer_event_callbacks_t *cbs,
                                            void *user_data);
```

---

### 报警配置

#### gptimer_set_alarm_action()

配置报警动作（周期或单次）。

```c
esp_err_t gptimer_set_alarm_action(gptimer_handle_t timer,
                                    const gptimer_alarm_config_t *config);
```

---

### 使能与启动

#### gptimer_enable()

使能定时器（分配中断资源）。

```c
esp_err_t gptimer_enable(gptimer_handle_t timer);
```

---

#### gptimer_disable()

禁用定时器（释放中断资源）。

```c
esp_err_t gptimer_disable(gptimer_handle_t timer);
```

---

#### gptimer_start()

启动计数。

```c
esp_err_t gptimer_start(gptimer_handle_t timer);
```

---

#### gptimer_stop()

停止计数（保留当前计数值）。

```c
esp_err_t gptimer_stop(gptimer_handle_t timer);
```

---

### 计数值操作

#### gptimer_get_raw_count()

读取当前计数值。

```c
esp_err_t gptimer_get_raw_count(gptimer_handle_t timer, uint64_t *value);
```

---

#### gptimer_set_raw_count()

设置计数值（通常用于复位到 0）。

```c
esp_err_t gptimer_set_raw_count(gptimer_handle_t timer, uint64_t value);
```

---

## 💡 完整应用示例

### 示例1：1ms 周期定时中断

```c
#include "driver/gptimer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "GPTIMER";
static volatile uint32_t tick_ms = 0;

/* ISR 回调（在中断上下文中执行）*/
static bool IRAM_ATTR timer_1ms_cb(gptimer_handle_t timer,
                                    const gptimer_alarm_event_data_t *edata,
                                    void *user_ctx)
{
    tick_ms++;
    return false;  // 不需要任务切换
}

static gptimer_handle_t gptimer_1ms;

void gptimer_1ms_init(void)
{
    /* 1. 创建定时器，分辨率 1MHz（1μs/计数）*/
    gptimer_config_t cfg = {
        .clk_src       = GPTIMER_CLK_SRC_DEFAULT,
        .direction     = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000,  // 1MHz
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&cfg, &gptimer_1ms));

    /* 2. 注册回调 */
    gptimer_event_callbacks_t cbs = {
        .on_alarm = timer_1ms_cb,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer_1ms, &cbs, NULL));

    /* 3. 配置 1ms 周期报警（1000μs）*/
    gptimer_alarm_config_t alarm = {
        .alarm_count              = 1000,  // 1000 × 1μs = 1ms
        .reload_count             = 0,
        .flags.auto_reload_on_alarm = true,
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer_1ms, &alarm));

    /* 4. 使能并启动 */
    ESP_ERROR_CHECK(gptimer_enable(gptimer_1ms));
    ESP_ERROR_CHECK(gptimer_start(gptimer_1ms));

    ESP_LOGI(TAG, "1ms 定时器已启动");
}

void app_main(void)
{
    gptimer_1ms_init();

    while (1) {
        ESP_LOGI(TAG, "运行时间: %lu ms", tick_ms);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

---

### 示例2：单次延时（精确微秒级）

```c
#include "driver/gptimer.h"

static gptimer_handle_t delay_timer;
static volatile bool delay_done = false;

static bool IRAM_ATTR delay_cb(gptimer_handle_t timer,
                                const gptimer_alarm_event_data_t *edata,
                                void *user_ctx)
{
    delay_done = true;
    return false;
}

void precise_delay_us(uint32_t us)
{
    delay_done = false;

    /* 设置单次报警 */
    gptimer_alarm_config_t alarm = {
        .alarm_count              = us,
        .flags.auto_reload_on_alarm = false,  // 单次
    };
    gptimer_set_raw_count(delay_timer, 0);  // 复位计数
    gptimer_set_alarm_action(delay_timer, &alarm);
    gptimer_start(delay_timer);

    /* 等待报警 */
    while (!delay_done) {
        /* 忙等，适合极短延时 */
    }
    gptimer_stop(delay_timer);
}
```

---

### 示例3：超声波测距（脉宽测量）

```c
#include "driver/gptimer.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define TRIG_PIN  GPIO_NUM_12
#define ECHO_PIN  GPIO_NUM_13

static const char *TAG = "ULTRASONIC";
static gptimer_handle_t us_timer;
static volatile uint64_t echo_start = 0;
static volatile uint64_t echo_end   = 0;
static volatile bool     echo_done  = false;

/* ECHO 引脚中断回调 */
static void IRAM_ATTR echo_isr(void *arg)
{
    int level = gpio_get_level(ECHO_PIN);
    uint64_t count;
    gptimer_get_raw_count(us_timer, &count);

    if (level == 1) {
        echo_start = count;  // 上升沿：记录开始时间
    } else {
        echo_end  = count;   // 下降沿：记录结束时间
        echo_done = true;
    }
}

void ultrasonic_init(void)
{
    /* 初始化 1MHz 定时器（不需要报警，只读计数）*/
    gptimer_config_t cfg = {
        .clk_src       = GPTIMER_CLK_SRC_DEFAULT,
        .direction     = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000,
    };
    gptimer_new_timer(&cfg, &us_timer);
    gptimer_enable(us_timer);
    gptimer_start(us_timer);

    /* 配置 TRIG 引脚（输出）*/
    gpio_config_t trig_cfg = {
        .pin_bit_mask = (1ULL << TRIG_PIN),
        .mode         = GPIO_MODE_OUTPUT,
    };
    gpio_config(&trig_cfg);

    /* 配置 ECHO 引脚（双边沿中断）*/
    gpio_config_t echo_cfg = {
        .pin_bit_mask = (1ULL << ECHO_PIN),
        .mode         = GPIO_MODE_INPUT,
        .intr_type    = GPIO_INTR_ANYEDGE,
    };
    gpio_config(&echo_cfg);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(ECHO_PIN, echo_isr, NULL);
}

float ultrasonic_measure_cm(void)
{
    echo_done = false;

    /* 发送 10μs 触发脉冲 */
    gpio_set_level(TRIG_PIN, 1);
    esp_rom_delay_us(10);
    gpio_set_level(TRIG_PIN, 0);

    /* 等待测量完成（最多 30ms）*/
    uint32_t timeout = 30000;
    while (!echo_done && timeout--) {
        esp_rom_delay_us(1);
    }

    if (!echo_done) return -1.0f;  // 超时

    /* 计算距离：声速 340m/s，往返除以 2 */
    uint64_t pulse_us = echo_end - echo_start;
    return (float)pulse_us * 0.017f;  // cm = μs × 340/2/10000
}
```

---

### 示例4：步进电机脉冲（动态调速）

```c
#include "driver/gptimer.h"
#include "driver/gpio.h"

#define STEP_PIN  GPIO_NUM_10
#define DIR_PIN   GPIO_NUM_11

static gptimer_handle_t step_timer;
static volatile uint32_t step_count = 0;
static volatile uint32_t target_steps = 0;

static bool IRAM_ATTR step_cb(gptimer_handle_t timer,
                               const gptimer_alarm_event_data_t *edata,
                               void *user_ctx)
{
    if (step_count < target_steps) {
        gpio_set_level(STEP_PIN, 1);
        esp_rom_delay_us(2);
        gpio_set_level(STEP_PIN, 0);
        step_count++;
    } else {
        gptimer_stop(timer);
    }
    return false;
}

/* 以指定速度运动指定步数 */
void stepper_move(uint32_t steps, uint32_t speed_hz, bool forward)
{
    gpio_set_level(DIR_PIN, forward ? 1 : 0);
    step_count   = 0;
    target_steps = steps;

    /* 动态修改报警周期（改变速度）*/
    uint64_t period_us = 1000000 / speed_hz;
    gptimer_alarm_config_t alarm = {
        .alarm_count              = period_us,
        .flags.auto_reload_on_alarm = true,
    };
    gptimer_set_raw_count(step_timer, 0);
    gptimer_set_alarm_action(step_timer, &alarm);
    gptimer_start(step_timer);
}
```

---

## ⚠️ 注意事项

### 1. ISR 回调限制

报警回调在中断上下文中执行：
- 必须加 `IRAM_ATTR` 属性（确保代码在 IRAM 中）
- 不能调用非 IRAM 函数（如 `printf`、`malloc`）
- 不能调用阻塞 API（`vTaskDelay` 等）
- 执行时间要尽量短

### 2. 使能顺序

必须严格按顺序：
```
gptimer_new_timer → gptimer_register_event_callbacks
→ gptimer_set_alarm_action → gptimer_enable → gptimer_start
```

### 3. 分辨率与最大定时时间

64 位计数器，1MHz 分辨率时：
- 最大计数值：2^64 ≈ 1.8 × 10^19
- 最大定时时间：约 58 万年（实际无限制）

### 4. 动态修改报警

运行中可以调用 `gptimer_set_alarm_action` 修改报警值（实现动态调速），
无需停止定时器。

---

## 📖 相关文档

- [IDF_LEDC.md](IDF_LEDC.md) — PWM 输出
- [IDF_MCPWM.md](IDF_MCPWM.md) — 电机控制 PWM
- [IDF_EspTimer.md](IDF_EspTimer.md) — 软件定时器
- [README.md](README.md) — 模块导航
