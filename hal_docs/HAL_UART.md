# UART串口模块 API文档

## 📋 模块概述

UART(Universal Asynchronous Receiver/Transmitter)通用异步收发器模块提供串口通信功能,包括:
- 全双工异步通信
- 支持多种波特率
- 可配置数据位、停止位、校验位
- 支持硬件流控(RTS/CTS)
- 支持LIN总线模式
- 支持多处理器通信模式
- 支持半双工模式
- 阻塞、中断、DMA三种传输模式

---

## 📚 相关文件

- **头文件**: `stm32f1xx_hal_uart.h`
- **源文件**: `stm32f1xx_hal_uart.c`

---

## 🔧 数据类型

### UART_InitTypeDef - UART初始化结构体

```c
typedef struct {
    uint32_t BaudRate;      // 波特率
    uint32_t WordLength;    // 数据位长度
    uint32_t StopBits;      // 停止位
    uint32_t Parity;        // 校验位
    uint32_t Mode;          // 收发模式
    uint32_t HwFlowCtl;     // 硬件流控
    uint32_t OverSampling;  // 过采样率
} UART_InitTypeDef;
```

**成员说明:**

| 成员 | 说明 | 可选值 |
|------|------|--------|
| `BaudRate` | 波特率 | 常用: 9600, 115200, 921600等 |
| `WordLength` | 数据位长度 | `UART_WORDLENGTH_8B`, `UART_WORDLENGTH_9B` |
| `StopBits` | 停止位 | `UART_STOPBITS_1`, `UART_STOPBITS_2` |
| `Parity` | 校验位 | `UART_PARITY_NONE/EVEN/ODD` |
| `Mode` | 收发模式 | `UART_MODE_TX`, `UART_MODE_RX`, `UART_MODE_TX_RX` |
| `HwFlowCtl` | 硬件流控 | `UART_HWCONTROL_NONE/RTS/CTS/RTS_CTS` |
| `OverSampling` | 过采样 | `UART_OVERSAMPLING_16`(默认) |

---

### UART_HandleTypeDef - UART句柄结构体

```c
typedef struct {
    USART_TypeDef *Instance;        // UART寄存器基地址
    UART_InitTypeDef Init;          // 初始化参数
    uint8_t *pTxBuffPtr;            // 发送缓冲区指针
    uint16_t TxXferSize;            // 发送数据大小
    uint16_t TxXferCount;           // 发送计数器
    uint8_t *pRxBuffPtr;            // 接收缓冲区指针
    uint16_t RxXferSize;            // 接收数据大小
    uint16_t RxXferCount;           // 接收计数器
    DMA_HandleTypeDef *hdmatx;      // 发送DMA句柄
    DMA_HandleTypeDef *hdmarx;      // 接收DMA句柄
    HAL_LockTypeDef Lock;           // 锁定对象
    HAL_UART_StateTypeDef gState;   // 全局状态
    HAL_UART_StateTypeDef RxState;  // 接收状态
    uint32_t ErrorCode;             // 错误代码
} UART_HandleTypeDef;
```

---

### HAL_UART_StateTypeDef - UART状态枚举

```c
typedef enum {
    HAL_UART_STATE_RESET      = 0x00,  // 未初始化
    HAL_UART_STATE_READY      = 0x20,  // 就绪
    HAL_UART_STATE_BUSY       = 0x24,  // 忙
    HAL_UART_STATE_BUSY_TX    = 0x21,  // 发送中
    HAL_UART_STATE_BUSY_RX    = 0x22,  // 接收中
    HAL_UART_STATE_BUSY_TX_RX = 0x23,  // 收发中
    HAL_UART_STATE_TIMEOUT    = 0xA0,  // 超时
    HAL_UART_STATE_ERROR      = 0xE0   // 错误
} HAL_UART_StateTypeDef;
```

---

## 📌 常量定义

### 波特率

常用波特率值:
- 9600
- 19200
- 38400
- 57600
- 115200
- 230400
- 460800
- 921600

⚠️ **注意**: 最大波特率取决于系统时钟频率。

---

### 数据位长度

| 常量 | 说明 |
|------|------|
| `UART_WORDLENGTH_8B` | 8位数据位 |
| `UART_WORDLENGTH_9B` | 9位数据位(通常用于校验) |

---

### 停止位

| 常量 | 说明 |
|------|------|
| `UART_STOPBITS_1` | 1个停止位 |
| `UART_STOPBITS_2` | 2个停止位 |

---

### 校验位

| 常量 | 说明 |
|------|------|
| `UART_PARITY_NONE` | 无校验 |
| `UART_PARITY_EVEN` | 偶校验 |
| `UART_PARITY_ODD` | 奇校验 |

---

### 硬件流控

| 常量 | 说明 |
|------|------|
| `UART_HWCONTROL_NONE` | 无流控 |
| `UART_HWCONTROL_RTS` | RTS流控 |
| `UART_HWCONTROL_CTS` | CTS流控 |
| `UART_HWCONTROL_RTS_CTS` | RTS+CTS流控 |

---

### 错误代码

