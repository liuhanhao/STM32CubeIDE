# GPIO 通用 IO 模块 API 文档（ESP-IDF）

## 📋 模块概述

ESP-IDF 的 GPIO 驱动提供数字输入输出控制，包括：
- 引脚方向、上下拉、驱动能力配置
- 数字读写与翻转
- 中断触发（边沿/电平）
- GPIO Matrix 信号路由

---

## 📚 相关文件

- **头文件**: `driver/gpio.h`
- **源文件**: `driver/gpio.c`（IDF 内部）
- **Kconfig**: `CONFIG_GPIO_*`

---

## 🔧 数据类型

### gpio_config_t — GPIO 配置结构体

```c
typedef struct {
    uint64_t pin_bit_mask;    // 引脚位掩码，支持同时配置多个引脚
    gpio_mode_t mode;         // 输入/输出模式
    gpio_pullup_t pull_up_en; // 内部上拉
    gpio_pulldown_t pull_down_en; // 内部下拉
    gpio_int_type_t intr_type;    // 中断触发类型
} gpio_config_t;
```

**成员说明：**

| 成员 | 说明 | 可选值 |
|------|------|--------|
| `pin_bit_mask` | 引脚掩码，`1ULL << GPIO_NUM` | 如 `(1ULL<<4)|(1ULL<<5)` |
| `mode` | IO 方向 | 见[GPIO 工作模式](#gpio-工作模式) |
| `pull_up_en` | 上拉 | `GPIO_PULLUP_ENABLE / DISABLE` |
| `pull_down_en` | 下拉 | `GPIO_PULLDOWN_ENABLE / DISABLE` |
| `intr_type` | 中断类型 | 见[中断类型](#中断类型) |

---

### gpio_mode_t — 工作模式枚举

```c
typedef enum {
    GPIO_MODE_DISABLE           = 0,  // 禁用（高阻）
    GPIO_MODE_INPUT             = 1,  // 仅输入
    GPIO_MODE_OUTPUT            = 2,  // 仅输出（推挽）
    GPIO_MODE_OUTPUT_OD         = 6,  // 开漏输出
    GPIO_MODE_INPUT_OUTPUT_OD   = 7,  // 开漏输入输出
    GPIO_MODE_INPUT_OUTPUT      = 3,  // 推挽输入输出
} gpio_mode_t;
```

---

### gpio_int_type_t — 中断类型枚举

```c
typedef enum {
    GPIO_INTR_DISABLE   = 0,  // 禁用中断
    GPIO_INTR_POSEDGE   = 1,  // 上升沿
    GPIO_INTR_NEGEDGE   = 2,  // 下降沿
    GPIO_INTR_ANYEDGE   = 3,  // 双边沿
    GPIO_INTR_LOW_LEVEL = 4,  // 低电平
    GPIO_INTR_HIGH_LEVEL= 5,  // 高电平
} gpio_int_type_t;
```

---

## 📌 常量定义

### GPIO 引脚编号

ESP32-S3 共 45 个 GPIO（GPIO0 ~ GPIO48，部分编号保留）：

```c
/* 常用引脚宏 */
#define GPIO_NUM_0   (gpio_num_t)0
#define GPIO_NUM_1   (gpio_num_t)1
// ... 以此类推
#define GPIO_NUM_48  (gpio_num_t)48
#define GPIO_NUM_MAX (gpio_num_t)49
```

**ESP32-S3 引脚注意事项：**

| 引脚范围 | 说明 |
|---------|------|
| GPIO0 | Strapping 引脚，影响启动模式，慎用 |
| GPIO3 | Strapping 引脚 |
| GPIO19, GPIO20 | USB D-/D+，使用 USB 功能时不可复用 |
| GPIO26~GPIO32 | 连接 PSRAM（N16R8），不可用作普通 GPIO |
| GPIO33~GPIO37 | 连接 Flash，不可用 |
| GPIO38~GPIO48 | 可用，但部分开发板未引出 |

---

## 🎯 核心函数

### 初始化函数

#### gpio_config()

批量配置 GPIO，推荐使用此函数（比逐个调用更高效）。

```c
esp_err_t gpio_config(const gpio_config_t *pGPIOConfig);
```

**参数：**
- `pGPIOConfig`: 配置结构体指针

**返回值：**
- `ESP_OK`: 成功
- `ESP_ERR_INVALID_ARG`: 参数无效

**使用示例：**

```c
// 示例1：配置 GPIO4 为推挽输出（控制 LED）
gpio_config_t io_conf = {
    .pin_bit_mask = (1ULL << GPIO_NUM_4),
    .mode         = GPIO_MODE_OUTPUT,
    .pull_up_en   = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type    = GPIO_INTR_DISABLE,
};
ESP_ERROR_CHECK(gpio_config(&io_conf));
```

```c
// 示例2：配置 GPIO0 为上拉输入（按键）
gpio_config_t btn_conf = {
    .pin_bit_mask = (1ULL << GPIO_NUM_0),
    .mode         = GPIO_MODE_INPUT,
    .pull_up_en   = GPIO_PULLUP_ENABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type    = GPIO_INTR_DISABLE,
};
ESP_ERROR_CHECK(gpio_config(&btn_conf));
```

```c
// 示例3：同时配置多个引脚为输出
gpio_config_t multi_conf = {
    .pin_bit_mask = (1ULL << GPIO_NUM_4) | (1ULL << GPIO_NUM_5) | (1ULL << GPIO_NUM_6),
    .mode         = GPIO_MODE_OUTPUT,
    .pull_up_en   = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type    = GPIO_INTR_DISABLE,
};
ESP_ERROR_CHECK(gpio_config(&multi_conf));
```

---

#### gpio_reset_pin()

将引脚复位为默认状态（输入，无上下拉，禁用中断）。

```c
esp_err_t gpio_reset_pin(gpio_num_t gpio_num);
```

**使用示例：**
```c
gpio_reset_pin(GPIO_NUM_4);
```

---

### IO 操作函数

#### gpio_set_level()

设置输出电平。

```c
esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level);
```

**参数：**
- `gpio_num`: 引脚编号
- `level`: `0` = 低电平，`1` = 高电平

**使用示例：**
```c
gpio_set_level(GPIO_NUM_4, 1);   // 高电平
gpio_set_level(GPIO_NUM_4, 0);   // 低电平
```

---

#### gpio_get_level()

读取引脚电平。

```c
int gpio_get_level(gpio_num_t gpio_num);
```

**返回值：**
- `0`: 低电平
- `1`: 高电平

**使用示例：**
```c
int level = gpio_get_level(GPIO_NUM_0);
if (level == 0) {
    ESP_LOGI("GPIO", "按键按下");
}
```

---

#### gpio_set_direction()

单独设置引脚方向（不影响其他配置）。

```c
esp_err_t gpio_set_direction(gpio_num_t gpio_num, gpio_mode_t mode);
```

---

#### gpio_set_pull_mode()

设置上下拉模式。

```c
esp_err_t gpio_set_pull_mode(gpio_num_t gpio_num, gpio_pull_mode_t pull);
```

**可选值：**

| 常量 | 说明 |
|------|------|
| `GPIO_PULLUP_ONLY` | 仅上拉 |
| `GPIO_PULLDOWN_ONLY` | 仅下拉 |
| `GPIO_PULLUP_PULLDOWN` | 上下拉同时（弱保持） |
| `GPIO_FLOATING` | 浮空 |

---

### 中断函数

#### gpio_install_isr_service()

安装 GPIO 中断服务（必须在注册中断前调用一次）。

```c
esp_err_t gpio_install_isr_service(int intr_alloc_flags);
```

**参数：**
- `intr_alloc_flags`: 通常传 `0` 或 `ESP_INTR_FLAG_IRAM`（中断处理函数在 IRAM 中）

---

#### gpio_isr_handler_add()

为指定引脚注册中断回调。

```c
esp_err_t gpio_isr_handler_add(gpio_num_t gpio_num,
                                gpio_isr_t isr_handler,
                                void *args);
```

**参数：**
- `gpio_num`: 引脚编号
- `isr_handler`: 中断回调函数（`void func(void *arg)` 格式）
- `args`: 传给回调的参数

---

#### gpio_isr_handler_remove()

移除引脚的中断回调。

```c
esp_err_t gpio_isr_handler_remove(gpio_num_t gpio_num);
```

---

#### gpio_set_intr_type()

修改中断触发类型（不重新配置整个引脚）。

```c
esp_err_t gpio_set_intr_type(gpio_num_t gpio_num, gpio_int_type_t intr_type);
```

---

#### gpio_intr_enable() / gpio_intr_disable()

使能/禁用指定引脚的中断。

```c
esp_err_t gpio_intr_enable(gpio_num_t gpio_num);
esp_err_t gpio_intr_disable(gpio_num_t gpio_num);
```

---

## 💡 完整应用示例

### 示例1：LED 闪烁

```c
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define LED_PIN GPIO_NUM_4

void led_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));
}

void app_main(void)
{
    led_init();
    while (1) {
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(500));
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
```

---

### 示例2：按键中断（带消抖）

```c
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#define BTN_PIN  GPIO_NUM_0
#define LED_PIN  GPIO_NUM_4

static const char *TAG = "BTN";
static QueueHandle_t gpio_evt_queue = NULL;

/* 中断回调（在 ISR 中，不能调用非 IRAM 函数）*/
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

/* 按键处理任务 */
static void button_task(void *arg)
{
    uint32_t io_num;
    while (1) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            /* 软件消抖：等待 20ms 后再读电平 */
            vTaskDelay(pdMS_TO_TICKS(20));
            if (gpio_get_level(io_num) == 0) {
                ESP_LOGI(TAG, "GPIO%lu 按下", io_num);
                gpio_set_level(LED_PIN, !gpio_get_level(LED_PIN));
            }
        }
    }
}

void app_main(void)
{
    /* 配置 LED */
    gpio_config_t led_cfg = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_cfg);

    /* 配置按键（下降沿中断）*/
    gpio_config_t btn_cfg = {
        .pin_bit_mask = (1ULL << BTN_PIN),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .intr_type    = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&btn_cfg);

    /* 创建事件队列 */
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    /* 安装 ISR 服务并注册回调 */
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BTN_PIN, gpio_isr_handler, (void *)BTN_PIN);

    /* 创建处理任务 */
    xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);
}
```

---

### 示例3：开漏输出（模拟 I2C 总线）

```c
#define SDA_PIN GPIO_NUM_21
#define SCL_PIN GPIO_NUM_22

void soft_i2c_gpio_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << SDA_PIN) | (1ULL << SCL_PIN),
        .mode         = GPIO_MODE_INPUT_OUTPUT_OD,  // 开漏，可读可写
        .pull_up_en   = GPIO_PULLUP_ENABLE,          // 内部上拉（正式项目建议外部 4.7kΩ）
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);

    /* 释放总线（高电平）*/
    gpio_set_level(SDA_PIN, 1);
    gpio_set_level(SCL_PIN, 1);
}
```

---

## ⚠️ 注意事项

### 1. Strapping 引脚

GPIO0、GPIO3、GPIO45、GPIO46 是 Strapping 引脚，影响芯片启动模式。
上电时这些引脚的电平决定启动行为，使用时需确保上电状态正确。

### 2. 输入输出同时使用

ESP32 的 GPIO 支持同时配置为输入输出（`GPIO_MODE_INPUT_OUTPUT`），
可以在输出的同时读回实际电平，用于检测总线冲突。

### 3. 中断回调限制

GPIO 中断回调运行在 ISR 上下文中：
- 不能调用非 `IRAM_ATTR` 的函数
- 不能使用 `printf`（用 `ESP_DRAM_LOGI` 代替）
- 不能调用阻塞 API（`vTaskDelay` 等）
- 推荐通过队列将事件发送给普通任务处理

### 4. 驱动能力

```c
/* 设置驱动能力（影响上升/下降沿速度和 EMI）*/
gpio_set_drive_capability(GPIO_NUM_4, GPIO_DRIVE_CAP_2);  // 默认中等驱动
```

| 等级 | 电流 | 说明 |
|------|------|------|
| `GPIO_DRIVE_CAP_0` | ~5mA | 最弱 |
| `GPIO_DRIVE_CAP_1` | ~10mA | 弱 |
| `GPIO_DRIVE_CAP_2` | ~20mA | 中（默认） |
| `GPIO_DRIVE_CAP_3` | ~40mA | 最强 |

### 5. RTC GPIO

GPIO0~GPIO21 同时也是 RTC GPIO，可在深度睡眠中保持状态或唤醒芯片：

```c
#include "driver/rtc_io.h"

/* 配置为 RTC 输出，深度睡眠中保持电平 */
rtc_gpio_init(GPIO_NUM_4);
rtc_gpio_set_direction(GPIO_NUM_4, RTC_GPIO_MODE_OUTPUT_ONLY);
rtc_gpio_set_level(GPIO_NUM_4, 1);
rtc_gpio_hold_en(GPIO_NUM_4);   // 睡眠中保持电平
```

---

## 📖 相关文档

- [IDF_UART.md](IDF_UART.md) — 串口通信
- [IDF_I2C.md](IDF_I2C.md) — I2C 总线
- [IDF_SPI.md](IDF_SPI.md) — SPI 总线
- [IDF_TIM.md](IDF_TIM.md) — 定时器与 PWM
- [README.md](README.md) — 模块导航
