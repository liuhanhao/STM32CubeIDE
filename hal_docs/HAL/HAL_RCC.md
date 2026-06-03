# RCC时钟控制模块 API文档

## 📋 模块概述

RCC(Reset and Clock Control)复位和时钟控制模块负责管理STM32F1的时钟系统,包括:
- 系统时钟配置(SYSCLK)
- 外设时钟使能/禁用
- PLL锁相环配置
- 振荡器管理(HSE/HSI/LSE/LSI)
- 时钟分频器配置
- 时钟安全系统(CSS)
- MCO时钟输出

---

## 🗺️ 使用场景速查

**时钟源怎么选**

| 时钟源 | 精度 | 硬件要求 | 适用场景 |
|--------|------|----------|----------|
| HSE（外部晶振，8MHz） | 高（<±50ppm） | 需外接晶振（OSC_IN/OUT） | 正式产品，USB / CAN / 精确波特率 |
| HSI（内部 RC，8MHz） | 低（±1%） | 无需外部器件 | 原型开发，不需要 USB，成本敏感 |
| PLL（HSE 或 HSI 倍频） | 取决于来源 | 无额外器件 | 需要高主频（72MHz）时必用 |
| LSE（32.768kHz 晶振） | 高 | 需外接晶振（PC14/PC15） | RTC 实时时钟 |
| LSI（内部 RC，~40kHz） | 低（±5%） | 无需外部器件 | IWDG，低精度 RTC |

**常见场景配置参考**

| 场景 | 推荐配置 |
|------|----------|
| 默认全速运行 | HSE(8MHz) × 9 → 72MHz SYSCLK |
| 没有外部晶振 | HSI/2 × 16 → 64MHz SYSCLK |
| 需要 USB 功能 | 必须用 HSE，72MHz 经 1.5 分频得 48MHz USB 时钟 |
| CAN 精确波特率 | 用 HSE，APB1=36MHz，保证整除分频 |
| 低功耗模式 | 降低 SYSCLK，关闭不用外设的时钟 |

**外设挂在哪条总线（影响时钟频率和性能）**

| 总线 | 最高频率 | 挂载的主要外设 |
|------|----------|---------------|
| AHB | 72MHz | DMA、SRAM、Flash、CRC |
| APB2 | 72MHz | GPIOA~G、USART1、SPI1、ADC1/2/3、TIM1/8 |
| APB1 | 36MHz | USART2/3、SPI2、I2C1/2、CAN、TIM2~7、DAC、USB |

> ADC 时钟来自 APB2，上限 14MHz，需额外在 RCC 中配置 ADC 预分频（通常 6 分频：72MHz/6=12MHz）。

**外设时钟使能顺序提示**

使用任何外设前必须先使能对应时钟，否则读写寄存器无效或硬件故障：

```c
__HAL_RCC_GPIOC_CLK_ENABLE();   // 先使能
HAL_GPIO_WritePin(GPIOC, ...);  // 再操作
```

---

## 📚 相关文件

- **头文件**: `stm32f1xx_hal_rcc.h`, `stm32f1xx_hal_rcc_ex.h`
- **源文件**: `stm32f1xx_hal_rcc.c`, `stm32f1xx_hal_rcc_ex.c`

---

## 🔧 数据类型

### RCC_OscInitTypeDef - 振荡器初始化结构体

```c
typedef struct {
    uint32_t OscillatorType;  // 振荡器类型
    uint32_t HSEState;        // HSE状态
    uint32_t HSEPredivValue;  // HSE预分频值
    uint32_t LSEState;        // LSE状态
    uint32_t HSIState;        // HSI状态
    uint32_t HSICalibrationValue;  // HSI校准值
    uint32_t LSIState;        // LSI状态
    RCC_PLLInitTypeDef PLL;   // PLL配置
} RCC_OscInitTypeDef;
```

---

### RCC_PLLInitTypeDef - PLL初始化结构体

```c
typedef struct {
    uint32_t PLLState;   // PLL状态
    uint32_t PLLSource;  // PLL时钟源
    uint32_t PLLMUL;     // PLL倍频系数
} RCC_PLLInitTypeDef;
```

**PLL倍频系数:**
- `RCC_PLL_MUL2` ~ `RCC_PLL_MUL16`: 2倍~16倍频

