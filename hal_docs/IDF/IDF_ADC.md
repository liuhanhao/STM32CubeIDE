# ADC 模数转换模块 API 文档（ESP-IDF）

## 📋 模块概述

ESP32-S3 内置两个 12 位 SAR ADC，共 20 个通道，ESP-IDF 提供：
- 单次采样（One-shot）模式
- 连续采样（Continuous）模式（DMA 驱动）
- 硬件自动校准（补偿温漂和电压偏差）
- 电压换算（原始值 → mV）

---

## 🗺️ 使用场景速查

| 场景 | 推荐模式 |
|------|----------|
| 偶发读一次（电位器、电池电压） | One-shot 模式 |
| 持续采集单路信号 | One-shot 循环 或 Continuous |
| 多路并行高速采集（音频、波形） | Continuous + DMA |
| 精确电压测量 | One-shot + 校准 + `adc_cali_raw_to_voltage` |

---

## 📚 相关文件

- **头文件**: `esp_adc/adc_oneshot.h`（One-shot）
- **头文件**: `esp_adc/adc_continuous.h`（Continuous）
- **头文件**: `esp_adc/adc_cali.h`（校准）
- **头文件**: `esp_adc/adc_cali_scheme.h`（校准方案）

---

## 📌 硬件规格

### ADC 通道映射（ESP32-S3）

| ADC 单元 | 通道 | GPIO |
|---------|------|------|
| ADC1 | CH0 | GPIO1 |
| ADC1 | CH1 | GPIO2 |
| ADC1 | CH2 | GPIO3 |
| ADC1 | CH3 | GPIO4 |
| ADC1 | CH4 | GPIO5 |
| ADC1 | CH5 | GPIO6 |
| ADC1 | CH6 | GPIO7 |
| ADC1 | CH7 | GPIO8 |
| ADC1 | CH8 | GPIO9 |
| ADC1 | CH9 | GPIO10 |
| ADC2 | CH0 | GPIO11 |
| ADC2 | CH1 | GPIO12 |
| ADC2 | CH2 | GPIO13 |
| ADC2 | CH3 | GPIO14 |
| ADC2 | CH4 | GPIO15 |
| ADC2 | CH5 | GPIO16 |
| ADC2 | CH6 | GPIO17 |
| ADC2 | CH7 | GPIO18 |
| ADC2 | CH8 | GPIO19 |
| ADC2 | CH9 | GPIO20 |

> ⚠️ **ADC2 限制：** ADC2 与 WiFi 共用，WiFi 启用时 ADC2 不可用。需要 WiFi 的项目只用 ADC1。

### 衰减与量程

| 衰减 | 有效量程 | 说明 |
|------|---------|------|
| `ADC_ATTEN_DB_0` | 0 ~ 750mV | 无衰减，精度最高 |
| `ADC_ATTEN_DB_2_5` | 0 ~ 1050mV | 2.5dB 衰减 |
| `ADC_ATTEN_DB_6` | 0 ~ 1300mV | 6dB 衰减 |
| `ADC_ATTEN_DB_12` | 0 ~ 3100mV | 12dB 衰减（最常用，覆盖 3.3V 大部分范围） |

---

## 🎯 核心函数（One-shot 模式）

### adc_oneshot_new_unit()

创建 ADC 单元句柄。

```c
esp_err_t adc_oneshot_new_unit(const adc_oneshot_unit_init_cfg_t *init_config,
                                adc_oneshot_unit_handle_t *ret_unit);
```

**配置结构体：**
```c
typedef struct {
    adc_unit_t unit_id;          // ADC_UNIT_1 或 ADC_UNIT_2
    adc_oneshot_clk_src_t clk_src; // 时钟源（ADC_RTC_CLK_SRC_DEFAULT）
    adc_ulp_mode_t ulp_mode;     // ULP 模式（ADC_ULP_MODE_DISABLE）
} adc_oneshot_unit_init_cfg_t;
```

---

