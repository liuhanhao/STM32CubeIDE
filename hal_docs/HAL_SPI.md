# SPI总线模块 API文档

## 📋 模块概述

SPI(Serial Peripheral Interface)串行外设接口是一种高速全双工同步通信协议,包括:
- 主从模式
- 全双工/半双工通信
- 可编程时钟极性和相位
- 可编程数据帧格式(8位/16位)
- 硬件NSS管理
- DMA传输支持
- 最高速度可达18Mbps(APB2/2)

---

## 🗺️ 使用场景速查

**SPI vs I2C 怎么选**

| 对比项 | SPI | I2C |
|--------|-----|-----|
| 速度 | 最高 18Mbps（F1） | 最高 400KHz |
| 引脚数 | 4 根（SCK/MOSI/MISO/CS） | 2 根（SCL/SDA） |
| 多设备 | 每个设备独立 CS，无地址概念 | 共享总线，靠地址区分 |
| 适合场景 | 高速、大数据量（Flash、屏幕、ADC） | 低速、多设备（传感器、配置寄存器） |

**你想做什么 → 用哪个函数/模式**

| 场景 | 推荐方案 |
|------|----------|
| 读写 SPI Flash（W25Q 系列） | 阻塞 `Transmit` / `Receive`，手动拉 CS |
| 驱动 SPI OLED / TFT LCD（少量命令） | 阻塞发送命令 + 数据 |
| 高速刷屏（大量像素数据） | DMA 发送 `HAL_SPI_Transmit_DMA`，刷屏期间 CPU 可做其他事 |
| 读取 SPI ADC（如 MCP3208） | 全双工 `TransmitReceive`，同时发命令收数据 |
| 两块 MCU 互联通信 | 一主一从，全双工模式 |

**SPI 模式（CPOL/CPHA）速查**

| 常见芯片 | SPI 模式 |
|----------|----------|
| W25Q 系列 Flash | Mode 0（CPOL=0，CPHA=0） |
| SSD1306 OLED | Mode 0 |
| MCP3208 ADC | Mode 0 |
| SD 卡（SPI 模式） | Mode 0 |
| MAX31855 热电偶 ADC | Mode 0 |

> 不确定时优先尝试 Mode 0，绝大多数 SPI 设备都兼容。

---

## 📚 相关文件

- **头文件**: `stm32f1xx_hal_spi.h`
- **源文件**: `stm32f1xx_hal_spi.c`

---

## 🔧 数据类型

### SPI_InitTypeDef - SPI初始化结构体

```c
typedef struct {
    uint32_t Mode;              // 主从模式
    uint32_t Direction;         // 传输方向
    uint32_t DataSize;          // 数据帧大小
    uint32_t CLKPolarity;       // 时钟极性
    uint32_t CLKPhase;          // 时钟相位
    uint32_t NSS;               // NSS信号管理
    uint32_t BaudRatePrescaler; // 波特率预分频
    uint32_t FirstBit;          // 数据传输顺序
    uint32_t TIMode;            // TI模式
    uint32_t CRCCalculation;    // CRC计算
    uint32_t CRCPolynomial;     // CRC多项式
} SPI_InitTypeDef;
```

---

## 📌 常量定义

### SPI模式

```c
SPI_MODE_MASTER  // 主机模式
SPI_MODE_SLAVE   // 从机模式
```

---

### 传输方向

```c
SPI_DIRECTION_2LINES        // 全双工
SPI_DIRECTION_2LINES_RXONLY // 只接收
SPI_DIRECTION_1LINE         // 半双工
```

---

### 数据帧大小

```c
SPI_DATASIZE_8BIT   // 8位数据帧
SPI_DATASIZE_16BIT  // 16位数据帧
```

---

### 时钟极性和相位

| 模式 | CPOL | CPHA | 说明 |
|------|------|------|------|
| Mode 0 | 0 | 0 | 空闲低电平,第一个边沿采样 |
| Mode 1 | 0 | 1 | 空闲低电平,第二个边沿采样 |
| Mode 2 | 1 | 0 | 空闲高电平,第一个边沿采样 |
| Mode 3 | 1 | 1 | 空闲高电平,第二个边沿采样 |

```c
SPI_POLARITY_LOW   // CPOL = 0
SPI_POLARITY_HIGH  // CPOL = 1
SPI_PHASE_1EDGE    // CPHA = 0
SPI_PHASE_2EDGE    // CPHA = 1
```

---

### 波特率预分频

```c
SPI_BAUDRATEPRESCALER_2    // 2分频
SPI_BAUDRATEPRESCALER_4    // 4分频
SPI_BAUDRATEPRESCALER_8    // 8分频
SPI_BAUDRATEPRESCALER_16   // 16分频
SPI_BAUDRATEPRESCALER_32   // 32分频
SPI_BAUDRATEPRESCALER_64   // 64分频
SPI_BAUDRATEPRESCALER_128  // 128分频
SPI_BAUDRATEPRESCALER_256  // 256分频
```

