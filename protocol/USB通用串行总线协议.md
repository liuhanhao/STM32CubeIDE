# USB 通用串行总线协议

## 概述

USB（Universal Serial Bus，通用串行总线）是一种即插即用的串行通讯标准，由 Intel、Microsoft 等公司联合制定。单片机通常实现 USB 设备端（Device），连接到 PC 主机（Host）。

- 通讯方式：同步/异步混合、半双工（USB 2.0）
- 所需引脚：D+、D-（差分信号对）
- 速度：
  - Low Speed：1.5 Mbps
  - Full Speed：12 Mbps（单片机最常用）
  - High Speed：480 Mbps
  - USB 3.x：5 Gbps+
- 供电：5V，最大 500mA（USB 2.0）

---

## USB 系统架构

```
PC（主机/Host）                    单片机（设备/Device）
  USB Host Controller                USB Device Controller
        │                                    │
        │ D+ ────────────────────────────── D+│
        │ D- ────────────────────────────── D-│
        │ VBUS(5V) ──────────────────────── 5V│
        │ GND ───────────────────────────── GND
```

> USB 是严格的主从架构，**主机（Host）主导所有通讯**，设备只能响应主机请求。

---

## USB 设备类（Device Class）

单片机常实现以下设备类，无需安装驱动：

| 设备类 | 说明 | 典型应用 |
|--------|------|----------|
| HID | Human Interface Device | 键盘、鼠标、游戏手柄、自定义数据传输 |
| CDC | Communication Device Class | 虚拟串口（最常用于调试） |
| MSC | Mass Storage Class | U盘、SD卡读卡器 |
| Audio | 音频设备 | USB 麦克风、USB 声卡 |
| DFU | Device Firmware Upgrade | 固件升级 |

---

## USB 枚举过程（设备插入时）

这是 USB 最重要的过程，决定了设备如何被识别：

```
步骤1: 设备插入，VBUS 上电
  主机检测到 D+ 或 D- 上拉（Full Speed 设备 D+ 上拉到 3.3V）

步骤2: 主机复位设备
  主机拉低 D+/D- 至少 10ms（USB Reset）

步骤3: 主机获取设备描述符（前8字节）
  主机 → 设备：GET_DESCRIPTOR 请求（设备描述符）
  设备 → 主机：返回设备描述符前8字节（包含最大包大小）

步骤4: 主机分配地址
  主机 → 设备：SET_ADDRESS（分配地址，如 0x05）
  设备 → 主机：ACK

步骤5: 主机获取完整描述符
  主机 → 设备：GET_DESCRIPTOR（设备描述符、配置描述符、字符串描述符）
  设备 → 主机：返回所有描述符

步骤6: 主机加载驱动
  主机根据 VID/PID 或设备类加载对应驱动

步骤7: 枚举完成，设备可正常使用
```

---

## USB 描述符体系

描述符是设备向主机介绍自己的"自我说明书"：

```
设备描述符（Device Descriptor）
  └── 配置描述符（Configuration Descriptor）
        └── 接口描述符（Interface Descriptor）
              └── 端点描述符（Endpoint Descriptor）× N
              └── 类特定描述符（如 HID 描述符）
```

### 设备描述符示例（CDC 虚拟串口）

```c
const uint8_t DeviceDescriptor[] = {
    0x12,        // bLength: 描述符长度 18字节
    0x01,        // bDescriptorType: 设备描述符类型
    0x00, 0x02,  // bcdUSB: USB 2.0
    0x02,        // bDeviceClass: CDC 类
    0x00,        // bDeviceSubClass
    0x00,        // bDeviceProtocol
    0x40,        // bMaxPacketSize0: 端点0最大包大小 64字节
    0x83, 0x04,  // idVendor: VID = 0x0483（ST公司）
    0x40, 0x57,  // idProduct: PID = 0x5740（CDC设备）
    0x00, 0x02,  // bcdDevice: 设备版本 2.00
    0x01,        // iManufacturer: 厂商字符串索引
    0x02,        // iProduct: 产品字符串索引
    0x03,        // iSerialNumber: 序列号字符串索引
    0x01,        // bNumConfigurations: 1个配置
};
```

---

## 完整通讯示例

### 示例1：CDC 虚拟串口（最常用）

**场景：** 单片机通过 USB CDC 向 PC 发送 "Hello USB!"

```
PC 端（串口助手打开 COM3）          单片机（USB CDC 设备）
         │                                    │
         │── 枚举完成，COM3 已就绪 ──────────→│
         │                                    │
         │                                    │ 准备发送数据
         │                                    │ 调用 CDC_Transmit()
         │←── IN 事务（数据包）───────────────│
         │    数据："Hello USB!\r\n"           │
         │                                    │
         │ 串口助手显示：Hello USB!            │
```

```c
// 单片机端 CDC 发送数据
void USB_CDC_SendString(const char *str) {
    uint16_t len = strlen(str);
    // 等待上一次发送完成
    while (CDC_Transmit_FS((uint8_t*)str, len) == USBD_BUSY) {
        HAL_Delay(1);
    }
}

// 接收 PC 发来的数据（回调函数）
void CDC_Receive_Callback(uint8_t *buf, uint32_t len) {
    // buf 中是 PC 发来的数据
    // 例如 PC 发送 "LED_ON\r\n"
    if (strncmp((char*)buf, "LED_ON", 6) == 0) {
        LED_On();
    }
}
```

### 示例2：HID 自定义数据传输

**场景：** 单片机作为 HID 设备，向 PC 上报传感器数据（无需驱动）

