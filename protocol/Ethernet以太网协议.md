# Ethernet 以太网协议

## 概述

以太网（Ethernet）是目前最广泛使用的局域网技术，单片机通过 **MAC + PHY** 芯片实现以太网通讯，常用于 IoT 设备、工业控制器等需要网络连接的场合。

- 通讯方式：全双工（100BASE-TX）
- 所需引脚：通过 PHY 芯片连接，MCU 与 PHY 之间使用 MII/RMII 接口
- 典型速度：10 Mbps、100 Mbps、1 Gbps
- 协议栈：以太网只是物理层和数据链路层，上层通常运行 TCP/IP 协议栈

---

## 硬件架构

```
单片机（MCU）
  ┌─────────────────────────────────────────┐
  │  应用层（HTTP/MQTT/Modbus TCP）          │
  │  传输层（TCP/UDP）                       │
  │  网络层（IP/ICMP/ARP）                  │
  │  MAC（媒体访问控制，内置于MCU）          │
  └──────────────┬──────────────────────────┘
                 │ RMII/MII 接口（数据总线）
                 │ MDIO/MDC 接口（管理总线）
  ┌──────────────▼──────────────────────────┐
  │  PHY 芯片（LAN8720、DP83848 等）        │
  │  负责模拟信号处理、编解码               │
  └──────────────┬──────────────────────────┘
                 │ RJ45 网口
                 │ 差分信号（TX+/TX-/RX+/RX-）
              网络交换机 / 路由器
```

---

## 以太网帧结构

```
┌──────────┬──────────┬──────┬──────────────────┬─────┐
│ 目标MAC  │ 源MAC    │ 类型 │    数据（负载）   │ FCS │
│  6字节   │  6字节   │ 2字节│  46～1500字节    │ 4字节│
└──────────┴──────────┴──────┴──────────────────┴─────┘
```

| 字段 | 说明 |
|------|------|
| 目标MAC | 接收方 MAC 地址，FF:FF:FF:FF:FF:FF 为广播 |
| 源MAC | 发送方 MAC 地址（全球唯一） |
| 类型 | 0x0800=IPv4，0x0806=ARP，0x86DD=IPv6 |
| 数据 | 上层协议数据（IP 包等） |
| FCS | 帧校验序列（CRC32） |

---

## TCP/IP 协议栈层次

```
应用层:   HTTP、MQTT、FTP、DNS、NTP
          ↕
传输层:   TCP（可靠）、UDP（快速）
          ↕
网络层:   IP（路由）、ICMP（ping）、ARP（地址解析）
          ↕
链路层:   以太网帧（MAC地址）
          ↕
物理层:   PHY芯片（电信号）
```

---

## 完整通讯示例

### 示例1：单片机发送 HTTP GET 请求

**场景：** IoT 设备通过以太网向服务器上报温度数据

```
单片机（IP: 192.168.1.100）        服务器（IP: 192.168.1.1）
         │                                    │
         │ 步骤1: ARP 查询服务器MAC地址        │
         │── ARP Request（广播）──────────────→│ 所有设备
         │   "谁是 192.168.1.1？"              │
         │←── ARP Reply ──────────────────────│
         │   "我是，MAC=AA:BB:CC:DD:EE:FF"     │
         │                                    │
         │ 步骤2: TCP 三次握手                 │
         │── SYN（seq=100）──────────────────→│
         │←── SYN+ACK（seq=200, ack=101）─────│
         │── ACK（ack=201）──────────────────→│
         │   TCP 连接建立完成                  │
         │                                    │
         │ 步骤3: 发送 HTTP 请求               │
         │── HTTP GET /api/temp?value=25.6 ──→│
         │←── HTTP 200 OK（{"status":"ok"}）──│
         │                                    │
         │ 步骤4: TCP 四次挥手（关闭连接）      │
         │── FIN ────────────────────────────→│
         │←── ACK ────────────────────────────│
         │←── FIN ────────────────────────────│
         │── ACK ────────────────────────────→│
```

### 代码示例（使用 LwIP 协议栈）