---

### RCC_ClkInitTypeDef - 时钟初始化结构体

```c
typedef struct {
    uint32_t ClockType;      // 要配置的时钟类型
    uint32_t SYSCLKSource;   // 系统时钟源
    uint32_t AHBCLKDivider;  // AHB分频系数
    uint32_t APB1CLKDivider; // APB1分频系数
    uint32_t APB2CLKDivider; // APB2分频系数
} RCC_ClkInitTypeDef;
```

---

## 📌 常量定义

### 振荡器类型

| 常量 | 说明 | 频率 |
|------|------|------|
| `RCC_OSCILLATORTYPE_HSE` | 外部高速振荡器 | 4-16MHz(典型8MHz) |
| `RCC_OSCILLATORTYPE_HSI` | 内部高速振荡器 | 8MHz |
| `RCC_OSCILLATORTYPE_LSE` | 外部低速振荡器 | 32.768KHz |
| `RCC_OSCILLATORTYPE_LSI` | 内部低速振荡器 | ~40KHz |

---

### HSE配置

| 常量 | 说明 |
|------|------|
| `RCC_HSE_OFF` | 关闭HSE |
| `RCC_HSE_ON` | 使能HSE晶振 |
| `RCC_HSE_BYPASS` | HSE旁路模式(外部时钟源) |

---

### 系统时钟源

| 常量 | 说明 |
|------|------|
| `RCC_SYSCLKSOURCE_HSI` | HSI作为系统时钟 |
| `RCC_SYSCLKSOURCE_HSE` | HSE作为系统时钟 |
| `RCC_SYSCLKSOURCE_PLLCLK` | PLL作为系统时钟 |

---

### AHB分频系数

| 常量 | 分频系数 |
|------|----------|
| `RCC_SYSCLK_DIV1` | 不分频 |
| `RCC_SYSCLK_DIV2` | 2分频 |
| `RCC_SYSCLK_DIV4` | 4分频 |
| `RCC_SYSCLK_DIV8` | 8分频 |
| `RCC_SYSCLK_DIV16` | 16分频 |
| `RCC_SYSCLK_DIV64` | 64分频 |
| `RCC_SYSCLK_DIV128` | 128分频 |
| `RCC_SYSCLK_DIV256` | 256分频 |
| `RCC_SYSCLK_DIV512` | 512分频 |

---

### APB1/APB2分频系数

| 常量 | 分频系数 |
|------|----------|
| `RCC_HCLK_DIV1` | 不分频 |
| `RCC_HCLK_DIV2` | 2分频 |
| `RCC_HCLK_DIV4` | 4分频 |
| `RCC_HCLK_DIV8` | 8分频 |
| `RCC_HCLK_DIV16` | 16分频 |

⚠️ **注意**: APB1最大频率36MHz, APB2最大频率72MHz

---

## 🎯 核心函数

### HAL_RCC_OscConfig()

配置振荡器。

```c
HAL_StatusTypeDef HAL_RCC_OscConfig(RCC_OscInitTypeDef *RCC_OscInitStruct);
```

**参数:**
- `RCC_OscInitStruct`: 振荡器配置结构体指针

**返回值:**
- `HAL_OK`: 配置成功
- `HAL_ERROR`: 配置失败
- `HAL_TIMEOUT`: 超时

---

### HAL_RCC_ClockConfig()

配置系统时钟。

```c
HAL_StatusTypeDef HAL_RCC_ClockConfig(RCC_ClkInitTypeDef *RCC_ClkInitStruct, 
                                      uint32_t FLatency);
```

**参数:**
- `RCC_ClkInitStruct`: 时钟配置结构体指针
- `FLatency`: Flash延迟周期
  - `FLASH_LATENCY_0`: 0等待周期 (SYSCLK ≤ 24MHz)
  - `FLASH_LATENCY_1`: 1等待周期 (24MHz < SYSCLK ≤ 48MHz)
  - `FLASH_LATENCY_2`: 2等待周期 (48MHz < SYSCLK ≤ 72MHz)

---

### HAL_RCC_GetSysClockFreq()

获取系统时钟频率。

```c
uint32_t HAL_RCC_GetSysClockFreq(void);
```

**返回值:**
- 系统时钟频率(Hz)

---