### adc_oneshot_config_channel()

配置 ADC 通道（衰减和位宽）。

```c
esp_err_t adc_oneshot_config_channel(adc_oneshot_unit_handle_t handle,
                                      adc_channel_t channel,
                                      const adc_oneshot_chan_cfg_t *config);
```

**配置结构体：**
```c
typedef struct {
    adc_atten_t atten;    // 衰减（ADC_ATTEN_DB_12 最常用）
    adc_bitwidth_t bitwidth; // 位宽（ADC_BITWIDTH_DEFAULT = 12 位）
} adc_oneshot_chan_cfg_t;
```

---

### adc_oneshot_read()

读取一次 ADC 原始值。

```c
esp_err_t adc_oneshot_read(adc_oneshot_unit_handle_t handle,
                            adc_channel_t channel,
                            int *out_raw);
```

**返回值：** `out_raw` 范围 0~4095（12 位）

---

### adc_oneshot_del_unit()

删除 ADC 单元，释放资源。

```c
esp_err_t adc_oneshot_del_unit(adc_oneshot_unit_handle_t handle);
```

---

## 🎯 核心函数（校准）

### adc_cali_create_scheme_curve_fitting()

创建曲线拟合校准方案（ESP32-S3 推荐）。

```c
esp_err_t adc_cali_create_scheme_curve_fitting(
    const adc_cali_curve_fitting_config_t *config,
    adc_cali_handle_t *ret_handle);
```

**配置结构体：**
```c
typedef struct {
    adc_unit_t unit_id;    // ADC 单元
    adc_channel_t chan;    // 通道
    adc_atten_t atten;     // 衰减（必须与通道配置一致）
    adc_bitwidth_t bitwidth; // 位宽
} adc_cali_curve_fitting_config_t;
```

---

### adc_cali_raw_to_voltage()

将原始 ADC 值转换为电压（mV）。

```c
esp_err_t adc_cali_raw_to_voltage(adc_cali_handle_t handle,
                                   int raw,
                                   int *voltage);
```

---

### adc_cali_delete_scheme_curve_fitting()

删除校准方案。

```c
esp_err_t adc_cali_delete_scheme_curve_fitting(adc_cali_handle_t handle);
```

---

## 🎯 核心函数（Continuous 模式）

### adc_continuous_new_handle()

创建连续采样句柄。

```c
esp_err_t adc_continuous_new_handle(const adc_continuous_handle_cfg_t *hdl_config,
                                     adc_continuous_handle_t *ret_handle);
```

**配置结构体：**
```c
typedef struct {
    uint32_t max_store_buf_size;  // 内部缓冲区大小（字节）
    uint32_t conv_frame_size;     // 每次 DMA 传输的帧大小（字节）
} adc_continuous_handle_cfg_t;
```

---

### adc_continuous_config()

配置连续采样参数。

```c
esp_err_t adc_continuous_config(adc_continuous_handle_t handle,
                                 const adc_continuous_config_t *config);
```

---

### adc_continuous_start() / adc_continuous_stop()

启动/停止连续采样。

```c
esp_err_t adc_continuous_start(adc_continuous_handle_t handle);
esp_err_t adc_continuous_stop(adc_continuous_handle_t handle);
```

---

### adc_continuous_read()

从 DMA 缓冲区读取采样数据。

```c
esp_err_t adc_continuous_read(adc_continuous_handle_t handle,
                               uint8_t *buf,
                               uint32_t length_max,
                               uint32_t *out_length,
                               uint32_t timeout_ms);
```

---

## 💡 完整应用示例

### 示例1：One-shot 读取电位器电压

