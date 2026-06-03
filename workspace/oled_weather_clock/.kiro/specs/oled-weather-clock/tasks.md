# 实现计划：OLED 天气钟（oled-weather-clock）

## 概述

基于 STM32F103C8T6 裸机 C 工程，在现有 OLED.c / delay.c 不变的基础上，新增
at_controller、ntp_client、weather_client、dht11、display_manager 五个模块，并修改
main.c 完成系统初始化与主循环调度。整体采用时间片轮询架构，无 RTOS。

---

## 任务

- [x] 1. 新建公共头文件与数据模型
  - [x] 1.1 创建共享数据结构头文件
    - 在 `Core/Inc/` 新建 `app_globals.h`
    - 定义 `WiFiStatus_t`、`WeatherData_t`（含 `text[17]`、`tempMax`、`tempMin`、`valid`）、`DHT11Data_t`（含 `temperature`、`humidity`、`valid`）、`DisplayPage_t`（PAGE_CLOCK / PAGE_WEATHER / PAGE_DHT11 / PAGE_COUNT）
    - 使用 `#ifndef` 头文件保护，包含必要的 `stdint.h`
    - _需求：数据模型为需求 1–6 全模块共用_

  - [x] 1.2 创建 `at_controller.h` 接口头文件
    - 在 `Core/Inc/` 新建 `at_controller.h`
    - 声明 `AT_Init()`、`AT_ClearRxBuf()`、`AT_Send()`、`AT_SendWait()`、`AT_Contains()`、`AT_GetRxBuf()`、`AT_USART1_IRQHandler()`
    - 定义 `AT_RX_BUF_SIZE 512`、`AT_TX_BUF_SIZE 256`
    - _需求：7.1、7.2、7.3、7.4_

  - [x] 1.3 创建 `ntp_client.h`、`weather_client.h`、`dht11.h`、`display_manager.h`
    - `ntp_client.h`：声明 `NTP_Sync()`，定义 `NTP_EPOCH_OFFSET`、`UTC8_OFFSET_SEC`
    - `weather_client.h`：声明 `Weather_Fetch()`，定义 `WEATHER_TEXT_LEN 17`；引用 `app_globals.h`
    - `dht11.h`：声明 `DHT11_Init()`、`DHT11_Read(DHT11Data_t *out)`；定义 `DHT11_GPIO_PORT GPIOA`、`DHT11_GPIO_PIN GPIO_PIN_0`
    - `display_manager.h`：声明 `Display_Init()`、`Display_Update()`、`Display_ShowBoot(const char *msg)`；定义 `PAGE_SWITCH_INTERVAL_MS 5000U`
    - _需求：2~6 全模块接口_

- [x] 2. 实现 AT_Controller 模块
  - [x] 2.1 实现 `Core/Src/at_controller.c`
    - 声明 512 字节环形接收缓冲区（`rx_buf[AT_RX_BUF_SIZE]`）及读写索引
    - 实现 `AT_Init()`：调用 `MX_USART1_UART_Init()`（或直接写 HAL 初始化代码），启用 RXNE 中断
    - 实现 `AT_ClearRxBuf()`：重置读写索引
    - 实现 `AT_Send()`：通过 `HAL_UART_Transmit` 发送字符串，再发送 `"\r\n"`（满足属性 1）
    - 实现 `AT_Contains()`：在环形缓冲区线性快照中执行 `strstr()`（满足属性 2）
    - 实现 `AT_GetRxBuf()`：返回缓冲区只读指针
    - 实现 `AT_SendWait()`：清缓冲、发送、轮询 `AT_Contains()` 直到超时
    - 实现 `AT_USART1_IRQHandler()`：读取 `huart1` DR，写入环形缓冲区
    - _需求：7.1、7.2、7.3、7.4_

  - [ ]* 2.2 为属性 1（AT 指令 \r\n 结尾）编写属性测试
    - **属性 1：AT 指令末尾格式**
    - **验证：需求 7.2**
    - 在宿主机编译单元测试框架（如 Unity 或 CUnit）中，mock UART 输出捕获发送字节序列
    - 对任意长度的指令字符串，验证最后两字节为 0x0D 0x0A

  - [ ]* 2.3 为属性 2（WiFi 连接状态响应解析）编写属性测试
    - **属性 2：WiFi 连接状态响应解析**
    - **验证：需求 2.2**
    - 枚举包含 / 不包含 "WIFI CONNECTED" 的字符串，验证 `AT_Contains()` 返回值