### HAL_RCC_GetHCLKFreq()

获取HCLK频率。

```c
uint32_t HAL_RCC_GetHCLKFreq(void);
```

---

### HAL_RCC_GetPCLK1Freq()

获取PCLK1频率。

```c
uint32_t HAL_RCC_GetPCLK1Freq(void);
```

---

### HAL_RCC_GetPCLK2Freq()

获取PCLK2频率。

```c
uint32_t HAL_RCC_GetPCLK2Freq(void);
```

---

## 🔌 外设时钟使能宏

### AHB总线外设

```c
__HAL_RCC_DMA1_CLK_ENABLE();      // DMA1时钟使能
__HAL_RCC_DMA2_CLK_ENABLE();      // DMA2时钟使能
__HAL_RCC_SRAM_CLK_ENABLE();      // SRAM时钟使能
__HAL_RCC_FLITF_CLK_ENABLE();     // FLITF时钟使能
__HAL_RCC_CRC_CLK_ENABLE();       // CRC时钟使能
__HAL_RCC_FSMC_CLK_ENABLE();      // FSMC时钟使能
__HAL_RCC_SDIO_CLK_ENABLE();      // SDIO时钟使能
```

---

### APB1总线外设

```c
__HAL_RCC_TIM2_CLK_ENABLE();      // TIM2时钟使能
__HAL_RCC_TIM3_CLK_ENABLE();      // TIM3时钟使能
__HAL_RCC_TIM4_CLK_ENABLE();      // TIM4时钟使能
__HAL_RCC_TIM5_CLK_ENABLE();      // TIM5时钟使能
__HAL_RCC_TIM6_CLK_ENABLE();      // TIM6时钟使能
__HAL_RCC_TIM7_CLK_ENABLE();      // TIM7时钟使能
__HAL_RCC_WWDG_CLK_ENABLE();      // WWDG时钟使能
__HAL_RCC_SPI2_CLK_ENABLE();      // SPI2时钟使能
__HAL_RCC_SPI3_CLK_ENABLE();      // SPI3时钟使能
__HAL_RCC_USART2_CLK_ENABLE();    // USART2时钟使能
__HAL_RCC_USART3_CLK_ENABLE();    // USART3时钟使能
__HAL_RCC_UART4_CLK_ENABLE();     // UART4时钟使能
__HAL_RCC_UART5_CLK_ENABLE();     // UART5时钟使能
__HAL_RCC_I2C1_CLK_ENABLE();      // I2C1时钟使能
__HAL_RCC_I2C2_CLK_ENABLE();      // I2C2时钟使能
__HAL_RCC_USB_CLK_ENABLE();       // USB时钟使能
__HAL_RCC_CAN1_CLK_ENABLE();      // CAN1时钟使能
__HAL_RCC_BKP_CLK_ENABLE();       // BKP时钟使能
__HAL_RCC_PWR_CLK_ENABLE();       // PWR时钟使能
__HAL_RCC_DAC_CLK_ENABLE();       // DAC时钟使能
```

---

### APB2总线外设

```c
__HAL_RCC_AFIO_CLK_ENABLE();      // AFIO时钟使能
__HAL_RCC_GPIOA_CLK_ENABLE();     // GPIOA时钟使能
__HAL_RCC_GPIOB_CLK_ENABLE();     // GPIOB时钟使能
__HAL_RCC_GPIOC_CLK_ENABLE();     // GPIOC时钟使能
__HAL_RCC_GPIOD_CLK_ENABLE();     // GPIOD时钟使能
__HAL_RCC_GPIOE_CLK_ENABLE();     // GPIOE时钟使能
__HAL_RCC_GPIOF_CLK_ENABLE();     // GPIOF时钟使能
__HAL_RCC_GPIOG_CLK_ENABLE();     // GPIOG时钟使能
__HAL_RCC_ADC1_CLK_ENABLE();      // ADC1时钟使能
__HAL_RCC_ADC2_CLK_ENABLE();      // ADC2时钟使能
__HAL_RCC_TIM1_CLK_ENABLE();      // TIM1时钟使能
__HAL_RCC_SPI1_CLK_ENABLE();      // SPI1时钟使能
__HAL_RCC_TIM8_CLK_ENABLE();      // TIM8时钟使能
__HAL_RCC_USART1_CLK_ENABLE();    // USART1时钟使能
__HAL_RCC_ADC3_CLK_ENABLE();      // ADC3时钟使能
```

