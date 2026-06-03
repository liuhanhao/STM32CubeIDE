# 技术设计文档

## 简介

本文档描述 OLED 天气钟（oled-weather-clock）在 STM32F103C8T6 上的完整技术架构设计。系统采用裸机轮询架构，无 RTOS，通过分层模块化设计将硬件驱动、网络通信、数据处理和显示逻辑解耦。

---

## 架构总览

```
┌─────────────────────────────────────────────────────────┐
│                        main.c                           │
│          系统初始化 → 主循环（轮询调度）                  │
└───────────┬──────────────┬──────────────────────────────┘
            │              │
     ┌──────▼──────┐  ┌────▼────────────────────────────┐
     │Display_Manager│  │       任务调度（时间片轮询）      │
     │  页面切换状态机 │  │  NTP刷新 / 天气刷新 / DHT11采集  │
     └──────┬──────┘  └──────────────────────────────────┘
            │
  ┌─────────┼─────────────────────┐
  │         │                     │
┌─▼──────┐ ┌▼───────────┐ ┌──────▼───────┐
│Page_   │ │Page_Weather│ │ Page_DHT11   │
│Clock   │ │            │ │              │
└─▼──────┘ └─▼──────────┘ └──────▼───────┘
  │RTC读取   │天气缓存读取    │DHT11缓存读取
  │        │               │
┌─▼──────┐ ┌▼───────────┐ ┌──────▼───────┐
│RTC     │ │Weather_    │ │DHT11_Driver  │
│(HAL)   │ │Client      │ │              │
└────────┘ └─▼──────────┘ └──────────────┘
             │AT_Controller
           ┌─▼──────────┐
           │USART1驱动   │
           │(中断接收)    │
           └────────────┘
```

---

## 硬件资源分配

| 外设    | 引脚/接口      | 配置                             | 用途              |
| ------- | ------------- | -------------------------------- | ----------------- |
| GPIOB8  | PB8           | 开漏输出，高速                   | OLED SCL（软件 I2C）|
| GPIOB9  | PB9           | 开漏输出，高速                   | OLED SDA（软件 I2C）|
| USART1  | PA9/PA10      | 115200/8N1，中断接收             | ESP01S AT 通信    |
| DHT11   | 待定（建议PA0）| 推挽/输入动态切换                | 温湿度单总线       |
| RTC     | 内部           | LSE 32.768kHz（或 LSI 备选）     | 实时时钟          |
| SysTick | 内部           | 1ms 中断（HAL 默认）             | 延时与时间片计数   |

> **DHT11 引脚选择建议：** 使用 PA0（或任意空余 GPIO），远离 USART 引脚避免干扰。

---

## 模块划分与文件结构

新增文件全部放入 `Core/Src/` 和 `Core/Inc/`，保持现有工程结构。

```
Core/
├── Inc/
│   ├── main.h              （已有）
│   ├── delay.h             （已有）
│   ├── at_controller.h     【新建】AT 指令收发
│   ├── ntp_client.h        【新建】NTP 时间同步
│   ├── weather_client.h    【新建】天气数据获取
│   ├── dht11.h             【新建】DHT11 温湿度驱动
│   └── display_manager.h   【新建】OLED 分页显示管理
├── Src/
│   ├── OLED.c / OLED.h     （已有，不修改）
│   ├── OLED_Font.h         （已有，不修改）
│   ├── delay.c             （已有，不修改）
│   ├── main.c              【修改】集成初始化与主循环
│   ├── at_controller.c     【新建】
│   ├── ntp_client.c        【新建】
│   ├── weather_client.c    【新建】
│   ├── dht11.c             【新建】
│   └── display_manager.c   【新建】
```

---

## 数据模型

### 全局共享数据结构

