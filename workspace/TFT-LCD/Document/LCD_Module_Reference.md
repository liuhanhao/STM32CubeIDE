# TFT-LCD 触摸屏模块参考文档

> 本文档整理自购买的 2.4 寸 TFT-LCD 触摸屏模块的所有原始资料，供开发时查阅。

---

## 目录

1. [模块总览](#1-模块总览)
2. [LCD 裸屏规格](#2-lcd-裸屏规格)
3. [ILI9341 LCD 控制器](#3-ili9341-lcd-控制器)
4. [TSC2046 触摸控制器](#4-tsc2046-触摸控制器)
5. [ME6206 LDO 稳压器](#5-me6206-ldo-稳压器)
6. [模块原理图说明](#6-模块原理图说明)
7. [编程快速参考](#7-编程快速参考)

---

## 1. 模块总览

本模块为 **2.4 寸 TFT-LCD 触摸屏模块**，集成以下芯片/组件：

| 组件 | 型号 | 功能 |
|------|------|------|
| LCD 裸屏 | H24TM84A (FPC240A8230) | 2.4寸 240×320 TFT 屏 |
| LCD 控制器 | ILI9341V | SPI/并口驱动，内置显存 |
| 触摸控制器 | ADS7843 / XPT2046 (TSC2046 兼容) | 4线电阻式触摸，SPI接口 |
| SD 卡座 | 标准 MicroSD/TF 卡 | SPI 接口 |
| 电源稳压 | ME6206A33 | 3.3V LDO |

### 模块接口引脚（18-PIN FPC）

| 引脚 | 信号名 | 方向 | 说明 |
|------|--------|------|------|
| 1 | GND | - | 电源地 |
| 2 | /RESET | 输入 | LCD 复位，低有效 |
| 3 | SCL | 输入 | SPI 时钟 |
| 4 | RS (D/C) | 输入 | 数据/命令选择（1=数据，0=命令） |
| 5 | CS/ | 输入 | 片选，低有效 |
| 6 | SDA | 输入 | SPI 数据输入（MOSI） |
| 7 | SDO | 输出 | SPI 数据输出（MISO） |
| 8 | GND | - | 电源地 |
| 9 | VDD | 电源 | 模拟电源（2.6~3.0V） |
| 10 | LEDA | - | LED 阳极（背光正极） |
| 11~14 | K1~K4 | - | LED 阴极（背光负极） |
| 15 | XL | 触摸 | 触摸屏 X 左端 |
| 16 | YU | 触摸 | 触摸屏 Y 上端 |
| 17 | XR | 触摸 | 触摸屏 X 右端 |
| 18 | YD | 触摸 | 触摸屏 Y 下端 |

---

## 2. LCD 裸屏规格

**型号：** H24TM84A（Ver. A）  
**制造商：** SHENZHEN HANYUMODERN ELECTRONICS CO.,LTD.

### 基本参数

| 参数 | 值 | 单位 |
|------|-----|------|
| 显示类型 | TFT Negative Transmissive | - |
| 分辨率 | 240(RGB) × 320 | dots |
| 像素排列 | RGB 垂直条纹 | - |
| 像素点距 | 0.153(W) × 0.153(H) | μm |
| 显示面积 | 36.72(W) × 48.96(H) | mm |
| 模块尺寸 | 42.72(W) × 60.26(H) × 2.30(T) | mm |
| 视角方向 | 6 O'clock | - |
| 驱动 IC | ILI9341V | - |
| 背光 | 白色 LED，If=60mA | - |
| 颜色数 | 65K / 262K 色 | - |

### 绝对最大额定值

| 参数 | 符号 | 最小 | 最大 | 单位 |
|------|------|------|------|------|
| 逻辑电源电压 | V_DD | -0.3 | 3.0 | V |
| 逻辑输入电压 | V_IN | -0.5 | V_DD+0.3 | V |
| 单颗 LED 电流 | I_LED | - | 20 | mA |
| 工作温度 | T_OP | -20 | +70 | °C |
| 存储温度 | T_ST | -30 | +80 | °C |

### 电气特性

| 参数 | 符号 | 最小 | 典型 | 最大 | 单位 |
|------|------|------|------|------|------|
| 逻辑电源电压 | V_DD | 2.6 | 2.8 | 3.0 | V |
| LED 正向电压 | V_f | 3.0 | 3.2 | 3.3 | V |
| 背光电流（单串） | I_LED | - | 60 | - | mA |

### 光学特性

| 参数 | 符号 | 最小 | 典型 | 单位 |
|------|------|------|------|------|
| 亮度 | B | - | 150 | cd/m² |
| 对比度 | CR | 100 | 120 | - |
| 响应时间 | Tr+Tf | - | 25 | ms |
| 视角（水平） | - | 40 | 45 | deg |
| 视角（垂直） | - | 30 | 35 | deg |
| 均匀性 | Un | 80 | 85 | % |

---

## 3. ILI9341 LCD 控制器

**版本：** V1.09  
**制造商：** ILI Technology Corp.

### 功能概述

ILI9341 是 a-Si TFT LCD 单芯片 SOC 驱动器，主要特性：

- 分辨率：240(RGB) × 320
- 片内显存：172,800 字节（240×320×18bit）
- 支持 262,144 色（18bit）/ 65,536 色（16bit）
- 接口：8/9/16/18 位并口（8080 I/II 系列）、3线/4线 SPI
- 工作电压：VDDI = 1.65V~3.3V；VCI = 2.5V~3.3V
- 工作温度：-40°C ~ +85°C

### 接口选择（IM[3:0] 引脚）

| IM3 | IM2 | IM1 | IM0 | 接口模式 |
|-----|-----|-----|-----|---------|
| 0 | 0 | 0 | 0 | 8080 8位并口 I |
| 0 | 0 | 0 | 1 | 8080 16位并口 I |
| 0 | 0 | 1 | 0 | 8080 9位并口 I |
| 0 | 0 | 1 | 1 | 8080 18位并口 I |
| **0** | **1** | **0** | **1** | **3线 9位 SPI I** |
| **0** | **1** | **1** | **0** | **4线 8位 SPI I（最常用）** |
| 1 | 1 | 0 | 1 | 3线 9位 SPI II |
| 1 | 1 | 1 | 0 | 4线 8位 SPI II |

> **本模块使用 4 线 SPI 接口（IM=0110）**

### SPI 接口信号说明

| 信号 | 说明 |
|------|------|
| CSX | 片选，低有效 |
| SCL | SPI 时钟（D/CX 在3线模式下即为时钟） |
| SDA（SDI） | 数据输入（MOSI），上升沿采样 |
| SDO | 数据输出（MISO），下降沿更新 |
| D/CX | 数据/命令选择：1=数据，0=命令 |
| RESX | 硬件复位，低有效，复位脉冲 ≥ 10μs |

### 4 线 SPI 时序

- **写操作：** CS 拉低 → 发送 8bit 数据，D/CX 区分数据/命令 → CS 拉高
- **读操作：** 发命令后，第一个字节为 dummy，之后读取有效数据
- 最小时钟周期（写）：100ns；最小时钟周期（读）：150ns
- 数据在 SCL 上升沿被采样，MSB 先发

### 上电初始化序列

```
1. 硬件复位（RESX 低脉冲 ≥ 10μs）
2. 等待 ≥ 5ms
3. 发送 Sleep Out 命令（0x11）
4. 等待 ≥ 120ms（等待内部振荡器稳定）
5. 配置各寄存器（颜色格式、方向、Gamma等）
6. 发送 Display ON 命令（0x29）
```

### 常用命令速查表

#### 基础控制命令

| 命令 | 十六进制 | 说明 |
|------|---------|------|
| NOP | 0x00 | 空操作 |
| Software Reset | 0x01 | 软件复位，等待 5ms 后可发命令 |
| Sleep Mode IN | 0x10 | 进入睡眠，低功耗 |
| Sleep Mode OUT | 0x11 | 退出睡眠，等待 120ms |
| Partial Mode ON | 0x12 | 开启局部显示模式 |
| Normal Display Mode ON | 0x13 | 回到正常全屏模式 |
| Display Inversion OFF | 0x20 | 关闭反色 |
| Display Inversion ON | 0x21 | 开启反色 |
| Gamma Set | 0x26 | 选择 Gamma 曲线（参数 0x01 = GC0） |
| Display OFF | 0x28 | 关闭显示（不影响帧缓存） |
| Display ON | 0x29 | 开启显示 |
| Idle Mode OFF | 0x38 | 关闭 Idle 模式（全色） |
| Idle Mode ON | 0x39 | 开启 Idle 模式（8色） |

#### 地址与存储器命令

| 命令 | 十六进制 | 参数 | 说明 |
|------|---------|------|------|
| Column Address Set | 0x2A | SC[15:8], SC[7:0], EC[15:8], EC[7:0] | 设置列地址范围（X方向） |
| Page Address Set | 0x2B | SP[15:8], SP[7:0], EP[15:8], EP[7:0] | 设置行地址范围（Y方向） |
| Memory Write | 0x2C | 连续像素数据 | 写像素数据到帧缓存 |
| Memory Read | 0x2E | - | 从帧缓存读取像素 |
| Pixel Format Set | 0x3A | DPI[2:0] RGB, DBI[2:0] MCU | 设置颜色深度 |
| Memory Access Control | 0x36 | MY,MX,MV,ML,BGR,MH | 控制显示方向/BGR顺序 |

#### Pixel Format Set (0x3A) 参数

| DBI[2:0] | MCU 接口颜色深度 |
|----------|----------------|
| 101 | 16 bit/pixel（RGB565，常用）|
| 110 | 18 bit/pixel（RGB666）|

#### Memory Access Control (0x36) 位说明

| 位 | 名称 | 0 | 1 |
|----|------|---|---|
| D7 | MY | 行地址从上到下 | 行地址从下到上 |
| D6 | MX | 列地址从左到右 | 列地址从右到左 |
| D5 | MV | 正常（列优先） | 行列交换（转置） |
| D4 | ML | 从上往下刷新 | 从下往上刷新 |
| D3 | BGR | RGB 顺序 | BGR 顺序 |
| D2 | MH | 从左往右刷新 | 从右往左刷新 |

> 通常初始化时设 BGR=1（0x08）以匹配面板物理排列。

### 绘图流程

```
// 设置画点区域
发送命令 0x2A（CASET）
发送参数 x_start >> 8, x_start & 0xFF, x_end >> 8, x_end & 0xFF

发送命令 0x2B（PASET）
发送参数 y_start >> 8, y_start & 0xFF, y_end >> 8, y_end & 0xFF

// 写像素数据
发送命令 0x2C（RAMWR）
发送像素数据（RGB565: 每像素 2 字节）
```

### 重要扩展命令（需要 EXTC 引脚拉高）

| 命令 | 十六进制 | 说明 |
|------|---------|------|
| Power Control A | 0xCB | 电源控制A |
| Power Control B | 0xCF | 电源控制B |
| Driver Timing Control A | 0xE8 | 驱动时序控制A |
| Driver Timing Control B | 0xEA | 驱动时序控制B |
| Power On Sequence Control | 0xED | 上电时序控制 |
| Enable 3G | 0xF2 | 使能3Gamma |
| Pump Ratio Control | 0xF7 | 泵比率控制 |
| Frame Rate Control (Normal) | 0xB1 | 帧率控制（正常模式） |
| Display Function Control | 0xB6 | 显示功能控制 |
| Power Control 1 | 0xC0 | 电源控制1（GVDD电压） |
| Power Control 2 | 0xC1 | 电源控制2（VGH/VGL倍增） |
| VCOM Control 1 | 0xC5 | VCOM 高低电压设置 |
| VCOM Control 2 | 0xC7 | VCOM 偏移调整 |
| Positive Gamma Correction | 0xE0 | 正 Gamma 校正（15参数） |
| Negative Gamma Correction | 0xE1 | 负 Gamma 校正（15参数） |
| Read ID4 | 0xD3 | 读芯片ID（返回 0x009341） |

### 典型初始化代码示例（SPI 4线模式）

```c
// 硬件复位
HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_RESET);
HAL_Delay(10);
HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET);
HAL_Delay(120);

// 以下命令发送格式：LCD_WriteCmd(cmd); LCD_WriteData(data);

LCD_WriteCmd(0xCF); LCD_WriteData(0x00); LCD_WriteData(0x83); LCD_WriteData(0x30); // Power B
LCD_WriteCmd(0xED); LCD_WriteData(0x64); LCD_WriteData(0x03); LCD_WriteData(0x12); LCD_WriteData(0x81); // Power on seq
LCD_WriteCmd(0xE8); LCD_WriteData(0x85); LCD_WriteData(0x01); LCD_WriteData(0x79); // Timing A
LCD_WriteCmd(0xCB); LCD_WriteData(0x39); LCD_WriteData(0x2C); LCD_WriteData(0x00); LCD_WriteData(0x34); LCD_WriteData(0x02); // Power A
LCD_WriteCmd(0xF7); LCD_WriteData(0x20); // Pump ratio
LCD_WriteCmd(0xEA); LCD_WriteData(0x00); LCD_WriteData(0x00); // Timing B

LCD_WriteCmd(0xC0); LCD_WriteData(0x26); // Power control 1, GVDD=4.75V
LCD_WriteCmd(0xC1); LCD_WriteData(0x11); // Power control 2
LCD_WriteCmd(0xC5); LCD_WriteData(0x35); LCD_WriteData(0x3E); // VCOM control 1
LCD_WriteCmd(0xC7); LCD_WriteData(0xBE); // VCOM control 2

LCD_WriteCmd(0x36); LCD_WriteData(0x48); // Memory access: MX=1, BGR=1（横屏可调）
LCD_WriteCmd(0x3A); LCD_WriteData(0x55); // 16-bit/pixel (RGB565)

LCD_WriteCmd(0xB1); LCD_WriteData(0x00); LCD_WriteData(0x1B); // Frame rate 70Hz
LCD_WriteCmd(0xB6); LCD_WriteData(0x0A); LCD_WriteData(0x82); LCD_WriteData(0x27); LCD_WriteData(0x00); // Display function

LCD_WriteCmd(0x26); LCD_WriteData(0x01); // Gamma curve 1
LCD_WriteCmd(0xE0); // Positive gamma correction (15 params)
LCD_WriteData(0x0F); LCD_WriteData(0x31); LCD_WriteData(0x2B); LCD_WriteData(0x0C);
LCD_WriteData(0x0E); LCD_WriteData(0x08); LCD_WriteData(0x4E); LCD_WriteData(0xF1);
LCD_WriteData(0x37); LCD_WriteData(0x07); LCD_WriteData(0x10); LCD_WriteData(0x03);
LCD_WriteData(0x0E); LCD_WriteData(0x09); LCD_WriteData(0x00);
LCD_WriteCmd(0xE1); // Negative gamma correction (15 params)
LCD_WriteData(0x00); LCD_WriteData(0x0E); LCD_WriteData(0x14); LCD_WriteData(0x03);
LCD_WriteData(0x11); LCD_WriteData(0x07); LCD_WriteData(0x31); LCD_WriteData(0xC1);
LCD_WriteData(0x48); LCD_WriteData(0x08); LCD_WriteData(0x0F); LCD_WriteData(0x0C);
LCD_WriteData(0x31); LCD_WriteData(0x36); LCD_WriteData(0x0F);

LCD_WriteCmd(0x11); // Sleep OUT
HAL_Delay(120);
LCD_WriteCmd(0x29); // Display ON
```

### AC 时序参数（SPI 模式）

| 参数 | 符号 | 最小值 | 单位 |
|------|------|--------|------|
| SPI 时钟周期（写） | tscycw | 100 | ns |
| SCL 高电平宽度（写） | tshw | 40 | ns |
| SCL 低电平宽度（写） | tslw | 40 | ns |
| SPI 时钟周期（读） | tscycr | 150 | ns |
| 数据建立时间 | tsds | 30 | ns |
| 数据保持时间 | tsdh | 30 | ns |
| CS-SCL 时间 | tcss | 60 | ns |

---

## 4. TSC2046 触摸控制器

**型号：** TSC2046（100% 兼容 ADS7846，本模块使用 XPT2046）  
**制造商：** Texas Instruments

### 主要特性

- 支持 2.2V ~ 5.25V 工作（数字 I/O 1.5V ~ 5.25V）
- 内置 2.5V 参考电压
- 12 位 ADC 精度，分辨率约 0.3°C
- 支持 4 线电阻式触摸屏
- SPI / QSPI 接口（最高 125kHz 采样率）
- 工作温度：-40°C ~ +85°C
- 功耗极低：< 0.75mW（2.7V，ref off）
- 支持笔中断（PENIRQ）

### 引脚说明（TSSOP-16）

| 引脚号 | 名称 | 说明 |
|--------|------|------|
| 1 | +VCC | 电源（2.7V ~ 5.25V） |
| 2 | X+ | 触摸屏 X+ 端 |
| 3 | Y+ | 触摸屏 Y+ 端 |
| 4 | X- | 触摸屏 X- 端 |
| 5 | Y- | 触摸屏 Y- 端 |
| 6 | GND | 地 |
| 7 | VBAT | 电池监测输入（0~6V） |
| 8 | AUX | 辅助 ADC 输入 |
| 9 | VREF | 参考电压输入/输出 |
| 10 | IOVDD | 数字 I/O 电源 |
| 11 | PENIRQ | 笔触中断输出（低有效） |
| 12 | DOUT | SPI 数据输出 |
| 13 | BUSY | 转换忙标志 |
| 14 | DIN | SPI 数据输入 |
| 15 | CS | SPI 片选（低有效） |
| 16 | DCLK | SPI 时钟 |

### SPI 接口时序

- **时钟频率：** 最高 2MHz（fCLK = 16 × fSAMPLE，最高 125kHz 采样率）
- **数据格式：** MSB 先发
- **通信帧：** 每次完整转换需 24 个时钟（发8位控制字，读12位结果）

### 控制字格式（8bit，发送到 DIN）

| 位 | 名称 | 说明 |
|----|------|------|
| 7（MSB）| S | 起始位，必须为 1 |
| 6-4 | A2:A0 | 通道选择 |
| 3 | MODE | 0=12bit，1=8bit |
| 2 | SER/DFR | 0=差分模式（推荐），1=单端模式 |
| 1-0 | PD1:PD0 | 省电控制 |

### 通道选择（A2:A0）

| A2:A0 | 测量内容（差分模式，SER/DFR=0）|
|-------|-------------------------------|
| 001 | Y 坐标（Y+ 驱动，测 X+） |
| 101 | X 坐标（X+ 驱动，测 Y+） |
| 011 | Z1 压力测量 |
| 100 | Z2 压力测量 |

### 典型控制字

```c
#define TSC2046_READ_X  0xD0  // 1101 0000: S=1, A=101(X), MODE=0(12bit), SER/DFR=0, PD=00
#define TSC2046_READ_Y  0x90  // 1001 0000: S=1, A=001(Y), MODE=0(12bit), SER/DFR=0, PD=00
#define TSC2046_READ_Z1 0xB0  // 1011 0000: Z1
#define TSC2046_READ_Z2 0xC0  // 1100 0000: Z2
```

### 通信时序（24 时钟模式）

```
1. CS 拉低
2. 发送 8 位控制字（DIN）
   （此时 TSC2046 进行采样，3个时钟内完成）
3. 继续发送 8 个时钟（接收高 8 位结果从 DOUT）
4. 再发送 8 个时钟（接收低 4 位结果，然后是 4 个 0）
5. CS 拉高（或继续发下一次转换控制字）

12位结果 = (DOUT[byte1] << 5) | (DOUT[byte2] >> 3)
```

### 电气特性摘要

| 参数 | 条件 | 最小 | 典型 | 最大 | 单位 |
|------|------|------|------|------|------|
| 电源电压 VCC | 额定 | 2.7 | - | 5.25 | V |
| 数字 I/O IOVDD | - | 1.5 | - | VCC | V |
| 分辨率 | - | - | 12 | - | bits |
| 积分线性误差 | - | - | - | ±2 | LSB |
| 采样率 | - | - | - | 125 | kHz |
| 转换时间 | - | - | 12 CLK | - | cycles |
| 内部参考电压 | - | 2.45 | 2.50 | 2.55 | V |
| 静态电流 | ref off | - | 280 | 650 | μA |
| 掉电电流 | CS=DCLK=DIN=IOVDD | - | - | 3 | μA |

### PENIRQ 中断机制

- 当 PD1:PD0 = 00（省电模式）时，Y- 驱动接地，X+ 通过内部 50kΩ 上拉连到 PENIRQ
- 触屏被按下时，PENIRQ 被拉低，触发外部中断
- 建议在 MCU 发送控制字前屏蔽 PENIRQ 中断，防止误触发

### 压力检测

触摸压力可用以下公式计算（用于判断是否真正触摸）：

```
R_touch = R_Xplate × (X_pos/4096) × (Z2/Z1 - 1)
```

R_touch 值越小表示按压力越大，一般设阈值 < 200Ω 认为有效触摸。

---

## 5. ME6206 LDO 稳压器

**型号：** ME6206A33（3.3V 输出版本）  
**制造商：** 南京微盟电子（Nanjing Micro One Electronics）

### 主要特性

- 高精度：±2%
- 输出电压：1.5V ~ 5.0V（按型号，本模块为 3.3V）
- 低静态电流：8μA（典型）
- 最大输出电流：300mA
- 输入电压：最高 6V
- 压差：100mA 时 0.2V，200mA 时 0.4V
- 封装：SOT23-3, SOT89-3, TO-92

### 引脚定义（SOT23-3）

| 引脚 | 名称 | 功能 |
|------|------|------|
| 1 | VSS | 地 |
| 2 | VOUT | 输出（3.3V） |
| 3 | VIN | 输入 |

### 电气参数（ME6206A33）

| 参数 | 条件 | 典型值 | 单位 |
|------|------|--------|------|
| 输出电压 | IOUT=10mA, VIN=4.3V | 3.3V | V |
| 最大输出电流 | - | 300 | mA |
| 负载调整率 | 1mA≤IOUT≤100mA | 14 | mV |
| 压差 | IOUT=80mA | 180 | mV |
| 压差 | IOUT=200mA | 380 | mV |
| 静态电流 | VIN=4.3V | 8 | μA |
| 线路调整率 | IOUT=40mA | 0.03 | %/V |
| 电源纹波抑制 | f=1kHz | 50 | dB |
| 过流保护 | - | 500 | mA |

---

## 6. 模块原理图说明

根据原理图（sch模块原理图.pdf），模块主要连接关系如下：

### 电源部分
- 外部输入 3.3V（或通过 ME6206 从 5V 稳压至 3.3V）
- LCD VCC = 3.3V，背光 LED 串联 3.9Ω 限流电阻

### TFT LCD 接口（SPI）

```
STM32/MCU        LCD Module
─────────────────────────────
任意 GPIO   →   CS（TFT_CS）     # 片选
任意 GPIO   →   RS/D/C（TFT_D/C）# 数据/命令
SPI_SCK     →   SCK（TFT_SCK）  # SPI 时钟
SPI_MOSI    →   SDI（TFT_SDI）  # 数据写入
SPI_MISO    ←   SDO（TFT_SDO）  # 数据读出（可选）
任意 GPIO   →   RST（REST）     # 复位
```

### 触摸屏接口（SPI，与 ADS7843/XPT2046 兼容）

```
STM32/MCU        Touch Controller
───────────────────────────────────
任意 GPIO   →   CS（T_CS）      # 片选
SPI_SCK     →   CLK（T_CLK）   # SPI 时钟
SPI_MOSI    →   DIN（T_DIN）   # 数据写入
SPI_MISO    ←   DOUT（T_OUT）  # 数据读出
任意 GPIO   ←   IRQ（T_IRQ）   # 触摸中断（低有效）

触摸屏四线：X+, Y+, X-, Y- 直接连到 TSC2046 对应引脚
```

### SD 卡接口（SPI）

```
STM32/MCU        SD Card
──────────────────────────
任意 GPIO   →   CS（SD_CS）
SPI_SCK     →   CLK（SD_SCK）
SPI_MOSI    →   MOSI（SD_MOSI）
SPI_MISO    ←   MISO（SD_MISO）
```

> **注意：** LCD、触摸屏、SD 卡共享同一 SPI 总线（SCK/MOSI/MISO），通过各自的 CS 引脚分时复用。

### SDO 电平转换

原理图中 TFT_SDO 通过 1kΩ 电阻连接，防止总线冲突。SD 卡的 MISO 也通过电阻隔离。

---

## 7. 编程快速参考

### STM32 HAL 库 SPI 操作封装示例

```c
/* LCD 基本操作 */

// 写命令
void LCD_WriteCmd(uint8_t cmd) {
    HAL_GPIO_WritePin(LCD_DC_GPIO, LCD_DC_PIN, GPIO_PIN_RESET); // D/C=0，命令
    HAL_GPIO_WritePin(LCD_CS_GPIO, LCD_CS_PIN, GPIO_PIN_RESET); // CS=0
    HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(LCD_CS_GPIO, LCD_CS_PIN, GPIO_PIN_SET);   // CS=1
}

// 写数据（单字节）
void LCD_WriteData8(uint8_t data) {
    HAL_GPIO_WritePin(LCD_DC_GPIO, LCD_DC_PIN, GPIO_PIN_SET);   // D/C=1，数据
    HAL_GPIO_WritePin(LCD_CS_GPIO, LCD_CS_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, &data, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(LCD_CS_GPIO, LCD_CS_PIN, GPIO_PIN_SET);
}

// 写 16bit 颜色数据
void LCD_WriteColor(uint16_t color) {
    uint8_t buf[2] = {color >> 8, color & 0xFF};
    HAL_GPIO_WritePin(LCD_DC_GPIO, LCD_DC_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_CS_GPIO, LCD_CS_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, buf, 2, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(LCD_CS_GPIO, LCD_CS_PIN, GPIO_PIN_SET);
}

// 设置绘图窗口
void LCD_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    LCD_WriteCmd(0x2A);                  // CASET
    LCD_WriteData8(x0 >> 8);
    LCD_WriteData8(x0 & 0xFF);
    LCD_WriteData8(x1 >> 8);
    LCD_WriteData8(x1 & 0xFF);

    LCD_WriteCmd(0x2B);                  // PASET
    LCD_WriteData8(y0 >> 8);
    LCD_WriteData8(y0 & 0xFF);
    LCD_WriteData8(y1 >> 8);
    LCD_WriteData8(y1 & 0xFF);

    LCD_WriteCmd(0x2C);                  // RAMWR（准备写数据）
}

// 画单点
void LCD_DrawPoint(uint16_t x, uint16_t y, uint16_t color) {
    LCD_SetWindow(x, y, x, y);
    LCD_WriteColor(color);
}

// 填充矩形
void LCD_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    LCD_SetWindow(x, y, x+w-1, y+h-1);
    uint8_t buf[2] = {color >> 8, color & 0xFF};
    HAL_GPIO_WritePin(LCD_DC_GPIO, LCD_DC_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_CS_GPIO, LCD_CS_PIN, GPIO_PIN_RESET);
    for (uint32_t i = 0; i < (uint32_t)w * h; i++) {
        HAL_SPI_Transmit(&hspi1, buf, 2, HAL_MAX_DELAY);
    }
    HAL_GPIO_WritePin(LCD_CS_GPIO, LCD_CS_PIN, GPIO_PIN_SET);
}
```

### RGB565 颜色常量

```c
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_YELLOW  0xFFE0
#define COLOR_CYAN    0x07FF
#define COLOR_MAGENTA 0xF81F

// RGB888 转 RGB565
#define RGB888_TO_RGB565(r, g, b) \
    (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3))
```

### 触摸屏读取示例

```c
// 读取触摸坐标（原始 ADC 值）
uint16_t TP_ReadADC(uint8_t cmd) {
    uint8_t tx[3] = {cmd, 0x00, 0x00};
    uint8_t rx[3] = {0};
    
    HAL_GPIO_WritePin(TP_CS_GPIO, TP_CS_PIN, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(&hspi1, tx, rx, 3, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(TP_CS_GPIO, TP_CS_PIN, GPIO_PIN_SET);
    
    // 12 位结果在 rx[1] 的高 7 位和 rx[2] 的高 5 位
    return ((rx[1] << 8) | rx[2]) >> 3;
}

// 读取并转换为屏幕坐标（需校准）
void TP_GetCoord(uint16_t *x, uint16_t *y) {
    uint16_t raw_x = TP_ReadADC(0xD0); // X 通道
    uint16_t raw_y = TP_ReadADC(0x90); // Y 通道
    
    // 线性映射（实际需根据校准点调整）
    *x = (uint16_t)((float)raw_x * 240.0f / 4096.0f);
    *y = (uint16_t)((float)raw_y * 320.0f / 4096.0f);
}

// 检测是否有触摸（PENIRQ 低表示有触摸）
uint8_t TP_IsTouched(void) {
    return (HAL_GPIO_ReadPin(TP_IRQ_GPIO, TP_IRQ_PIN) == GPIO_PIN_RESET);
}
```

### 方向设置参考（0x36 命令参数）

| 方向 | 参数值 | 分辨率 |
|------|--------|--------|
| 竖屏正常（0°） | 0x48（MX=1, BGR=1） | 240×320 |
| 竖屏翻转（180°）| 0x88（MY=1, BGR=1） | 240×320 |
| 横屏（90°）| 0x28（MV=1, BGR=1） | 320×240 |
| 横屏翻转（270°）| 0xE8（MY=1,MX=1,MV=1,BGR=1） | 320×240 |

---

*文档生成日期：2026-05-24*  
*原始资料来源：购买的 TFT-LCD 触摸屏模块附带文档*
