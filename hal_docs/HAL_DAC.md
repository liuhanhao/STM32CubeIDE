# DAC数模转换模块 API文档

## 📋 模块概述

DAC(Digital-to-Analog Converter)数模转换器将数字信号转换为模拟信号:
- 2个DAC通道
- 12位分辨率
- 输出范围: 0 ~ VDDA
- 支持DMA传输
- 支持三角波和噪声波形生成
- 可由定时器触发

---

## 📚 相关文件

- **头文件**: `stm32f1xx_hal_dac.h`, `stm32f1xx_hal_dac_ex.h`
- **源文件**: `stm32f1xx_hal_dac.c`, `stm32f1xx_hal_dac_ex.c`

---

## 🎯 核心函数

### HAL_DAC_Init()

初始化DAC。

```c
HAL_StatusTypeDef HAL_DAC_Init(DAC_HandleTypeDef *hdac);
```

---

### HAL_DAC_Start()

启动DAC输出。

```c
HAL_StatusTypeDef HAL_DAC_Start(DAC_HandleTypeDef *hdac, uint32_t Channel);
```

---

### HAL_DAC_SetValue()

设置DAC输出值。

```c
HAL_StatusTypeDef HAL_DAC_SetValue(DAC_HandleTypeDef *hdac,
                                   uint32_t Channel,
                                   uint32_t Alignment,
                                   uint32_t Data);
```

**参数:**
- `Channel`: `DAC_CHANNEL_1` 或 `DAC_CHANNEL_2`
- `Alignment`: 数据对齐方式
  - `DAC_ALIGN_12B_R`: 12位右对齐
  - `DAC_ALIGN_12B_L`: 12位左对齐
  - `DAC_ALIGN_8B_R`: 8位右对齐
- `Data`: 输出数据值(0-4095)

---

## 💡 完整应用示例

### 示例1: 输出固定电压

```c
DAC_HandleTypeDef hdac;

void DAC_Init(void) {
    DAC_ChannelConfTypeDef sConfig = {0};
    
    // DAC初始化
    hdac.Instance = DAC;
    HAL_DAC_Init(&hdac);
    
    // 配置通道1
    sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
    sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
    HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1);
    
    // 启动DAC
    HAL_DAC_Start(&hdac, DAC_CHANNEL_1);
}

void DAC_SetVoltage(float voltage) {
    // 计算DAC值: voltage / 3.3V * 4095
    uint16_t dac_value = (uint16_t)(voltage / 3.3f * 4095.0f);
    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, dac_value);
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    DAC_Init();
    
    // 输出1.65V
    DAC_SetVoltage(1.65f);
    
    while (1) {
        // 主循环
    }
}
```

---

### 示例2: 生成正弦波

```c
#define SINE_SAMPLES 100
uint16_t sine_wave[SINE_SAMPLES];

void Generate_Sine_Wave(void) {
    for (int i = 0; i < SINE_SAMPLES; i++) {
        float angle = 2.0f * 3.14159f * i / SINE_SAMPLES;
        sine_wave[i] = (uint16_t)(2047.5f + 2047.5f * sinf(angle));
    }
}

void DAC_Sine_Init(void) {
    DAC_ChannelConfTypeDef sConfig = {0};
    
    hdac.Instance = DAC;
    HAL_DAC_Init(&hdac);
    
    // 定时器触发
    sConfig.DAC_Trigger = DAC_TRIGGER_T2_TRGO;
    sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
    HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1);
    
    // 生成正弦波数据
    Generate_Sine_Wave();
    
    // 启动DMA输出
    HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, 
                      (uint32_t*)sine_wave, SINE_SAMPLES,
                      DAC_ALIGN_12B_R);
}
```

---

### 示例3: 双通道输出

```c
void DAC_Dual_Init(void) {
    DAC_ChannelConfTypeDef sConfig = {0};
    
    hdac.Instance = DAC;
    HAL_DAC_Init(&hdac);
    
    // 配置通道1
    sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
    sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
    HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1);
    HAL_DAC_Start(&hdac, DAC_CHANNEL_1);
    
    // 配置通道2
    HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_2);
    HAL_DAC_Start(&hdac, DAC_CHANNEL_2);
    
    // 通道1输出1.0V
    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 1241);
    
    // 通道2输出2.0V
    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_2, DAC_ALIGN_12B_R, 2482);
}
```

---

## ⚠️ 注意事项

### 1. GPIO配置

DAC输出引脚必须配置为模拟模式:
- DAC_OUT1: PA4
- DAC_OUT2: PA5

```c
GPIO_InitTypeDef GPIO_InitStruct = {0};
GPIO_InitStruct.Pin = GPIO_PIN_4;
GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
```

### 2. 输出电压计算

```
输出电压 = (DAC值 / 4095) × VDDA
```

### 3. 输出缓冲

建议使能输出缓冲以提高驱动能力。

### 4. 触发源

可由定时器、外部触发或软件触发。

---

## 📖 相关文档

- [ADC模数转换](HAL_ADC.md)
- [TIM定时器](HAL_TIM.md)
- [DMA直接内存访问](HAL_DMA.md)