```c
/* at_controller.h */
#define AT_RX_BUF_SIZE  512   /* 接收缓冲区大小，满足需求 7.3 */
#define AT_TX_BUF_SIZE  256

/* 连接状态枚举 */
typedef enum {
    WIFI_DISCONNECTED = 0,
    WIFI_CONNECTED    = 1
} WiFiStatus_t;

/* weather_client.h */
#define WEATHER_TEXT_LEN  17  /* 最长 16 字符 + '\0' */

typedef struct {
    char    text[WEATHER_TEXT_LEN]; /* 天气状况文本，异常时为 "No Data" */
    int8_t  tempMax;                /* 最高温度，单位 °C */
    int8_t  tempMin;                /* 最低温度，单位 °C */
    uint8_t valid;                  /* 0 = 无效/未获取，1 = 有效 */
} WeatherData_t;

/* dht11.h */
typedef struct {
    int8_t  temperature;  /* 温度整数部分，单位 °C */
    uint8_t humidity;     /* 湿度整数部分，单位 % */
    uint8_t valid;        /* 0 = 无效，1 = 有效 */
} DHT11Data_t;

/* display_manager.h */
typedef enum {
    PAGE_CLOCK   = 0,
    PAGE_WEATHER = 1,
    PAGE_DHT11   = 2,
    PAGE_COUNT   = 3
} DisplayPage_t;
```

### 全局变量声明

```c
/* 在 main.c 中定义，其余模块 extern 引用 */
extern volatile WiFiStatus_t g_wifi_status;
extern WeatherData_t         g_weather_data;
extern DHT11Data_t           g_dht11_data;
```

---

## 模块详细设计

### 1. AT_Controller 模块

**职责：** 封装 USART1 的发送/接收，向上层提供"发送指令 + 等待关键字"的同步接口，同时支持原始缓冲区访问。

**关键接口：**

```c
/* at_controller.h */

/* 初始化 USART1（115200/8N1，开启 RXNE 中断） */
void     AT_Init(void);

/* 清空接收缓冲区 */
void     AT_ClearRxBuf(void);

/* 发送原始字符串（自动追加 \r\n） */
void     AT_Send(const char *cmd);

/* 发送指令并等待关键字，返回 1=成功 0=超时 */
uint8_t  AT_SendWait(const char *cmd, const char *expect, uint32_t timeout_ms);

/* 检查接收缓冲区中是否包含关键字 */
uint8_t  AT_Contains(const char *keyword);

/* 获取接收缓冲区指针（只读） */
const char *AT_GetRxBuf(void);

/* USART1 中断服务例程（在 stm32f1xx_it.c 中调用） */
void     AT_USART1_IRQHandler(void);
```

**接收缓冲区设计：**
- 使用环形 FIFO 缓冲区，大小 512 字节，由 USART1 RXNE 中断填充。
- 主循环中通过 `AT_Contains()` / `AT_GetRxBuf()` 轮询解析，不在 ISR 中做字符串处理。

**AT_SendWait 实现逻辑：**

```c
uint8_t AT_SendWait(const char *cmd, const char *expect, uint32_t timeout_ms)
{
    AT_ClearRxBuf();
    AT_Send(cmd);                          /* 发送，自动追加 \r\n */
    uint32_t start = HAL_GetTick();
    while (HAL_GetTick() - start < timeout_ms)
    {
        if (AT_Contains(expect)) return 1;
    }
    return 0;                              /* 超时返回 0，由调用方处理 */
}
```

---

### 2. NTP_Client 模块

**职责：** 构造 48 字节 NTP v3 请求包，通过 AT 指令经 UDP 发送，接收响应后解析 Transmit Timestamp（偏移 40~43 字节），完成 UTC→UTC+8 换算，写入 STM32 RTC。

**关键接口：**

```c
/* ntp_client.h */

/* 执行一次完整 NTP 同步流程，返回 1=成功 0=失败 */
uint8_t NTP_Sync(void);
```

**NTP 报文与时间换算：**

```c
/* NTP 时间戳起点：1900-01-01 00:00:00 UTC */
#define NTP_EPOCH_OFFSET   2208988800UL

/* UTC+8 偏移秒数 */
#define UTC8_OFFSET_SEC    (8 * 3600UL)

/* 将 NTP 秒数转换为 Unix 时间戳，再换算为 UTC+8 */
static void ntp_parse_timestamp(uint32_t ntp_sec, RTC_TimeTypeDef *t, RTC_DateTypeDef *d)
{
    uint32_t unix_sec = ntp_sec - NTP_EPOCH_OFFSET + UTC8_OFFSET_SEC;
    /* 逐步拆分 unix_sec 为年/月/日/时/分/秒 */
    /* ...（标准 Unix 时间戳拆解逻辑）... */
}
```

