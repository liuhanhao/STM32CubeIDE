# ADC模数转换模块 API文档

## 📋 模块概述

ADC(Analog-to-Digital Converter)模数转换器模块提供模拟信号到数字信号的转换功能,包括:
- 12位分辨率
- 多通道转换(最多16个外部通道)
- 单次/连续/扫描转换模式
- 注入通道转换
- 模拟看门狗功能
- DMA传输支持
- 双ADC模式(ADC1+ADC2)

---

## 📚 相关文件

- **头文件**: `stm32f1xx_hal_adc.h`, `stm32f1xx_hal_adc_ex.h`
- **源文件**: `stm32f1xx_hal_adc.c`, `stm32f1xx_hal_adc_ex.c`

---

## 🔧 数据类型

### ADC_InitTypeDef - ADC初始化结构体

```c
typedef struct {
    uint32_t DataAlign;              // 数据对齐方式
    uint32_t ScanConvMode;           // 扫描模式
    uint32_t ContinuousConvMode;     // 连续转换模式
    uint32_t NbrOfConversion;        // 规则通道数量
    uint32_t DiscontinuousConvMode;  // 间断转换模式
    uint32_t NbrOfDiscConversion;    // 间断转换通道数
    uint32_t ExternalTrigConv;       // 外部触发源
} ADC_InitTypeDef;
```

---

### ADC_ChannelConfTypeDef - ADC通道配置结构体

```c
typedef struct {
    uint32_t Channel;      // ADC通道
    uint32_t Rank;         // 转换顺序
    uint32_t SamplingTime; // 采样时间
} ADC_ChannelConfTypeDef;
```

---

## 📌 常量定义

### ADC通道

```c
ADC_CHANNEL_0  ~ ADC_CHANNEL_17  // 外部通道0-17
ADC_CHANNEL_TEMPSENSOR           // 内部温度传感器
ADC_CHANNEL_VREFINT              // 内部参考电压
```

---

### 采样时间

| 常量 | 采样周期 |
|------|----------|
| `ADC_SAMPLETIME_1CYCLE_5` | 1.5个ADC时钟周期 |
| `ADC_SAMPLETIME_7CYCLES_5` | 7.5个ADC时钟周期 |
| `ADC_SAMPLETIME_13CYCLES_5` | 13.5个ADC时钟周期 |
| `ADC_SAMPLETIME_28CYCLES_5` | 28.5个ADC时钟周期 |
| `ADC_SAMPLETIME_41CYCLES_5` | 41.5个ADC时钟周期 |
| `ADC_SAMPLETIME_55CYCLES_5` | 55.5个ADC时钟周期 |
| `ADC_SAMPLETIME_71CYCLES_5` | 71.5个ADC时钟周期 |
| `ADC_SAMPLETIME_239CYCLES_5` | 239.5个ADC时钟周期 |

---

## 🎯 核心函数

### HAL_ADC_Init()

初始化ADC。

```c
HAL_StatusTypeDef HAL_ADC_Init(ADC_HandleTypeDef *hadc);
```

---

### HAL_ADC_ConfigChannel()

配置ADC通道。

```c
HAL_StatusTypeDef HAL_ADC_ConfigChannel(ADC_HandleTypeDef *hadc, 
                                        ADC_ChannelConfTypeDef *sConfig);
```

---

### HAL_ADC_Start()

启动ADC转换(阻塞模式)。

```c
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *hadc);
```

---

### HAL_ADC_PollForConversion()

等待转换完成。

```c
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *hadc, 
                                            uint32_t Timeout);
```

---

### HAL_ADC_GetValue()

获取转换结果。

```c
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *hadc);
```

---

### HAL_ADC_Start_IT()

启动ADC转换(中断模式)。

```c
HAL_StatusTypeDef HAL_ADC_Start_IT(ADC_HandleTypeDef *hadc);
```

---

### HAL_ADC_Start_DMA()

启动ADC转换(DMA模式)。

```c
HAL_StatusTypeDef HAL_ADC_Start_DMA(ADC_HandleTypeDef *hadc, 
                                    uint32_t *pData, 
                                    uint32_t Length);
```

