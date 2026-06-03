# LEDC — PWM 输出模块 API 文档（ESP-IDF）

## 📋 模块概述

LEDC（LED Control）是 ESP32-S3 专用的 PWM 控制器，提供：
- 8 个独立 PWM 通道（低速模式）
- 频率与分辨率灵活配置
- 硬件渐变（Fade），无需 CPU 干预
- 引脚可路由到任意 GPIO

> **ESP32-S3 说明：** 只有低速模式（`LEDC_LOW_SPEED_MODE`），无高速模式。

---

## 🗺️ 使用场景速查

| 场景 | 频率 | 分辨率 |
|------|------|--------|
| LED 调光 | 1~5kHz | 13 位（8192 级）|
| RGB LED | 1~5kHz | 8 位（256 级）|
| 舵机控制 | 50Hz | 16 位（精确脉宽）|
| 电机调速（直流）| 10~20kHz | 10~13 位 |
| 蜂鸣器 | 1~4kHz | 1 位（开关）|
| 背光控制 | 1kHz | 8 位 |

**频率与分辨率关系（80MHz 时钟）：**

$$最大分辨率位数 = \lfloor\log_2\left(\frac{80,000,000}{频率}\right)\rfloor$$

| PWM 频率 | 最大分辨率 | 最大占空比级数 |
|---------|----------|-------------|
| 50Hz | 20 位 | 1,048,576 |
| 1kHz | 16 位 | 65,536 |
| 5kHz | 13 位 | 8,192 |
| 10kHz | 12 位 | 4,096 |
| 40kHz | 11 位 | 2,048 |

---

## 📚 相关文件

- **头文件**: `driver/ledc.h`

---

## 🔧 数据类型

### ledc_timer_config_t — 定时器配置

```c
typedef struct {
    ledc_mode_t speed_mode;           // 速度模式（ESP32-S3 只有 LEDC_LOW_SPEED_MODE）
    ledc_timer_bit_t duty_resolution; // 分辨率位数
    ledc_timer_t timer_num;           // 定时器编号（LEDC_TIMER_0~3）
    uint32_t freq_hz;                 // PWM 频率（Hz）
    ledc_clk_cfg_t clk_cfg;          // 时钟源（LEDC_AUTO_CLK）
} ledc_timer_config_t;
```

---

### ledc_channel_config_t — 通道配置

```c
typedef struct {
    int gpio_num;               // 输出 GPIO 引脚
    ledc_mode_t speed_mode;     // 速度模式
    ledc_channel_t channel;     // 通道编号（LEDC_CHANNEL_0~7）
    ledc_intr_type_t intr_type; // 中断类型（LEDC_INTR_DISABLE）
    ledc_timer_t timer_sel;     // 使用的定时器
    uint32_t duty;              // 初始占空比（0 ~ 2^分辨率-1）
    int hpoint;                 // 高点（通常为 0）
    struct {
        unsigned int output_invert : 1; // 输出反相（1=反相）
    } flags;
} ledc_channel_config_t;
```

---

## 🎯 核心函数

### 初始化

#### ledc_timer_config()

配置 LEDC 定时器（频率和分辨率）。每个定时器可被多个通道共用。

```c
esp_err_t ledc_timer_config(const ledc_timer_config_t *timer_conf);
```

---

#### ledc_channel_config()

配置 LEDC 通道（绑定引脚、定时器、初始占空比）。

```c
esp_err_t ledc_channel_config(const ledc_channel_config_t *ledc_conf);
```

---

### 占空比控制

#### ledc_set_duty() + ledc_update_duty()

设置占空比（两步操作，先写入后生效）。

```c
esp_err_t ledc_set_duty(ledc_mode_t speed_mode, ledc_channel_t channel, uint32_t duty);
esp_err_t ledc_update_duty(ledc_mode_t speed_mode, ledc_channel_t channel);
```

**使用示例：**
```c
/* 设置 50% 占空比（13 位分辨率，最大 8191）*/
ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 4095);
ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
```

---

#### ledc_get_duty()

读取当前占空比。

```c
uint32_t ledc_get_duty(ledc_mode_t speed_mode, ledc_channel_t channel);
```

---

#### ledc_set_freq()

动态修改 PWM 频率（不影响占空比）。

```c
esp_err_t ledc_set_freq(ledc_mode_t speed_mode, ledc_timer_t timer_num, uint32_t freq_hz);
```