```c
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"

#define ADC_CHANNEL  ADC_CHANNEL_3   // GPIO4
#define ADC_ATTEN    ADC_ATTEN_DB_12

static const char *TAG = "ADC";

static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t cali_handle;
static bool cali_enable = false;

void adc_init(void)
{
    /* 1. 创建 ADC1 单元 */
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id  = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc_handle));

    /* 2. 配置通道 */
    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten    = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &chan_cfg));

    /* 3. 创建校准方案（曲线拟合，ESP32-S3 支持）*/
    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id  = ADC_UNIT_1,
        .chan     = ADC_CHANNEL,
        .atten    = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    esp_err_t ret = adc_cali_create_scheme_curve_fitting(&cali_cfg, &cali_handle);
    if (ret == ESP_OK) {
        cali_enable = true;
        ESP_LOGI(TAG, "ADC 校准已启用");
    } else {
        ESP_LOGW(TAG, "ADC 校准不可用，使用原始值");
    }
}

void app_main(void)
{
    adc_init();

    int raw, voltage_mv;
    while (1) {
        /* 读取原始值 */
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw));

        if (cali_enable) {
            /* 转换为电压（mV）*/
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, raw, &voltage_mv));
            ESP_LOGI(TAG, "原始值: %d, 电压: %d mV (%.2f V)",
                     raw, voltage_mv, voltage_mv / 1000.0f);
        } else {
            /* 无校准时手动换算（不精确）*/
            voltage_mv = raw * 3300 / 4095;
            ESP_LOGI(TAG, "原始值: %d, 估算电压: %d mV", raw, voltage_mv);
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
```

---

### 示例2：多通道采集（多次 One-shot）

```c
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

#define NUM_CHANNELS 4
static const adc_channel_t channels[NUM_CHANNELS] = {
    ADC_CHANNEL_0,  // GPIO1
    ADC_CHANNEL_1,  // GPIO2
    ADC_CHANNEL_2,  // GPIO3
    ADC_CHANNEL_3,  // GPIO4
};

static adc_oneshot_unit_handle_t adc_handle;

void multi_channel_init(void)
{
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&unit_cfg, &adc_handle);

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten    = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    for (int i = 0; i < NUM_CHANNELS; i++) {
        adc_oneshot_config_channel(adc_handle, channels[i], &chan_cfg);
    }
}

void read_all_channels(int *results)
{
    for (int i = 0; i < NUM_CHANNELS; i++) {
        adc_oneshot_read(adc_handle, channels[i], &results[i]);
    }
}
```

---

### 示例3：Continuous 模式高速采集

```c
#include "esp_adc/adc_continuous.h"
#include "esp_log.h"

#define SAMPLE_FREQ_HZ  20000   // 20kHz 采样率
#define CONV_FRAME_SIZE 256     // 每帧 256 字节

static const char *TAG = "ADC_CONT";
static adc_continuous_handle_t cont_handle;

static bool IRAM_ATTR adc_conv_done_cb(adc_continuous_handle_t handle,
                                        const adc_continuous_evt_data_t *edata,
                                        void *user_data)
{
    BaseType_t high_task_wakeup = pdFALSE;
    /* 通知任务有数据可读 */
    TaskHandle_t task = (TaskHandle_t)user_data;
    vTaskNotifyGiveFromISR(task, &high_task_wakeup);
    return high_task_wakeup == pdTRUE;
}

void adc_continuous_task(void *arg)
{
    uint8_t result[CONV_FRAME_SIZE];
    uint32_t ret_num;

    while (1) {
        /* 等待 DMA 完成通知 */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        esp_err_t ret = adc_continuous_read(cont_handle, result,
                                             CONV_FRAME_SIZE, &ret_num, 0);
        if (ret == ESP_OK) {
            /* 解析数据（每个采样点 4 字节：2 字节数据 + 2 字节通道信息）*/
            for (int i = 0; i < ret_num; i += SOC_ADC_DIGI_RESULT_BYTES) {
                adc_digi_output_data_t *p = (adc_digi_output_data_t *)&result[i];
                ESP_LOGD(TAG, "通道 %d: %d", p->type2.channel, p->type2.data);
            }
        }
    }
}

void adc_continuous_init(void)
{
    /* 1. 创建句柄 */
    adc_continuous_handle_cfg_t hdl_cfg = {
        .max_store_buf_size = 1024,
        .conv_frame_size    = CONV_FRAME_SIZE,
    };
    adc_continuous_new_handle(&hdl_cfg, &cont_handle);

    /* 2. 配置采样参数 */
    adc_digi_pattern_config_t adc_pattern = {
        .atten     = ADC_ATTEN_DB_12,
        .channel   = ADC_CHANNEL_3,
        .unit      = ADC_UNIT_1,
        .bit_width = ADC_BITWIDTH_12,
    };
    adc_continuous_config_t cont_cfg = {
        .pattern_num    = 1,
        .adc_pattern    = &adc_pattern,
        .sample_freq_hz = SAMPLE_FREQ_HZ,
        .conv_mode      = ADC_CONV_SINGLE_UNIT_1,
        .format         = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
    };
    adc_continuous_config(cont_handle, &cont_cfg);

    /* 3. 注册回调 */
    TaskHandle_t task_handle;
    xTaskCreate(adc_continuous_task, "adc_task", 4096, NULL, 5, &task_handle);

    adc_continuous_evt_cbs_t cbs = {
        .on_conv_done = adc_conv_done_cb,
    };
    adc_continuous_register_event_callbacks(cont_handle, &cbs, task_handle);

    /* 4. 启动 */
    adc_continuous_start(cont_handle);
}
```