- [x] 3. 实现 NTP_Client 模块
  - [x] 3.1 实现 `Core/Src/ntp_client.c`
    - 构造 48 字节 NTP v3 请求包（LI=0, VN=3, Mode=3，其余字段置 0）
    - 通过 AT 指令序列建立 UDP 连接到 `pool.ntp.org:123`，发送请求，接收 `+IPD,48:` 后的 48 字节响应
    - 解析 Transmit Timestamp（偏移 40~43 字节，大端序），调用 `ntp_parse_timestamp()` 换算为 UTC+8 并写入 RTC（`HAL_RTC_SetTime` / `HAL_RTC_SetDate`）
    - `NTP_Sync()` 内实现最多 3 次重试，每次 5 秒超时；全部失败时返回 0，保持 RTC 原有值（需求 3.3）
    - _需求：3.1、3.2、3.3、3.4_

  - [ ]* 3.2 为属性 3（NTP UTC→UTC+8 换算）编写属性测试
    - **属性 3：NTP 时间戳 UTC→UTC+8 换算**
    - **验证：需求 3.2**
    - 对范围 `[NTP_EPOCH_OFFSET, 0xFFFFFFFF]` 内随机 NTP 秒数值，验证：
      - `beijing_hour == ((ntp_sec - NTP_EPOCH_OFFSET) / 3600 + 8) % 24`
      - 小时 ∈ [0, 23]，分钟 ∈ [0, 59]，秒 ∈ [0, 59]

- [x] 4. 实现 DHT11_Driver 模块
  - [x] 4.1 实现 `Core/Src/dht11.c`
    - 实现 `dht11_set_output()` / `dht11_set_input()`：动态切换 PA0 引脚模式（推挽输出 / 浮空输入，使用 GPIO_NOPULL，因模块已内置上拉电阻）
    - 实现 `DHT11_Init()`：初始化引脚并拉高总线
    - 实现 `DHT11_Read()`：完整单总线时序——主机拉低 ≥18ms、等待响应（低 80μs + 高 80μs）、接收 40 位数据，使用 `delay_us()` 精确计时
    - 校验：`data[0]+data[1]+data[2]+data[3] == (data[4] & 0xFF)`，失败返回 0
    - 范围验证：温度 −20~60°C、湿度 0~100%，超出范围置 `valid=0`（满足属性 5）
    - _需求：5.1、5.2、5.4_

  - [ ]* 4.2 为属性 5（DHT11 数值范围约束）编写属性测试
    - **属性 5：DHT11 数值范围约束**
    - **验证：需求 5.2**
    - 对随机 `(raw_temp, raw_humi)` 对：若超出合法范围则 `valid==0`；若在范围内则 `valid==1` 且值与输入相等

- [x] 5. 实现 Weather_Client 模块
  - [x] 5.1 实现 `Core/Src/weather_client.c`
    - 实现 `json_find_value()`：基于 `strstr()` + 手动指针偏移提取 `"key":"value"` 格式字段（满足需求 8.2）
    - 通过 AT 指令建立 TCP 连接到 `devapi.qweather.com:80`，发送 HTTP GET 请求（城市 ID 与 API Key 作为宏定义在文件顶部）
    - 检查响应中是否含 `"200"`（HTTP 状态码），若包含则提取 `text`（截断至 16 字符）、`tempMax`、`tempMin` 写入 `g_weather_data`，置 `valid=1`
    - 失败时：保留缓存；若 `valid==0` 则填入 `"No Data"`（需求 4.3）
    - _需求：4.1、4.2、4.3、8.2_

  - [ ]* 5.2 为属性 4（天气 JSON 字段提取）编写属性测试
    - **属性 4：天气 JSON 字段提取**
    - **验证：需求 4.2**
    - 对任意含合法 `"text":"..."` 字段的 JSON 字符串，验证提取长度 ≤ 16；对含 `"tempMax":"XX"` / `"tempMin":"XX"` 的字符串，验证整数解析与原始值一致

- [x] 6. 实现 Display_Manager 模块
  - [x] 6.1 实现 `Core/Src/display_manager.c`
    - 实现 `display_page_clock()`：调用 `HAL_RTC_GetTime` / `HAL_RTC_GetDate`，`snprintf` 格式化为 `HH:MM:SS`（第 1 行）和 `20YY-MM-DD`（第 3 行）；失败时显示占位符（满足属性 7、需求 6.6）
    - 实现 `display_page_weather()`：从 `g_weather_data` 读取，格式化第 1 行天气文本（满足属性 8）和第 3 行 `"Hi:%2dC Lo:%2dC"`（满足属性 8、需求 6.3）
    - 实现 `display_page_dht11()`：`g_dht11_data.valid` 为 1 时格式化显示，否则显示 `"--"`（需求 6.4）
    - 实现 `Display_Update()`：页面轮转状态机，5 秒切换一次，顺序严格为 PAGE_CLOCK → PAGE_WEATHER → PAGE_DHT11 → 循环（满足属性 6），切换时先 `OLED_Clear()`（需求 6.5）
    - 实现 `Display_Init()`：初始化 `s_page_tick`，立即绘制第一个页面
    - 实现 `Display_ShowBoot()`：直接调用 `OLED_ShowString()` 显示启动信息
    - _需求：6.1、6.2、6.3、6.4、6.5、6.6_

  - [ ]* 6.2 为属性 6（页面轮换顺序不变量）编写属性测试
    - **属性 6：页面轮换顺序不变量**
    - **验证：需求 6.1**
    - 对任意初始页面 P，验证 `next_page == (P + 1) % PAGE_COUNT`

  - [ ]* 6.3 为属性 7 和属性 8（时钟/天气/温湿度格式化）编写属性测试
    - **属性 7：时钟页面时间格式化**、**属性 8：天气与温湿度页面格式化**
    - **验证：需求 6.2、6.3、6.4**
    - 对合法范围内随机 RTC 值，验证输出字符串长度与固定格式（`HH:MM:SS`、`20YY-MM-DD`、`Hi:%2dC Lo:%2dC`、`Temp: XX C`、`Humi: XX %`）匹配

