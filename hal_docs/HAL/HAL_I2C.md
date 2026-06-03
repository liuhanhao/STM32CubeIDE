# I2C总线模块 API文档

## 📋 模块概述

I2C(Inter-Integrated Circuit)总线是一种双线串行通信协议,包括:
- 主从模式通信
- 标准速度(100KHz)和快速模式(400KHz)
- 7位/10位设备地址
- 多主机支持
- DMA传输支持
- 硬件CRC校验

---

## 🗺️ 使用场景速查

**两类函数怎么选**

| 场景 | 推荐函数 | 说明 |
|------|----------|------|
| 读写带内部寄存器的设备（EEPROM、传感器） | `HAL_I2C_Mem_Write` / `Mem_Read` | 自动处理寄存器地址发送，最常用 |
| 自定义协议、无寄存器地址的设备 | `HAL_I2C_Master_Transmit` / `Receive` | 完全手动控制数据内容 |
| 上电检测设备是否存在 / 扫描地址 | `HAL_I2C_IsDeviceReady` | 遍历 1~127 找设备 |
| 大量数据传输，不想占用 CPU | `HAL_I2C_Mem_Write_DMA` / `Mem_Read_DMA` | DMA 模式减少等待 |

**常见 I2C 设备速查**

| 设备 | 地址（7位） | 推荐函数 | 注意点 |
|------|-----------|----------|--------|
| AT24C02 EEPROM | 0x50~0x57 | `Mem_Write` / `Mem_Read` | 每次写后必须延时 5ms |
| MPU6050 陀螺仪加速度计 | 0x68 / 0x69 | `Mem_Write` / `Mem_Read` | 上电后写 0x6B=0x00 唤醒 |
| SSD1306 OLED | 0x3C / 0x3D | `Master_Transmit` | 命令前加 0x00，数据前加 0x40 |
| BMP280 气压温度计 | 0x76 / 0x77 | `Mem_Read` | 需读取补偿参数做换算 |
| PCF8574 IO 扩展 | 0x20~0x27 | `Master_Transmit` / `Receive` | 直接收发 1 字节控制 8 路 IO |

**速度怎么选**

| 场景 | 推荐速度 |
|------|----------|
| 通用场景、长距离导线（>30cm）、多设备共线 | 标准模式 100KHz |
| 短距离、设备支持快速模式、需要更高吞吐 | 快速模式 400KHz |

> I2C 引脚必须配为开漏输出，外接 4.7kΩ 上拉到 3.3V，内部上拉驱动能力不足。

---

## 📚 相关文件

- **头文件**: `stm32f1xx_hal_i2c.h`
- **源文件**: `stm32f1xx_hal_i2c.c`

---

## 🔧 数据类型

### I2C_InitTypeDef - I2C初始化结构体

```c
typedef struct {
    uint32_t ClockSpeed;       // I2C时钟速度
    uint32_t DutyCycle;        // 占空比(快速模式)
    uint32_t OwnAddress1;      // 本机地址1
    uint32_t AddressingMode;   // 寻址模式
    uint32_t DualAddressMode;  // 双地址模式
    uint32_t OwnAddress2;      // 本机地址2
    uint32_t GeneralCallMode;  // 广播呼叫模式
    uint32_t NoStretchMode;    // 时钟延展模式
} I2C_InitTypeDef;
```

---

## 📌 常量定义

### I2C速度

```c
#define I2C_SPEED_STANDARD    100000  // 标准模式 100KHz
#define I2C_SPEED_FAST        400000  // 快速模式 400KHz
```

---

### 寻址模式

```c
I2C_ADDRESSINGMODE_7BIT   // 7位地址模式
I2C_ADDRESSINGMODE_10BIT  // 10位地址模式
```

---

## 🎯 核心函数

### HAL_I2C_Init()

初始化I2C。

```c
HAL_StatusTypeDef HAL_I2C_Init(I2C_HandleTypeDef *hi2c);
```

---

### HAL_I2C_Master_Transmit()

主机发送数据(阻塞模式)。

```c
HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef *hi2c, 
                                          uint16_t DevAddress,
                                          uint8_t *pData, 
                                          uint16_t Size, 
                                          uint32_t Timeout);
```

---

### HAL_I2C_Master_Receive()

主机接收数据(阻塞模式)。

```c
HAL_StatusTypeDef HAL_I2C_Master_Receive(I2C_HandleTypeDef *hi2c, 
                                         uint16_t DevAddress,
                                         uint8_t *pData, 
                                         uint16_t Size, 
                                         uint32_t Timeout);
```

---

### HAL_I2C_Mem_Write()

写入设备内存(阻塞模式)。

```c
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *hi2c, 
                                    uint16_t DevAddress,
                                    uint16_t MemAddress, 
                                    uint16_t MemAddSize,
                                    uint8_t *pData, 
                                    uint16_t Size, 
                                    uint32_t Timeout);
```

---

### HAL_I2C_Mem_Read()

读取设备内存(阻塞模式)。

```c
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *hi2c, 
                                   uint16_t DevAddress,
                                   uint16_t MemAddress, 
                                   uint16_t MemAddSize,
                                   uint8_t *pData, 
                                   uint16_t Size, 
                                   uint32_t Timeout);
```

---

### HAL_I2C_IsDeviceReady()

检测设备是否就绪。

```c
HAL_StatusTypeDef HAL_I2C_IsDeviceReady(I2C_HandleTypeDef *hi2c, 
                                        uint16_t DevAddress,
                                        uint32_t Trials, 
                                        uint32_t Timeout);
```

---

## 💡 完整应用示例

### 示例1: I2C初始化

