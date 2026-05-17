# SPI 串行外设接口协议

## 概述

SPI（Serial Peripheral Interface，串行外设接口）是一种**同步、全双工**的串行通讯协议，由摩托罗拉公司提出。它使用主从架构，速度快，是单片机与外设通讯最常用的高速接口之一。

- 通讯方式：同步、全双工
- 所需引脚：MOSI、MISO、SCK、CS（每个从设备一根 CS）
- 典型速度：1 MHz ～ 100 MHz

---

## 引脚说明

| 引脚 | 全称 | 方向 | 说明 |
|------|------|------|------|
| SCK | Serial Clock | 主→从 | 时钟信号，由主机产生 |
| MOSI | Master Out Slave In | 主→从 | 主机发送，从机接收 |
| MISO | Master In Slave Out | 从→主 | 从机发送，主机接收 |
| CS/NSS | Chip Select / Slave Select | 主→从 | 片选，低电平有效，选中对应从机 |

---

## 硬件连接

### 单主单从

```
主机（MCU）              从机（Flash/传感器）
   SCK  ──────────────→  SCK
   MOSI ──────────────→  MOSI
   MISO ←──────────────  MISO
   CS   ──────────────→  CS
   GND  ───────────────  GND
```

### 单主多从（每个从机独立 CS）

```
主机（MCU）
   SCK  ──────┬──────────→ 从机1 SCK
   MOSI ──────┼──────────→ 从机1 MOSI
   MISO ──────┼──────────← 从机1 MISO
   CS1  ──────┘──────────→ 从机1 CS
              │
              ├──────────→ 从机2 SCK
              ├──────────→ 从机2 MOSI
              ├──────────← 从机2 MISO
   CS2  ──────┘──────────→ 从机2 CS
```

> 同一时刻只能有一个 CS 为低电平，其余从机 CS 保持高电平（未选中）。

---

## SPI 四种工作模式

SPI 通过 **CPOL**（时钟极性）和 **CPHA**（时钟相位）组合出 4 种模式：

| 模式 | CPOL | CPHA | 空闲时钟 | 采样时刻 |
|------|------|------|----------|----------|
| Mode 0 | 0 | 0 | 低电平 | 上升沿采样 |
| Mode 1 | 0 | 1 | 低电平 | 下降沿采样 |
| Mode 2 | 1 | 0 | 高电平 | 下降沿采样 |
| Mode 3 | 1 | 1 | 高电平 | 上升沿采样 |

> 最常用的是 **Mode 0** 和 **Mode 3**，使用前需查阅从机芯片手册确认支持的模式。

---

## 通讯过程详解（以 Mode 0 为例）

### 基本时序

```
CS:   ‾‾‾‾‾‾‾‾_________________________‾‾‾‾‾‾‾‾
              ↑片选拉低，通讯开始        ↑片选拉高，通讯结束

SCK:          _‾_‾_‾_‾_‾_‾_‾_‾
              ↑上升沿采样数据

MOSI:         ‾D7‾D6‾D5‾D4‾D3‾D2‾D1‾D0‾
              主机发送数据（MSB先发）

MISO:         ‾R7‾R6‾R5‾R4‾R3‾R2‾R1‾R0‾
              从机同时返回数据
```

### 通讯步骤

1. **主机拉低 CS**：选中目标从机，通讯开始
2. **主机产生 SCK 时钟**：每个时钟周期传输 1 位
3. **MOSI 发送数据**：主机在时钟边沿前准备好数据
4. **MISO 接收数据**：主机在采样边沿读取从机数据
5. **主机拉高 CS**：通讯结束，释放总线

> SPI 是全双工的，MOSI 和 MISO 同时传输，发送和接收同步进行。

---

## 完整通讯示例

**场景：** 单片机通过 SPI 读取 W25Q64 Flash 芯片的 JEDEC ID

### W25Q64 读 JEDEC ID 命令

发送命令 `0x9F`，Flash 返回 3 字节：制造商ID + 设备ID

### 时序过程

```
步骤1: 拉低 CS，选中 Flash
CS:   ‾‾‾‾‾‾‾‾_

步骤2: 发送命令字节 0x9F = 0b10011111（MSB先发）
MOSI: 1 0 0 1 1 1 1 1
MISO: x x x x x x x x  （此时 MISO 为无效数据）

步骤3: 发送3个哑字节 0x00，同时读取返回数据
MOSI: 0 0 0 0 0 0 0 0 | 0 0 0 0 0 0 0 0 | 0 0 0 0 0 0 0 0
MISO: [制造商ID:0xEF] | [设备ID高:0x40] | [设备ID低:0x17]

步骤4: 拉高 CS，通讯结束
CS:                                                        ‾
```

### 代码示例

```c
// SPI 读取 W25Q64 JEDEC ID
uint8_t W25Q64_ReadJEDECID(uint8_t *manufacturer, uint16_t *deviceID) {
    uint8_t id[3];

    CS_Low();                        // 拉低片选，选中芯片

    SPI_SendByte(0x9F);              // 发送 JEDEC ID 命令

    id[0] = SPI_SendByte(0x00);      // 发送哑字节，读取制造商ID
    id[1] = SPI_SendByte(0x00);      // 读取设备ID高字节
    id[2] = SPI_SendByte(0x00);      // 读取设备ID低字节

    CS_High();                       // 拉高片选，结束通讯

    *manufacturer = id[0];           // 0xEF = Winbond
    *deviceID = (id[1] << 8) | id[2]; // 0x4017 = W25Q64
    return 0;
}

// SPI 发送并接收一个字节（Mode 0）
uint8_t SPI_SendByte(uint8_t data) {
    uint8_t received = 0;
    for (int i = 7; i >= 0; i--) {
        // 准备 MOSI 数据
        MOSI_Write((data >> i) & 0x01);
        SCK_High();                  // 上升沿：从机采样 MOSI，主机采样 MISO
        received = (received << 1) | MISO_Read();
        SCK_Low();                   // 下降沿：准备下一位
    }
    return received;
}
```

---

## SPI 写操作示例

**场景：** 向 W25Q64 写入一页数据（Page Program）

```
1. 发送写使能命令 0x06
   CS低 → 发送0x06 → CS高

2. 发送页编程命令 0x02 + 24位地址 + 数据
   CS低 → 发送0x02 → 发送地址(3字节) → 发送数据(最多256字节) → CS高

3. 等待写入完成（轮询状态寄存器）
   CS低 → 发送0x05(读状态) → 读取状态字节，BUSY位=0则完成 → CS高
```

---

## 与 UART/I²C 对比

| 特性 | SPI | UART | I²C |
|------|-----|------|-----|
| 速度 | 最快（可达100MHz） | 较慢 | 中等（最高3.4MHz） |
| 引脚数 | 4根（+每从机1根CS） | 2根 | 2根 |
| 多从机 | 支持（每个需独立CS） | 不支持 | 支持（地址区分） |
| 全双工 | 是 | 是 | 否 |
| 硬件复杂度 | 中 | 低 | 低 |

---

## 典型应用

- SPI Flash 存储（W25Q64、GD25Q128）
- LCD/OLED 显示屏驱动
- ADC/DAC 芯片
- 无线模块（NRF24L01）
- SD 卡（SPI 模式）
