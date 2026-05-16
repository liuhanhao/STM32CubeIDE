# TIM定时器模块 API文档

## 📋 模块概述

TIM(Timer)定时器模块提供多种定时和计数功能,包括:
- 基本定时功能
- PWM输出
- 输入捕获
- 输出比较
- 编码器接口
- 单脉冲模式
- 主从同步

STM32F1包含多种定时器:
- **高级定时器**: TIM1, TIM8 (带死区控制和刹车功能)
- **通用定时器**: TIM2, TIM3, TIM4, TIM5 (4通道)
- **基本定时器**: TIM6, TIM7 (仅定时功能)

---

## 📚 相关文件

- **头文件**: `stm32f1xx_hal_tim.h`, `stm32f1xx_hal_tim_ex.h`
- **源文件**: `stm32f1xx_hal_tim.c`, `stm32f1xx_hal_tim_ex.c`

---

## 🔧 数据类型

### TIM_Base_InitTypeDef - 定时器基础配置

```c
typedef struct {
    uint32_t Prescaler;         // 预分频器 (0-65535)
    uint32_t CounterMode;       // 计数模式
    uint32_t Period;            // 自动重装载值 (0-65535)
    uint32_t ClockDivision;     // 时钟分频
    uint32_t RepetitionCounter; // 重复计数器(仅高级定时器)
} TIM_Base_InitTypeDef;
```

---

### TIM_OC_InitTypeDef - 输出比较配置

```c
typedef struct {
    uint32_t OCMode;        // 输出比较模式
    uint32_t Pulse;         // 脉冲宽度(CCR值)
    uint32_t OCPolarity;    // 输出极性
    uint32_t OCFastMode;    // 快速模式
} TIM_OC_InitTypeDef;
```

---

### TIM_IC_InitTypeDef - 输入捕获配置

```c
typedef struct {
    uint32_t ICPolarity;   // 输入极性
    uint32_t ICSelection;  // 输入选择
    uint32_t ICPrescaler;  // 输入预分频
    uint32_t ICFilter;     // 输入滤波器
} TIM_IC_InitTypeDef;
```

---

## 📌 常量定义

### 计数模式

| 常量 | 说明 |
|------|------|
| `TIM_COUNTERMODE_UP` | 向上计数 |
| `TIM_COUNTERMODE_DOWN` | 向下计数 |
| `TIM_COUNTERMODE_CENTERALIGNED1` | 中心对齐模式1 |
| `TIM_COUNTERMODE_CENTERALIGNED2` | 中心对齐模式2 |
| `TIM_COUNTERMODE_CENTERALIGNED3` | 中心对齐模式3 |

---

### PWM模式

| 常量 | 说明 |
|------|------|
| `TIM_OCMODE_PWM1` | PWM模式1 |
| `TIM_OCMODE_PWM2` | PWM模式2 |

---

### 定时器通道

```c
TIM_CHANNEL_1  // 通道1
TIM_CHANNEL_2  // 通道2
TIM_CHANNEL_3  // 通道3
TIM_CHANNEL_4  // 通道4
TIM_CHANNEL_ALL // 所有通道
```

---

## 🎯 核心函数

### 基础定时器

#### HAL_TIM_Base_Init()

初始化定时器基础功能。

```c
HAL_StatusTypeDef HAL_TIM_Base_Init(TIM_HandleTypeDef *htim);
```

---

#### HAL_TIM_Base_Start()

启动定时器(阻塞模式)。

```c
HAL_StatusTypeDef HAL_TIM_Base_Start(TIM_HandleTypeDef *htim);
```

---

#### HAL_TIM_Base_Start_IT()

启动定时器(中断模式)。

```c
HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *htim);
```

---

### PWM输出

#### HAL_TIM_PWM_Init()

初始化PWM功能。

```c
HAL_StatusTypeDef HAL_TIM_PWM_Init(TIM_HandleTypeDef *htim);
```

---

#### HAL_TIM_PWM_ConfigChannel()

配置PWM通道。

```c
HAL_StatusTypeDef HAL_TIM_PWM_ConfigChannel(TIM_HandleTypeDef *htim, 
                                            TIM_OC_InitTypeDef *sConfig, 
                                            uint32_t Channel);
```

---

#### HAL_TIM_PWM_Start()

启动PWM输出。

```c
HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef *htim, uint32_t Channel);
```

---

#### __HAL_TIM_SET_COMPARE()

设置PWM占空比。

```c
#define __HAL_TIM_SET_COMPARE(htim, channel, value)
```

---

### 输入捕获

#### HAL_TIM_IC_Init()

初始化输入捕获功能。

```c
HAL_StatusTypeDef HAL_TIM_IC_Init(TIM_HandleTypeDef *htim);
```

---

#### HAL_TIM_IC_ConfigChannel()

配置输入捕获通道。

```c
HAL_StatusTypeDef HAL_TIM_IC_ConfigChannel(TIM_HandleTypeDef *htim, 
                                           TIM_IC_InitTypeDef *sConfig, 
                                           uint32_t Channel);
```

---

#### HAL_TIM_IC_Start_IT()

启动输入捕获(中断模式)。

```c
HAL_StatusTypeDef HAL_TIM_IC_Start_IT(TIM_HandleTypeDef *htim, uint32_t Channel);
```

---