**重试逻辑（对应需求 3.3）：**

```c
uint8_t NTP_Sync(void)
{
    for (uint8_t retry = 0; retry < 3; retry++)
    {
        if (ntp_send_and_receive())  /* 5 秒超时 */
        {
            ntp_write_rtc();
            return 1;
        }
    }
    return 0;  /* 3 次均失败，保持 RTC 原有值 */
}
```

**AT 指令序列：**
1. `AT+CIPSTART="UDP","pool.ntp.org",123` — 建立 UDP 连接
2. `AT+CIPSEND=48` — 声明发送 48 字节
3. 写入 48 字节 NTP 请求包
4. 接收 `+IPD,48:` 前缀后的 48 字节响应
5. `AT+CIPCLOSE` — 关闭连接

---

### 3. Weather_Client 模块

**职责：** 通过 AT 指令发起 HTTP GET 请求到和风天气 API，使用轻量字符串搜索解析 JSON 响应，提取天气数据写入全局缓存。

**关键接口：**

```c
/* weather_client.h */

/* 执行一次天气数据请求，成功更新 g_weather_data，返回 1=成功 0=失败 */
uint8_t Weather_Fetch(void);
```

**HTTP 请求格式：**

```
GET /v7/weather/now?location=<城市ID>&key=<API_KEY> HTTP/1.1\r\n
Host: devapi.qweather.com\r\n
Connection: close\r\n
\r\n
```

> **Flash 节约设计：** 不引入 cJSON 等完整库，改用 `strstr()` + 手动指针偏移提取字段值，满足需求 8.2。

**轻量 JSON 解析伪代码：**

```c
/*
 * 从响应字符串中提取 "key":"value" 或 "key":value 格式的值
 * 返回指向值起始位置的指针，len 输出值长度，失败返回 NULL
 */
static const char *json_find_value(const char *json, const char *key, uint8_t *len);
```

**示例提取逻辑：**

```c
/* 提取 "text":"晴" */
char text_buf[17] = {0};
const char *p = strstr(response, "\"text\":\"");
if (p) {
    p += 8;  /* 跳过 "text":" */
    uint8_t i = 0;
    while (*p && *p != '"' && i < 16) text_buf[i++] = *p++;
}
```

**错误处理（对应需求 4.3）：**

```c
if (!http_ok || parse_failed)
{
    /* 保留 g_weather_data 不变，仅在首次（valid==0）时填入 "No Data" */
    if (!g_weather_data.valid)
        snprintf(g_weather_data.text, WEATHER_TEXT_LEN, "No Data");
}
```

**定时刷新（对应需求 4.4）：**  
由 `main.c` 主循环中的时间片计数控制，每 30 分钟（1800000ms）调用一次 `Weather_Fetch()`。

---

### 4. DHT11_Driver 模块

**职责：** 实现 DHT11 单总线协议时序，读取温湿度原始数据，完成校验，写入全局缓存。

**关键接口：**

```c
/* dht11.h */
#define DHT11_GPIO_PORT  GPIOA
#define DHT11_GPIO_PIN   GPIO_PIN_0  /* 可在此处修改引脚 */

/* 初始化 DHT11 引脚 */
void    DHT11_Init(void);

/* 执行一次采集，返回 1=成功 0=失败（超时或校验错误） */
uint8_t DHT11_Read(DHT11Data_t *out);
```

**单总线时序：**

```
主机拉低总线 ≥18ms（起始信号）
主机拉高总线 20~40μs
等待 DHT11 响应（低 80μs → 高 80μs）
接收 40 位数据（每位：低 50μs + 高电平宽度区分 0/1）
  - 高电平 26~28μs → bit=0
  - 高电平 70μs    → bit=1
校验：data[0]+data[1]+data[2]+data[3] == data[4]（低8位）
```

**引脚模式动态切换：**