---

## 🎯 核心函数

### HAL_SPI_Init()

初始化SPI。

```c
HAL_StatusTypeDef HAL_SPI_Init(SPI_HandleTypeDef *hspi);
```

---

### HAL_SPI_Transmit()

发送数据(阻塞模式)。

```c
HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef *hspi, 
                                   uint8_t *pData, 
                                   uint16_t Size, 
                                   uint32_t Timeout);
```

---

### HAL_SPI_Receive()

接收数据(阻塞模式)。

```c
HAL_StatusTypeDef HAL_SPI_Receive(SPI_HandleTypeDef *hspi, 
                                  uint8_t *pData, 
                                  uint16_t Size, 
                                  uint32_t Timeout);
```

---

### HAL_SPI_TransmitReceive()

全双工收发(阻塞模式)。

```c
HAL_StatusTypeDef HAL_SPI_TransmitReceive(SPI_HandleTypeDef *hspi, 
                                          uint8_t *pTxData, 
                                          uint8_t *pRxData,
                                          uint16_t Size, 
                                          uint32_t Timeout);
```

---

### HAL_SPI_Transmit_IT()

发送数据(中断模式)。

```c
HAL_StatusTypeDef HAL_SPI_Transmit_IT(SPI_HandleTypeDef *hspi, 
                                      uint8_t *pData, 
                                      uint16_t Size);
```

---

### HAL_SPI_Transmit_DMA()

发送数据(DMA模式)。

```c
HAL_StatusTypeDef HAL_SPI_Transmit_DMA(SPI_HandleTypeDef *hspi, 
                                       uint8_t *pData, 
                                       uint16_t Size);
```

---

## 💡 完整应用示例

### 示例1: SPI初始化

```c
SPI_HandleTypeDef hspi1;

void SPI1_Init(void) {
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;  // 72MHz/8 = 9MHz
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    
    if (HAL_SPI_Init(&hspi1) != HAL_OK) {
        Error_Handler();
    }
}

// MSP初始化
void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    if (hspi->Instance == SPI1) {
        // 使能时钟
        __HAL_RCC_SPI1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        
        // PA5: SPI1_SCK, PA6: SPI1_MISO, PA7: SPI1_MOSI
        GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_7;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        
        GPIO_InitStruct.Pin = GPIO_PIN_6;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_INPUT;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        
        // PA4: SPI1_NSS (软件控制)
        GPIO_InitStruct.Pin = GPIO_PIN_4;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);  // NSS拉高
    }
}
```

---

### 示例2: 读写W25Q128 Flash

```c
#define W25Q_CS_LOW()   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET)
#define W25Q_CS_HIGH()  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET)

// 读取设备ID
uint16_t W25Q_ReadID(void) {
    uint8_t tx_data[4] = {0x90, 0x00, 0x00, 0x00};
    uint8_t rx_data[2];
    
    W25Q_CS_LOW();
    HAL_SPI_Transmit(&hspi1, tx_data, 4, 100);
    HAL_SPI_Receive(&hspi1, rx_data, 2, 100);
    W25Q_CS_HIGH();
    
    return (rx_data[0] << 8) | rx_data[1];
}

// 写使能
void W25Q_WriteEnable(void) {
    uint8_t cmd = 0x06;
    
    W25Q_CS_LOW();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
    W25Q_CS_HIGH();
}

// 扇区擦除
void W25Q_SectorErase(uint32_t addr) {
    uint8_t cmd[4];
    
    W25Q_WriteEnable();
    
    cmd[0] = 0x20;  // 扇区擦除命令
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;
    
    W25Q_CS_LOW();
    HAL_SPI_Transmit(&hspi1, cmd, 4, 100);
    W25Q_CS_HIGH();
    
    HAL_Delay(100);  // 等待擦除完成
}

// 页编程
void W25Q_PageProgram(uint32_t addr, uint8_t *data, uint16_t len) {
    uint8_t cmd[4];
    
    W25Q_WriteEnable();
    
    cmd[0] = 0x02;  // 页编程命令
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;
    
    W25Q_CS_LOW();
    HAL_SPI_Transmit(&hspi1, cmd, 4, 100);
    HAL_SPI_Transmit(&hspi1, data, len, 100);
    W25Q_CS_HIGH();
    
    HAL_Delay(5);  // 等待编程完成
}

// 读取数据
void W25Q_ReadData(uint32_t addr, uint8_t *data, uint16_t len) {
    uint8_t cmd[4];
    
    cmd[0] = 0x03;  // 读数据命令
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;
    
    W25Q_CS_LOW();
    HAL_SPI_Transmit(&hspi1, cmd, 4, 100);
    HAL_SPI_Receive(&hspi1, data, len, 100);
    W25Q_CS_HIGH();
}

// 使用示例
void W25Q_Test(void) {
    uint16_t id = W25Q_ReadID();
    printf("Flash ID: 0x%04X\n", id);
    
    uint8_t write_data[] = "Hello SPI Flash!";
    uint8_t read_data[20];
    
    // 擦除扇区
    W25Q_SectorErase(0x000000);
    
    // 写入数据
    W25Q_PageProgram(0x000000, write_data, sizeof(write_data));
    
    // 读取数据
    W25Q_ReadData(0x000000, read_data, sizeof(write_data));
    
    printf("读取: %s\n", read_data);
}
```