---

#### ledc_get_freq()

读取当前频率。

```c
uint32_t ledc_get_freq(ledc_mode_t speed_mode, ledc_timer_t timer_num);
```

---

### 渐变功能

#### ledc_fade_func_install()

安装渐变功能（使用渐变前必须调用一次）。

```c
esp_err_t ledc_fade_func_install(int intr_alloc_flags);
/* 通常传 0 */
```

---

#### ledc_fade_with_time()

在指定时间内平滑改变占空比。

```c
esp_err_t ledc_fade_with_time(ledc_mode_t speed_mode,
                               ledc_channel_t channel,
                               uint32_t target_duty,
                               int max_fade_time_ms);
```

---

#### ledc_fade_with_step()

按步长渐变。

```c
esp_err_t ledc_fade_with_step(ledc_mode_t speed_mode,
                               ledc_channel_t channel,
                               uint32_t target_duty,
                               uint32_t scale,
                               uint32_t cycle_num);
```

---

#### ledc_fade_start()

启动渐变。

```c
esp_err_t ledc_fade_start(ledc_mode_t speed_mode,
                           ledc_channel_t channel,
                           ledc_fade_mode_t fade_mode);
```

**fade_mode：**
- `LEDC_FADE_NO_WAIT`：非阻塞，立即返回
- `LEDC_FADE_WAIT_DONE`：阻塞，等待渐变完成

---

#### ledc_fade_stop()

停止渐变。

```c
esp_err_t ledc_fade_stop(ledc_mode_t speed_mode, ledc_channel_t channel);
```

---

### 停止输出

#### ledc_stop()

停止 PWM 输出，引脚保持指定电平。

```c
esp_err_t ledc_stop(ledc_mode_t speed_mode, ledc_channel_t channel, uint32_t idle_level);
```

---

## 💡 完整应用示例

### 示例1：LED 调光（带渐变）

```c
#include "driver/ledc.h"
#include "esp_log.h"

#define LED_PIN      GPIO_NUM_4
#define LEDC_TIMER   LEDC_TIMER_0
#define LEDC_CH      LEDC_CHANNEL_0
#define LEDC_FREQ    5000
#define LEDC_RES     LEDC_TIMER_13_BIT
#define LEDC_MAX     ((1 << 13) - 1)   // 8191

static const char *TAG = "LEDC";

void ledc_led_init(void)
{
    ledc_timer_config_t timer = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_RES,
        .timer_num       = LEDC_TIMER,
        .freq_hz         = LEDC_FREQ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    ledc_channel_config_t ch = {
        .gpio_num   = LED_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CH,
        .timer_sel  = LEDC_TIMER,
        .duty       = 0,
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch));

    /* 安装渐变功能 */
    ESP_ERROR_CHECK(ledc_fade_func_install(0));
    ESP_LOGI(TAG, "LEDC 初始化完成，频率=%dHz，分辨率=%d位", LEDC_FREQ, 13);
}

/* 设置亮度（0~100%）*/
void led_set_brightness(uint8_t percent)
{
    uint32_t duty = (uint32_t)((uint64_t)LEDC_MAX * percent / 100);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CH, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CH);
}

/* 渐变到指定亮度 */
void led_fade_to(uint8_t percent, uint32_t time_ms)
{
    uint32_t duty = (uint32_t)((uint64_t)LEDC_MAX * percent / 100);
    ledc_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CH, duty, time_ms);
    ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CH, LEDC_FADE_NO_WAIT);
}

void app_main(void)
{
    ledc_led_init();

    while (1) {
        led_fade_to(100, 1000);  // 1秒渐亮
        vTaskDelay(pdMS_TO_TICKS(1200));
        led_fade_to(0, 1000);    // 1秒渐暗
        vTaskDelay(pdMS_TO_TICKS(1200));
    }
}
```

---

### 示例2：RGB LED 控制

