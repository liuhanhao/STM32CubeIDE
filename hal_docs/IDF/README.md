# ESP-IDF 驱动文档导航

本文档是 ESP-IDF 各模块 API 文档的导航与概览，聚焦于每个外设/模块的**使用特点**和**适用场景**，帮助快速判断"该用哪个模块"，再去对应文档查具体 API。

> **平台说明：** 本文档以 **ESP32-S3** 为主，基于 **ESP-IDF 5.x**，使用 **PlatformIO + ESP-IDF 框架**开发。

---

## 目录

- [基础外设](#基础外设)
- [通信外设](#通信外设)
- [模拟外设](#模拟外设)
- [定时器与 PWM](#定时器与-pwm)
- [存储](#存储)
- [无线连接](#无线连接)
- [模块选型速查](#模块选型速查)
- [与 STM32 HAL 对比](#与-stm32-hal-对比)

---

## 基础外设

### GPIO — 通用 IO
**文档**: [IDF_GPIO.md](IDF_GPIO.md)

控制数字输入输出引脚，ESP32-S3 共 45 个 GPIO，可通过 GPIO Matrix 路由到任意引脚。

**核心特点**

| 模式 | 特点 | 典型用途 |
|------|------|---------|
| 输入 | 浮空/上拉/下拉可配 | 按键、传感器信号 |
| 推挽输出 | 主动输出高低电平 | LED、继电器控制 |
| 开漏输出 | 只能拉低，需外部上拉 | I2C 总线、电平转换 |
| 输入输出开漏 | 可读可写，开漏 | 软件模拟 I2C |
| 中断 | 边沿/电平触发 | 按键、传感器变化检测 |

**与 STM32 HAL 的主要区别：**
- 引脚可路由到任意 GPIO（GPIO Matrix），不像 STM32 固定引脚
- 使用 `gpio_config_t` 结构体批量配置，比 STM32 更简洁
- 中断回调通过 `gpio_isr_handler_add` 注册，推荐用队列传递事件

**注意引脚限制：**
- GPIO26~GPIO32：连接 PSRAM（N16R8），不可用
- GPIO33~GPIO37：连接 Flash，不可用
- GPIO0、GPIO3：Strapping 引脚，影响启动模式

---

## 通信外设

### UART — 异步串口
**文档**: [IDF_UART.md](IDF_UART.md)

最常用的调试和通信接口，引脚可任意配置。

**核心特点**
- 3 个 UART 端口（UART0 默认调试，UART1/2 用户使用）
- 支持事件队列接收（推荐），比轮询更高效
- 引脚通过 GPIO Matrix 任意路由
- 支持 RS485 半双工模式

**何时选哪种接收方式**

| 场景 | 方式 |
|------|------|
| 调试打印 | `uart_write_bytes` 阻塞发送 |
| 不定长命令接收 | 事件队列 + `UART_DATA` 事件 |
| GPS/蓝牙连续数据流 | 事件队列 + 大缓冲区 |
| RS485 MODBUS | 半双工模式 + DE 引脚 |

---

### I2C — I²C 总线
**文档**: [IDF_I2C.md](IDF_I2C.md)

双线串行总线，适合低速多设备场景。

**核心特点**
- IDF 5.x 新 API（`i2c_master.h`）更简洁，推荐使用
- `i2c_master_transmit_receive` 一步完成"写寄存器地址 + 读数据"
- `i2c_master_probe` 可扫描总线上的设备地址
- 内部上拉约 45kΩ，正式产品建议外接 4.7kΩ

**常见设备地址**

| 设备 | 地址 |
|------|------|
| MPU6050 | 0x68 / 0x69 |
| SSD1306 OLED | 0x3C / 0x3D |
| BMP280 | 0x76 / 0x77 |
| FT6206 触摸 | 0x38 |
| GT911 触摸 | 0x5D / 0x14 |

---

### SPI — SPI 总线
**文档**: [IDF_SPI.md](IDF_SPI.md)

高速同步串行总线，适合大数据量传输。

**核心特点**
- SPI2_HOST 和 SPI3_HOST 可用（SPI1 被 Flash 占用）
- 支持 DMA 传输，刷屏期间 CPU 可处理其他任务
- 多设备共享总线，各自独立 CS 引脚
- `spi_device_polling_transmit`（短数据）vs `spi_device_transmit`（长数据+DMA）

**适用场景**
- TFT/OLED 刷屏 → DMA 模式，40~80MHz
- SPI Flash（W25Q）→ DMA 模式，Mode 0
- 触摸屏（XPT2046）→ 与 LCD 共用总线，独立 CS，低速

---

## 模拟外设

### ADC — 模数转换
**文档**: [IDF_ADC.md](IDF_ADC.md)

12 位 SAR ADC，2 个单元共 20 个通道。

**核心特点**
- ADC1（GPIO1~GPIO10）：不受 WiFi 影响，推荐使用
- ADC2（GPIO11~GPIO20）：WiFi 启用时不可用
- 支持硬件校准（曲线拟合），精度更高
- One-shot 模式（偶发读取）vs Continuous 模式（高速连续采集）

**衰减与量程**

| 衰减 | 有效量程 |
|------|---------|
| `ADC_ATTEN_DB_0` | 0~750mV |
| `ADC_ATTEN_DB_12` | 0~3100mV（最常用）|

---

## 定时器与 PWM

### TIM — 定时器与 PWM
**文档**: [IDF_TIM.md](IDF_TIM.md)

ESP32-S3 提供多种定时器外设，各有专长。

**外设选型**

| 需求 | 外设 |
|------|------|
| LED 调光、舵机、电机 PWM | LEDC |
| 精确周期中断（1ms 心跳） | GPTimer |
| H 桥互补 PWM + 死区 | MCPWM |
| 旋转编码器 | MCPWM 编码器模式 |
| LVGL Tick、软件延时 | esp_timer |

**LEDC 频率与分辨率（80MHz 时钟）**

| PWM 频率 | 最大分辨率 |
|---------|----------|
| 50Hz（舵机） | 20 位 |
| 1kHz | 16 位 |
| 10kHz | 13 位 |

---

## 存储

### NVS — 非易失性存储
**文档**: [IDF_NVS.md](IDF_NVS.md)

键值对存储，数据保存在 Flash 中，断电不丢失。

**核心特点**
- 命名空间隔离，不同模块数据互不干扰
- 支持 int8/16/32/64、字符串、blob 等类型
- 磨损均衡，自动分散写入
- 写入后必须调用 `nvs_commit()` 才真正写入 Flash

**NVS vs 其他存储**

| 方案 | 适用 |
|------|------|
| NVS | 配置参数，偶发写入（首选）|
| SPIFFS/LittleFS | 文件存储，中等频率写入 |
| 外部 EEPROM | 频繁写入的小数据 |
| 外部 SPI Flash | 大量数据 |

---

## 无线连接

### WiFi — 无线局域网
**文档**: [IDF_WiFi.md](IDF_WiFi.md)

802.11 b/g/n，支持 Station、AP、Station+AP 三种模式。

**核心特点**
- 事件驱动架构，通过事件循环通知连接状态
- Station 模式：连接路由器，访问互联网
- AP 模式：创建热点，用于配网或点对点通信
- 结合 NVS 保存 WiFi 凭据，实现上电自动连接

**连接流程**
```
nvs_flash_init → esp_netif_init → esp_event_loop_create_default
→ esp_netif_create_default_wifi_sta → esp_wifi_init
→ 注册事件 → esp_wifi_set_config → esp_wifi_start
→ [WIFI_EVENT_STA_START] → esp_wifi_connect
→ [IP_EVENT_STA_GOT_IP] → 可以通信
```

---

## 模块选型速查

### 按需求找模块

| 需求 | 模块 |
|------|------|
| 控制 LED / 继电器 | GPIO（推挽输出）|
| 读取按键 | GPIO（输入 + 上拉）|
| 按键触发中断 | GPIO（中断模式）|
| 调试打印 / ESP_LOG | UART0（默认）|
| 与传感器通信（低速，多设备）| I2C |
| 与 Flash / 屏幕通信（高速）| SPI |
| 采集模拟传感器值 | ADC |
| 生成 PWM 控制电机 / 舵机 | LEDC |
| 精确周期定时 | GPTimer |
| LVGL Tick 源 | esp_timer |
| 保存配置参数 | NVS |
| 连接 WiFi | WiFi |
| 多任务并发调度 | FreeRTOS（IDF 内置）|
| 触摸屏显示 | SPI（显示）+ I2C（触摸）+ LVGL |

---

## 与 STM32 HAL 对比

| 功能 | STM32 HAL | ESP-IDF |
|------|-----------|---------|
| GPIO 配置 | `HAL_GPIO_Init` + 固定引脚 | `gpio_config` + 任意引脚（GPIO Matrix）|
| 串口 | `HAL_UART_Init` + 固定引脚 | `uart_param_config` + 任意引脚 |
| I2C | `HAL_I2C_Init` + 固定引脚 | `i2c_new_master_bus` + 任意引脚 |
| SPI | `HAL_SPI_Init` + 固定引脚 | `spi_bus_initialize` + 任意引脚 |
| ADC | `HAL_ADC_Init` + 固定通道 | `adc_oneshot_new_unit` + 固定通道 |
| PWM | `HAL_TIM_PWM_Start` | `ledc_timer_config` + `ledc_channel_config` |
| 定时器 | `HAL_TIM_Base_Start_IT` | `gptimer_new_timer` |
| 配置存储 | HAL_FLASH | NVS（更简单）|
| RTOS | FreeRTOS（外部集成）| FreeRTOS（内置，直接用）|
| 无线 | 无 | WiFi / BLE / ESP-NOW |

**ESP-IDF 相比 HAL 的主要优势：**
- 引脚灵活（GPIO Matrix），不受固定引脚限制
- FreeRTOS 内置，无需额外配置
- NVS 比直接操作 Flash 简单得多
- 内置 WiFi/BLE，无需外部模块

---

## 文档列表

| 文件 | 模块 |
|------|------|
| [IDF_Core.md](IDF_Core.md) | 系统初始化、日志、内存、重启 |
| [IDF_GPIO.md](IDF_GPIO.md) | GPIO 数字 IO、中断、RTC GPIO |
| [IDF_UART.md](IDF_UART.md) | 异步串口、RS485 |
| [IDF_I2C.md](IDF_I2C.md) | I2C 总线（新旧 API）|
| [IDF_SPI.md](IDF_SPI.md) | SPI 总线、DMA、多设备 |
| [IDF_ADC.md](IDF_ADC.md) | 模数转换、校准 |
| [IDF_DAC.md](IDF_DAC.md) | 数模转换、余弦波、音频 |
| [IDF_LEDC.md](IDF_LEDC.md) | PWM 输出（LED、舵机、电机）|
| [IDF_GPTimer.md](IDF_GPTimer.md) | 通用定时器、精确定时、脉宽测量 |
| [IDF_MCPWM.md](IDF_MCPWM.md) | 电机控制 PWM、互补输出、编码器 |
| [IDF_EspTimer.md](IDF_EspTimer.md) | 软件定时器（LVGL Tick、周期任务）|
| [IDF_RTC.md](IDF_RTC.md) | 实时时钟、SNTP 同步、DS3231 |
| [IDF_DeepSleep.md](IDF_DeepSleep.md) | 深度睡眠、唤醒源、RTC 内存 |
| [IDF_NVS.md](IDF_NVS.md) | 非易失性存储、键值对 |
| [IDF_SPIFFS.md](IDF_SPIFFS.md) | 文件系统（SPIFFS/LittleFS）|
| [IDF_WiFi.md](IDF_WiFi.md) | WiFi Station/AP 模式 |
| [IDF_HTTP.md](IDF_HTTP.md) | HTTP 客户端/服务端 |
| [IDF_MQTT.md](IDF_MQTT.md) | MQTT 消息协议 |
