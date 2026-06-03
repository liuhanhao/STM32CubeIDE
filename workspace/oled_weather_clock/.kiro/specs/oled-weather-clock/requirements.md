# 需求文档

## 简介

本功能为基于 STM32F103C8T6 的 OLED 天气钟系统。系统通过 ESP01S WiFi 模块连接网络，从 NTP 服务器同步时间并写入 STM32 内置 RTC，从和风天气 API（QWeather）获取当前天气数据，通过 DHT11 传感器采集本地温湿度，最终将时间、天气、本地温湿度三类信息在 128×64 OLED 屏幕上分页轮流显示。

## 术语表

- **System**：整个 OLED 天气钟嵌入式系统（STM32F103C8T6 主控 + 所有外设）
- **RTC**：STM32F103C8T6 内置实时时钟模块
- **OLED_Driver**：已有的 OLED 显示驱动模块（软件 I2C，PB8=SCL / PB9=SDA，128×64，8×16 字体）
- **ESP01S_Module**：通过 USART1（PA9=TX / PA10=RX）与主控通信的 ESP01S WiFi 模块
- **AT_Controller**：负责向 ESP01S_Module 发送 AT 指令并解析响应的软件模块
- **NTP_Client**：通过 AT_Controller 连接 NTP 服务器、解析 NTP 报文并输出 UTC 时间戳的软件模块
- **Weather_Client**：通过 AT_Controller 向和风天气 API 发起 HTTP 请求并解析 JSON 响应的软件模块
- **DHT11_Driver**：驱动 DHT11 温湿度传感器、采集本地温湿度数据的软件模块
- **Display_Manager**：管理三个显示页面定时切换逻辑的软件模块
- **Page_Clock**：主屏页面，显示当前时间（大字）与日期
- **Page_Weather**：天气屏页面，显示联网天气状况与温度范围
- **Page_DHT11**：温湿度屏页面，显示 DHT11 本地温度与湿度
- **NTP服务器**：提供网络时间协议服务的外部服务器（pool.ntp.org）
- **QWeather API**：和风天气开放平台提供的天气数据 HTTP API

---

## 需求

### 需求 1：系统初始化

**用户故事：** 作为用户，我希望设备上电后能自动完成所有外设初始化，使系统进入正常工作状态，而无需手动配置。

#### 验收标准

1. THE System SHALL 在上电后依次完成 USART1、RTC、DHT11_Driver 和 OLED_Driver 的初始化，然后进入主循环。
2. WHEN OLED_Driver 初始化完成，THE System SHALL 在 OLED 屏幕上显示启动提示字符串 "Booting..."。
3. IF 任意外设初始化失败，THEN THE System SHALL 在 OLED 屏幕上显示对应模块名称的错误提示字符串，并停止执行后续初始化步骤。

---

### 需求 2：WiFi 连接

**用户故事：** 作为用户，我希望设备上电后能自动连接预设 WiFi 网络，以便后续进行网络时间同步和天气数据获取。

#### 验收标准

1. WHEN System 初始化完成，THE AT_Controller SHALL 向 ESP01S_Module 发送 AT+CWJAP 指令以连接预设 WiFi 网络。
2. WHEN ESP01S_Module 返回 "WIFI CONNECTED" 字符串，THE AT_Controller SHALL 将 WiFi 连接状态标记为已连接。
3. IF AT_Controller 在发送 AT+CWJAP 指令后 10 秒内未收到 "WIFI CONNECTED" 字符串，THEN THE AT_Controller SHALL 将 WiFi 连接状态标记为未连接，并通知 Display_Manager 显示 "WiFi Fail" 提示。
4. WHILE WiFi 连接状态为已连接，THE System SHALL 继续执行 NTP 时间同步流程。

---

### 需求 3：NTP 时间同步

**用户故事：** 作为用户，我希望设备上电联网后能从 NTP 服务器获取准确时间并同步到 RTC，以便在断网后仍能依靠 RTC 正常计时。

#### 验收标准

1. WHEN WiFi 连接状态变为已连接，THE NTP_Client SHALL 通过 AT_Controller 向 pool.ntp.org 的 UDP 端口 123 发送 NTP 请求报文。
2. WHEN NTP_Client 收到合法的 NTP 响应报文，THE NTP_Client SHALL 解析报文中的 Transmit Timestamp 字段，将 UTC 时间换算为 UTC+8 北京时间，并将年、月、日、时、分、秒写入 RTC。
3. IF NTP_Client 在发送请求后 5 秒内未收到合法的 NTP 响应报文，THEN THE NTP_Client SHALL 重试最多 3 次；若 3 次均超时，THE System SHALL 保持 RTC 当前已有时间并继续运行。
4. WHEN NTP 同步成功，THE System SHALL 此后由 RTC 独立计时，不再依赖网络时间。

---

### 需求 4：天气数据获取

**用户故事：** 作为用户，我希望设备能定期从和风天气 API 获取最新天气数据，以便在天气屏上展示实时天气状况。

#### 验收标准

