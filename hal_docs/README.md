# STM32F1xx HAL库 API文档

本文档基于STM32F1xx HAL库头文件生成,提供完整的中文API参考手册。

---

## � 目录

- [文档结构](#文档结构)
- [快速开始](#快速开始)
- [快速参考](#快速参考)
- [其他模块简介](#其他模块简介)
- [使用说明](#使用说明)
- [版本信息](#版本信息)

---

## 📚 文档结构

### 🔧 核心模块
- [HAL_Core.md](HAL_Core.md) - HAL库核心功能和通用定义
- [HAL_RCC.md](HAL_RCC.md) - 复位和时钟控制
- [HAL_GPIO.md](HAL_GPIO.md) - 通用输入输出端口
- [HAL_CORTEX.md](HAL_CORTEX.md) - Cortex-M3内核功能
- [HAL_DMA.md](HAL_DMA.md) - 直接内存访问控制器
- [HAL_EXTI.md](HAL_EXTI.md) - 外部中断/事件控制器

### 📡 模拟外设
- [HAL_ADC.md](HAL_ADC.md) - 模数转换器
- [HAL_DAC.md](HAL_DAC.md) - 数模转换器

### 🔌 通信外设
- [HAL_UART.md](HAL_UART.md) - 通用异步收发器
- [HAL_USART.md](HAL_USART.md) - 通用同步异步收发器
- [HAL_I2C.md](HAL_I2C.md) - I2C总线接口
- [HAL_SPI.md](HAL_SPI.md) - SPI总线接口
- [HAL_CAN.md](HAL_CAN.md) - CAN总线控制器

### ⏱️ 定时器
- [HAL_TIM.md](HAL_TIM.md) - 通用/高级/基本定时器
- [HAL_RTC.md](HAL_RTC.md) - 实时时钟
- [HAL_IWDG.md](HAL_IWDG.md) - 独立看门狗
- [HAL_WWDG.md](HAL_WWDG.md) - 窗口看门狗

### 💾 存储器
- [HAL_FLASH.md](HAL_FLASH.md) - 内部Flash存储器
- [HAL_PWR.md](HAL_PWR.md) - 电源控制

### 🔧 其他外设
- [HAL_CRC.md](HAL_CRC.md) - CRC计算单元

---

## � 快速开始

### 基本初始化流程

```c
int main(void) {
    // 1. HAL库初始化
    HAL_Init();
    
    // 2. 配置系统时钟
    SystemClock_Config();
    
    // 3. 初始化外设
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    // ...
    
    // 4. 主循环
    while (1) {
        // 应用代码
    }
}
```

### 初学者推荐阅读顺序

1. **HAL_Core.md** - 了解HAL库基础
2. **HAL_RCC.md** - 学习时钟配置
3. **HAL_GPIO.md** - 掌握IO控制
4. **HAL_UART.md** - 学习串口通信
5. **HAL_TIM.md** - 学习定时器使用
6. **HAL_ADC.md** - 学习模拟信号采集

### 常用模块快速索引

| 应用场景 | 推荐文档 |
|----------|----------|
| LED控制 | HAL_GPIO.md |
| 串口调试 | HAL_UART.md |
| 传感器读取 | HAL_I2C.md, HAL_SPI.md, HAL_ADC.md |
| PWM控制 | HAL_TIM.md |
| 定时任务 | HAL_TIM.md |
| 数据存储 | HAL_FLASH.md |
| 低功耗 | HAL_PWR.md, HAL_RTC.md |

---

## 📌 快速参考

### GPIO快速参考

#### 初始化

```c
// 使能时钟
__HAL_RCC_GPIOC_CLK_ENABLE();

// 配置引脚
GPIO_InitTypeDef GPIO_InitStruct = {0};
GPIO_InitStruct.Pin = GPIO_PIN_13;
GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;  // 推挽输出
GPIO_InitStruct.Pull = GPIO_NOPULL;
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
```

#### 常用操作

```c
// 写引脚
HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);    // 高电平
HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);  // 低电平

// 翻转引脚
HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

// 读引脚
GPIO_PinState state = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
```

#### GPIO模式速查

| 模式 | 说明 | 应用 |
|------|------|------|
| `GPIO_MODE_INPUT` | 浮空输入 | 按键、传感器 |
| `GPIO_MODE_OUTPUT_PP` | 推挽输出 | LED、控制信号 |
| `GPIO_MODE_OUTPUT_OD` | 开漏输出 | I2C |
| `GPIO_MODE_AF_PP` | 复用推挽 | UART TX, SPI |
| `GPIO_MODE_AF_OD` | 复用开漏 | I2C |
| `GPIO_MODE_AF_INPUT` | 复用输入 | UART RX |
| `GPIO_MODE_ANALOG` | 模拟模式 | ADC, DAC |
| `GPIO_MODE_IT_RISING` | 上升沿中断 | 外部中断 |
| `GPIO_MODE_IT_FALLING` | 下降沿中断 | 外部中断 |

---

### UART快速参考

#### 初始化

```c
UART_HandleTypeDef huart1;

huart1.Instance = USART1;
huart1.Init.BaudRate = 115200;
huart1.Init.WordLength = UART_WORDLENGTH_8B;
huart1.Init.StopBits = UART_STOPBITS_1;
huart1.Init.Parity = UART_PARITY_NONE;
huart1.Init.Mode = UART_MODE_TX_RX;
huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
HAL_UART_Init(&huart1);
```

#### 常用操作

```c
// 发送数据
uint8_t data[] = "Hello!";
HAL_UART_Transmit(&huart1, data, sizeof(data)-1, 1000);

// 接收数据
uint8_t rx_buffer[10];
HAL_UART_Receive(&huart1, rx_buffer, 10, 1000);

// 中断接收
HAL_UART_Receive_IT(&huart1, rx_buffer, 10);

// 重定向printf
int fputc(int ch, FILE *f) {
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 100);
    return ch;
}
```

---

### TIM定时器快速参考

#### 基础定时器(1ms中断)

```c
TIM_HandleTypeDef htim2;

// 72MHz / 7200 / 10 = 1KHz (1ms)
htim2.Instance = TIM2;
htim2.Init.Prescaler = 7200 - 1;
htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
htim2.Init.Period = 10 - 1;
HAL_TIM_Base_Init(&htim2);
HAL_TIM_Base_Start_IT(&htim2);
```

#### PWM输出

```c
TIM_OC_InitTypeDef sConfigOC = {0};

// 1KHz PWM
htim3.Instance = TIM3;
htim3.Init.Prescaler = 72 - 1;
htim3.Init.Period = 1000 - 1;
HAL_TIM_PWM_Init(&htim3);

sConfigOC.OCMode = TIM_OCMODE_PWM1;
sConfigOC.Pulse = 500;  // 50%占空比
HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1);
HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);

// 修改占空比
__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 750);  // 75%
```

---

### ADC快速参考

#### 单通道转换

```c
ADC_HandleTypeDef hadc1;
ADC_ChannelConfTypeDef sConfig = {0};

hadc1.Instance = ADC1;
hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
hadc1.Init.ContinuousConvMode = DISABLE;
hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
hadc1.Init.NbrOfConversion = 1;
HAL_ADC_Init(&hadc1);

sConfig.Channel = ADC_CHANNEL_0;
sConfig.Rank = ADC_REGULAR_RANK_1;
sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
HAL_ADC_ConfigChannel(&hadc1, &sConfig);

// 校准
HAL_ADCEx_Calibration_Start(&hadc1);

// 读取
HAL_ADC_Start(&hadc1);
HAL_ADC_PollForConversion(&hadc1, 100);
uint16_t value = HAL_ADC_GetValue(&hadc1);

// 转换为电压
float voltage = value * 3.3f / 4096.0f;
```

---

### I2C快速参考

#### 初始化

```c
I2C_HandleTypeDef hi2c1;

hi2c1.Instance = I2C1;
hi2c1.Init.ClockSpeed = 100000;  // 100KHz
hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
hi2c1.Init.OwnAddress1 = 0;
hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
HAL_I2C_Init(&hi2c1);
```

#### 读写操作

```c
#define DEVICE_ADDR  0xA0

// 写入寄存器
uint8_t data = 0x55;
HAL_I2C_Mem_Write(&hi2c1, DEVICE_ADDR, 0x00, 
                  I2C_MEMADD_SIZE_8BIT, &data, 1, 100);

// 读取寄存器
uint8_t rx_data;
HAL_I2C_Mem_Read(&hi2c1, DEVICE_ADDR, 0x00, 
                 I2C_MEMADD_SIZE_8BIT, &rx_data, 1, 100);

// 检测设备
if (HAL_I2C_IsDeviceReady(&hi2c1, DEVICE_ADDR, 3, 100) == HAL_OK) {
    // 设备存在
}
```

---

### SPI快速参考

#### 初始化

```c
SPI_HandleTypeDef hspi1;

hspi1.Instance = SPI1;
hspi1.Init.Mode = SPI_MODE_MASTER;
hspi1.Init.Direction = SPI_DIRECTION_2LINES;
hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
hspi1.Init.NSS = SPI_NSS_SOFT;
hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
HAL_SPI_Init(&hspi1);
```

#### 读写操作

```c
// 片选控制
#define CS_LOW()   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET)
#define CS_HIGH()  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET)

// 发送数据
uint8_t tx_data[] = {0x01, 0x02, 0x03};
CS_LOW();
HAL_SPI_Transmit(&hspi1, tx_data, 3, 100);
CS_HIGH();

// 接收数据
uint8_t rx_data[3];
CS_LOW();
HAL_SPI_Receive(&hspi1, rx_data, 3, 100);
CS_HIGH();

// 全双工收发
CS_LOW();
HAL_SPI_TransmitReceive(&hspi1, tx_data, rx_data, 3, 100);
CS_HIGH();
```

---

### RCC时钟快速参考

#### 配置72MHz系统时钟(外部8MHz晶振)

```c
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    
    // 配置振荡器和PLL
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;  // 8MHz * 9 = 72MHz
    HAL_RCC_OscConfig(&RCC_OscInitStruct);
    
    // 配置系统时钟
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;   // 72MHz
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;    // 36MHz
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;    // 72MHz
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}
```

#### 外设时钟使能

```c
// GPIO
__HAL_RCC_GPIOA_CLK_ENABLE();
__HAL_RCC_GPIOB_CLK_ENABLE();
__HAL_RCC_GPIOC_CLK_ENABLE();

// 通信外设
__HAL_RCC_USART1_CLK_ENABLE();
__HAL_RCC_I2C1_CLK_ENABLE();
__HAL_RCC_SPI1_CLK_ENABLE();

// 定时器
__HAL_RCC_TIM2_CLK_ENABLE();
__HAL_RCC_TIM3_CLK_ENABLE();

// ADC
__HAL_RCC_ADC1_CLK_ENABLE();

// DMA
__HAL_RCC_DMA1_CLK_ENABLE();
```

---

### 常用宏定义

```c
// 延时
HAL_Delay(1000);  // 延时1000ms

// 获取时钟节拍
uint32_t tick = HAL_GetTick();  // 获取当前ms数

// 位操作
SET_BIT(reg, bit);      // 置位
CLEAR_BIT(reg, bit);    // 清零
READ_BIT(reg, bit);     // 读取
WRITE_REG(reg, val);    // 写寄存器
READ_REG(reg);          // 读寄存器
```

---

### 常见问题速查

#### Q: 如何配置串口打印?

```c
// 1. 初始化UART
// 2. 重定向fputc
int fputc(int ch, FILE *f) {
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 100);
    return ch;
}
// 3. 使用printf
printf("Hello STM32!\n");
```

#### Q: 如何生成PWM?

```c
// 1. 初始化TIM PWM模式
// 2. 配置通道
// 3. 启动PWM
HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
// 4. 修改占空比
__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, value);
```

#### Q: 如何读取ADC?

```c
// 1. 初始化ADC
// 2. 校准
HAL_ADCEx_Calibration_Start(&hadc1);
// 3. 启动转换
HAL_ADC_Start(&hadc1);
HAL_ADC_PollForConversion(&hadc1, 100);
// 4. 读取结果
uint16_t value = HAL_ADC_GetValue(&hadc1);
```

#### Q: 如何使用定时器中断?

```c
// 1. 初始化定时器
// 2. 启动中断模式
HAL_TIM_Base_Start_IT(&htim2);
// 3. 实现回调函数
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2) {
        // 定时任务
    }
}
// 4. 实现中断服务函数
void TIM2_IRQHandler(void) {
    HAL_TIM_IRQHandler(&htim2);
}
```

---

## 🔧 其他模块简介

本节简要介绍STM32F1xx HAL库中使用频率较低但在特定场景下很有用的模块。

### 📡 低频通信接口

#### I2S音频接口
- **用途**: 数字音频传输
- **特点**: 主从模式、多种音频格式、可配置采样率、支持DMA
- **应用**: 音频编解码芯片通信
- **文件**: `stm32f1xx_hal_i2s.h`

#### IRDA红外通信
- **用途**: 红外数据通信
- **特点**: 基于UART、SIR模式、波特率最高115.2Kbps
- **应用**: 红外遥控、红外数据传输
- **文件**: `stm32f1xx_hal_irda.h`

#### SMARTCARD智能卡
- **用途**: 智能卡接口
- **特点**: ISO 7816-3标准、支持T=0和T=1协议、自动重传
- **应用**: IC卡读写器
- **文件**: `stm32f1xx_hal_smartcard.h`

#### CEC消费电子控制
- **用途**: HDMI-CEC协议
- **特点**: 单线双向通信、家电互联
- **应用**: HDMI设备控制
- **文件**: `stm32f1xx_hal_cec.h`

---

### 💾 外部存储器接口

#### FSMC灵活静态存储控制器
- **用途**: 连接外部存储器
- **支持**: SRAM、NOR Flash、NAND Flash、PC Card
- **应用**: 扩展外部RAM、连接LCD、外部Flash
- **文件**: `stm32f1xx_ll_fsmc.h`

#### SRAM外部静态存储器
- **特点**: 异步访问、可配置时序、8/16位数据宽度
- **应用**: 扩展内存空间
- **文件**: `stm32f1xx_hal_sram.h`

#### NAND Flash
- **特点**: 页编程、块擦除、ECC校验支持
- **应用**: 大容量数据存储
- **文件**: `stm32f1xx_hal_nand.h`

#### NOR Flash
- **特点**: 字节编程、扇区擦除、XIP执行
- **应用**: 代码存储、数据存储
- **文件**: `stm32f1xx_hal_nor.h`

---

### 🔌 USB和存储卡

#### PCD USB设备控制器
- **特点**: USB 2.0全速(12Mbps)、8个端点
- **应用**: USB虚拟串口、USB存储设备、HID设备
- **文件**: `stm32f1xx_hal_pcd.h`

**基本使用:**
```c
PCD_HandleTypeDef hpcd_USB_FS;
hpcd_USB_FS.Instance = USB;
hpcd_USB_FS.Init.dev_endpoints = 8;
hpcd_USB_FS.Init.speed = PCD_SPEED_FULL;
HAL_PCD_Init(&hpcd_USB_FS);
```

#### HCD USB主机控制器
- **特点**: USB 2.0全速、8个通道
- **应用**: USB主机、读取U盘
- **文件**: `stm32f1xx_hal_hcd.h`

#### SD存储卡
- **特点**: SD 1.1/2.0协议、1位/4位数据线、DMA支持
- **应用**: SD卡读写、数据记录
- **文件**: `stm32f1xx_hal_sd.h`

**基本使用:**
```c
SD_HandleTypeDef hsd;
hsd.Instance = SDIO;
HAL_SD_Init(&hsd);

// 读取扇区
uint8_t buffer[512];
HAL_SD_ReadBlocks(&hsd, buffer, 0, 1, 1000);

// 写入扇区
HAL_SD_WriteBlocks(&hsd, buffer, 0, 1, 1000);
```

#### MMC存储卡
- **特点**: MMC 4.2协议、与SD卡接口类似
- **应用**: eMMC存储
- **文件**: `stm32f1xx_hal_mmc.h`

---

### 🌐 以太网

#### ETH以太网MAC (仅STM32F107)
- **特点**: 10/100Mbps、MII/RMII接口、DMA传输、硬件校验和
- **应用**: 网络通信、物联网
- **文件**: `stm32f1xx_hal_eth.h`
- **注意**: 需要外部PHY芯片(如LAN8720)

---

### ⚡ LL低级驱动层

#### LL驱动概述

LL(Low Layer)驱动提供直接寄存器操作的轻量级API:

**特点:**
- ✅ 代码体积小
- ✅ 执行效率高
- ✅ 直接操作寄存器
- ⚠️ 无状态管理

**适用场景:**
- 对代码大小有严格要求
- 需要极致性能
- 熟悉硬件寄存器

**命名规范:**
```c
LL_<外设>_Enable()      // 使能外设
LL_<外设>_Disable()     // 禁用外设
LL_<外设>_Set<参数>()   // 设置参数
LL_<外设>_Get<参数>()   // 获取参数
LL_<外设>_Is<状态>()    // 检查状态
```

**示例:**
```c
// LL层GPIO操作
LL_GPIO_SetOutputPin(GPIOC, LL_GPIO_PIN_13);
LL_GPIO_ResetOutputPin(GPIOC, LL_GPIO_PIN_13);
LL_GPIO_TogglePin(GPIOC, LL_GPIO_PIN_13);

// LL层USART操作
LL_USART_Enable(USART1);
LL_USART_TransmitData8(USART1, 'A');
while (!LL_USART_IsActiveFlag_TXE(USART1));
```

**可用的LL模块:**
- LL_GPIO, LL_USART, LL_SPI, LL_I2C
- LL_TIM, LL_ADC, LL_DMA
- LL_RCC, LL_PWR, LL_EXTI, LL_CORTEX
- 等等...

---

### 💡 HAL层 vs LL层选择建议

#### 推荐使用HAL层:
- ✅ 快速开发原型
- ✅ 复杂功能实现
- ✅ 团队协作项目
- ✅ 初学者学习
- ✅ 需要跨平台移植

#### 推荐使用LL层:
- ✅ 代码空间受限
- ✅ 性能要求极高
- ✅ 简单的寄存器操作
- ✅ 高级开发者
- ✅ 实时性要求高

#### 混合使用:
可以在同一项目中混合使用HAL和LL:
- 复杂模块用HAL(如USB、以太网)
- 简单操作用LL(如GPIO翻转)

---

## 📖 使用说明

### HAL层 vs LL层

#### HAL层 (Hardware Abstraction Layer - 硬件抽象层)
- ✅ 高级抽象API,易于使用
- ✅ 提供完整的外设初始化和控制功能
- ✅ 包含错误处理和状态管理
- ⚠️ 代码体积较大,但开发效率高
- 💡 **适用场景**: 快速开发、复杂应用、初学者

#### LL层 (Low Layer - 低级层)
- ✅ 直接操作寄存器的轻量级API
- ✅ 代码体积小,执行效率高
- ⚠️ 需要更深入了解硬件细节
- 💡 **适用场景**: 性能要求高、代码空间受限、高级开发者

---

### 🏷️ 命名规范

#### HAL层函数命名

| 函数格式 | 说明 | 示例 |
|---------|------|------|
| `HAL_<外设>_Init()` | 初始化外设 | `HAL_UART_Init()` |
| `HAL_<外设>_DeInit()` | 反初始化外设 | `HAL_UART_DeInit()` |
| `HAL_<外设>_<操作>()` | 阻塞模式操作 | `HAL_UART_Transmit()` |
| `HAL_<外设>_<操作>_IT()` | 中断模式操作 | `HAL_UART_Transmit_IT()` |
| `HAL_<外设>_<操作>_DMA()` | DMA模式操作 | `HAL_UART_Transmit_DMA()` |
| `HAL_<外设>_IRQHandler()` | 中断处理函数 | `HAL_UART_IRQHandler()` |
| `HAL_<外设>_<操作>Callback()` | 回调函数 | `HAL_UART_TxCpltCallback()` |

#### LL层函数命名

| 函数格式 | 说明 | 示例 |
|---------|------|------|
| `LL_<外设>_Enable()` | 使能外设 | `LL_USART_Enable()` |
| `LL_<外设>_Disable()` | 禁用外设 | `LL_USART_Disable()` |
| `LL_<外设>_Set<参数>()` | 设置参数 | `LL_USART_SetBaudRate()` |
| `LL_<外设>_Get<参数>()` | 获取参数 | `LL_USART_GetBaudRate()` |
| `LL_<外设>_Is<状态>()` | 检查状态 | `LL_USART_IsActiveFlag_TXE()` |

---

### 📊 返回值类型

HAL层函数通常返回 `HAL_StatusTypeDef` 枚举类型:

| 返回值 | 数值 | 说明 |
|--------|------|------|
| `HAL_OK` | 0x00 | ✅ 操作成功 |
| `HAL_ERROR` | 0x01 | ❌ 操作失败 |
| `HAL_BUSY` | 0x02 | ⏳ 外设忙 |
| `HAL_TIMEOUT` | 0x03 | ⏱️ 操作超时 |

---

### 💡 使用示例

#### HAL层示例 - UART发送数据

```c
// 初始化UART
UART_HandleTypeDef huart1;
huart1.Instance = USART1;
huart1.Init.BaudRate = 115200;
huart1.Init.WordLength = UART_WORDLENGTH_8B;
huart1.Init.StopBits = UART_STOPBITS_1;
huart1.Init.Parity = UART_PARITY_NONE;
HAL_UART_Init(&huart1);

// 发送数据(阻塞模式)
uint8_t data[] = "Hello STM32!";
HAL_StatusTypeDef status = HAL_UART_Transmit(&huart1, data, sizeof(data), 1000);
if (status == HAL_OK) {
    // 发送成功
}
```

#### LL层示例 - GPIO控制

```c
// 使能GPIOC时钟
LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOC);

// 配置PC13为输出
LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_13, LL_GPIO_MODE_OUTPUT);
LL_GPIO_SetPinSpeed(GPIOC, LL_GPIO_PIN_13, LL_GPIO_SPEED_FREQ_LOW);

// 控制LED
LL_GPIO_SetOutputPin(GPIOC, LL_GPIO_PIN_13);   // 点亮
LL_GPIO_ResetOutputPin(GPIOC, LL_GPIO_PIN_13); // 熄灭
LL_GPIO_TogglePin(GPIOC, LL_GPIO_PIN_13);      // 翻转
```

---

## ℹ️ 版本信息

- **芯片系列**: STM32F1xx
- **HAL库版本**: 基于官方最新版本
- **文档生成日期**: 2026-05-11
- **适用型号**: STM32F103、STM32F101、STM32F105、STM32F107等

---

## 📝 文档说明

### 文档内容

本文档从STM32F1xx HAL库头文件中提取API信息,包括:
- 函数原型和参数说明
- 数据结构定义
- 宏定义和常量
- 使用注意事项
- 完整的代码示例

### 文档特点

✅ **完整性**: 每个模块包含完整的API说明和使用示例  
✅ **实用性**: 提供真实的应用场景代码  
✅ **易读性**: 中文说明,结构清晰,排版美观  
✅ **可维护性**: 模块化组织,便于更新和扩展

### 使用建议

建议配合以下资料使用:
- ST官方参考手册(Reference Manual)
- 数据手册(Datasheet)
- 应用笔记(Application Notes)
- STM32CubeMX生成的示例代码

---

## 📚 学习资源

### 官方文档
- **参考手册**: 详细的寄存器说明
- **数据手册**: 电气特性和引脚定义
- **应用笔记**: 特定功能的应用指南
- **勘误表**: 已知问题和解决方案

### 示例代码
- STM32CubeMX生成的代码包含各模块的初始化示例
- ST官方提供的示例工程

### 社区资源
- ST官方论坛
- GitHub开源项目
- 技术博客和教程

---

## 📊 文档统计

### 已完成文档 (25个)

**指南文档**: 1个主文档  
**核心模块**: 6个 (Core, RCC, GPIO, CORTEX, DMA, EXTI)  
**通信外设**: 5个 (UART, USART, I2C, SPI, CAN)  
**定时器**: 4个 (TIM, RTC, IWDG, WWDG)  
**模拟外设**: 2个 (ADC, DAC)  
**存储器**: 2个 (FLASH, PWR)  
**其他**: 1个 (CRC)

**总计**: 约8000行文档内容

### 覆盖范围

✅ **已覆盖**: 所有常用核心模块  
✅ **已覆盖**: 主要通信接口  
✅ **已覆盖**: 定时器和看门狗  
✅ **已覆盖**: 模拟外设和存储器  
📝 **简介**: 低频使用的特殊模块

---

## 🎯 项目开发指南

### 典型项目开发流程

1. **系统初始化**
   - 调用 `HAL_Init()`
   - 配置系统时钟 `SystemClock_Config()`
   - 初始化所需外设

2. **外设配置**
   - 使能外设时钟
   - 配置GPIO引脚
   - 初始化通信接口

3. **功能实现**
   - 实现业务逻辑
   - 配置中断和回调
   - 实现数据处理

4. **调试优化**
   - 使用串口打印调试信息
   - 检查返回值和错误处理
   - 性能优化

### 常见应用场景

| 应用 | 需要的模块 |
|------|-----------|
| LED闪烁 | GPIO, TIM |
| 串口通信 | UART, DMA |
| 传感器采集 | I2C/SPI, ADC, TIM |
| PWM调光 | TIM, GPIO |
| 数据存储 | FLASH, SD |
| 低功耗应用 | PWR, RTC, IWDG |
| 电机控制 | TIM, GPIO, ADC |
| 网络通信 | ETH, UART |

---

## 💬 反馈与贡献

如果您在使用文档过程中有任何问题或建议,欢迎反馈:
- 文档不清楚的地方
- 需要补充的示例
- 发现的错误
- 希望改进的内容

---

## ✨ 总结

本文档为STM32F1xx HAL库提供了完整的中文API参考,涵盖了所有常用模块。文档结构清晰,包含丰富的代码示例,适合初学者学习和开发者参考。

**建议使用方式:**
1. 初学者从快速参考开始,掌握基本用法
2. 根据项目需求查阅对应模块的详细文档
3. 参考示例代码快速实现功能
4. 遇到问题查阅注意事项和常见问题

祝您开发顺利! 🚀