| 错误代码 | 说明 |
|----------|------|
| `HAL_UART_ERROR_NONE` | 无错误 |
| `HAL_UART_ERROR_PE` | 校验错误 |
| `HAL_UART_ERROR_NE` | 噪声错误 |
| `HAL_UART_ERROR_FE` | 帧错误 |
| `HAL_UART_ERROR_ORE` | 溢出错误 |
| `HAL_UART_ERROR_DMA` | DMA传输错误 |

---

## 🎯 核心函数

### 初始化函数

#### HAL_UART_Init()

初始化UART外设。

```c
HAL_StatusTypeDef HAL_UART_Init(UART_HandleTypeDef *huart);
```

**参数:**
- `huart`: UART句柄指针

**返回值:**
- `HAL_OK`: 初始化成功
- `HAL_ERROR`: 初始化失败

**使用示例:**
```c
UART_HandleTypeDef huart1;

// 配置UART参数
huart1.Instance = USART1;
huart1.Init.BaudRate = 115200;
huart1.Init.WordLength = UART_WORDLENGTH_8B;
huart1.Init.StopBits = UART_STOPBITS_1;
huart1.Init.Parity = UART_PARITY_NONE;
huart1.Init.Mode = UART_MODE_TX_RX;
huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
huart1.Init.OverSampling = UART_OVERSAMPLING_16;

// 初始化UART
if (HAL_UART_Init(&huart1) != HAL_OK) {
    Error_Handler();
}
```

---

#### HAL_UART_DeInit()

反初始化UART外设。

```c
HAL_StatusTypeDef HAL_UART_DeInit(UART_HandleTypeDef *huart);
```

---

#### HAL_UART_MspInit()

UART MSP初始化函数(用户实现)。

```c
void HAL_UART_MspInit(UART_HandleTypeDef *huart);
```

**使用示例:**
```c
void HAL_UART_MspInit(UART_HandleTypeDef *huart) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    if (huart->Instance == USART1) {
        // 使能时钟
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        
        // 配置GPIO
        // PA9: UART TX (复用推挽输出)
        GPIO_InitStruct.Pin = GPIO_PIN_9;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        
        // PA10: UART RX (浮空输入)
        GPIO_InitStruct.Pin = GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_INPUT;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        
        // 配置NVIC(如果使用中断)
        HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
    }
}
```

---

### 阻塞模式传输

#### HAL_UART_Transmit()

阻塞模式发送数据。

```c
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, 
                                    const uint8_t *pData, 
                                    uint16_t Size, 
                                    uint32_t Timeout);
```

**参数:**
- `huart`: UART句柄指针
- `pData`: 发送数据缓冲区指针
- `Size`: 发送数据字节数
- `Timeout`: 超时时间(ms)

**返回值:**
- `HAL_OK`: 发送成功
- `HAL_ERROR`: 发送失败
- `HAL_BUSY`: UART忙
- `HAL_TIMEOUT`: 超时

**使用示例:**
```c
uint8_t tx_data[] = "Hello STM32!\r\n";
HAL_StatusTypeDef status;

status = HAL_UART_Transmit(&huart1, tx_data, sizeof(tx_data)-1, 1000);
if (status == HAL_OK) {
    // 发送成功
}
```

---

#### HAL_UART_Receive()

阻塞模式接收数据。

```c
HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, 
                                   uint8_t *pData, 
                                   uint16_t Size, 
                                   uint32_t Timeout);
```

**参数:**
- `huart`: UART句柄指针
- `pData`: 接收数据缓冲区指针
- `Size`: 期望接收的字节数
- `Timeout`: 超时时间(ms)

**使用示例:**
```c
uint8_t rx_data[100];
HAL_StatusTypeDef status;

// 接收10个字节,超时1秒
status = HAL_UART_Receive(&huart1, rx_data, 10, 1000);
if (status == HAL_OK) {
    // 接收成功,处理数据
    printf("收到: %s\n", rx_data);
}
```

---

### 中断模式传输

#### HAL_UART_Transmit_IT()

中断模式发送数据。

```c
HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *huart, 
                                       const uint8_t *pData, 
                                       uint16_t Size);
```

**使用示例:**
```c
uint8_t tx_data[] = "Hello World!\r\n";

// 启动中断发送
if (HAL_UART_Transmit_IT(&huart1, tx_data, sizeof(tx_data)-1) == HAL_OK) {
    // 发送已启动,等待中断完成
}

// 在中断服务函数中
void USART1_IRQHandler(void) {
    HAL_UART_IRQHandler(&huart1);
}

// 发送完成回调
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        // 发送完成处理
    }
}
```

---

#### HAL_UART_Receive_IT()

中断模式接收数据。

```c
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *huart, 
                                      uint8_t *pData, 
                                      uint16_t Size);
```

**使用示例:**
```c
uint8_t rx_buffer[100];

// 启动中断接收
HAL_UART_Receive_IT(&huart1, rx_buffer, 10);

// 接收完成回调
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        // 处理接收到的数据
        printf("收到: %s\n", rx_buffer);
        
        // 重新启动接收
        HAL_UART_Receive_IT(&huart1, rx_buffer, 10);
    }
}
```