```c
I2C_HandleTypeDef hi2c1;

void I2C1_Init(void) {
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;  // 100KHz
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        Error_Handler();
    }
}

// MSP初始化
void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    if (hi2c->Instance == I2C1) {
        // 使能时钟
        __HAL_RCC_I2C1_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        
        // PB6: I2C1_SCL, PB7: I2C1_SDA
        GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;  // 开漏输出
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }
}
```

---

### 示例2: 读写EEPROM (AT24C02)

```c
#define EEPROM_ADDR  0xA0  // EEPROM地址(7位地址<<1)

// 写入单个字节
HAL_StatusTypeDef EEPROM_WriteByte(uint8_t mem_addr, uint8_t data) {
    HAL_StatusTypeDef status;
    status = HAL_I2C_Mem_Write(&hi2c1, EEPROM_ADDR, mem_addr, 
                               I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
    HAL_Delay(5);  // EEPROM写入延时
    return status;
}

// 读取单个字节
HAL_StatusTypeDef EEPROM_ReadByte(uint8_t mem_addr, uint8_t *data) {
    return HAL_I2C_Mem_Read(&hi2c1, EEPROM_ADDR, mem_addr, 
                            I2C_MEMADD_SIZE_8BIT, data, 1, 100);
}

// 写入多个字节
HAL_StatusTypeDef EEPROM_WriteBytes(uint8_t mem_addr, uint8_t *data, uint16_t len) {
    HAL_StatusTypeDef status;
    status = HAL_I2C_Mem_Write(&hi2c1, EEPROM_ADDR, mem_addr, 
                               I2C_MEMADD_SIZE_8BIT, data, len, 100);
    HAL_Delay(5);
    return status;
}

// 读取多个字节
HAL_StatusTypeDef EEPROM_ReadBytes(uint8_t mem_addr, uint8_t *data, uint16_t len) {
    return HAL_I2C_Mem_Read(&hi2c1, EEPROM_ADDR, mem_addr, 
                            I2C_MEMADD_SIZE_8BIT, data, len, 100);
}

// 使用示例
void EEPROM_Test(void) {
    uint8_t write_data[] = "Hello I2C!";
    uint8_t read_data[20];
    
    // 写入数据
    EEPROM_WriteBytes(0, write_data, sizeof(write_data));
    
    // 读取数据
    EEPROM_ReadBytes(0, read_data, sizeof(write_data));
    
    printf("读取: %s\n", read_data);
}
```

---

### 示例3: 扫描I2C设备

```c
void I2C_Scan(void) {
    printf("扫描I2C总线...\n");
    
    for (uint8_t addr = 1; addr < 128; addr++) {
        if (HAL_I2C_IsDeviceReady(&hi2c1, addr << 1, 1, 10) == HAL_OK) {
            printf("发现设备: 0x%02X\n", addr);
        }
    }
    
    printf("扫描完成\n");
}
```

---

### 示例4: 读取MPU6050传感器

```c
#define MPU6050_ADDR  0xD0  // MPU6050地址

// 读取寄存器
uint8_t MPU6050_ReadReg(uint8_t reg) {
    uint8_t data;
    HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, reg, 
                     I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
    return data;
}

// 写入寄存器
void MPU6050_WriteReg(uint8_t reg, uint8_t data) {
    HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, reg, 
                      I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
}

// 初始化MPU6050
void MPU6050_Init(void) {
    // 检测设备
    if (HAL_I2C_IsDeviceReady(&hi2c1, MPU6050_ADDR, 3, 100) != HAL_OK) {
        printf("MPU6050未找到!\n");
        return;
    }
    
    // 复位设备
    MPU6050_WriteReg(0x6B, 0x80);
    HAL_Delay(100);
    
    // 唤醒设备
    MPU6050_WriteReg(0x6B, 0x00);
    
    // 配置陀螺仪量程 ±250°/s
    MPU6050_WriteReg(0x1B, 0x00);
    
    // 配置加速度计量程 ±2g
    MPU6050_WriteReg(0x1C, 0x00);
    
    printf("MPU6050初始化完成\n");
}

// 读取加速度数据
void MPU6050_ReadAccel(int16_t *ax, int16_t *ay, int16_t *az) {
    uint8_t data[6];
    HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, 0x3B, 
                     I2C_MEMADD_SIZE_8BIT, data, 6, 100);
    
    *ax = (int16_t)((data[0] << 8) | data[1]);
    *ay = (int16_t)((data[2] << 8) | data[3]);
    *az = (int16_t)((data[4] << 8) | data[5]);
}
```

---

## ⚠️ 注意事项

### 1. 上拉电阻

I2C总线需要外部上拉电阻(通常4.7KΩ):
- SCL和SDA都需要上拉到VCC
- STM32内部上拉电阻不够强,必须使用外部上拉

---

### 2. GPIO配置

I2C引脚必须配置为开漏输出:

```c
GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;  // 开漏输出
```

---

### 3. 设备地址

I2C设备地址分为7位和10位:
- 7位地址: 实际传输时左移1位(最低位为R/W位)
- 例如: 设备地址0x50 → 传输地址0xA0(写) / 0xA1(读)

---

### 4. 时钟速度

I2C时钟速度受APB1时钟限制:
- 标准模式: 100KHz
- 快速模式: 400KHz
- 确保APB1时钟足够高

---

### 5. 超时处理

I2C通信可能因总线占用而超时,建议:
- 设置合理的超时时间
- 检查返回值并处理错误
- 必要时复位I2C外设

---

## 📖 相关文档

- [HAL核心](HAL_Core.md)
- [GPIO通用IO](HAL_GPIO.md)
- [DMA直接内存访问](HAL_DMA.md)