```
HID 报告描述符定义（8字节自定义数据）：
  Usage Page: 0xFF00（厂商自定义）
  Usage: 0x01
  Input: 8字节数据

通讯过程：
PC（HID 主机）                    单片机（HID 设备）
  │                                    │
  │── 每 1ms 轮询一次（Interrupt IN）──→│
  │←── 返回8字节传感器数据 ────────────│
  │    [温度高][温度低][湿度高][湿度低]  │
  │    [压力高][压力低][保留][保留]      │
```

```c
// HID 上报数据
uint8_t hid_report[8] = {0};

void HID_SendSensorData(uint16_t temp, uint16_t humi) {
    hid_report[0] = (temp >> 8) & 0xFF;
    hid_report[1] = temp & 0xFF;
    hid_report[2] = (humi >> 8) & 0xFF;
    hid_report[3] = humi & 0xFF;

    USBD_HID_SendReport(&hUsbDeviceFS, hid_report, 8);
}
```

---

## 四种传输类型详解

USB 定义了四种传输类型，适用于不同场景：

| 传输类型 | 特点 | 典型用途 |
|----------|------|----------|
| 控制传输（Control） | 双向，有握手，保证可靠 | 枚举、配置（所有设备必须支持） |
| 中断传输（Interrupt） | 周期性轮询，低延迟 | HID 键鼠、游戏手柄 |
| 批量传输（Bulk） | 大数据量，无时间保证 | U盘、打印机、CDC 数据 |
| 同步传输（Isochronous） | 固定带宽，不重传 | USB 音频、摄像头 |

### 控制传输（Control Transfer）流程

用于枚举阶段和设备配置，分三个阶段：

```
阶段1 - SETUP 阶段：
  主机 → 设备：SETUP 令牌 + 8字节请求数据
  设备 → 主机：ACK

  8字节请求格式：
  [bmRequestType][bRequest][wValue高][wValue低][wIndex高][wIndex低][wLength高][wLength低]

  例：GET_DESCRIPTOR 请求（获取设备描述符）：
  [0x80]  [0x06]  [0x00][0x01]  [0x00][0x00]  [0x12][0x00]
    ↑       ↑         ↑              ↑              ↑
  设备→主机 GET_DESC  设备描述符(0x01) 索引0  请求18字节

阶段2 - DATA 阶段（可选）：
  主机 ↔ 设备：传输实际数据（分多个数据包）

阶段3 - STATUS 阶段：
  设备 → 主机：确认完成（零长度包 ZLP）
```

### 中断传输（Interrupt Transfer）流程

主机按固定间隔轮询设备，HID 设备最常用：

```
时间轴（轮询间隔 1ms）：

t=0ms:   主机 → 设备：IN 令牌（查询有无数据）
         设备 → 主机：8字节 HID 报告（或 NAK 表示无数据）

t=1ms:   主机 → 设备：IN 令牌
         设备 → 主机：NAK（无新数据）

t=2ms:   主机 → 设备：IN 令牌
         设备 → 主机：8字节 HID 报告（按键状态更新）

注意：主机每 1ms 必然发起一次查询，设备不能主动发送，只能等待查询
```

### 批量传输（Bulk Transfer）流程

CDC 虚拟串口的数据收发使用批量传输：

```
单片机发送大量数据到 PC（批量 IN）：

数据：1000字节，每包最大 64字节（Full Speed）

包1：主机 IN 令牌 → 设备返回 64字节 → 主机 ACK
包2：主机 IN 令牌 → 设备返回 64字节 → 主机 ACK
...
包15：主机 IN 令牌 → 设备返回 40字节 → 主机 ACK
包16：主机 IN 令牌 → 设备返回 0字节（ZLP，表示传输结束）→ 主机 ACK

共 16 次事务完成 1000 字节传输
```

### 同步传输（Isochronous Transfer）流程

USB 音频使用同步传输，保证固定带宽，不重传：

```
USB 音频（44.1kHz，16位立体声）：
每帧（1ms）传输：44100 × 2 × 2 / 1000 ≈ 176字节

t=0ms:   主机 IN 令牌 → 设备返回 176字节音频数据（无 ACK，不重传）
t=1ms:   主机 IN 令牌 → 设备返回 176字节音频数据
t=2ms:   主机 IN 令牌 → 设备返回 176字节音频数据（即使有错也不重传）
...

特点：即使某帧数据出错，也不影响音频连续播放（人耳感知不到单帧丢失）
```

---

## 端点（Endpoint）

端点是 USB 设备上的数据缓冲区，每个端点有固定方向：

```
端点0（EP0）：控制端点，双向，用于枚举和控制
端点1 IN：设备→主机（如 HID 上报、CDC 数据发送）
端点1 OUT：主机→设备（如 CDC 数据接收）
端点2 IN：CDC 通知端点
```

### CDC 虚拟串口端点分配示例

```
EP0（控制）：枚举和配置
EP1 IN（中断）：CDC 通知（如串口状态变化）
EP2 IN（批量）：单片机 → PC 数据通道
EP2 OUT（批量）：PC → 单片机 数据通道

通讯流程：
PC 发送数据给单片机：
  PC → EP2 OUT → 单片机接收缓冲区 → 触发接收回调

单片机发送数据给 PC：
  单片机调用 CDC_Transmit() → 数据写入 EP2 IN 缓冲区
  → 等待主机下次 IN 令牌 → 数据发出
```

---

## 典型应用

- USB 转串口（CDC，调试最常用）
- USB HID 自定义数据传输（无需驱动）
- USB 键盘/鼠标模拟
- USB 大容量存储（U盘）
- USB DFU 固件升级
- USB 音频设备
