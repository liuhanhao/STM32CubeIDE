# DMA直接内存访问模块 API文档

## 📋 模块概述

DMA(Direct Memory Access)直接内存访问控制器可以在外设和内存之间直接传输数据,无需CPU干预,包括:
- 内存到内存传输
- 外设到内存传输
- 内存到外设传输
- 支持循环模式
- 支持半传输和传输完成中断
- 7个通道(DMA1)和5个通道(DMA2)

---

## 🗺️ 使用场景速查

**什么时候必须用 DMA**

| 场景 | 说明 |
|------|------|
| UART 收发大量数据（>100 字节） | 中断/阻塞会频繁打断 CPU，DMA 一次搬完 |
| ADC 多通道连续采集 | CPU 来不及逐个读，DMA 自动搬运到数组 |
| SPI 刷屏（TFT/OLED 大量像素） | 每帧数据量大，DMA 刷屏期间 CPU 可处理其他事 |
| DAC 连续波形输出（正弦波、音频） | 需精确时序，DMA 循环模式 + 定时器触发 |
| 内存块快速复制（大块 memcpy） | DMA 内存到内存模式，释放 CPU |

**什么时候不值得用 DMA**

- 数据量小（<32 字节），初始化开销大于收益
- 时序不严格，阻塞模式够用
- DMA 通道已被更高优先级外设占用

**DMA 通道分配（固定，不能随意选）**

| 外设方向 | DMA1 通道 |
|----------|-----------|
| ADC1 | Channel 1 |
| USART1 TX | Channel 4 |
| USART1 RX | Channel 5 |
| USART2 RX | Channel 6 |
| USART2 TX | Channel 7 |
| SPI1 RX | Channel 2 |
| SPI1 TX | Channel 3 |
| I2C1 TX | Channel 6 |
| I2C1 RX | Channel 7 |

> 同一通道同时只能服务一个外设。完整映射见参考手册 Table 78。

**循环 vs 正常模式怎么选**

| 模式 | 适用场景 |
|------|----------|
| `DMA_NORMAL`（单次） | 发送一帧数据后停止，如 UART 发一包 |
| `DMA_CIRCULAR`（循环） | 持续采集/输出，如 ADC 连续采集、DAC 波形输出 |

---

## 📚 相关文件

- **头文件**: `stm32f1xx_hal_dma.h`
- **源文件**: `stm32f1xx_hal_dma.c`

---

## 🔧 数据类型

### DMA_InitTypeDef - DMA初始化结构体

```c
typedef struct {
    uint32_t Direction;           // 传输方向
    uint32_t PeriphInc;          // 外设地址增量模式
    uint32_t MemInc;             // 内存地址增量模式
    uint32_t PeriphDataAlignment; // 外设数据宽度
    uint32_t MemDataAlignment;    // 内存数据宽度
    uint32_t Mode;               // 传输模式
    uint32_t Priority;           // 优先级
} DMA_InitTypeDef;
```

---

## 📌 常量定义

### 传输方向

```c
DMA_PERIPH_TO_MEMORY  // 外设到内存
DMA_MEMORY_TO_PERIPH  // 内存到外设
DMA_MEMORY_TO_MEMORY  // 内存到内存
```

### 传输模式

```c
DMA_NORMAL    // 正常模式(单次传输)
DMA_CIRCULAR  // 循环模式(自动重载)
```

### 优先级

```c
DMA_PRIORITY_LOW       // 低优先级
DMA_PRIORITY_MEDIUM    // 中优先级
DMA_PRIORITY_HIGH      // 高优先级
DMA_PRIORITY_VERY_HIGH // 最高优先级
```

---

## 🎯 核心函数

### HAL_DMA_Init()

初始化DMA通道。

```c
HAL_StatusTypeDef HAL_DMA_Init(DMA_HandleTypeDef *hdma);
```

---

### HAL_DMA_Start()

启动DMA传输(阻塞模式)。

```c
HAL_StatusTypeDef HAL_DMA_Start(DMA_HandleTypeDef *hdma,
                                uint32_t SrcAddress,
                                uint32_t DstAddress,
                                uint32_t DataLength);
```

---

### HAL_DMA_Start_IT()

启动DMA传输(中断模式)。

```c
HAL_StatusTypeDef HAL_DMA_Start_IT(DMA_HandleTypeDef *hdma,
                                   uint32_t SrcAddress,
                                   uint32_t DstAddress,
                                   uint32_t DataLength);
```

---

## 💡 完整应用示例

### 示例1: 内存到内存传输