1. WHEN NTP 时间同步完成，THE Weather_Client SHALL 通过 AT_Controller 向和风天气 API 发起 HTTP GET 请求，获取当前城市的实时天气数据（天气状况文本、最高温度、最低温度）。
2. WHEN Weather_Client 收到 HTTP 200 响应，THE Weather_Client SHALL 解析 JSON 响应体，提取 `text`（天气状况）、`tempMax`（最高温度）、`tempMin`（最低温度）字段，并将结果存入全局天气数据缓存。
3. IF HTTP 响应状态码不为 200 或 JSON 解析失败，THEN THE Weather_Client SHALL 保留上一次成功获取的天气数据缓存，并将天气状况字段标记为 "No Data"。
4. WHEN 天气数据缓存更新成功，THE Weather_Client SHALL 每隔 30 分钟重新发起一次天气数据请求。
5. WHILE WiFi 连接状态为未连接，THE Weather_Client SHALL 跳过天气数据请求，直到 WiFi 连接恢复。

---

### 需求 5：DHT11 本地温湿度采集

**用户故事：** 作为用户，我希望设备能持续采集本地环境的温度和湿度，以便在温湿度屏上展示实时本地数据。

#### 验收标准

1. THE DHT11_Driver SHALL 每隔 2 秒向 DHT11 传感器发起一次采集请求，并将采集结果（整数部分温度、整数部分湿度）存入全局本地气象数据缓存。
2. WHEN DHT11_Driver 完成一次采集，THE DHT11_Driver SHALL 将读取到的温度值限定在 -20°C 至 60°C 范围内、湿度值限定在 0% 至 100% 范围内，超出范围的值视为无效。
3. IF DHT11_Driver 连续 3 次采集失败（超时或校验错误），THEN THE DHT11_Driver SHALL 将本地气象数据缓存中的温度和湿度标记为无效，并在 Page_DHT11 页面显示 "--" 占位符。
4. THE DHT11_Driver SHALL 每次采集完成后至少等待 1 秒再发起下一次采集，以满足 DHT11 传感器的最短采样间隔要求。

---

### 需求 6：OLED 分页显示

**用户故事：** 作为用户，我希望 OLED 屏幕能在三个页面之间自动循环切换，使我能依次看到时钟、天气和本地温湿度信息。

#### 验收标准

1. THE Display_Manager SHALL 按照 Page_Clock → Page_Weather → Page_DHT11 的顺序循环切换页面，每个页面的显示时长为 5 秒。
2. WHEN Display_Manager 切换到 Page_Clock，THE OLED_Driver SHALL 在屏幕第 1 行显示 HH:MM:SS 格式的当前时间，在第 3 行显示 YYYY-MM-DD 格式的当前日期，时间和日期数据均从 RTC 读取。
3. WHEN Display_Manager 切换到 Page_Weather，THE OLED_Driver SHALL 在屏幕第 1 行显示天气状况字符串（最长 16 个 ASCII 字符），在第 3 行显示格式为 "Hi:XXC Lo:XXC" 的最高和最低温度，数据来源为天气数据缓存。
4. WHEN Display_Manager 切换到 Page_DHT11，THE OLED_Driver SHALL 在屏幕第 1 行显示格式为 "Temp: XX C" 的本地温度，在第 3 行显示格式为 "Humi: XX %" 的本地湿度，数据来源为本地气象数据缓存。
5. WHEN Display_Manager 切换页面，THE OLED_Driver SHALL 先调用 OLED_Clear 清屏，再绘制新页面内容，切换时间不超过 100ms。
6. IF RTC 读取失败，THEN THE OLED_Driver SHALL 在 Page_Clock 对应的时间和日期位置显示 "--:--:--" 和 "----/--/--" 占位符。

---

### 需求 7：USART1 串口通信

**用户故事：** 作为用户，我希望 STM32 能通过 USART1 与 ESP01S 模块可靠地收发 AT 指令和数据，以支持 WiFi 连接、NTP 同步及天气数据获取。

#### 验收标准

1. THE System SHALL 以波特率 115200 bps、8 位数据位、无奇偶校验、1 位停止位初始化 USART1（PA9=TX，PA10=RX）。
2. WHEN AT_Controller 发送 AT 指令，THE AT_Controller SHALL 在指令字符串末尾附加 "\r\n" 后通过 USART1 发送。
3. THE AT_Controller SHALL 使用接收缓冲区（不少于 512 字节）收集 USART1 接收中断中的数据，并在主循环中解析缓冲区内容以提取 AT 响应字符串。
4. IF AT_Controller 在规定超时时间内未收到预期响应字符串，THEN THE AT_Controller SHALL 返回超时错误码给调用模块。

---

### 需求 8：Flash 体积约束

**用户故事：** 作为开发者，我希望所有新增代码编译后的 Flash 占用总量控制在安全范围内，以确保能烧录至 STM32F103C8T6 的 64KB Flash。

#### 验收标准

1. THE System SHALL 确保全部代码（含已有 OLED 驱动、HAL 库及新增模块）编译后的 Flash 使用量不超过 56KB（为 Bootloader 和调试保留 8KB 余量）。
2. WHERE 轻量 JSON 解析方案可用，THE Weather_Client SHALL 使用基于字符串搜索的轻量解析方式替代完整 JSON 库，以减少 Flash 占用。
