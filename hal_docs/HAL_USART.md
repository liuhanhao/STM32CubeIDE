# USART通用同步异步收发器模块 API文档

## 📋 模块概述

USART(Universal Synchronous Asynchronous Receiver Transmitter)是UART的增强版本,支持:
- 异步通信(与UART相同)
- 同步通信(带时钟信号)
- 智能卡模式
- IrDA模式
- LIN模式
- 硬件流控

---

## 📚 相关文件

- **头文件**: `stm32f1xx_hal_usart.h`
- **源文件**: `stm32f1xx_hal_usart.c`

---

## 🎯 核心函数

USART的API与UART基本相同:

### HAL_USART_Init()

初始化USART。

```c
HAL_StatusTypeDef HAL_USART_Init(USART_HandleTypeDef *husart);
```

---

### HAL_USART_Transmit()

发送数据(阻塞模式)。

```c
HAL_StatusTypeDef HAL_USART_Transmit(USART_HandleTypeDef *husart,
                                     uint8_t *pTxData,
                                     uint16_t Size,
                                     uint32_t Timeout);
```

---

### HAL_USART_Receive()

接收数据(阻塞模式)。

```c
HAL_StatusTypeDef HAL_USART_Receive(USART_HandleTypeDef *husart,
                                    uint8_t *pRxData,
                                    uint16_t Size,
                                    uint32_t Timeout);
```

---

### HAL_USART_TransmitReceive()

同步收发数据。

```c
HAL_StatusTypeDef HAL_USART_TransmitReceive(USART_HandleTypeDef *husart,
                                            uint8_t *pTxData,
                                            uint8_t *pRxData,
                                            uint16_t Size,
                                            uint32_t Timeout);
```

---

## 💡 应用示例

### 示例1: 同步模式通信(主从设备数据交换)

USART 同步模式带时钟线，适合与没有独立时钟的从设备通信。

```c
USART_HandleTypeDef husart1;

void USART_Sync_Init(void) {
    husart1.Instance = USART1;
    husart1.Init.BaudRate = 115200;
    husart1.Init.WordLength = USART_WORDLENGTH_8B;
    husart1.Init.StopBits = USART_STOPBITS_1;
    husart1.Init.Parity = USART_PARITY_NONE;
    husart1.Init.Mode = USART_MODE_TX_RX;
    husart1.Init.CLKPolarity = USART_POLARITY_LOW;   // 空闲时钟低电平
    husart1.Init.CLKPhase = USART_PHASE_1EDGE;       // 第一个边沿采样
    husart1.Init.CLKLastBit = USART_LASTBIT_DISABLE; // 最后一位不输出时钟
    
    HAL_USART_Init(&husart1);
}

// 同步全双工收发：发送 3 字节的同时接收 3 字节
void USART_Sync_Transfer(void) {
    uint8_t tx_data[] = {0x01, 0x02, 0x03};
    uint8_t rx_data[3] = {0};
    
    // 同步模式下发送和接收同时进行
    HAL_StatusTypeDef ret = HAL_USART_TransmitReceive(&husart1, tx_data, rx_data, 3, 1000);
    if (ret == HAL_OK) {
        printf("收到: 0x%02X 0x%02X 0x%02X\n", rx_data[0], rx_data[1], rx_data[2]);
    }
}
```

---

### 示例2: 同步模式驱动 74HC595 移位寄存器

74HC595 是串行输入并行输出的移位寄存器，可以用 USART 同步模式驱动，
比软件模拟 SPI 效率更高。

```c
// USART1 TX → 74HC595 SER(数据)
// USART1 CK → 74HC595 SRCLK(移位时钟)
// PA2(普通 GPIO) → 74HC595 RCLK(锁存时钟)

#define LATCH_PIN  GPIO_PIN_2
#define LATCH_PORT GPIOA

void HC595_Init(void) {
    // 初始化锁存引脚
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStruct.Pin = LATCH_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LATCH_PORT, &GPIO_InitStruct);
    
    // 初始化 USART 同步模式(只需要 TX 和 CK)
    husart1.Instance = USART1;
    husart1.Init.BaudRate = 1000000;  // 1Mbps
    husart1.Init.WordLength = USART_WORDLENGTH_8B;
    husart1.Init.StopBits = USART_STOPBITS_1;
    husart1.Init.Parity = USART_PARITY_NONE;
    husart1.Init.Mode = USART_MODE_TX;  // 只需要发送
    husart1.Init.CLKPolarity = USART_POLARITY_LOW;
    husart1.Init.CLKPhase = USART_PHASE_1EDGE;
    husart1.Init.CLKLastBit = USART_LASTBIT_ENABLE;
    HAL_USART_Init(&husart1);
}

// 向 74HC595 写入一个字节
void HC595_Write(uint8_t data) {
    uint8_t dummy;
    
    HAL_GPIO_WritePin(LATCH_PORT, LATCH_PIN, GPIO_PIN_RESET);  // 锁存低
    HAL_USART_TransmitReceive(&husart1, &data, &dummy, 1, 100); // 发送数据
    HAL_GPIO_WritePin(LATCH_PORT, LATCH_PIN, GPIO_PIN_SET);    // 锁存高，输出生效
}

// 使用示例：控制 8 个 LED
int main(void) {
    HAL_Init();
    SystemClock_Config();
    HC595_Init();
    
    while (1) {
        for (int i = 0; i < 8; i++) {
            HC595_Write(1 << i);  // 流水灯
            HAL_Delay(100);
        }
    }
}
```

---

### 示例3: USART 与 UART 的区别对比

```c
// UART 异步模式(HAL_UART.md 中的用法)
// 只需要 TX、RX 两根线，双方约定好波特率
UART_HandleTypeDef huart1;
huart1.Instance = USART1;
huart1.Init.BaudRate = 115200;
huart1.Init.WordLength = UART_WORDLENGTH_8B;
huart1.Init.StopBits = UART_STOPBITS_1;
huart1.Init.Parity = UART_PARITY_NONE;
huart1.Init.Mode = UART_MODE_TX_RX;
huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
HAL_UART_Init(&huart1);

// USART 同步模式(本文档的用法)
// 需要 TX、RX、CK 三根线，由主机提供时钟
// 优点：不需要双方精确匹配波特率，通信更可靠
// 缺点：多一根时钟线，传输距离受限
USART_HandleTypeDef husart1;
husart1.Instance = USART1;
husart1.Init.BaudRate = 115200;
husart1.Init.CLKPolarity = USART_POLARITY_LOW;
husart1.Init.CLKPhase = USART_PHASE_1EDGE;
// ... 其余配置相同
HAL_USART_Init(&husart1);
```

> 💡 **选择建议：**
> - 与 PC、蓝牙模块、GPS 等通信 → 用 **UART**（异步，只需 TX/RX）
> - 驱动移位寄存器、与从设备同步通信 → 用 **USART 同步模式**（带时钟）
> - 大多数场景下 UART 就够用了，USART 同步模式是特殊需求

---

## ⚠️ 注意事项

### 1. 同步模式

同步模式需要额外的CK引脚输出时钟信号。

### 2. 与UART的区别

- USART支持同步通信
- USART有时钟输出
- 其他功能基本相同

---

## 📖 相关文档

- [UART串口](HAL_UART.md)
- [GPIO通用IO](HAL_GPIO.md)
