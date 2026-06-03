# DAC — 数模转换 API 文档（ESP-IDF）

## 📋 模块概述

ESP32-S3 内置 2 路 8 位 DAC，提供：
- 直接输出固定电压
- 余弦波发生器（硬件生成，无需 CPU）
- DMA 连续输出（音频播放）

**DAC 通道与引脚（固定，不可更改）：**

| 通道 | GPIO | 说明 |
|------|------|------|
| DAC_CHAN_0 | GPIO17 | DAC 通道 0 |
| DAC_CHAN_1 | GPIO18 | DAC 通道 1 |

> **注意：** ESP32-S3 的 DAC 分辨率为 8 位（0~255），输出范围 0~VDD（3.3V）。
> 精度低于 STM32 的 12 位 DAC，高精度场景建议外接 MCP4725（I2C，12 位）。

---

## 🗺️ 使用场景速查

| 场景 | 方案 |
|------|------|
| 输出固定偏置电压 | `dac_oneshot_output_voltage` |
| 音频播放（PCM）| DAC + DMA 连续模式 |
| 正弦波/余弦波生成 | DAC 余弦波发生器 |
| 高精度模拟输出 | 外接 MCP4725（I2C，12 位）|
| 电机调速（模拟量）| DAC 或 LEDC PWM + 低通滤波 |

**DAC vs PWM + 低通滤波：**

| 特性 | DAC | PWM + 滤波 |
|------|-----|-----------|
| 输出质量 | 平滑，无纹波 | 有纹波（取决于滤波器）|
| 分辨率 | 8 位（256 级）| 取决于 PWM 分辨率（可达 16 位）|
| 成本 | 无需外部器件 | 需要 RC 滤波电路 |
| 适用 | 音频、精确偏置 | 电机调速、LED 调光 |

---

## 📚 相关文件

- **头文件**: `driver/dac_oneshot.h`（单次输出）
- **头文件**: `driver/dac_cosine.h`（余弦波）
- **头文件**: `driver/dac_continuous.h`（DMA 连续）

---

## 一、单次输出模式

### 初始化与输出

```c
#include "driver/dac_oneshot.h"
#include "esp_log.h"

static const char *TAG = "DAC";
static dac_oneshot_handle_t dac_handle;

void dac_init(void)
{
    dac_oneshot_config_t cfg = {
        .chan_id = DAC_CHAN_0,  // GPIO17
    };
    ESP_ERROR_CHECK(dac_oneshot_new_channel(&cfg, &dac_handle));
    ESP_LOGI(TAG, "DAC 初始化完成");
}

/* 输出指定电压（0~3300 mV）*/
void dac_set_voltage_mv(uint32_t mv)
{
    if (mv > 3300) mv = 3300;
    uint8_t value = (uint8_t)(mv * 255 / 3300);
    dac_oneshot_output_voltage(dac_handle, value);
}

/* 输出原始值（0~255）*/
void dac_set_raw(uint8_t value)
{
    dac_oneshot_output_voltage(dac_handle, value);
}

void dac_deinit(void)
{
    dac_oneshot_del_channel(dac_handle);
}
```

---

## 二、余弦波发生器

硬件生成余弦波，无需 CPU 干预。

```c
#include "driver/dac_cosine.h"

static dac_cosine_handle_t cosine_handle;

void dac_cosine_init(uint32_t freq_hz)
{
    dac_cosine_config_t cfg = {
        .chan_id   = DAC_CHAN_0,
        .freq_hz   = freq_hz,   // 频率（Hz）
        .clk_src   = DAC_COSINE_CLK_SRC_DEFAULT,
        .offset    = 0,         // 直流偏置（-128~127）
        .phase     = DAC_COSINE_PHASE_0,  // 初始相位
        .atten     = DAC_COSINE_ATTEN_DB_0,  // 衰减（0dB = 满幅）
        .flags.force_set_freq = false,
    };
    ESP_ERROR_CHECK(dac_cosine_new_channel(&cfg, &cosine_handle));
    ESP_ERROR_CHECK(dac_cosine_start(cosine_handle));
}

void dac_cosine_stop_output(void)
{
    dac_cosine_stop(cosine_handle);
    dac_cosine_del_channel(cosine_handle);
}
```

**衰减选项：**

| 值 | 衰减 | 输出幅度 |
|----|------|---------|
| `DAC_COSINE_ATTEN_DB_0` | 0dB | 满幅（0~3.3V）|
| `DAC_COSINE_ATTEN_DB_6` | 6dB | 1/2 幅 |
| `DAC_COSINE_ATTEN_DB_12` | 12dB | 1/4 幅 |
| `DAC_COSINE_ATTEN_DB_18` | 18dB | 1/8 幅 |

---

## 三、DMA 连续输出（音频）