---

## 💡 完整应用示例

### 示例1: 使用外部8MHz晶振配置72MHz系统时钟

```c
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    
    // 配置振荡器
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;  // 8MHz * 9 = 72MHz
    
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }
    
    // 配置系统时钟
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | 
                                  RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | 
                                  RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;   // 72MHz
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;    // 36MHz
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;    // 72MHz
    
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}
```

---

### 示例2: 使用内部HSI配置64MHz系统时钟

```c
void SystemClock_Config_HSI(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    
    // 配置HSI和PLL
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;  // 4MHz * 16 = 64MHz
    
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }
    
    // 配置系统时钟
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | 
                                  RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | 
                                  RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;   // 64MHz
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;    // 32MHz
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;    // 64MHz
    
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}
```

---

### 示例3: 查询时钟频率

```c
void Print_Clock_Frequencies(void) {
    uint32_t sysclk = HAL_RCC_GetSysClockFreq();
    uint32_t hclk = HAL_RCC_GetHCLKFreq();
    uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
    uint32_t pclk2 = HAL_RCC_GetPCLK2Freq();
    
    printf("SYSCLK: %lu Hz\n", sysclk);
    printf("HCLK:   %lu Hz\n", hclk);
    printf("PCLK1:  %lu Hz\n", pclk1);
    printf("PCLK2:  %lu Hz\n", pclk2);
}
```

---

## 📊 STM32F1时钟树

```
HSE (8MHz)  ──┐
              ├─→ PLL (×9) ──→ SYSCLK (72MHz)
HSI (8MHz)  ──┘                    │
                                   ├─→ AHB (÷1) ──→ HCLK (72MHz)
                                   │                    │
                                   │                    ├─→ APB1 (÷2) ──→ PCLK1 (36MHz)
                                   │                    │                      │
                                   │                    │                      └─→ TIM2-7 (×2=72MHz)
                                   │                    │
                                   │                    └─→ APB2 (÷1) ──→ PCLK2 (72MHz)
                                   │                                          │
                                   │                                          └─→ TIM1,8 (72MHz)
                                   │
LSE (32.768KHz) ──→ RTC
LSI (~40KHz)    ──→ IWDG
```

---

## ⚠️ 注意事项

### 1. 时钟频率限制

| 时钟 | 最大频率 |
|------|----------|
| SYSCLK | 72MHz |
| HCLK | 72MHz |
| PCLK1 | 36MHz |
| PCLK2 | 72MHz |
| ADC | 14MHz |

---

### 2. Flash等待周期

根据系统时钟频率设置Flash等待周期:

| SYSCLK频率 | Flash等待周期 |
|------------|---------------|
| 0-24MHz | `FLASH_LATENCY_0` |
| 24-48MHz | `FLASH_LATENCY_1` |
| 48-72MHz | `FLASH_LATENCY_2` |

---

### 3. 外设时钟使能顺序

使用外设前必须先使能时钟:

```c
// 错误示例
HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);  // 时钟未使能!

// 正确示例
__HAL_RCC_GPIOC_CLK_ENABLE();  // 先使能时钟
HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
```

---

### 4. PLL配置注意事项

- PLL输入频率范围: 1-25MHz
- PLL输出频率范围: 16-72MHz
- 修改PLL配置前必须先禁用PLL
- PLL作为系统时钟时不能禁用

---

### 5. HSE启动时间

HSE晶振启动需要时间(通常几ms),配置时需要设置超时:

```c
// HAL库会自动等待HSE就绪,超时返回HAL_TIMEOUT
if (HAL_RCC_OscConfig(&RCC_OscInitStruct) == HAL_TIMEOUT) {
    // HSE启动失败,可以切换到HSI
}
```

---

### 6. 时钟安全系统(CSS)

使能CSS可以在HSE故障时自动切换到HSI:

```c
HAL_RCC_EnableCSS();  // 使能时钟安全系统
```

---

## 📖 相关文档

- [HAL核心](HAL_Core.md)
- [GPIO通用IO](HAL_GPIO.md)
- [PWR电源](HAL_PWR.md)
- [FLASH闪存](HAL_FLASH.md)
