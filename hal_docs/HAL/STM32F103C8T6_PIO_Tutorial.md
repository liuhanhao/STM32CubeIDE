# 使用 PlatformIO 开发 STM32F103C8T6 完整教程

> 本教程以 **STM32F103C8T6**（Blue Pill）为主线，涉及其他 STM32 芯片差异的地方会用 `⚠️ 芯片差异` 标注。

---

## 目录

1. [环境准备](#1-环境准备)
2. [创建工程](#2-创建工程)
3. [工程结构说明](#3-工程结构说明)
4. [platformio.ini 配置详解](#4-platformioini-配置详解)
5. [第一个程序：Blink LED](#5-第一个程序blink-led)
6. [串口调试](#6-串口调试)
7. [烧录方式](#7-烧录方式)
8. [常用库的使用](#8-常用库的使用)
9. [调试（Debug）](#9-调试debug)
10. [常见问题](#10-常见问题)
11. [不同 STM32 芯片对比参考](#11-不同-stm32-芯片对比参考)

---

## 1. 环境准备

### 1.1 安装 VS Code

前往 [https://code.visualstudio.com](https://code.visualstudio.com) 下载并安装。

### 1.2 安装 PlatformIO 插件

1. 打开 VS Code
2. 点击左侧扩展图标（或 `Ctrl+Shift+X`）
3. 搜索 `PlatformIO IDE`
4. 点击 **Install**
5. 安装完成后重启 VS Code

### 1.3 安装驱动

| 烧录器 | 驱动 |
|--------|------|
| ST-Link V2 | [ST官网驱动](https://www.st.com/en/development-tools/stsw-link009.html) |
| J-Link | [Segger官网](https://www.segger.com/downloads/jlink/) |
| USB转串口（CH340/CP2102） | 对应芯片驱动 |

---

## 2. 创建工程

### 2.1 通过 PIO Home 创建

1. 点击 VS Code 左侧 PlatformIO 图标
2. 选择 **PIO Home → New Project**
3. 填写配置：

| 字段 | 填写内容 |
|------|----------|
| **Name** | 项目名称，如 `stm32f103_template` |
| **Board** | 搜索 `bluepill`，选择 `ST STM32F103C8 (Blue Pill)` |
| **Framework** | 选择 `Arduino` 或 `STM32Cube` |
| **Location** | 选择保存路径 |

4. 点击 **Finish**，等待 PIO 自动下载工具链（首次较慢）

### 2.2 通过命令行创建

```bash
# 安装 PlatformIO Core（如果只用命令行）
pip install platformio

# 创建工程
mkdir stm32_blink && cd stm32_blink
pio project init --board bluepill_f103c8 --ide vscode
```

---

## 3. 工程结构说明

```
stm32_blink/
├── .pio/                  # PIO 编译缓存（自动生成，勿手动修改）
├── .vscode/               # VS Code 配置（自动生成）
├── include/               # 头文件目录（.h 文件）
├── lib/                   # 本地库目录
├── src/                   # 源代码目录（主要编写区域）
│   └── main.cpp           # 主程序入口
├── test/                  # 单元测试目录
└── platformio.ini         # 工程核心配置文件
```

---

## 4. platformio.ini 配置详解

```ini
[env:bluepill_f103c8]
; ── 基础配置 ──────────────────────────────────────────
platform  = ststm32          ; STM32 平台
board     = bluepill_f103c8  ; 目标板型号
framework = arduino          ; 开发框架（arduino / stm32cube）

; ── 烧录配置 ──────────────────────────────────────────
upload_protocol = stlink     ; 烧录方式：stlink / jlink / serial（串口）
; upload_port = COM3         ; 串口烧录时指定端口（Windows）
; upload_port = /dev/ttyUSB0 ; 串口烧录时指定端口（Linux/macOS）

; ── 调试配置 ──────────────────────────────────────────
debug_tool = stlink          ; 调试工具

; ── 串口监视器 ────────────────────────────────────────
monitor_speed = 115200       ; 串口波特率

; ── 编译选项 ──────────────────────────────────────────
build_flags =
    -DSTM32F1xx              ; 定义芯片系列宏
    -Os                      ; 优化大小

; ── 依赖库 ────────────────────────────────────────────
; lib_deps =
;     Wire
;     SPI
;     adafruit/Adafruit GFX Library @ ^1.11.0
```

### ⚠️ 芯片差异：board 字段对照表

| 芯片型号 | board 字段 |
|----------|-----------|
| STM32F103C8T6 (Blue Pill) | `bluepill_f103c8` |
| STM32F103C8T6 (128K Flash) | `bluepill_f103c8t6` |
| STM32F103RCT6 | `genericSTM32F103RC` |
| STM32F103ZET6 | `genericSTM32F103ZE` |
| STM32F401CCU6 (Black Pill) | `blackpill_f401cc` |
| STM32F411CEU6 (Black Pill) | `blackpill_f411ce` |
| STM32F407VGT6 | `disco_f407vg` |
| STM32G0B1RE | `nucleo_g0b1re` |
| STM32H743ZI | `nucleo_h743zi` |

> 完整列表执行：`pio boards ststm32 | grep stm32`

---

## 5. 第一个程序：Blink LED

### 5.1 Arduino 框架

STM32F103C8T6 的板载 LED 连接在 **PC13**（低电平点亮）。

```cpp
// src/main.cpp
#include <Arduino.h>

// PC13 是 Blue Pill 板载 LED 引脚
#define LED_PIN PC13

void setup() {
    // 初始化 LED 引脚为输出模式
    pinMode(LED_PIN, OUTPUT);
}

void loop() {
    digitalWrite(LED_PIN, LOW);   // 低电平 → LED 亮
    delay(500);
    digitalWrite(LED_PIN, HIGH);  // 高电平 → LED 灭
    delay(500);
}
```

### ⚠️ 芯片差异：板载 LED 引脚

| 开发板 | 板载 LED 引脚 | 点亮电平 |
|--------|--------------|---------|
| Blue Pill (F103C8T6) | PC13 | 低电平 |
| Black Pill (F401/F411) | PC13 | 低电平 |
| Nucleo-F401RE | PA5 | 高电平 |
| Nucleo-F103RB | PA5 | 高电平 |
| STM32F407 Discovery | PD12~PD15 | 高电平 |

### 5.2 STM32Cube 框架（HAL 库）

```ini
; platformio.ini 修改 framework
framework = stm32cube
```

```cpp
// src/main.cpp
#include "stm32f1xx_hal.h"

void SystemClock_Config(void);

int main(void) {
    HAL_Init();
    SystemClock_Config();

    // 使能 GPIOC 时钟
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = GPIO_PIN_13;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;  // 推挽输出
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    while (1) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        HAL_Delay(500);
    }
}

void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    // 使用外部晶振 HSE（8MHz）+ PLL 倍频到 72MHz
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL     = RCC_PLL_MUL9;  // 8MHz × 9 = 72MHz
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;  // APB1 最大 36MHz
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}
```

### ⚠️ 芯片差异：最大主频

| 芯片系列 | 最大主频 | PLL 配置要点 |
|----------|---------|-------------|
| STM32F103xx | 72 MHz | HSE(8M) × 9 |
| STM32F401xx | 84 MHz | HSE(25M) / 25 × 336 / 4 |
| STM32F411xx | 100 MHz | HSE(25M) / 25 × 400 / 4 |
| STM32F407xx | 168 MHz | HSE(8M) / 8 × 336 / 2 |
| STM32H743xx | 480 MHz | 需配置 VOS0 电压域 |
| STM32G0B1xx | 64 MHz | 内部 HSI 16MHz × 4 |

---

## 6. 串口调试

### 6.1 Arduino 框架串口

```cpp
#include <Arduino.h>

void setup() {
    Serial.begin(115200);  // USART1：PA9(TX) / PA10(RX)
    // Serial2 → USART2：PA2(TX) / PA3(RX)
    // Serial3 → USART3：PB10(TX) / PB11(RX)
}

void loop() {
    Serial.println("Hello STM32F103C8T6!");
    delay(1000);

    // 接收数据
    if (Serial.available()) {
        String data = Serial.readString();
        Serial.print("收到: ");
        Serial.println(data);
    }
}
```

### 6.2 打开串口监视器

```bash
# 命令行方式
pio device monitor --baud 115200

# 或在 VS Code 底部状态栏点击插头图标
```

### ⚠️ 芯片差异：默认串口引脚

| 芯片 | USART1 TX/RX | USART2 TX/RX | USART3 TX/RX |
|------|-------------|-------------|-------------|
| F103C8T6 | PA9 / PA10 | PA2 / PA3 | PB10 / PB11 |
| F401/F411 | PA9 / PA10 | PA2 / PA3 | PB10 / PB11 |
| F407VGT6 | PA9 / PA10 | PA2 / PA3 | PB10 / PB11 |
| G0B1RE | PA9 / PA10 | PA2 / PA3 | PB10 / PB11 |

> 大多数 STM32 USART1 引脚相同，但部分芯片支持引脚重映射，使用前查阅对应数据手册。

---

## 7. 烧录方式

### 7.1 ST-Link 烧录（推荐）

**接线：**

| ST-Link V2 | STM32F103C8T6 |
|-----------|---------------|
| SWDIO | PA13 (SWDIO) |
| SWCLK | PA14 (SWCLK) |
| GND | GND |
| 3.3V | 3.3V（可选，也可外部供电） |

**platformio.ini 配置：**

```ini
upload_protocol = stlink
debug_tool      = stlink
```

**执行烧录：**

```bash
pio run --target upload
# 或点击 VS Code 底部状态栏的 → 箭头图标
```

### 7.2 串口烧录（BOOT0 模式）

**进入 Bootloader：**
1. 将 **BOOT0** 跳线拨到 **1**（高电平）
2. **BOOT1** 保持 **0**（低电平）
3. 按下复位键

**接线（USB转串口）：**

| USB转串口 | STM32F103C8T6 |
|----------|---------------|
| TX | PA10 (RX) |
| RX | PA9 (TX) |
| GND | GND |
| 3.3V/5V | 3.3V/5V |

**platformio.ini 配置：**

```ini
upload_protocol = serial
upload_port     = /dev/ttyUSB0   ; macOS/Linux
; upload_port   = COM3           ; Windows
upload_flags    = --go           ; 烧录后自动运行
```

**烧录完成后：** 将 BOOT0 跳线拨回 **0**，按复位键正常运行。

### ⚠️ 芯片差异：BOOT 引脚行为

| 芯片 | BOOT0=1,BOOT1=0 进入 | 说明 |
|------|---------------------|------|
| STM32F103xx | 系统 Bootloader（串口） | 标准方式 |
| STM32F4xx | 系统 Bootloader（串口/USB DFU） | 支持 USB DFU |
| STM32F411 Black Pill | BOOT0 按钮 | 板载按钮控制 |
| STM32G0xx | BOOT0 引脚或选项字节 | 可通过软件配置 |

### 7.3 J-Link 烧录

```ini
upload_protocol = jlink
debug_tool      = jlink
```

---

## 8. 常用库的使用

### 8.1 在 platformio.ini 中添加库

```ini
lib_deps =
    Wire                                          ; I2C（内置）
    SPI                                           ; SPI（内置）
    adafruit/Adafruit GFX Library @ ^1.11.9
    adafruit/Adafruit SSD1306 @ ^2.5.7
    adafruit/DHT sensor library @ ^1.4.4
    bblanchon/ArduinoJson @ ^6.21.3
```

### 8.2 I2C 示例（OLED SSD1306）

```cpp
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// STM32F103C8T6 默认 I2C1：PB6(SCL) / PB7(SDA)
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
    Wire.begin();  // 使用默认 I2C 引脚
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("STM32F103C8T6");
    display.display();
}

void loop() {}
```

### ⚠️ 芯片差异：默认 I2C 引脚

| 芯片 | I2C1 SCL/SDA | I2C2 SCL/SDA |
|------|-------------|-------------|
| F103C8T6 | PB6 / PB7 | PB10 / PB11 |
| F401/F411 | PB6 / PB7 | PB10 / PB3 |
| F407VGT6 | PB6 / PB7 | PB10 / PB11 |

### 8.3 SPI 示例

```cpp
#include <Arduino.h>
#include <SPI.h>

// STM32F103C8T6 默认 SPI1：PA5(SCK) / PA6(MISO) / PA7(MOSI)
#define CS_PIN PA4

void setup() {
    pinMode(CS_PIN, OUTPUT);
    digitalWrite(CS_PIN, HIGH);
    SPI.begin();
    SPI.setClockDivider(SPI_CLOCK_DIV8);  // 72MHz / 8 = 9MHz
}

void loop() {
    digitalWrite(CS_PIN, LOW);
    SPI.transfer(0xAB);  // 发送数据
    digitalWrite(CS_PIN, HIGH);
    delay(100);
}
```

---

## 9. 调试（Debug）

### 9.1 配置调试

```ini
; platformio.ini
[env:bluepill_f103c8]
platform         = ststm32
board            = bluepill_f103c8
framework        = arduino
debug_tool       = stlink
debug_init_break = tbreak main  ; 在 main 函数处暂停
```

### 9.2 启动调试

1. 在代码行号左侧点击设置**断点**（红点）
2. 按 `F5` 或点击左侧调试图标 → **Start Debugging**
3. 使用调试工具栏：继续（F5）、单步跳过（F10）、单步进入（F11）

### 9.3 查看变量和寄存器

- **变量面板**：左侧调试视图自动显示局部变量
- **监视面板**：添加表达式监视，如 `GPIOC->ODR`
- **外设寄存器**：PIO 调试器支持 SVD 文件查看外设寄存器状态

### ⚠️ 芯片差异：调试工具支持

| 芯片 | ST-Link V2 | ST-Link V3 | J-Link |
|------|-----------|-----------|--------|
| F103C8T6 | ✅ | ✅ | ✅ |
| F4xx | ✅ | ✅ | ✅ |
| F7xx | ✅ | ✅ | ✅ |
| H7xx | ❌（不支持） | ✅ | ✅ |
| G0xx | ✅ | ✅ | ✅ |

---

## 10. 常见问题

### Q1：烧录报错 `Error: open failed`

**原因：** ST-Link 未识别或驱动未安装。

**解决：**
```bash
# 检查已连接设备
pio device list

# 重新安装 OpenOCD 工具
pio pkg install --tool tool-openocd
```

### Q2：Blue Pill 烧录后程序不运行

**原因：** 部分 Blue Pill 板的 Flash 实际是 128KB，但标注为 64KB，导致链接脚本错误。

**解决：** 在 `platformio.ini` 中指定正确的 board：
```ini
; 如果芯片实际是 128KB Flash
board = bluepill_f103c8t6
; 或手动指定 Flash 大小
board_upload.maximum_size = 131072
```

### Q3：串口无输出

**原因：** 波特率不匹配或引脚接错。

**解决：** 确认 `monitor_speed` 与代码中 `Serial.begin()` 的波特率一致：
```ini
monitor_speed = 115200
```

### Q4：编译报错 `multiple definition of main`

**原因：** Arduino 框架下自定义了 `main()` 函数与框架冲突。

**解决：** 使用 `setup()` 和 `loop()` 替代自定义 `main()`，或切换到 STM32Cube 框架。

### Q5：I2C 设备无响应

**原因：** 缺少上拉电阻或地址错误。

**解决：** 运行 I2C 扫描程序确认设备地址：
```cpp
#include <Wire.h>
void setup() {
    Serial.begin(115200);
    Wire.begin();
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.print("发现设备，地址: 0x");
            Serial.println(addr, HEX);
        }
    }
}
void loop() {}
```

---

## 11. 不同 STM32 芯片对比参考

| 特性 | F103C8T6 | F401CCU6 | F411CEU6 | F407VGT6 | H743ZI |
|------|----------|----------|----------|----------|--------|
| 主频 | 72 MHz | 84 MHz | 100 MHz | 168 MHz | 480 MHz |
| Flash | 64/128 KB | 256 KB | 512 KB | 1 MB | 2 MB |
| RAM | 20 KB | 64 KB | 128 KB | 192 KB | 1 MB |
| GPIO 数量 | 37 | 36 | 36 | 82 | 114 |
| USART 数量 | 3 | 3 | 3 | 6 | 8 |
| I2C 数量 | 2 | 3 | 3 | 3 | 4 |
| SPI 数量 | 2 | 3 | 5 | 3 | 6 |
| USB | 全速 | 全速 OTG | 全速 OTG | 高速 OTG | 高速 OTG |
| DAC | ❌ | ❌ | ❌ | ✅ 2路 | ✅ 2路 |
| FPU | ❌ | ✅ | ✅ | ✅ | ✅ |
| 典型开发板 | Blue Pill | Black Pill | Black Pill | Discovery | Nucleo |
| PIO board 字段 | `bluepill_f103c8` | `blackpill_f401cc` | `blackpill_f411ce` | `disco_f407vg` | `nucleo_h743zi` |

---

## 快速参考：platformio.ini 完整模板

```ini
; ============================================================
; STM32F103C8T6 Blue Pill - PlatformIO 工程配置模板
; ============================================================
[env:bluepill_f103c8]
platform        = ststm32
board           = bluepill_f103c8
framework       = arduino

; 烧录 & 调试
upload_protocol = stlink
debug_tool      = stlink

; 串口监视器
monitor_speed   = 115200
monitor_filters = direct

; 编译标志
build_flags     =
    -DSTM32F1xx
    -DARDUINO_ARCH_STM32

; 依赖库（按需取消注释）
; lib_deps =
;     Wire
;     SPI
;     adafruit/Adafruit GFX Library @ ^1.11.9
;     adafruit/Adafruit SSD1306 @ ^2.5.7
;     bblanchon/ArduinoJson @ ^6.21.3
```

---

*文档版本：v1.0 | 适用 PlatformIO Core 6.x | 最后更新：2026-05*