---

## 💡 完整应用示例

### 示例1: 单通道单次转换

```c
ADC_HandleTypeDef hadc1;

void ADC_Init(void) {
    ADC_ChannelConfTypeDef sConfig = {0};
    
    // ADC初始化
    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    HAL_ADC_Init(&hadc1);
    
    // 配置通道
    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    
    // 校准ADC
    HAL_ADCEx_Calibration_Start(&hadc1);
}

uint16_t ADC_Read(void) {
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 100);
    return HAL_ADC_GetValue(&hadc1);
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    ADC_Init();
    
    while (1) {
        uint16_t adc_value = ADC_Read();
        float voltage = adc_value * 3.3f / 4096.0f;
        printf("ADC: %d, 电压: %.2fV\n", adc_value, voltage);
        HAL_Delay(1000);
    }
}
```

---

### 示例2: 多通道DMA连续转换

```c
#define ADC_CHANNELS 4
uint16_t adc_buffer[ADC_CHANNELS];

void ADC_Multi_Init(void) {
    ADC_ChannelConfTypeDef sConfig = {0};
    
    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;  // 扫描模式
    hadc1.Init.ContinuousConvMode = ENABLE;     // 连续转换
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = ADC_CHANNELS;
    HAL_ADC_Init(&hadc1);
    
    // 配置通道0
    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    
    // 配置通道1
    sConfig.Channel = ADC_CHANNEL_1;
    sConfig.Rank = ADC_REGULAR_RANK_2;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    
    // 配置通道2
    sConfig.Channel = ADC_CHANNEL_2;
    sConfig.Rank = ADC_REGULAR_RANK_3;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    
    // 配置通道3
    sConfig.Channel = ADC_CHANNEL_3;
    sConfig.Rank = ADC_REGULAR_RANK_4;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    
    // 校准并启动DMA转换
    HAL_ADCEx_Calibration_Start(&hadc1);
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, ADC_CHANNELS);
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    ADC_Multi_Init();
    
    while (1) {
        printf("CH0: %d, CH1: %d, CH2: %d, CH3: %d\n",
               adc_buffer[0], adc_buffer[1], 
               adc_buffer[2], adc_buffer[3]);
        HAL_Delay(1000);
    }
}
```

---

### 示例3: 读取内部温度传感器

```c
float Read_Temperature(void) {
    ADC_ChannelConfTypeDef sConfig = {0};
    
    // 配置温度传感器通道
    sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;  // 温度传感器需要长采样时间
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    
    // 读取ADC值
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 100);
    uint16_t adc_value = HAL_ADC_GetValue(&hadc1);
    
    // 转换为温度(℃)
    // V25 = 1.43V, Avg_Slope = 4.3mV/℃
    float voltage = adc_value * 3.3f / 4096.0f;
    float temperature = (1.43f - voltage) / 0.0043f + 25.0f;
    
    return temperature;
}
```

---

## ⚠️ 注意事项

### 1. ADC时钟配置

ADC时钟由PCLK2分频得到,最大14MHz:

```c
// 在RCC配置中设置ADC预分频
// PCLK2=72MHz时,使用6分频得到12MHz
RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);
```

---

### 2. ADC校准

使用ADC前必须进行校准:

```c
HAL_ADCEx_Calibration_Start(&hadc1);
```

---

### 3. GPIO配置

ADC输入引脚必须配置为模拟模式:

```c
GPIO_InitTypeDef GPIO_InitStruct = {0};
GPIO_InitStruct.Pin = GPIO_PIN_0;
GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
```

---

### 4. 采样时间选择

采样时间 = (采样周期 + 12.5) / ADC时钟频率

选择足够长的采样时间以确保转换精度。

---

### 5. 参考电压

ADC参考电压为VDDA(通常连接到VDD=3.3V):
- 转换结果 = (输入电压 / VDDA) × 4096

---

## 📖 相关文档

- [HAL核心](HAL_Core.md)
- [GPIO通用IO](HAL_GPIO.md)
- [DMA直接内存访问](HAL_DMA.md)
- [TIM定时器](HAL_TIM.md)