```c
static void dht11_set_output(void) {
    GPIO_InitTypeDef g = {DHT11_GPIO_PIN, GPIO_MODE_OUTPUT_PP,
                          GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH};
    HAL_GPIO_Init(DHT11_GPIO_PORT, &g);
}
static void dht11_set_input(void) {
    GPIO_InitTypeDef g = {DHT11_GPIO_PIN, GPIO_MODE_INPUT,
                          GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH};
    HAL_GPIO_Init(DHT11_GPIO_PORT, &g);
}
```

**数值范围验证（对应需求 5.2）：**

```c
/* 温度有效范围 -20~60°C，湿度 0~100%，超出范围标记无效 */
if (raw_temp < -20 || raw_temp > 60 || raw_humi > 100)
{
    out->valid = 0;
    return 0;
}
out->temperature = raw_temp;
out->humidity    = (uint8_t)raw_humi;
out->valid       = 1;
```

**连续失败计数（对应需求 5.3）：**

```c
/* 在 main.c 或 display_manager.c 中维护 */
static uint8_t dht11_fail_count = 0;
if (!DHT11_Read(&g_dht11_data)) {
    if (++dht11_fail_count >= 3) g_dht11_data.valid = 0;
} else {
    dht11_fail_count = 0;
}
```

---

### 5. Display_Manager 模块

**职责：** 维护当前显示页面状态，按固定时间间隔切换页面，调用对应的绘制函数更新 OLED。

**关键接口：**

```c
/* display_manager.h */

/* 初始化显示管理器，切换到初始页面 */
void Display_Init(void);

/* 在主循环中调用，检查是否需要切换页面并刷新显示 */
void Display_Update(void);

/* 直接在 OLED 上显示启动提示（初始化阶段调用）*/
void Display_ShowBoot(const char *msg);
```

**页面切换状态机：**

```c
/* PAGE_CLOCK → PAGE_WEATHER → PAGE_DHT11 → PAGE_CLOCK → ... */
#define PAGE_SWITCH_INTERVAL_MS  5000U

static DisplayPage_t s_current_page = PAGE_CLOCK;
static uint32_t      s_page_tick    = 0;

void Display_Update(void)
{
    if (HAL_GetTick() - s_page_tick >= PAGE_SWITCH_INTERVAL_MS)
    {
        s_current_page = (DisplayPage_t)((s_current_page + 1) % PAGE_COUNT);
        s_page_tick    = HAL_GetTick();
        OLED_Clear();  /* 先清屏 */
        switch (s_current_page)
        {
            case PAGE_CLOCK:   display_page_clock();   break;
            case PAGE_WEATHER: display_page_weather(); break;
            case PAGE_DHT11:   display_page_dht11();   break;
            default: break;
        }
    }
}
```

**Page_Clock 绘制（对应需求 6.2）：**

```c
/* 从 HAL_RTC_GetTime / HAL_RTC_GetDate 读取，若失败显示占位符 */
static void display_page_clock(void)
{
    RTC_TimeTypeDef t; RTC_DateTypeDef d;
    char buf[17];
    if (HAL_RTC_GetTime(&hrtc, &t, RTC_FORMAT_BIN) == HAL_OK &&
        HAL_RTC_GetDate(&hrtc, &d, RTC_FORMAT_BIN) == HAL_OK)
    {
        /* 第1行：HH:MM:SS */
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
                 t.Hours, t.Minutes, t.Seconds);
        OLED_ShowString(1, 1, buf);
        /* 第3行：YYYY-MM-DD */
        snprintf(buf, sizeof(buf), "20%02d-%02d-%02d",
                 d.Year, d.Month, d.Date);
        OLED_ShowString(3, 1, buf);
    }
    else
    {
        OLED_ShowString(1, 1, "--:--:--");
        OLED_ShowString(3, 1, "----/--/--");
    }
}
```

**Page_Weather 绘制（对应需求 6.3）：**

```c
static void display_page_weather(void)
{
    char buf[17];
    /* 第1行：天气状况，截断至 16 字符 */
    snprintf(buf, sizeof(buf), "%-16s", g_weather_data.text);
    OLED_ShowString(1, 1, buf);
    /* 第3行：Hi/Lo 温度 */
    snprintf(buf, sizeof(buf), "Hi:%2dC Lo:%2dC",
             g_weather_data.tempMax, g_weather_data.tempMin);
    OLED_ShowString(3, 1, buf);
}
```