```c
#include "driver/dac_continuous.h"

static dac_continuous_handle_t dac_cont_handle;

void dac_audio_init(uint32_t sample_rate_hz)
{
    dac_continuous_config_t cfg = {
        .chan_mask   = DAC_CHANNEL_MASK_CH0,  // 使用通道 0
        .desc_num    = 4,                      // DMA 描述符数量
        .buf_size    = 2048,                   // 每个缓冲区大小（字节）
        .freq_hz     = sample_rate_hz,         // 采样率
        .offset      = 0,
        .clk_src     = DAC_DIGI_CLK_SRC_DEFAULT,
        .chan_mode    = DAC_CHANNEL_MODE_SIMUL,
    };
    ESP_ERROR_CHECK(dac_continuous_new_channels(&cfg, &dac_cont_handle));
    ESP_ERROR_CHECK(dac_continuous_enable(dac_cont_handle));
}

/* 写入 PCM 数据（阻塞直到写入完成）*/
void dac_audio_write(const uint8_t *data, size_t len)
{
    size_t bytes_written;
    dac_continuous_write(dac_cont_handle, data, len, &bytes_written, portMAX_DELAY);
}

void dac_audio_deinit(void)
{
    dac_continuous_disable(dac_cont_handle);
    dac_continuous_del_channels(dac_cont_handle);
}
```

---

## 💡 完整应用示例

### 示例1：输出参考电压（1.65V 中点）

```c
#include "driver/dac_oneshot.h"

void dac_reference_voltage_init(void)
{
    dac_oneshot_handle_t handle;
    dac_oneshot_config_t cfg = { .chan_id = DAC_CHAN_0 };
    dac_oneshot_new_channel(&cfg, &handle);

    /* 输出 1.65V（3.3V 的一半，用作运放参考）*/
    dac_oneshot_output_voltage(handle, 128);  // 128/255 × 3.3V ≈ 1.65V
}
```

---

### 示例2：生成正弦波（1kHz）

```c
#include "driver/dac_cosine.h"
#include "esp_log.h"

void sine_wave_demo(void)
{
    dac_cosine_handle_t handle;
    dac_cosine_config_t cfg = {
        .chan_id  = DAC_CHAN_0,
        .freq_hz  = 1000,  // 1kHz
        .clk_src  = DAC_COSINE_CLK_SRC_DEFAULT,
        .offset   = 0,
        .phase    = DAC_COSINE_PHASE_0,
        .atten    = DAC_COSINE_ATTEN_DB_0,
    };
    dac_cosine_new_channel(&cfg, &handle);
    dac_cosine_start(handle);

    ESP_LOGI("DAC", "1kHz 余弦波输出中（GPIO17）");
    /* 持续输出，无需 CPU 干预 */
}
```

---

### 示例3：简单音频播放（8kHz PCM）

```c
#include "driver/dac_continuous.h"
#include "esp_log.h"

/* 简单的 440Hz 正弦波 PCM 数据（8kHz 采样率，1 个周期 ≈ 18 个采样点）*/
static const uint8_t sine_440hz[] = {
    128, 172, 210, 238, 253, 253, 238, 210,
    172, 128,  84,  46,  18,   3,   3,  18,
     46,  84
};

void audio_demo(void)
{
    dac_continuous_handle_t handle;
    dac_continuous_config_t cfg = {
        .chan_mask = DAC_CHANNEL_MASK_CH0,
        .desc_num  = 4,
        .buf_size  = 2048,
        .freq_hz   = 8000,  // 8kHz 采样率
        .offset    = 0,
        .clk_src   = DAC_DIGI_CLK_SRC_DEFAULT,
        .chan_mode  = DAC_CHANNEL_MODE_SIMUL,
    };
    dac_continuous_new_channels(&cfg, &handle);
    dac_continuous_enable(handle);

    /* 循环播放 440Hz 音调 */
    for (int i = 0; i < 8000; i++) {  // 播放 1 秒
        size_t written;
        dac_continuous_write(handle, sine_440hz, sizeof(sine_440hz),
                             &written, pdMS_TO_TICKS(100));
    }

    dac_continuous_disable(handle);
    dac_continuous_del_channels(handle);
}
```

---

## ⚠️ 注意事项

### 1. 引脚固定

DAC 引脚固定为 GPIO17（通道 0）和 GPIO18（通道 1），不可更改。
使用 DAC 时这两个引脚不能用于其他功能。

### 2. 8 位分辨率

ESP32-S3 的 DAC 只有 8 位（256 级），精度约 13mV/步。
需要更高精度时使用外接 DAC 芯片（MCP4725 12 位，通过 I2C 控制）。

### 3. 输出阻抗

DAC 输出阻抗较高（约 25kΩ），驱动能力弱。
驱动低阻抗负载（如扬声器）需要加运放缓冲。

### 4. 余弦波频率范围

余弦波发生器频率范围约 130Hz~100kHz，超出范围会报错。

---

## 📖 相关文档

- [IDF_ADC.md](IDF_ADC.md) — 模数转换
- [IDF_LEDC.md](IDF_LEDC.md) — PWM（可替代 DAC 做模拟输出）
- [IDF_I2C.md](IDF_I2C.md) — I2C（外接 MCP4725）
- [README.md](README.md) — 模块导航