- [x] 7. 修改 main.c —— 集成初始化与主循环
  - [x] 7.1 添加 RTC 与 USART1 初始化
    - 在 `main.c` 中添加 `RTC_HandleTypeDef hrtc` 和 `UART_HandleTypeDef huart1` 全局声明
    - 实现 `MX_RTC_Init()`：配置 LSE 32.768kHz（或 LSI 备选），预分频为 `RTC_AUTO_1_SECOND`，首次上电默认时间 2000-01-01 00:00:00
    - 实现 `MX_USART1_UART_Init()`：115200/8N1，开启 RXNE 中断（需求 7.1）
    - 在 `stm32f1xx_it.c` 中的 `USART1_IRQHandler` 里调用 `AT_USART1_IRQHandler()`
    - 定义全局变量 `g_wifi_status`、`g_weather_data`、`g_dht11_data`
    - _需求：1.1、7.1_

  - [x] 7.2 编写启动序列
    - 在 `main()` 中按顺序调用：`HAL_Init()` → `SystemClock_Config()` → `MX_GPIO_Init()` → `MX_RTC_Init()` → `MX_USART1_UART_Init()` → `OLED_Init()` → `Display_ShowBoot("Booting...")`
    - 调用 `AT_Init()`，`DHT11_Init()`
    - WiFi 连接：`AT_SendWait("AT+CWJAP=\"SSID\",\"PWD\"", "WIFI CONNECTED", 10000)`；成功则置 `g_wifi_status = WIFI_CONNECTED`，依次调用 `NTP_Sync()` 和 `Weather_Fetch()`；失败则 `Display_ShowBoot("WiFi Fail")`（需求 2.3）
    - 调用 `Display_Init()` 进入分页显示
    - _需求：1.1、1.2、1.3、2.1、2.3、2.4_

  - [x] 7.3 编写主循环轮询调度
    - 在 `while(1)` 中维护 `dht11_tick`（每 2000ms）、`dht11_fail_count`（连续失败 ≥3 次置 `valid=0`，需求 5.3）
    - WiFi 已连接时维护 `weather_tick`（每 1800000ms 调用 `Weather_Fetch()`，需求 4.4）
    - 每次循环末尾调用 `Display_Update()`
    - _需求：4.4、5.1、5.3、6.1_

- [x] 8. 检查点 —— 确保所有代码可编译
  - 在 STM32CubeIDE 中执行 Project → Build，确保 0 错误
  - 检查 Flash 占用：.text + .rodata ≤ 57344 字节（56KB）（需求 8.1）
  - 若有编译错误，根据错误信息修复相应模块，向用户报告问题后继续

---

## 说明

- 标有 `*` 的子任务为可选测试任务，适合在宿主机（PC）上以独立编译单元测试的方式运行，不影响嵌入式主工程编译
- 每个任务均引用了对应的需求编号，便于追溯
- 属性测试对应 design.md 中的属性 1~8，验证核心正确性约束
- WiFi SSID/密码和 QWeather API Key / 城市 ID 以宏定义方式集中在各自 `.c` 文件顶部，便于修改
- `OLED.c`、`OLED.h`、`OLED_Font.h`、`delay.c`、`delay.h` 保持不变

## 任务依赖图

```json
{
  "waves": [
    { "id": 0, "tasks": ["1.1", "1.2", "1.3"] },
    { "id": 1, "tasks": ["2.1", "4.1"] },
    { "id": 2, "tasks": ["2.2", "2.3", "3.1", "4.2", "5.1"] },
    { "id": 3, "tasks": ["3.2", "5.2", "6.1"] },
    { "id": 4, "tasks": ["6.2", "6.3", "7.1"] },
    { "id": 5, "tasks": ["7.2"] },
    { "id": 6, "tasks": ["7.3"] }
  ]
}
```