**Page_DHT11 绘制（对应需求 6.4）：**

```c
static void display_page_dht11(void)
{
    char buf[17];
    if (g_dht11_data.valid)
    {
        snprintf(buf, sizeof(buf), "Temp: %2d C", g_dht11_data.temperature);
        OLED_ShowString(1, 1, buf);
        snprintf(buf, sizeof(buf), "Humi: %2d %%", g_dht11_data.humidity);
        OLED_ShowString(3, 1, buf);
    }
    else
    {
        OLED_ShowString(1, 1, "Temp:  -- C");
        OLED_ShowString(3, 1, "Humi:  -- %");
    }
}
```

---

### 6. main.c 主循环设计

**初始化序列（对应需求 1）：**

```c
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_RTC_Init();        /* 新增：初始化 RTC */
    MX_USART1_UART_Init();/* 新增：初始化 USART1 */

    OLED_Init();
    OLED_ShowString(1, 1, "Booting...");   /* 需求 1.2 */

    AT_Init();
    DHT11_Init();

    /* WiFi 连接 */
    if (!AT_SendWait("AT+CWJAP=\"SSID\",\"PWD\"", "WIFI CONNECTED", 10000))
    {
        OLED_Clear();
        OLED_ShowString(1, 1, "WiFi Fail");
        /* g_wifi_status 保持 WIFI_DISCONNECTED */
    }
    else
    {
        g_wifi_status = WIFI_CONNECTED;
        NTP_Sync();         /* NTP 同步（含重试） */
        Weather_Fetch();    /* 首次天气获取 */
    }

    Display_Init();         /* 进入分页显示 */

    /* 主循环 */
    uint32_t dht11_tick   = 0;
    uint32_t weather_tick = 0;

    while (1)
    {
        /* DHT11 每 2 秒采集一次 */
        if (HAL_GetTick() - dht11_tick >= 2000)
        {
            dht11_tick = HAL_GetTick();
            dht11_poll();   /* 内含失败计数逻辑 */
        }

        /* 天气每 30 分钟刷新一次 */
        if (g_wifi_status == WIFI_CONNECTED &&
            HAL_GetTick() - weather_tick >= 1800000UL)
        {
            weather_tick = HAL_GetTick();
            Weather_Fetch();
        }

        Display_Update();   /* 检查并刷新分页显示 */
    }
}
```

---

## RTC 初始化设计

STM32F103C8T6 内置 RTC 使用 HAL 驱动：

```c
/* 在 main.c 中添加 */
RTC_HandleTypeDef hrtc;

static void MX_RTC_Init(void)
{
    hrtc.Instance            = RTC;
    hrtc.Init.AsynchPrediv  = RTC_AUTO_1_SECOND; /* 使用自动预分频 */
    hrtc.Init.OutPut         = RTC_OUTPUTSOURCE_NONE;
    HAL_RTC_Init(&hrtc);
    /* 首次上电时设置默认时间 2000-01-01 00:00:00 */
}
```

> 备用电源（VBAT 引脚接 3V 纽扣电池）可在断电后维持 RTC 计时。

---

## USART1 初始化设计

```c
UART_HandleTypeDef huart1;

static void MX_USART1_UART_Init(void)
{
    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = 115200;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE); /* 开启接收中断 */
}
```

---

## 错误处理策略

| 故障场景               | 处理方式                                    |
| --------------------- | ------------------------------------------ |
| WiFi 连接超时（10s）   | 标记 DISCONNECTED，显示 "WiFi Fail"         |
| NTP 3 次均超时         | 保持 RTC 原有值（可能为默认值 00:00:00）     |
| 天气 HTTP 非 200        | 保留缓存，首次失败时显示 "No Data"           |
| DHT11 连续 3 次失败    | 缓存标记无效，显示 "--"                     |
| RTC 读取失败           | 时钟页显示 "--:--:--" 和 "----/--/--"       |
| 任意外设 Init 失败     | OLED 显示模块名+错误信息，停止后续初始化    |

**无 `Error_Handler()` 死循环原则：** 除初始化阶段致命错误外，运行时错误均以降级显示处理，保持系统继续运行。

---

## Flash 体积估算