---

### 示例3: 驱动OLED显示屏(SSD1306)

```c
#define OLED_CS_LOW()   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET)
#define OLED_CS_HIGH()  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET)
#define OLED_DC_CMD()   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET)
#define OLED_DC_DATA()  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET)

// 写命令
void OLED_WriteCmd(uint8_t cmd) {
    OLED_DC_CMD();
    OLED_CS_LOW();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
    OLED_CS_HIGH();
}

// 写数据
void OLED_WriteData(uint8_t data) {
    OLED_DC_DATA();
    OLED_CS_LOW();
    HAL_SPI_Transmit(&hspi1, &data, 1, 100);
    OLED_CS_HIGH();
}

// 初始化OLED
void OLED_Init(void) {
    HAL_Delay(100);
    
    OLED_WriteCmd(0xAE);  // 关闭显示
    OLED_WriteCmd(0x20);  // 设置内存地址模式
    OLED_WriteCmd(0x10);  // 页地址模式
    OLED_WriteCmd(0xB0);  // 设置页起始地址
    OLED_WriteCmd(0xC8);  // 设置COM扫描方向
    OLED_WriteCmd(0x00);  // 设置低列地址
    OLED_WriteCmd(0x10);  // 设置高列地址
    OLED_WriteCmd(0x40);  // 设置起始行地址
    OLED_WriteCmd(0x81);  // 设置对比度
    OLED_WriteCmd(0xFF);
    OLED_WriteCmd(0xA1);  // 设置段重映射
    OLED_WriteCmd(0xA6);  // 正常显示
    OLED_WriteCmd(0xA8);  // 设置复用率
    OLED_WriteCmd(0x3F);
    OLED_WriteCmd(0xA4);  // 全局显示开启
    OLED_WriteCmd(0xD3);  // 设置显示偏移
    OLED_WriteCmd(0x00);
    OLED_WriteCmd(0xD5);  // 设置时钟分频
    OLED_WriteCmd(0xF0);
    OLED_WriteCmd(0xD9);  // 设置预充电周期
    OLED_WriteCmd(0x22);
    OLED_WriteCmd(0xDA);  // 设置COM引脚配置
    OLED_WriteCmd(0x12);
    OLED_WriteCmd(0xDB);  // 设置VCOMH电压
    OLED_WriteCmd(0x20);
    OLED_WriteCmd(0x8D);  // 使能电荷泵
    OLED_WriteCmd(0x14);
    OLED_WriteCmd(0xAF);  // 开启显示
}
```

---

## ⚠️ 注意事项

### 1. SPI时钟速度

SPI时钟由APB时钟分频得到:
- SPI1在APB2上: 最高72MHz
- SPI2/SPI3在APB1上: 最高36MHz

选择合适的分频系数以匹配从设备速度。

---

### 2. GPIO配置

SPI引脚配置:
- **SCK, MOSI**: 复用推挽输出 (`GPIO_MODE_AF_PP`)
- **MISO**: 浮空输入或上拉输入 (`GPIO_MODE_AF_INPUT`)
- **NSS**: 软件控制时配置为普通输出

---

### 3. NSS管理

软件NSS管理(推荐):
```c
hspi1.Init.NSS = SPI_NSS_SOFT;
// 手动控制CS引脚
```

硬件NSS管理:
```c
hspi1.Init.NSS = SPI_NSS_HARD_OUTPUT;
// 硬件自动控制NSS
```

---

### 4. 时钟极性和相位

根据从设备数据手册选择正确的CPOL和CPHA:
- 大多数设备使用Mode 0或Mode 3
- 配置错误会导致通信失败

---

### 5. 全双工通信

SPI是全双工的,发送和接收同时进行:
- 发送时会同时接收数据(可能是无效数据)
- 接收时需要发送时钟(通常发送0xFF)

---

## 📖 相关文档

- [HAL核心](HAL_Core.md)
- [GPIO通用IO](HAL_GPIO.md)
- [DMA直接内存访问](HAL_DMA.md)
