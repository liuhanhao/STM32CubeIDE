# GPIO通用IO模块 API文档

## 📋 模块概述

GPIO(General Purpose Input/Output)模块提供通用输入输出端口的配置和控制功能,包括:
- GPIO引脚初始化和配置
- 数字输入/输出操作
- 外部中断/事件配置
- 引脚锁定功能
- 复用功能配置

---

## 📚 相关文件

- **头文件**: `stm32f1xx_hal_gpio.h`, `stm32f1xx_hal_gpio_ex.h`
- **源文件**: `stm32f1xx_hal_gpio.c`, `stm32f1xx_hal_gpio_ex.c`

---

## 🔧 数据类型

### GPIO_InitTypeDef - GPIO初始化结构体

用于配置GPIO引脚的参数结构体。

```c
typedef struct {
    uint32_t Pin;    // 要配置的引脚编号
    uint32_t Mode;   // 工作模式
    uint32_t Pull;   // 上拉/下拉配置
    uint32_t Speed;  // 输出速度
} GPIO_InitTypeDef;
```

**成员说明:**

| 成员 | 说明 | 可选值 |
|------|------|--------|
| `Pin` | 引脚编号 | `GPIO_PIN_0` ~ `GPIO_PIN_15`, `GPIO_PIN_All` |
| `Mode` | 工作模式 | 见[GPIO工作模式](#gpio工作模式) |
| `Pull` | 上拉/下拉 | `GPIO_NOPULL`, `GPIO_PULLUP`, `GPIO_PULLDOWN` |
| `Speed` | 输出速度 | `GPIO_SPEED_FREQ_LOW/MEDIUM/HIGH` |

---

### GPIO_PinState - 引脚状态枚举

表示GPIO引脚的电平状态。

```c
typedef enum {
    GPIO_PIN_RESET = 0,  // 低电平
    GPIO_PIN_SET   = 1   // 高电平
} GPIO_PinState;
```

---

## 📌 常量定义

### GPIO引脚定义

```c
#define GPIO_PIN_0     0x0001  // 引脚0
#define GPIO_PIN_1     0x0002  // 引脚1
#define GPIO_PIN_2     0x0004  // 引脚2
#define GPIO_PIN_3     0x0008  // 引脚3
#define GPIO_PIN_4     0x0010  // 引脚4
#define GPIO_PIN_5     0x0020  // 引脚5
#define GPIO_PIN_6     0x0040  // 引脚6
#define GPIO_PIN_7     0x0080  // 引脚7
#define GPIO_PIN_8     0x0100  // 引脚8
#define GPIO_PIN_9     0x0200  // 引脚9
#define GPIO_PIN_10    0x0400  // 引脚10
#define GPIO_PIN_11    0x0800  // 引脚11
#define GPIO_PIN_12    0x1000  // 引脚12
#define GPIO_PIN_13    0x2000  // 引脚13
#define GPIO_PIN_14    0x4000  // 引脚14
#define GPIO_PIN_15    0x8000  // 引脚15
#define GPIO_PIN_All   0xFFFF  // 所有引脚
```

**使用说明:**
- 可以使用 `|` 运算符同时配置多个引脚
- 例如: `GPIO_PIN_0 | GPIO_PIN_1` 表示同时配置引脚0和引脚1

---

### GPIO工作模式

#### 基本输入输出模式

| 模式 | 说明 |
|------|------|
| `GPIO_MODE_INPUT` | 浮空输入模式 |
| `GPIO_MODE_OUTPUT_PP` | 推挽输出模式 |
| `GPIO_MODE_OUTPUT_OD` | 开漏输出模式 |
| `GPIO_MODE_ANALOG` | 模拟模式(用于ADC/DAC) |

---

#### 📖 四种基本模式详解与使用场景

##### GPIO_MODE_INPUT — 浮空输入

引脚处于高阻抗状态，既不上拉也不下拉，完全由外部电路决定电平。没有外部信号时引脚电平不确定，容易受干扰。

**典型场景：**

```c
// 场景1: 读取外部模块的数字输出信号(如红外传感器、霍尔传感器)
// 传感器自带推挽输出，不需要内部上拉
GPIO_InitStruct.Pin = GPIO_PIN_5;
GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
GPIO_InitStruct.Pull = GPIO_NOPULL;  // 传感器自己驱动，不需要上拉
HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

// 场景2: UART RX 引脚(复用输入，但原理相同)
// 场景3: 读取外部已有上拉/下拉电阻的信号线
```

> ⚠️ 实际使用中，浮空输入通常配合 `GPIO_PULLUP` 或 `GPIO_PULLDOWN` 使用，
> 或者依赖外部电阻，避免引脚悬空导致读值不稳定。

---

##### GPIO_MODE_OUTPUT_PP — 推挽输出

内部有两个 MOSFET，一个接 VCC（拉高），一个接 GND（拉低），交替工作。
能主动输出高电平和低电平，驱动能力强，是最常用的输出模式。

**典型场景：**

```c
// 场景1: 控制 LED 亮灭
// LED 一端接 PC13，另一端通过限流电阻接 GND
GPIO_InitStruct.Pin = GPIO_PIN_13;
GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
GPIO_InitStruct.Pull = GPIO_NOPULL;
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);    // 点亮
HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);  // 熄灭
```

```c
// 场景2: 控制继电器/蜂鸣器(通过三极管驱动)
// 场景3: SPI MOSI、SCK 信号线
// 场景4: 控制 74HC595 移位寄存器的时钟和数据线
GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_7;  // SCK | MOSI
GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;   // SPI 需要高速
HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
```

```c
// 场景5: 驱动数码管段选/位选
GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
                      GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
```

---

##### GPIO_MODE_OUTPUT_OD — 开漏输出

内部只有下拉 MOSFET，只能主动拉低。输出高电平时引脚处于高阻态，
**必须外接上拉电阻**才能输出高电平。多个开漏引脚可以"线与"连接到同一根线。

**典型场景：**

```c
// 场景1: I²C 总线 (SDA、SCL 都必须是开漏)
// I²C 允许多个设备挂在同一总线，任意一个拉低则总线为低
// STM32F1 的 I²C 外设会自动处理，但软件模拟 I²C 时需要手动配置
GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;  // SCL | SDA
GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
GPIO_InitStruct.Pull = GPIO_NOPULL;  // 使用外部 4.7kΩ 上拉到 3.3V
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
```

```c
// 场景2: 电平转换 — 外部上拉到 5V，实现 3.3V MCU 与 5V 设备通信
// MCU 引脚拉低 → 总线为低(两侧都看到低电平)
// MCU 引脚释放 → 外部 5V 上拉电阻将总线拉到 5V
// 这样 MCU 不会直接输出 5V，保护了引脚

// 场景3: 多主机共享的 ALERT/FAULT 信号线
// 任意一个设备检测到故障都可以拉低这根线通知主机
// 主机读到低电平后逐一查询是哪个设备报警
```

> 💡 **开漏 vs 推挽的选择口诀：**
> - 单设备、单方向、不需要电平转换 → 推挽
> - 多设备共享总线、需要电平转换、I²C → 开漏

---

##### GPIO_MODE_ANALOG — 模拟模式

引脚完全断开数字逻辑，直接连接到 ADC/DAC 的模拟通道。
禁用施密特触发器和数字缓冲，减少噪声干扰，也降低功耗。

**典型场景：**

```c
// 场景1: ADC 采集电位器电压(分压后接 PA0)
// 电位器两端接 3.3V 和 GND，中间抽头接 PA0
GPIO_InitStruct.Pin = GPIO_PIN_0;
GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;  // 必须配置为模拟模式
HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
// 之后通过 HAL_ADC_GetValue() 读取 0~4095 的数字值
```

```c
// 场景2: ADC 采集 NTC 热敏电阻温度
// NTC 与固定电阻分压后接 ADC 引脚，通过查表或公式换算温度
GPIO_InitStruct.Pin = GPIO_PIN_1;  // PA1 → ADC_CHANNEL_1
GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
```

```c
// 场景3: DAC 输出模拟电压(如驱动音频、控制模拟量)
// PA4 → DAC_OUT1，PA5 → DAC_OUT2
GPIO_InitStruct.Pin = GPIO_PIN_4;
GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
// 之后通过 HAL_DAC_SetValue() 设置输出电压
```

```c
// 场景4: 不使用的引脚配置为模拟模式以降低功耗
// 浮空的数字输入引脚会因电平不确定而反复翻转，增加功耗
// 配置为模拟模式可以避免这个问题
GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;  // 未使用的引脚
GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
```

> ⚠️ ADC 引脚输入电压不能超过 VDDA（通常 3.3V），也不能为负值，否则会损坏芯片。

#### 复用功能模式

| 模式 | 说明 |
|------|------|
| `GPIO_MODE_AF_PP` | 复用功能推挽输出 |
| `GPIO_MODE_AF_OD` | 复用功能开漏输出 |
| `GPIO_MODE_AF_INPUT` | 复用功能输入 |

#### 外部中断模式

| 模式 | 说明 |
|------|------|
| `GPIO_MODE_IT_RISING` | 上升沿触发中断 |
| `GPIO_MODE_IT_FALLING` | 下降沿触发中断 |
| `GPIO_MODE_IT_RISING_FALLING` | 双边沿触发中断 |

#### 外部事件模式

| 模式 | 说明 |
|------|------|
| `GPIO_MODE_EVT_RISING` | 上升沿触发事件 |
| `GPIO_MODE_EVT_FALLING` | 下降沿触发事件 |
| `GPIO_MODE_EVT_RISING_FALLING` | 双边沿触发事件 |

---

### GPIO输出速度

| 速度 | 最大频率 | 说明 |
|------|----------|------|
| `GPIO_SPEED_FREQ_LOW` | 2MHz | 低速(功耗最低) |
| `GPIO_SPEED_FREQ_MEDIUM` | 10MHz | 中速 |
| `GPIO_SPEED_FREQ_HIGH` | 50MHz | 高速(功耗最高) |

⚠️ **注意**: 速度越高,EMI干扰和功耗越大,应根据实际需求选择。

---

### GPIO上拉/下拉

| 配置 | 说明 |
|------|------|
| `GPIO_NOPULL` | 无上拉下拉(浮空) |
| `GPIO_PULLUP` | 内部上拉 |
| `GPIO_PULLDOWN` | 内部下拉 |

---

## 🎯 核心函数

### 初始化函数

#### HAL_GPIO_Init()

初始化GPIO引脚。

```c
void HAL_GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_Init);
```

**参数:**
- `GPIOx`: GPIO端口指针 (`GPIOA`, `GPIOB`, `GPIOC`, `GPIOD`, `GPIOE`)
- `GPIO_Init`: 初始化参数结构体指针

**使用示例:**

```c
// 示例1: 配置PC13为推挽输出(LED控制)
GPIO_InitTypeDef GPIO_InitStruct = {0};

// 使能GPIOC时钟
__HAL_RCC_GPIOC_CLK_ENABLE();

// 配置PC13
GPIO_InitStruct.Pin = GPIO_PIN_13;
GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
GPIO_InitStruct.Pull = GPIO_NOPULL;
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
```

```c
// 示例2: 配置PA0为上拉输入(按键检测)
GPIO_InitTypeDef GPIO_InitStruct = {0};

__HAL_RCC_GPIOA_CLK_ENABLE();

GPIO_InitStruct.Pin = GPIO_PIN_0;
GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
GPIO_InitStruct.Pull = GPIO_PULLUP;
HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
```

```c
// 示例3: 配置PA9/PA10为UART复用功能
GPIO_InitTypeDef GPIO_InitStruct = {0};

__HAL_RCC_GPIOA_CLK_ENABLE();

// PA9: UART TX (复用推挽输出)
GPIO_InitStruct.Pin = GPIO_PIN_9;
GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

// PA10: UART RX (浮空输入)
GPIO_InitStruct.Pin = GPIO_PIN_10;
GPIO_InitStruct.Mode = GPIO_MODE_AF_INPUT;
HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
```

```c
// 示例4: 配置PB0为外部中断(下降沿触发)
GPIO_InitTypeDef GPIO_InitStruct = {0};

__HAL_RCC_GPIOB_CLK_ENABLE();

GPIO_InitStruct.Pin = GPIO_PIN_0;
GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
GPIO_InitStruct.Pull = GPIO_PULLUP;
HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

// 配置NVIC
HAL_NVIC_SetPriority(EXTI0_IRQn, 2, 0);
HAL_NVIC_EnableIRQ(EXTI0_IRQn);
```

---

#### HAL_GPIO_DeInit()

反初始化GPIO引脚。

```c
void HAL_GPIO_DeInit(GPIO_TypeDef *GPIOx, uint32_t GPIO_Pin);
```

**参数:**
- `GPIOx`: GPIO端口指针
- `GPIO_Pin`: 要反初始化的引脚(可用 `|` 组合多个引脚)

**功能说明:**
- 将指定引脚恢复为复位状态(浮空输入)
- 禁用相关的EXTI配置

**使用示例:**
```c
// 反初始化PC13
HAL_GPIO_DeInit(GPIOC, GPIO_PIN_13);

// 反初始化PA9和PA10
HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9 | GPIO_PIN_10);
```

---

### IO操作函数

#### HAL_GPIO_ReadPin()

读取GPIO引脚状态。

```c
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
```

**参数:**
- `GPIOx`: GPIO端口指针
- `GPIO_Pin`: 要读取的引脚

**返回值:**
- `GPIO_PIN_RESET`: 引脚为低电平
- `GPIO_PIN_SET`: 引脚为高电平

**使用示例:**
```c
// 读取按键状态(PA0)
if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET) {
    // 按键按下(假设按键接地)
    printf("按键按下\n");
}

// 读取传感器输出(PB5)
GPIO_PinState sensor_state = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_5);
if (sensor_state == GPIO_PIN_SET) {
    // 传感器检测到信号
}
```

---

#### HAL_GPIO_WritePin()

设置GPIO引脚状态。

```c
void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState);
```

**参数:**
- `GPIOx`: GPIO端口指针
- `GPIO_Pin`: 要设置的引脚(可用 `|` 组合多个引脚)
- `PinState`: 要设置的状态
  - `GPIO_PIN_RESET`: 设置为低电平
  - `GPIO_PIN_SET`: 设置为高电平

**使用示例:**
```c
// 点亮LED(PC13)
HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

// 熄灭LED
HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

// 同时控制多个引脚
HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2, GPIO_PIN_SET);
```

---

#### HAL_GPIO_TogglePin()

翻转GPIO引脚状态。

```c
void HAL_GPIO_TogglePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
```

**参数:**
- `GPIOx`: GPIO端口指针
- `GPIO_Pin`: 要翻转的引脚(可用 `|` 组合多个引脚)

**功能说明:**
- 如果引脚当前为高电平,则变为低电平
- 如果引脚当前为低电平,则变为高电平

**使用示例:**
```c
// LED闪烁
while (1) {
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    HAL_Delay(500);  // 延时500ms
}

// 同时翻转多个LED
HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2);
```

---

#### HAL_GPIO_LockPin()

锁定GPIO引脚配置。

```c
HAL_StatusTypeDef HAL_GPIO_LockPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
```

**参数:**
- `GPIOx`: GPIO端口指针
- `GPIO_Pin`: 要锁定的引脚

**返回值:**
- `HAL_OK`: 锁定成功
- `HAL_ERROR`: 锁定失败

**功能说明:**
- 锁定后,引脚配置在下次复位前无法修改
- 用于防止意外修改关键引脚配置

**使用示例:**
```c
// 配置并锁定关键引脚
GPIO_InitTypeDef GPIO_InitStruct = {0};
GPIO_InitStruct.Pin = GPIO_PIN_13;
GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

// 锁定配置
if (HAL_GPIO_LockPin(GPIOC, GPIO_PIN_13) == HAL_OK) {
    // 锁定成功,配置无法被修改
}
```

⚠️ **注意**: 锁定后只能通过系统复位解除。

---

### 外部中断函数

#### HAL_GPIO_EXTI_IRQHandler()

GPIO外部中断处理函数。

```c
void HAL_GPIO_EXTI_IRQHandler(uint16_t GPIO_Pin);
```

**参数:**
- `GPIO_Pin`: 触发中断的引脚

**功能说明:**
- 在EXTI中断服务函数中调用
- 清除中断标志并调用回调函数

**使用示例:**
```c
// EXTI0中断服务函数
void EXTI0_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

// EXTI15_10中断服务函数(处理PIN10-15)
void EXTI15_10_IRQHandler(void) {
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_13) != RESET) {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);
    }
}
```

---

#### HAL_GPIO_EXTI_Callback()

GPIO外部中断回调函数。

```c
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);
```

**参数:**
- `GPIO_Pin`: 触发中断的引脚

**功能说明:**
- 弱定义函数,用户需要重新实现
- 在`HAL_GPIO_EXTI_IRQHandler()`中被调用
- 在此函数中处理中断事件

**使用示例:**
```c
// 用户实现的回调函数
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_0) {
        // PA0中断处理
        printf("PA0中断触发\n");
    }
    else if (GPIO_Pin == GPIO_PIN_13) {
        // PC13中断处理
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    }
}
```

---

## 🔍 常用宏定义

### EXTI标志位操作

#### __HAL_GPIO_EXTI_GET_FLAG()

检查EXTI标志位。

```c
#define __HAL_GPIO_EXTI_GET_FLAG(EXTI_LINE)
```

**参数:**
- `EXTI_LINE`: EXTI线(GPIO_PIN_x)

**返回值:**
- 非0: 标志位已设置
- 0: 标志位未设置

---

#### __HAL_GPIO_EXTI_CLEAR_FLAG()

清除EXTI标志位。

```c
#define __HAL_GPIO_EXTI_CLEAR_FLAG(EXTI_LINE)
```

**参数:**
- `EXTI_LINE`: EXTI线(可用 `|` 组合)

---

#### __HAL_GPIO_EXTI_GET_IT()

检查EXTI中断标志。

```c
#define __HAL_GPIO_EXTI_GET_IT(EXTI_LINE)
```

---

#### __HAL_GPIO_EXTI_CLEAR_IT()

清除EXTI中断标志。

```c
#define __HAL_GPIO_EXTI_CLEAR_IT(EXTI_LINE)
```

---

#### __HAL_GPIO_EXTI_GENERATE_SWIT()

软件触发EXTI中断。

```c
#define __HAL_GPIO_EXTI_GENERATE_SWIT(EXTI_LINE)
```

**使用示例:**
```c
// 软件触发PA0的EXTI中断
__HAL_GPIO_EXTI_GENERATE_SWIT(GPIO_PIN_0);
```

---

## 💡 完整应用示例

### 示例1: LED闪烁

```c
// LED初始化
void LED_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // 使能时钟
    __HAL_RCC_GPIOC_CLK_ENABLE();
    
    // 配置PC13为推挽输出
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    
    // 初始状态为熄灭
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
}

// 主循环
int main(void) {
    HAL_Init();
    SystemClock_Config();
    LED_Init();
    
    while (1) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        HAL_Delay(500);
    }
}
```

---

### 示例2: 按键检测(轮询方式)

```c
// 按键初始化
void Button_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    // 配置PA0为上拉输入
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

// 按键扫描(带消抖)
uint8_t Button_Scan(void) {
    static uint8_t key_state = 0;
    
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET) {
        HAL_Delay(10);  // 消抖延时
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET) {
            if (key_state == 0) {
                key_state = 1;
                return 1;  // 按键按下
            }
        }
    } else {
        key_state = 0;
    }
    return 0;
}

// 主循环
int main(void) {
    HAL_Init();
    SystemClock_Config();
    LED_Init();
    Button_Init();
    
    while (1) {
        if (Button_Scan()) {
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        }
    }
}
```

---

### 示例3: 外部中断方式按键

```c
// 按键中断初始化
void Button_IT_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    // 配置PA0为下降沿触发中断
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // 配置NVIC
    HAL_NVIC_SetPriority(EXTI0_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

// EXTI0中断服务函数
void EXTI0_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

// 中断回调函数
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_0) {
        // 简单消抖
        HAL_Delay(10);
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET) {
            // 按键确实按下,翻转LED
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        }
    }
}
```

---

### 示例4: 多路LED控制

```c
// 8路LED初始化(PB0-PB7)
void LED_Array_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    // 配置PB0-PB7为推挽输出
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
                          GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    // 全部熄灭
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_All, GPIO_PIN_RESET);
}

// 设置LED显示值(0-255)
void LED_Array_Set(uint8_t value) {
    for (int i = 0; i < 8; i++) {
        if (value & (1 << i)) {
            HAL_GPIO_WritePin(GPIOB, 1 << i, GPIO_PIN_SET);
        } else {
            HAL_GPIO_WritePin(GPIOB, 1 << i, GPIO_PIN_RESET);
        }
    }
}

// 流水灯效果
void LED_Running(void) {
    for (int i = 0; i < 8; i++) {
        LED_Array_Set(1 << i);
        HAL_Delay(100);
    }
}
```

---

## ⚠️ 注意事项

### 1. 时钟使能

使用GPIO前必须先使能对应端口的时钟:

```c
__HAL_RCC_GPIOA_CLK_ENABLE();  // 使能GPIOA时钟
__HAL_RCC_GPIOB_CLK_ENABLE();  // 使能GPIOB时钟
__HAL_RCC_GPIOC_CLK_ENABLE();  // 使能GPIOC时钟
// ...
```

---

### 2. 复用功能配置

STM32F1的复用功能不需要额外配置复用映射(与F4/F7不同),只需:
- 将引脚配置为复用功能模式(`GPIO_MODE_AF_PP`或`GPIO_MODE_AF_OD`)
- 使能对应外设时钟
- 初始化外设

---

### 3. 引脚重映射

某些外设支持引脚重映射,需要使用`__HAL_AFIO_REMAP_xxx()`宏:

```c
// 使能AFIO时钟
__HAL_RCC_AFIO_CLK_ENABLE();

// USART1重映射(TX:PB6, RX:PB7)
__HAL_AFIO_REMAP_USART1_ENABLE();

// SWD调试接口重映射(释放PB3/PB4/PA15)
__HAL_AFIO_REMAP_SWJ_NOJTAG();
```

---

### 4. 输出速度选择

| 应用场景 | 推荐速度 |
|----------|----------|
| LED控制 | LOW |
| 按键输入 | - (输入无速度设置) |
| I2C/UART(低速) | MEDIUM |
| SPI/UART(高速) | HIGH |
| 高频信号输出 | HIGH |

---

### 5. 上拉/下拉选择

| 应用场景 | 推荐配置 |
|----------|----------|
| 按键(接地) | PULLUP |
| 按键(接VCC) | PULLDOWN |
| I2C | PULLUP(外部上拉更好) |
| SPI | 根据协议选择 |
| 浮空输入 | NOPULL |

---

### 6. 中断优先级

配置外部中断时注意设置合理的优先级:

```c
// 优先级分组(在HAL_Init中已设置)
HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

// 设置中断优先级(抢占优先级, 子优先级)
HAL_NVIC_SetPriority(EXTI0_IRQn, 2, 0);
HAL_NVIC_EnableIRQ(EXTI0_IRQn);
```

---

### 7. 5V容忍引脚

STM32F1部分引脚支持5V输入,查阅数据手册确认:
- 大部分GPIO支持5V输入
- 模拟输入引脚(ADC)不支持5V

---

## 📖 相关文档

- [HAL核心](HAL_Core.md)
- [RCC时钟控制](HAL_RCC.md)
- [EXTI外部中断](HAL_EXTI.md)
- [CORTEX内核](HAL_CORTEX.md)