---

### DMA模式传输

#### HAL_UART_Transmit_DMA()

DMA模式发送数据。

```c
HAL_StatusTypeDef HAL_UART_Transmit_DMA(UART_HandleTypeDef *huart, 
                                        const uint8_t *pData, 
                                        uint16_t Size);
```

**使用示例:**
```c
uint8_t tx_data[1000];

// 启动DMA发送
HAL_UART_Transmit_DMA(&huart1, tx_data, sizeof(tx_data));

// 发送完成回调
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    // DMA发送完成
}
```

---

#### HAL_UART_Receive_DMA()

DMA模式接收数据。

```c
HAL_StatusTypeDef HAL_UART_Receive_DMA(UART_HandleTypeDef *huart, 
                                       uint8_t *pData, 
                                       uint16_t Size);
```

---

### 中断处理函数

#### HAL_UART_IRQHandler()

UART中断处理函数。

```c
void HAL_UART_IRQHandler(UART_HandleTypeDef *huart);
```

**使用示例:**
```c
void USART1_IRQHandler(void) {
    HAL_UART_IRQHandler(&huart1);
}
```

---

### 回调函数

所有回调函数都是弱定义,用户需要重新实现:

```c
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart);        // 发送完成
void HAL_UART_TxHalfCpltCallback(UART_HandleTypeDef *huart);    // 发送半完成
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);        // 接收完成
void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart);    // 接收半完成
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart);         // 错误回调
```

---

### 状态查询函数

#### HAL_UART_GetState()

获取UART状态。

```c
HAL_UART_StateTypeDef HAL_UART_GetState(const UART_HandleTypeDef *huart);
```

---

#### HAL_UART_GetError()

获取错误代码。

```c
uint32_t HAL_UART_GetError(const UART_HandleTypeDef *huart);
```

---

## 💡 完整应用示例

### 示例1: 简单串口打印

```c
#include "stm32f1xx_hal.h"
#include <stdio.h>

UART_HandleTypeDef huart1;

// 初始化UART
void UART_Init(void) {
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    
    HAL_UART_Init(&huart1);
}

// 重定向printf到UART
int fputc(int ch, FILE *f) {
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 100);
    return ch;
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    UART_Init();
    
    while (1) {
        printf("Hello STM32!\r\n");
        HAL_Delay(1000);
    }
}
```

---

### 示例2: 中断接收+回显

```c
uint8_t rx_byte;

void UART_Init(void) {
    // ... 初始化代码同上 ...
    
    // 启动接收
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
}

// 接收完成回调
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        // 回显接收到的字符
        HAL_UART_Transmit(&huart1, &rx_byte, 1, 100);
        
        // 重新启动接收
        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    }
}

// 中断服务函数
void USART1_IRQHandler(void) {
    HAL_UART_IRQHandler(&huart1);
}
```

---

### 示例3: DMA循环接收

```c
#define RX_BUFFER_SIZE 256
uint8_t rx_buffer[RX_BUFFER_SIZE];

void UART_DMA_Init(void) {
    // ... UART初始化 ...
    
    // 启动DMA循环接收
    HAL_UART_Receive_DMA(&huart1, rx_buffer, RX_BUFFER_SIZE);
}

// 接收完成回调(缓冲区满)
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    // 处理接收到的数据
    // 注意: DMA会自动重新开始接收
}

// 接收半完成回调
void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart) {
    // 处理前半部分数据
}
```

---

## ⚠️ 注意事项

### 1. 时钟配置

使用UART前必须:
- 使能UART外设时钟
- 使能GPIO时钟
- 配置系统时钟以获得准确波特率

```c
__HAL_RCC_USART1_CLK_ENABLE();
__HAL_RCC_GPIOA_CLK_ENABLE();
```

---

### 2. GPIO配置

STM32F1的UART引脚配置:
- **TX引脚**: 复用推挽输出 (`GPIO_MODE_AF_PP`)
- **RX引脚**: 浮空输入或上拉输入 (`GPIO_MODE_AF_INPUT`)

---

### 3. 波特率误差

波特率由系统时钟分频产生,可能存在误差:
- 误差应小于2%
- 使用标准波特率(9600, 115200等)
- 调整系统时钟以获得更准确的波特率

---

### 4. 中断优先级

配置合理的中断优先级:
```c
HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
HAL_NVIC_EnableIRQ(USART1_IRQn);
```

---

### 5. DMA配置

使用DMA时需要:
- 配置DMA通道
- 链接DMA句柄到UART句柄
- 使能DMA中断

---

### 6. 错误处理

定期检查错误状态:
```c
if (huart1.ErrorCode != HAL_UART_ERROR_NONE) {
    // 处理错误
    if (huart1.ErrorCode & HAL_UART_ERROR_ORE) {
        // 溢出错误
        __HAL_UART_CLEAR_OREFLAG(&huart1);
    }
}
```

---

## �� 相关文档

- [HAL核心](HAL_Core.md)
- [GPIO通用IO](HAL_GPIO.md)
- [DMA直接内存访问](HAL_DMA.md)
- [USART串口](HAL_USART.md)
