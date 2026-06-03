# YED-C780 产品资料手册

> 基于银尔达（Yinerda）官方文档 + 上海合宙 Air780E AT命令手册 V1.4.9 整理
> 模组平台：合宙 Air780E（移芯 EC618）

---

## 目录

1. [产品简介](#1-产品简介)
2. [产品规格](#2-产品规格)
3. [硬件资源](#3-硬件资源)
4. [管脚定义](#4-管脚定义)
5. [LED状态指示](#5-led状态指示)
6. [硬件连接与测试](#6-硬件连接与测试)
7. [AT固件使用](#7-at固件使用)
8. [DTU固件使用](#8-dtu固件使用)
9. [常用AT命令速查](#9-常用at命令速查)
10. [TCP/IP通信流程](#10-tcpip通信流程)
11. [MQTT通信流程](#11-mqtt通信流程)
12. [HTTP通信流程](#12-http通信流程)
13. [低功耗配置](#13-低功耗配置)
14. [OTA固件升级](#14-ota固件升级)
15. [常见问题](#15-常见问题)

---

## 1. 产品简介

YED-C780 DTU 是由银尔达（Yinerda）推出的超低成本 TTL 核心板，小巧、稳定、可靠。

**适用场景：** 设备控制、状态检测、传感器数据采集等通过 4G 网络与服务器通信的场景。

**核心特性：**

- 基于合宙 Air780E 模组（移芯 EC618 平台），支持 Cat.1 4G 全网通
- 支持中国移动、联通、电信三网
- 支持直流 5~12V 宽电压供电；支持 3.3~4.2V 供电
- 支持标准 2.54mm 间距 12PIN 双排排针
- 工作温度 -35℃ ~ +75℃
- 支持 2 路 TTL 串口，兼容 3.3V 和 5V 电平
- 支持 1 路 ADC 模拟量，最大输入 3.8V，分辨率 12bit
- 支持合宙 AT 固件：TCP、UDP、MQTT、HTTP、FTP、PPP、RNDIS 等协议
- 支持银尔达 DTU 透传固件，可对接阿里云、腾讯云、华为云、电信云、涂鸦云、ThingsCloud 等主流平台
- 支持 SSL 证书加密（TCPS / MQTTS / HTTPS）
- 支持 FOTA 空中升级
- 支持二次开发与固件定制

> ⚠️ **注意：** AT固件版本与DTU固件版本硬件不同，不兼容，固件不可互刷。

---

## 2. 产品规格

### 2.1 硬件参数

| 项目 | 说明 |
|------|------|
| 4G 模块 | 合宙 Air780E（移芯 EC618） |
| 网络标准 | Cat.1 4G 全网通，支持中国移动、联通、电信 |
| 网络频段 | LTE-FDD: B1/B3/B5/B8；LTE-TDD: B34/B38/B39/B40/B41 |
| 供电电压 | 5~12V，推荐 5V 以上，10W 电源 |
| 备用供电 | VBAT 接口，3.3~4.2V |
| 工作温度 | -35℃ ~ +75℃ |
| 工作湿度 | 5%~95%RH（无凝露） |
| 串口1 | 兼容 3.3V/5V，波特率 1200~460800，数据位8，停止位1/2，奇/偶/无校验 |
| 串口2 | 兼容 3.3V/5V，波特率 1200~460800，数据位8，停止位1/2，奇/偶/无校验 |
| ADC | 直接引出 ADC，最大采集电压 3.8V |
| PCB 尺寸 | 32×32mm |
| 安装方式 | 2.54mm 间距双排排针 |

---

## 3. 硬件资源

| 编号 | 标识 | 说明 |
|------|------|------|
| 1 | 排针 | 通信固定排针（2.54mm 双排） |
| 2 | BOOT | 配合 USB 进入升级模式 |
| 3 | RYD LED | 设备状态指示灯 |
| 4 | NET LED | 网络状态指示灯 |
| 5 | LET | 4G 天线接口，IPEX 1 代 |
| 6 | GPS 天线 | IPEX 1 代，支持 3.3V 有源天线（需Air780EG版本） |
| 7 | 外置SIM卡 | Nano SIM 卡，切口槽朝左 |
| 8 | 内置SIM卡 | 贴片 SIM 卡 |

---

## 4. 管脚定义

### 左侧管脚（1~12）

| 引脚 | PCB丝印 | 功能描述 |
|------|---------|---------|
| 1 | VIN | 供电正极，5~12V |
| 2 | GND | 负极（地） |
| 3 | ADC | ADC0，最大采集 3.8V |
| 4 | GND | 负极（地） |
| 5 | VBAT | 电池供电接口，3.3~4.2V |
| 6 | G27 | GPIO27（1.8V 电平）/ AT固件的 NET STATUS / DTU固件的 NET LED 管脚 |
| 7 | G26 | AT固件无用 / DTU固件的 RDY LED 管脚 |
| 8 | G17 | GPIO17（1.8V 电平）/ AT固件无用 / DTU固件的 Reload 管脚 |
| 9 | RST | 复位管脚，高电平 1 秒复位，有效电平 3.3V~VBAT |
| 10 | TX | 主通信串口 TX，兼容 3.3V~5V 电平 |
| 11 | RX | 主通信串口 RX，兼容 3.3V~5V 电平 |
| 12 | VCCIO | 串口参考电平：3.3V 串口悬空；5V 串口接 5V |

### 右侧管脚（1~12）

| 引脚 | PCB丝印 | 功能描述 |
|------|---------|---------|
| 1 | SP- | NC（不连接） |
| 2 | SP+ | NC（不连接） |
| 3 | GND | USB 接口地 |
| 4 | DP | USB D+ |
| 5 | DM | USB D- |
| 6 | VBUS | USB VBUS（最大 5V），用于固件升级和日志调试 |
| 7 | SV | SIM 卡引脚 |
| 8 | SR | SIM 卡引脚 |
| 9 | SD | SIM 卡引脚 |
| 10 | SC | SIM 卡引脚 |
| 11 | TX2 | AUX 串口 TX，兼容 3.3V~5V 电平 |
| 12 | RX2 | AUX 串口 RX，兼容 3.3V~5V 电平 |

> **注意事项：**
> 1. 不用的管脚可以悬空
> 2. 只有通信串口 TX/RX 做了电平转换，其他 GPIO 没有电平转换
> 3. RST 为高电平复位，强烈建议使用，方便在异常情况下恢复模组

---

## 5. LED状态指示

### DTU 固件下的 NET LED 和 RYD LED

| NET LED 状态 | 指示含义 |
|--------------|---------|
| NET LED 和 RYD LED 5000ms 闪烁 | SIM 卡不识别 |
| NET LED 100ms 快闪，RYD LED 熄灭 | SIM 卡正常，但注册不了网络 |
| NET LED 500ms 慢闪，RYD LED 熄灭 | 注册网络成功，但未连接服务器 |
| NET LED 1000ms 慢闪，RYD LED 常亮 | 成功连接服务器（至少一个通道） |

### AT 固件下通过 AT+SLEDS 配置灯效

默认配置：
- 未注册网络：200ms 亮 / 1800ms 灭
- 已注册网络：1800ms 亮 / 200ms 灭
- PPP 通信状态：125ms 亮 / 125ms 灭

---

## 6. 硬件连接与测试

### 6.1 SIM 卡安装

卡座上有方向标识，缺口与卡方向一致，Nano SIM 卡切口槽朝左插入。

### 6.2 供电要求

- 主供电：VIN 接 5~12V 直流电源，推荐功率 **10W 及以上**
- ⚠️ 不能用 USB 转串口的 USB 电源供电（功率不足，会导致异常重启）

### 6.3 串口连接

```
模块 TX  →  USB串口模块 RX
模块 RX  →  USB串口模块 TX
模块 GND →  USB串口模块 GND
```

- 若 MCU 串口为 5V，需将 VCCIO 接 5V
- 若 MCU 串口为 3.3V，VCCIO 悬空即可

### 6.4 复位连接

RST 管脚建议接 MCU 的一个 GPIO，用于在异常时复位模组：
- 将 RST 拉高保持 **1 秒以上**即可触发复位
- 有效电平：3.3V ~ VBAT

### 6.5 测试底板

推荐使用银尔达 YED-UUART-211 测试工具，方便供电和串口调试。  
需安装 **CP2102 驱动**。

---

## 7. AT固件使用

### 7.1 串口配置

| 参数 | 默认值 |
|------|--------|
| 波特率 | 自适应（输入几次 `AT` 后自动匹配） |
| 数据位 | 8 |
| 停止位 | 1 |
| 校验位 | 无 |
| 流控 | 无 |

> 建议使用固定波特率：`AT+IPR=115200;&W`

### 7.2 开机初始化序列

```
AT                          // 测试通信
AT+CGMI                     // 查询厂商名称，返回 AirM2M
AT+CGMM                     // 查询型号，返回 Air780E
AT+CGMR                     // 查询固件版本
AT+CGSN                     // 查询 IMEI
AT+CCID                     // 查询 SIM 卡 ICCID
AT+CIMI                     // 查询 IMSI
AT+CSQ                      // 查询信号质量（返回 rssi,ber）
AT+CREG?                    // 查询网络注册状态（1=已注册本地网）
AT+CGATT?                   // 查询 GPRS 附着状态（1=已附着）
AT+CGDCONT?                 // 查询 PDP 上下文（确认有 IP 地址）
```

### 7.3 信号强度说明

`AT+CSQ` 返回 `+CSQ:<rssi>,<ber>`

| rssi 值 | 对应信号强度 |
|---------|------------|
| 0 | ≤ -115 dBm（极弱） |
| 1~10 | 较弱 |
| 10~20 | 中等 |
| 20~30 | 较强 |
| 31 | ≥ -51 dBm（极强） |
| 99 | 未知或不可测 |

---

## 8. DTU固件使用

DTU 固件支持通过串口配置工具直接设置服务器地址、协议类型等参数，无需编写 AT 命令程序。

**支持的物联网平台：**
- 标准 TCP / UDP / MQTT / HTTP / WebSocket
- 阿里云 IoT
- 腾讯云 IoT Explorer
- 华为 IoT DA
- 电信 AioT（MQTT）
- 移动 OneNET
- 涂鸦云
- ThingsCloud
- 其他自定义服务器

---

## 9. 常用AT命令速查

### 9.1 基本信息查询

| AT 命令 | 说明 | 示例返回 |
|---------|------|---------|
| `AT+CGMI` | 查询厂商 | `+CGMI:"AirM2M"` |
| `AT+CGMM` | 查询型号 | `+CGMM:"Air780E"` |
| `AT+CGMR` | 查询固件版本 | `+CGMR:"AirM2M_780E_V1121_LTE_AT"` |
| `AT+CGSN` | 查询 IMEI | `864269060001662` |
| `AT+CCID` | 查询 ICCID | `89860xxxxxxxxxxxxxxx` |
| `ATI` | 查询模块信息 | `AirM2M_780E_V1120_LTE_AT` |
| `AT*I` | 查询所有信息 | 包含厂商、型号、版本、IMEI等 |
| `AT^HVER` | 查询平台硬件版本 | `^HVER:EC618` |

### 9.2 配置与控制命令

| AT 命令 | 说明 |
|---------|------|
| `AT&W` | 保存当前配置到 NV（重启后生效） |
| `AT&F` | 恢复出厂设置 |
| `AT+RESET` | 重启模块 |
| `AT+RSTSET` | 重启并恢复出厂设置 |
| `AT+IPR=115200;&W` | 设置固定波特率 115200 并保存 |
| `AT+CFUN=1,1` | 重启模块并进入全功能模式 |
| `AT+CFUN=4` | 进入飞行模式 |
| `ATE0` | 关闭命令回显 |
| `AT+CMEE=2` | 启用详细错误码上报 |

### 9.3 ADC 读取

YED-C780 ADC 管脚最大采集电压 **3.8V**，分辨率 12bit。

```
AT+CADC=0,1        // 打开 ADC0 通道
AT+CADC=0,2        // 读取 ADC0 电压值（单位：毫伏）
// 返回示例：+CADC:0,1086  即 1086mV
```

> 若需要测量 3.8V 以上的电压，需外接分压电阻电路。

### 9.4 VBAT 电压读取

```
AT+CBC            // 读取当前 VBAT 电压（单位：毫伏）
// 返回示例：+CBC:3800
```

### 9.5 GPIO 控制

> ⚠️ 仅适用于 EC618 系列，其他 GPIO 均为 1.8V 电平

```
AT+CGPIO=1,27,1   // GPIO27 输出高电平
AT+CGPIO=1,27,0   // GPIO27 输出低电平
AT+CGPIO=0,27,1   // GPIO27 配置为输入，上拉
```

### 9.6 SIM 卡管理

```
AT+CPIN?                    // 查询 SIM 卡状态
AT+CCID                     // 查询 ICCID
AT+CIMI                     // 查询 IMSI
AT+SIMCROSS=0               // 使用外置 SIM 卡槽（卡0）
AT+SIMCROSS=1               // 使用内置贴片卡（卡1）
AT+CFUN=0                   // 切换前先进飞行模式
AT+CFUN=1                   // 完成切换后恢复全功能
```

---

## 10. TCP/IP 通信流程

### 10.1 TCP 客户端（单链接，快发模式）

```
// 第一步：查询网络状态
AT+CREG?                    // 确认已注册
AT+CGATT?                   // 确认 GPRS 已附着

// 第二步：激活网络
AT+CIPMUX=0                 // 单链接模式
AT+CIPQSEND=1               // 快发模式（推荐）
AT+CSTT                     // 自动使用网络 APN
AT+CIICR                    // 激活移动场景
AT+CIFSR                    // 查询本地 IP 地址

// 第三步：建立连接
AT+CIPSTART="TCP","服务器IP或域名",端口号

// 等待 URC 上报：CONNECT OK

// 第四步：发送数据
AT+CIPSEND=10               // 发送 10 字节定长数据
> 1234567890                 // 输入数据（自动发送）
// 返回：DATAACCEPT:10

// 发送不定长数据
AT+CIPSEND
> 发送内容<Ctrl+Z>           // 0x1A 结束

// 第五步：确认数据到达（可选）
AT+CIPACK                   // 查询已发送/已确认字节数

// 第六步：关闭连接
AT+CIPCLOSE                 // 关闭 TCP 连接
AT+CIPSHUT                  // 关闭移动场景
```

### 10.2 TCP 透传模式

```
AT+CIPMODE=1                // 切换为透传模式
AT+CIPSTART="TCP","服务器IP",端口号
// 等待 CONNECT（透传就绪）

// 此后串口数据直接转发，无需 AT 命令
// 退出透传模式：先等待 1 秒无数据，输入 +++，再等待 500ms
// 返回 OK 表示回到 AT 命令模式
```

### 10.3 UDP 通信

```
AT+CIPSTART="UDP","服务器IP",端口号
// 等待 CONNECT OK
AT+CIPSEND=10
> 1234567890
// 返回：DATAACCEPT:10
```

### 10.4 多路连接（最多 6 路）

```
AT+CIPMUX=1                         // 多链接模式
AT+CSTT
AT+CIICR
AT+CIFSR
AT+CIPSTART=0,"TCP","服务器1",7500  // 连接0
AT+CIPSTART=1,"TCP","服务器2",8080  // 连接1

AT+CIPSEND=0,10                     // 在连接0发送10字节
> 1234567890
AT+CIPSEND=1,5                      // 在连接1发送5字节
> hello
```

### 10.5 SSL 加密连接

```
AT+CIPSSL=1                                    // 开启 SSL
AT+SSLCFG="sslversion",0,4                    // 支持所有 TLS 版本
AT+SSLCFG="seclevel",0,0                      // 不验证证书（简单模式）

// 单向认证（验证服务器证书）
AT+SSLCFG="cacert",0,"ca.crt"
AT+SSLCFG="seclevel",0,1

// 双向认证
AT+SSLCFG="cacert",0,"ca.crt"
AT+SSLCFG="clientcert",0,"client.crt"
AT+SSLCFG="clientkey",0,"client.key"
AT+SSLCFG="seclevel",0,2

AT+CIPSTART="TCP","服务器IP",端口号
```

---

## 11. MQTT通信流程

### 11.1 标准 MQTT（不带 SSL）

```
// 第一步：配置 MQTT 参数
AT+MCONFIG="设备ID","用户名","密码"

// 第二步：建立 TCP 连接
AT+MIPSTART="broker.example.com",1883
// 等待 URC：CONNECT OK

// 第三步：MQTT 握手
AT+MCONNECT=1,60               // clean_session=1，keepalive=60s
// 等待 URC：CONNACK OK

// 第四步：订阅主题
AT+MSUB="device/receive",0     // 订阅，QoS 0

// 第五步：发布消息
AT+MPUB="device/send",0,0,"Hello Server"

// 接收消息（自动 URC 上报）
// +MSUB:"device/receive",12,Hello Client

// 第六步：断开连接
AT+MDISCONNECT
AT+MIPCLOSE
```

### 11.2 MQTTS（带 SSL）

```
// 配置证书（同 TCP SSL 配置，SSL context id = 88）
AT+SSLCFG="cacert",88,"ca.crt"
AT+SSLCFG="seclevel",88,1

AT+MCONFIG="设备ID","用户名","密码"
AT+SSLMIPSTART="broker.example.com",8883    // 注意使用 SSLMIPSTART
AT+MCONNECT=1,60
```

### 11.3 MQTT 发布定长消息（适合二进制数据）

```
AT+MPUBEX="topic",0,0,10       // 发送 10 字节
> 1234567890                    // 输入数据（满10字节自动发送）
```

---

## 12. HTTP通信流程

### 12.1 HTTP GET 请求

```
// 激活 SAPBR 承载（HTTP/FTP 需要单独激活）
AT+SAPBR=3,1,"CONTYPE","GPRS"
AT+SAPBR=3,1,"APN",""
AT+SAPBR=1,1                                   // 激活 cid=1 的承载
AT+SAPBR=2,1                                   // 查询状态，确认有 IP

// 初始化 HTTP
AT+HTTPINIT
AT+HTTPPARA="CID",1
AT+HTTPPARA="URL","http://example.com/api/data"

// 执行 GET
AT+HTTPACTION=0
// 等待 URC：+HTTPACTION:0,200,<数据长度>

// 读取响应
AT+HTTPREAD

// 结束 HTTP 会话
AT+HTTPTERM
```

### 12.2 HTTP POST 请求

```
AT+SAPBR=1,1
AT+HTTPINIT
AT+HTTPPARA="CID",1
AT+HTTPPARA="URL","http://example.com/api/post"
AT+HTTPPARA="CONTENT","application/json"

// 准备 POST 数据（最大 3356 字节）
AT+HTTPDATA=27,10000
// 等待 DOWNLOAD 提示后输入数据
{"key":"value","num":123}

AT+HTTPACTION=1
// 等待 URC：+HTTPACTION:1,200,<长度>
AT+HTTPREAD
AT+HTTPTERM
```

### 12.3 HTTPS 请求

```
AT+HTTPSSL=1                   // 开启 HTTPS
AT+SSLCFG="seclevel",153,0    // SSL context id=153 对应 HTTP
// 后续流程与 HTTP 相同，URL 改为 https://
```

---

## 13. 低功耗配置

### 13.1 睡眠模式说明

| 模式 | 命令 | 说明 | 唤醒方式 |
|------|------|------|---------|
| 不睡眠 | `AT+CSCLK=0` | 默认，不进入睡眠 | — |
| 睡眠模式1 | `AT+CSCLK=1` | DTR 引脚控制 | DTR 拉低/串口/来电/来短信 |
| 睡眠模式2 | `AT+CSCLK=2` | 串口空闲自动睡眠 | 串口 AT/来电/来短信 |
| 睡眠模式3 | `AT+CSCLK=3` | 超低功耗 | 串口 AT/来电/来短信 |

### 13.2 典型低功耗配置

```
AT+CSCLK=3                  // 超低功耗模式
AT+WAKETIM=1                // 空闲 1 秒后进入睡眠
AT*RTIME=2                  // 数传模式空闲 2 秒后进入睡眠
```

### 13.3 超低功耗 PSM+ 模式（≥V1143 LPAT 版本）

```
AT+IPR=9600;&W              // 改为 9600 波特率（确保唤醒不丢包）
AT+POWERMODE="PSM+"         // 进入 PSM+ 模式，待机约 2.89uA
// 唤醒方式：DTR 拉低、VBUS 上拉、串口发数据
```

### 13.4 RI 引脚唤醒配置

```
AT+CFGRI=1                  // 开启 RI 脚 URC 指示
AT+CFGRISAVE=1              // 保存 RI 配置
// 收到 TCP/MQTT/HTTP 数据时 RI 会产生 120ms 低脉冲，可唤醒 MCU
```

---

## 14. OTA固件升级

### 14.1 FOTA 空中升级

```
// 方法1：使用合宙 IoT 服务器（需在 iot.openluat.com 创建项目并上传固件）
AT+UPGRADE="KEY","你的ProductKey"  // 设置产品 Key
AT+UPGRADE                          // 手动触发升级

// 方法2：使用自定义服务器
AT+UPGRADE="URL","http://yourserver.com/firmware.bin"
AT+UPGRADE                          // 触发升级

// 查询升级状态
AT+UPGRADE?
// 升级过程 URC：
// +UPGRADEIND:<percent>    (下载进度)
// +UPGRADEDL:<percent>     (烧录进度)
// +UPGRADEVER:<新版本>     (升级成功)
```

> ⚠️ 升级过程中禁止重启或断电！

### 14.2 串口升级（≥V1155 版本）

通过 UART 传输差分升级包：
```
AT+UARTUPGRADE=0            // 擦除 FOTA 下载区
AT+UARTUPGRADE=1,0,<字节数>,<数据HEX>,<校验>  // 分段传输差分包
AT+UARTUPGRADE=2            // 验证差分包
AT+UARTUPGRADE=5            // 执行升级
```

---

## 15. 常见问题

### Q1：上电后无任何输出或响应

- 检查供电电压是否在 5~12V 之间，电源功率是否 ≥ 10W
- 检查串口 TX/RX 是否交叉连接（模块 TX → MCU RX）
- 检查波特率设置（默认自适应，发送几次 `AT` 训练波特率）
- 检查 VCCIO 是否与串口电平匹配

### Q2：SIM 卡不识别（NET LED 5s 闪烁）

- 检查 SIM 卡插入方向（缺口朝左）
- 确认 SIM 卡已开通 4G 数据服务
- 尝试换一张 SIM 卡排查
- 用 `AT+CPIN?` 查询，返回 `READY` 表示正常
- 检查是否需要内外卡切换：`AT+SIMCROSS?`

### Q3：注册不上网络（NET LED 100ms 快闪）

- 检查天线是否已连接
- 检查所在区域 4G 网络覆盖
- 用 `AT+CSQ` 查询信号，rssi 低于 5 则信号过弱
- 检查 SIM 卡运营商与频段支持

### Q4：连接服务器失败

- 确认服务器 IP/域名、端口号正确
- 用 `AT+CDNSGIP` 测试域名解析
- 用 `AT+CIPPING` 测试网络连通性
- 检查服务器防火墙是否放行对应端口

### Q5：数据发送成功但服务器未收到

- 确认使用快发模式（`AT+CIPQSEND=1`）
- 用 `AT+CIPACK` 查询已确认字节数
- 检查数据长度是否超过 1460 字节（单次最大）

### Q6：模块异常重启

- 检查供电是否稳定（尤其是数据传输峰值电流时）
- 检查模组工作温度是否在 -35℃~75℃ 范围内
- 建议接 RST 管脚实现外部看门狗复位
- 用 `AT*EXINFO?` 查询开机原因（6=异常重启，8=看门狗重启）

### Q7：ADC 读数不准确

- 确保被测电压不超过 3.8V（超压会损坏模块）
- 先打开通道再读取：先 `AT+CADC=0,1`，再 `AT+CADC=0,2`
- ADC 引脚信号源阻抗尽量低（<1kΩ）

### Q8：如何判断 AT 固件版本

```
AT+CGMR                     // 返回固件版本号
// 示例：AirM2M_780E_V1121_LTE_AT
// V后面的数字即版本号（1121），用于判断支持哪些 AT 命令特性
```

---

## 附录：AT命令版本依赖表

| AT 命令 | 最低固件版本 | 说明 |
|---------|------------|------|
| `AT+CSCLK=3` | ≥V1026 | 睡眠模式3（超低功耗） |
| `AT+POWERMODE` | ≥V1143 (LPAT) | 超低功耗 PSM+ 模式 |
| `AT+CADC=0,2` | ≥V1115 | ADC 电压读取 |
| `AT+CGPIO` | ≥V1115 | GPIO 控制 |
| `AT^WAKEUPHEX` | ≥V1131 | 唤醒字符串设置 |
| `AT+WIFISCAN` | ≥V1150 | WiFi 扫描 |
| `AT+UARTUPGRADE` | ≥V1155 | 串口升级 |
| `AT+IOSET` | ≥V1155 | IO 电平控制 |
| `AT+HTTPGETTOFS` | ≥V1148 | HTTP 下载保存到文件系统 |
| `AT+HTTPEXACTION` | ≥V1106 | HTTP 扩展操作 |

---

*文档整理自：[银尔达 YED-C780 产品页](https://yinerda.yuque.com/yt1fh6/4gdtu/xm1e8782d2n5ig3u) 及《上海合宙Cat.1模组AT命令手册V1.4.9》*