```c
DMA_HandleTypeDef hdma_memtomem;
uint32_t src_buffer[100];
uint32_t dst_buffer[100];

void DMA_MemToMem_Init(void) {
    __HAL_RCC_DMA1_CLK_ENABLE();
    
    hdma_memtomem.Instance = DMA1_Channel1;
    hdma_memtomem.Init.Direction = DMA_MEMORY_TO_MEMORY;
    hdma_memtomem.Init.PeriphInc = DMA_PINC_ENABLE;
    hdma_memtomem.Init.MemInc = DMA_MINC_ENABLE;
    hdma_memtomem.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_memtomem.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_memtomem.Init.Mode = DMA_NORMAL;
    hdma_memtomem.Init.Priority = DMA_PRIORITY_HIGH;
    
    HAL_DMA_Init(&hdma_memtomem);
}

void DMA_Transfer(void) {
    // 填充源数据
    for (int i = 0; i < 100; i++) {
        src_buffer[i] = i;
    }
    
    // 启动DMA传输
    HAL_DMA_Start(&hdma_memtomem, 
                  (uint32_t)src_buffer,
                  (uint32_t)dst_buffer,
                  100);
    
    // 等待传输完成
    HAL_DMA_PollForTransfer(&hdma_memtomem, HAL_DMA_FULL_TRANSFER, 1000);
}
```

---

### 示例2: UART DMA发送

```c
UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_tx;

void UART_DMA_Init(void) {
    // DMA初始化
    __HAL_RCC_DMA1_CLK_ENABLE();
    
    hdma_usart1_tx.Instance = DMA1_Channel4;
    hdma_usart1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart1_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart1_tx.Init.Mode = DMA_NORMAL;
    hdma_usart1_tx.Init.Priority = DMA_PRIORITY_LOW;
    HAL_DMA_Init(&hdma_usart1_tx);
    
    // 链接DMA到UART
    __HAL_LINKDMA(&huart1, hdmatx, hdma_usart1_tx);
    
    // 配置NVIC
    HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
}

// DMA中断服务函数
void DMA1_Channel4_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_usart1_tx);
}

// 使用DMA发送
uint8_t tx_data[] = "Hello DMA!\r\n";
HAL_UART_Transmit_DMA(&huart1, tx_data, sizeof(tx_data)-1);
```

---

### 示例3: ADC DMA连续采集

```c
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;
uint16_t adc_buffer[100];

void ADC_DMA_Init(void) {
    // DMA初始化
    __HAL_RCC_DMA1_CLK_ENABLE();
    
    hdma_adc1.Instance = DMA1_Channel1;
    hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;
    hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_adc1.Init.Mode = DMA_CIRCULAR;  // 循环模式
    hdma_adc1.Init.Priority = DMA_PRIORITY_HIGH;
    HAL_DMA_Init(&hdma_adc1);
    
    // 链接DMA到ADC
    __HAL_LINKDMA(&hadc1, DMA_Handle, hdma_adc1);
    
    // 启动ADC DMA
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, 100);
}

// DMA传输完成回调
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
    // 处理采集到的数据
    for (int i = 0; i < 100; i++) {
        printf("ADC[%d] = %d\n", i, adc_buffer[i]);
    }
}
```

---

## ⚠️ 注意事项

### 1. DMA通道分配

STM32F1的DMA通道是固定分配的,不能随意选择:

**DMA1通道分配:**
- Channel1: ADC1, TIM2_CH3, TIM4_CH1
- Channel2: SPI1_RX, USART3_TX, TIM1_CH1, TIM2_UP, TIM3_CH3
- Channel3: SPI1_TX, USART3_RX, TIM1_CH2, TIM3_CH4, TIM3_UP
- Channel4: SPI2_RX, USART1_TX, I2C2_TX, TIM1_CH4, TIM1_TRIG, TIM1_COM, TIM4_CH2
- Channel5: SPI2_TX, USART1_RX, I2C2_RX, TIM1_UP, TIM2_CH1, TIM4_CH3
- Channel6: USART2_RX, I2C1_TX, TIM1_CH3, TIM3_CH1, TIM3_TRIG
- Channel7: USART2_TX, I2C1_RX, TIM2_CH2, TIM2_CH4, TIM4_UP

### 2. 数据对齐

确保源和目标地址的数据对齐:
- BYTE: 任意地址
- HALFWORD: 2字节对齐
- WORD: 4字节对齐

### 3. 循环模式

循环模式下DMA会自动重新加载计数器,适合:
- ADC连续采集
- DAC连续输出
- 音频数据流

### 4. 优先级

多个DMA通道同时请求时,按优先级仲裁:
- 软件优先级相同时,通道号小的优先

### 5. 中断配置

使用DMA中断时必须:
- 配置NVIC
- 实现中断服务函数
- 在中断中调用`HAL_DMA_IRQHandler()`

---

## 📖 相关文档

- [HAL核心](HAL_Core.md)
- [UART串口](HAL_UART.md)
- [ADC模数转换](HAL_ADC.md)
- [SPI总线](HAL_SPI.md)