#### HAL_TIM_IC_CaptureCallback()

输入捕获回调函数。

```c
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim);
```

---

## 💡 完整应用示例

### 示例1: 基础定时器中断(1ms)

```c
TIM_HandleTypeDef htim2;

void TIM2_Init(void) {
    // TIM2时钟频率 = 72MHz
    // 预分频 = 7200-1, 计数频率 = 72MHz/7200 = 10KHz
    // 周期 = 10-1, 中断频率 = 10KHz/10 = 1KHz (1ms)
    
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 7200 - 1;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 10 - 1;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_Base_Init(&htim2);
    
    HAL_TIM_Base_Start_IT(&htim2);
}

// 中断服务函数
void TIM2_IRQHandler(void) {
    HAL_TIM_IRQHandler(&htim2);
}

// 定时器回调函数(每1ms调用一次)
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2) {
        // 1ms定时任务
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    }
}
```

---

### 示例2: PWM输出(舵机控制)

```c
TIM_HandleTypeDef htim3;

void PWM_Init(void) {
    TIM_OC_InitTypeDef sConfigOC = {0};
    
    // TIM3时钟 = 72MHz
    // 预分频 = 72-1, 计数频率 = 1MHz
    // 周期 = 20000-1, PWM频率 = 1MHz/20000 = 50Hz (20ms)
    
    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 72 - 1;
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 20000 - 1;
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_PWM_Init(&htim3);
    
    // 配置PWM通道1
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 1500;  // 1.5ms脉宽(舵机中位)
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1);
    
    // 启动PWM
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
}

// 设置舵机角度(0-180度)
void Servo_SetAngle(uint8_t angle) {
    // 0度 = 0.5ms, 90度 = 1.5ms, 180度 = 2.5ms
    uint16_t pulse = 500 + (angle * 2000 / 180);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pulse);
}
```

---

### 示例3: PWM输出(LED呼吸灯)

```c
void LED_Breathing(void) {
    TIM_HandleTypeDef htim3;
    TIM_OC_InitTypeDef sConfigOC = {0};
    
    // PWM频率 = 1KHz
    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 72 - 1;
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 1000 - 1;
    HAL_TIM_PWM_Init(&htim3);
    
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    
    // 呼吸灯效果
    while (1) {
        // 渐亮
        for (uint16_t i = 0; i < 1000; i++) {
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, i);
            HAL_Delay(1);
        }
        // 渐暗
        for (uint16_t i = 1000; i > 0; i--) {
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, i);
            HAL_Delay(1);
        }
    }
}
```

---

### 示例4: 输入捕获(测量频率)

```c
TIM_HandleTypeDef htim2;
uint32_t capture_value1 = 0;
uint32_t capture_value2 = 0;
uint8_t capture_flag = 0;

void IC_Init(void) {
    TIM_IC_InitTypeDef sConfigIC = {0};
    
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 72 - 1;  // 1MHz计数频率
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 0xFFFF;
    HAL_TIM_IC_Init(&htim2);
    
    // 配置输入捕获通道1
    sConfigIC.ICPolarity = TIM_ICPOLARITY_RISING;  // 上升沿捕获
    sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
    sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
    sConfigIC.ICFilter = 0;
    HAL_TIM_IC_ConfigChannel(&htim2, &sConfigIC, TIM_CHANNEL_1);
    
    HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);
}

// 输入捕获回调
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2) {
        if (capture_flag == 0) {
            capture_value1 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
            capture_flag = 1;
        } else {
            capture_value2 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
            
            // 计算频率
            uint32_t diff = capture_value2 - capture_value1;
            float frequency = 1000000.0f / diff;  // Hz
            printf("频率: %.2f Hz\n", frequency);
            
            capture_flag = 0;
        }
    }
}
```

---

## ⚠️ 注意事项

### 1. 定时器时钟

- APB1定时器(TIM2-7): 时钟频率 = APB1×2 (APB1分频时)
- APB2定时器(TIM1,8): 时钟频率 = APB2×2 (APB2分频时)

典型配置(72MHz系统时钟):
- APB1 = 36MHz → TIM2-7时钟 = 72MHz
- APB2 = 72MHz → TIM1,8时钟 = 72MHz

---

### 2. 定时器周期计算

```
定时周期 = (Prescaler + 1) × (Period + 1) / 定时器时钟频率
```

例如: 1ms定时
```
72MHz / (7200 × 10) = 1KHz = 1ms
```

---

### 3. PWM频率和分辨率

PWM分辨率 = Period + 1

例如:
- Period = 999 → 1000级分辨率
- Period = 99 → 100级分辨率

PWM频率越高,分辨率越低。

---

### 4. GPIO配置

定时器通道对应的GPIO必须配置为复用功能:

```c
GPIO_InitTypeDef GPIO_InitStruct = {0};
GPIO_InitStruct.Pin = GPIO_PIN_6;  // TIM3_CH1
GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
```

---

### 5. 中断优先级

配置定时器中断优先级:

```c
HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
HAL_NVIC_EnableIRQ(TIM2_IRQn);
```

---

## 📖 相关文档

- [HAL核心](HAL_Core.md)
- [GPIO通用IO](HAL_GPIO.md)
- [RCC时钟控制](HAL_RCC.md)
- [ADC模数转换](HAL_ADC.md)