```c
#include "driver/ledc.h"

#define PIN_R  GPIO_NUM_4
#define PIN_G  GPIO_NUM_5
#define PIN_B  GPIO_NUM_6

#define LEDC_FREQ  5000
#define LEDC_RES   LEDC_TIMER_8_BIT   // 8 位，0~255
#define LEDC_MAX   255

void rgb_init(void)
{
    /* 三个通道共用一个定时器 */
    ledc_timer_config_t timer = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_RES,
        .timer_num       = LEDC_TIMER_0,
        .freq_hz         = LEDC_FREQ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer);

    int pins[3] = {PIN_R, PIN_G, PIN_B};
    ledc_channel_t chs[3] = {LEDC_CHANNEL_0, LEDC_CHANNEL_1, LEDC_CHANNEL_2};

    for (int i = 0; i < 3; i++) {
        ledc_channel_config_t ch = {
            .gpio_num   = pins[i],
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel    = chs[i],
            .timer_sel  = LEDC_TIMER_0,
            .duty       = 0,
            .hpoint     = 0,
        };
        ledc_channel_config(&ch);
    }
}

void rgb_set(uint8_t r, uint8_t g, uint8_t b)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, r);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, g);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, b);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2);
}
```

---

### 示例3：舵机控制（50Hz，500~2500μs）

```c
#include "driver/ledc.h"

#define SERVO_PIN    GPIO_NUM_5
#define SERVO_FREQ   50
#define SERVO_RES    LEDC_TIMER_16_BIT
#define SERVO_MAX    ((1 << 16) - 1)   // 65535
#define PERIOD_US    20000             // 20ms = 50Hz

/* 角度转占空比 */
static uint32_t angle_to_duty(int angle)
{
    /* 脉宽：500μs(0°) ~ 2500μs(180°) */
    uint32_t pulse_us = 500 + (uint32_t)((uint64_t)angle * 2000 / 180);
    return (uint32_t)((uint64_t)pulse_us * SERVO_MAX / PERIOD_US);
}

void servo_init(void)
{
    ledc_timer_config_t timer = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .duty_resolution = SERVO_RES,
        .timer_num       = LEDC_TIMER_1,
        .freq_hz         = SERVO_FREQ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t ch = {
        .gpio_num   = SERVO_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL_3,
        .timer_sel  = LEDC_TIMER_1,
        .duty       = angle_to_duty(90),
        .hpoint     = 0,
    };
    ledc_channel_config(&ch);
}

void servo_set_angle(int angle)
{
    if (angle < 0)   angle = 0;
    if (angle > 180) angle = 180;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3, angle_to_duty(angle));
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3);
}
```

---

### 示例4：蜂鸣器（频率可变）

```c
#include "driver/ledc.h"

#define BUZZER_PIN  GPIO_NUM_7

void buzzer_init(void)
{
    ledc_timer_config_t timer = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num       = LEDC_TIMER_2,
        .freq_hz         = 2000,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t ch = {
        .gpio_num   = BUZZER_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL_4,
        .timer_sel  = LEDC_TIMER_2,
        .duty       = 0,
        .hpoint     = 0,
    };
    ledc_channel_config(&ch);
}

/* 播放指定频率的音调 */
void buzzer_tone(uint32_t freq_hz, uint32_t duration_ms)
{
    ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_2, freq_hz);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_4, 512);  // 50% 占空比
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_4);
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_4, 0);
}

/* 播放 Do Re Mi */
void buzzer_play_scale(void)
{
    uint32_t notes[] = {262, 294, 330, 349, 392, 440, 494, 523};
    for (int i = 0; i < 8; i++) {
        buzzer_tone(notes[i], 300);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
```

---

## ⚠️ 注意事项

### 1. 定时器与通道的关系

- 4 个定时器（TIMER_0~3），每个定时器决定频率和分辨率
- 8 个通道（CHANNEL_0~7），每个通道绑定一个定时器
- 同一定时器的所有通道频率相同，但占空比独立
- 舵机和 LED 频率不同，必须用不同定时器

### 2. 渐变功能必须先安装

```c
ledc_fade_func_install(0);  // 只需调用一次
```
忘记调用会导致 `ledc_fade_with_time` 返回错误。

### 3. 占空比单位

占空比是整数，范围 `0 ~ (2^分辨率 - 1)`：
- 13 位：0~8191
- 16 位：0~65535
- 8 位：0~255

### 4. 输出反相

某些电路（共阳 LED、低电平有效）需要反相输出：
```c
ch.flags.output_invert = 1;  // 输出反相
```

---

## 📖 相关文档

- [IDF_GPIO.md](IDF_GPIO.md) — GPIO 配置
- [IDF_GPTimer.md](IDF_GPTimer.md) — 通用定时器
- [IDF_MCPWM.md](IDF_MCPWM.md) — 电机控制 PWM
- [README.md](README.md) — 模块导航