```c
// 使用 LwIP 发送 HTTP GET 请求
void HTTP_SendTemperature(float temperature) {
    struct tcp_pcb *pcb;
    ip_addr_t server_ip;

    // 服务器 IP 地址
    IP4_ADDR(&server_ip, 192, 168, 1, 1);

    // 创建 TCP 连接
    pcb = tcp_new();
    tcp_connect(pcb, &server_ip, 80, http_connected_callback);
}

// TCP 连接成功回调
err_t http_connected_callback(void *arg, struct tcp_pcb *pcb, err_t err) {
    char request[256];
    float temp = 25.6;

    // 构建 HTTP GET 请求
    snprintf(request, sizeof(request),
        "GET /api/temp?value=%.1f HTTP/1.1\r\n"
        "Host: 192.168.1.1\r\n"
        "Connection: close\r\n"
        "\r\n",
        temp);

    // 发送数据
    tcp_write(pcb, request, strlen(request), TCP_WRITE_FLAG_COPY);
    tcp_output(pcb);
    return ERR_OK;
}
```

---

### 示例2：UDP 广播发现设备

**场景：** IoT 设备通过 UDP 广播宣告自己的存在

```
单片机（IP: 192.168.1.100）        局域网内所有设备
         │                                    │
         │── UDP 广播（目标IP: 255.255.255.255）→│
         │   端口: 8888                        │
         │   数据: {"device":"sensor01",       │
         │          "ip":"192.168.1.100",      │
         │          "type":"temperature"}      │
         │                                    │
         │   所有监听 8888 端口的设备都能收到   │
```

```c
// UDP 广播发送设备信息
void UDP_BroadcastDeviceInfo(void) {
    struct udp_pcb *pcb;
    struct pbuf *p;
    ip_addr_t broadcast_ip;
    char msg[] = "{\"device\":\"sensor01\",\"ip\":\"192.168.1.100\"}";

    IP4_ADDR(&broadcast_ip, 255, 255, 255, 255);

    pcb = udp_new();
    udp_bind(pcb, IP_ADDR_ANY, 8888);

    p = pbuf_alloc(PBUF_TRANSPORT, strlen(msg), PBUF_RAM);
    memcpy(p->payload, msg, strlen(msg));

    udp_sendto(pcb, p, &broadcast_ip, 8888);
    pbuf_free(p);
    udp_remove(pcb);
}
```

---

### 示例3：MQTT 协议（IoT 最常用）

**场景：** 单片机通过 MQTT 向云平台上报传感器数据

```
单片机（MQTT Client）              MQTT Broker（服务器）
         │                                    │
         │── CONNECT ────────────────────────→│
         │←── CONNACK（连接成功）─────────────│
         │                                    │
         │── PUBLISH ────────────────────────→│
         │   Topic: "home/sensor/temperature" │
         │   Payload: "25.6"                  │
         │   QoS: 1                           │
         │←── PUBACK ─────────────────────────│ （QoS=1时确认）
         │                                    │
         │   订阅了该主题的其他设备自动收到数据 │
```

---

## ARP 地址解析过程

IP 通讯前必须知道对方的 MAC 地址，ARP 负责解析：

```
1. 单片机要发数据给 192.168.1.1，但不知道其 MAC 地址
2. 发送 ARP 广播：
   "局域网内所有人，谁的 IP 是 192.168.1.1？我的 MAC 是 11:22:33:44:55:66"
3. 192.168.1.1 收到后回复：
   "是我，我的 MAC 是 AA:BB:CC:DD:EE:FF"
4. 单片机将 IP→MAC 的映射存入 ARP 缓存表
5. 后续通讯直接使用缓存，无需再次 ARP
```

---

## DHCP 自动获取 IP 地址

```
单片机上电后：
1. 发送 DHCP Discover（广播）："有 DHCP 服务器吗？"
2. 路由器回复 DHCP Offer："给你 IP 192.168.1.100，有效期24小时"
3. 单片机发送 DHCP Request："我要这个 IP"
4. 路由器回复 DHCP ACK："确认，IP 已分配给你"
5. 单片机配置 IP、子网掩码、网关、DNS
```

---

## 典型应用

- IoT 传感器数据上报（HTTP/MQTT）
- 工业设备远程监控（Modbus TCP）
- 网络摄像头（RTSP 视频流）
- 嵌入式 Web 服务器（设备配置页面）
- OTA 固件升级（HTTP 下载）