| 模块                 | 估算 Flash（KB） |
| -------------------- | --------------- |
| HAL 库（已有）       | ~18             |
| OLED 驱动 + 字库     | ~3              |
| delay.c              | <1              |
| AT_Controller        | ~2              |
| NTP_Client           | ~2              |
| Weather_Client       | ~3              |
| DHT11_Driver         | ~1              |
| Display_Manager      | ~2              |
| main.c + 初始化      | ~2              |
| **合计（估算）**     | **~34**         |

目标上限 56KB，估算总占用约 34KB，余量充足。

---

## 正确性属性

*属性是在系统所有合法执行中应当恒成立的行为特征，是连接人类可读规范与机器可验证正确性保证的桥梁。*

### 属性 1：AT 指令末尾格式

*对于任意* AT 指令字符串，经 `AT_Send()` 发送时，实际写入 USART1 的字节序列必须以 `\r\n`（0x0D 0x0A）结尾。

**验证：需求 7.2**

---

### 属性 2：WiFi 连接状态响应解析

*对于任意* 包含子字符串 "WIFI CONNECTED" 的 AT 响应缓冲区内容，`AT_Contains("WIFI CONNECTED")` 必须返回非零值；*对于任意* 不包含该子字符串的缓冲区内容，必须返回零值。

**验证：需求 2.2**

---

### 属性 3：NTP 时间戳 UTC→UTC+8 换算

*对于任意* 合法的 NTP 时间戳值（32 位无符号整数，范围 `[NTP_EPOCH_OFFSET, 0xFFFFFFFF]`），换算后的北京时间（小时、分钟、秒）必须满足：

`beijing_hour = ((ntp_sec - NTP_EPOCH_OFFSET) / 3600 + 8) % 24`

且输出的小时在 0~23 范围内，分钟和秒均在 0~59 范围内。

**验证：需求 3.2**

---

### 属性 4：天气 JSON 字段提取

*对于任意* 包含格式合法的 `"text":"..."` 、`"tempMax":"..."` 和 `"tempMin":"..."` 字段的 JSON 字符串，`json_find_value()` 提取的 text 字段长度不超过 16 个字符，tempMax 和 tempMin 可被正确解析为整数值，且提取结果与原始 JSON 中的值一致。

**验证：需求 4.2**

---

### 属性 5：DHT11 数值范围约束

*对于任意* DHT11 原始采集值（温度和湿度整数），若温度不在 −20°C 至 60°C 范围内，或湿度不在 0% 至 100% 范围内，则 `DHT11_Read()` 返回的 `valid` 字段必须为 0；若均在合法范围内，则 `valid` 字段必须为 1，且输出值与输入值相等。

**验证：需求 5.2**

---

### 属性 6：页面轮换顺序不变量

*对于任意* 当前显示页面 P（`PAGE_CLOCK`、`PAGE_WEATHER`、`PAGE_DHT11` 之一），在经过 5 秒时间片后 `Display_Update()` 触发的下一页面必须等于 `(P + 1) % PAGE_COUNT`，即切换顺序严格为 Clock → Weather → DHT11 → Clock 循环。

**验证：需求 6.1**

---

### 属性 7：时钟页面时间格式化

*对于任意* 合法的 RTC 时间值（小时 0~23，分钟 0~59，秒 0~59）和日期值（年 0~99，月 1~12，日 1~31），`display_page_clock()` 写入 OLED 第 1 行的字符串必须符合 `HH:MM:SS` 格式（8 字符，冒号位于固定位置），写入第 3 行的字符串必须符合 `20YY-MM-DD` 格式（10 字符）。

**验证：需求 6.2**

---

### 属性 8：天气与温湿度页面格式化

*对于任意* 合法的天气数据（text 长度 0~16，tempMax/tempMin 为整数）和本地温湿度数据（temperature 为 −20~60 的整数，humidity 为 0~100 的整数），`display_page_weather()` 输出的第 3 行字符串必须匹配 `Hi:%2dC Lo:%2dC` 模式；`display_page_dht11()` 输出的第 1 行必须匹配 `Temp: XX C` 模式，第 3 行必须匹配 `Humi: XX %` 模式。

**验证：需求 6.3、6.4**