---

### 示例4：电池电压监测

```c
/* 典型应用：通过分压电阻监测锂电池电压
 * 电池 4.2V → 分压（100kΩ + 100kΩ）→ 2.1V → ADC
 * 读取 ADC 电压后乘以 2 得到实际电池电压 */

#define BATT_ADC_CHANNEL  ADC_CHANNEL_5   // GPIO6
#define DIVIDER_RATIO     2.0f            // 分压比

float read_battery_voltage(adc_oneshot_unit_handle_t handle,
                            adc_cali_handle_t cali)
{
    int raw, mv;
    adc_oneshot_read(handle, BATT_ADC_CHANNEL, &raw);
    adc_cali_raw_to_voltage(cali, raw, &mv);
    return (mv / 1000.0f) * DIVIDER_RATIO;
}
```

---

## ⚠️ 注意事项

### 1. ADC2 与 WiFi 冲突

WiFi 启用时 ADC2 不可用，会返回 `ESP_ERR_TIMEOUT`。
需要 WiFi 的项目只使用 ADC1（GPIO1~GPIO10）。

### 2. ADC 引脚不能有数字输出

ADC 引脚必须配置为模拟输入，不能同时作为数字 IO 使用。
IDF 的 ADC 驱动会自动配置引脚，无需手动调用 `gpio_config`。

### 3. 输入电压范围

ADC 输入电压不能超过 3.3V（VDDA），也不能为负值，否则损坏芯片。
使用 `ADC_ATTEN_DB_12` 时有效范围约 0~3.1V，超过 3.1V 的值会饱和在 4095。

### 4. 多次采样取平均

ADC 存在噪声，建议多次采样取平均：

```c
int adc_read_average(adc_oneshot_unit_handle_t handle,
                     adc_channel_t channel, int samples)
{
    int sum = 0;
    for (int i = 0; i < samples; i++) {
        int raw;
        adc_oneshot_read(handle, channel, &raw);
        sum += raw;
    }
    return sum / samples;
}
```

### 5. 校准方案选择

| 方案 | 函数 | 适用芯片 |
|------|------|---------|
| 曲线拟合 | `adc_cali_create_scheme_curve_fitting` | ESP32-S3（推荐） |
| 线性拟合 | `adc_cali_create_scheme_line_fitting` | ESP32、ESP32-S2 |

---

## 📖 相关文档

- [IDF_GPIO.md](IDF_GPIO.md) — GPIO 配置
- [IDF_TIM.md](IDF_TIM.md) — 定时器（定时触发 ADC）
- [README.md](README.md) — 模块导航
