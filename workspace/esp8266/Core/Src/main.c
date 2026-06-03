/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : ESP8266 WiFi 接收数据并显示到 OLED
  *                   ESP8266 工作模式：AP + TCP Server（端口 8080）
  *                   连接方式：手机/电脑连热点 ESP8266，用网络调试工具发送数据
  ******************************************************************************
  *
  * ============================================================
  *  接线说明（STM32F103C8T6 Blue Pill 板 + ESP-01S 模块）
  * ============================================================
  *
  *  【STM32F103C8T6 引脚位置示意图】
  *  （USB口朝上，从上往下看板子）
  *
  *           USB
  *        ┌───────┐
  *   GND  ┤1    1 ├  VIN(5V)
  *   GND  ┤2    2 ├  GND
  *   3.3V ┤3    3 ├  3.3V
  *   NRST ┤4    4 ├  PC13(LED)
  *   PB11 ┤5    5 ├  PC14
  *   PB10 ┤6    6 ├  PC15
  *   PB1  ┤7    7 ├  PA8
  *   PB0  ┤8    8 ├  PA9  ←←← UART1_TX（接ESP-01S的RXD）
  *   PA7  ┤9    9 ├  PA10 ←←← UART1_RX（接ESP-01S的TXD）
  *   PA6  ┤10  10 ├  PA11
  *   PA5  ┤11  11 ├  PA12
  *   PA4  ┤12  12 ├  PA15
  *   PA3  ┤13  13 ├  PB3
  *   PA2  ┤14  14 ├  PB4
  *   PA1  ┤15  15 ├  PB5
  *   PA0  ┤16  16 ├  PB6
  *  3.3V  ┤17  17 ├  PB7
  *        │  BOOT │  PB8  ←←← OLED_SCL
  *        │  0  1 │  PB9  ←←← OLED_SDA
  *        └───────┘
  *
  * ============================================================
  *  ESP-01S 模块引脚示意图
  * ============================================================
  *
  *          天线
  *     ┌──────────┐
  *     │          │
  *  VCC┤1        5├CH_PD/EN
  *  GND┤2        6├GPIO2
  *  RXD┤3        7├GPIO0
  *  TXD┤4        8├RST
  *     └──────────┘
  *
  * ============================================================
  *  ESP-01S 接线（直接使用 STM32 板载 3.3V 供电）
  * ============================================================
  *
  *  ┌──────────────┬──────────────────────────┬───────────────────────────────┐
  *  │ ESP-01S 引脚 │ STM32 引脚               │ 说明                          │
  *  ├──────────────┼──────────────────────────┼───────────────────────────────┤
  *  │ VCC  (1脚)   │ 板子左排第3脚 3.3V       │ 直接接 STM32 板载 3.3V        │
  *  │ GND  (2脚)   │ 板子左排第2脚 GND        │ 共地                          │
  *  │ RXD  (3脚)   │ 板子右排第8脚  PA9(TX)   │ STM32 发送 → ESP-01S 接收     │
  *  │ TXD  (4脚)   │ 板子右排第9脚  PA10(RX)  │ ESP-01S 发送 → STM32 接收     │
  *  │ CH_PD/EN(5脚)│ 板子左排第3脚 3.3V       │ 使能引脚（也叫EN），必须拉高  │
  *  │ GPIO2(6脚)   │ 悬空                     │ 不用接                        │
  *  │ GPIO0(7脚)   │ 悬空                     │ 正常运行悬空，刷固件才接GND   │
  *  │ RST  (8脚)   │ 悬空                     │ 不用接                        │
  *  └──────────────┴──────────────────────────┴───────────────────────────────┘
  *
  * ============================================================
  *  OLED 接线（软件 I2C）
  * ============================================================
  *
  *  ┌─────────────┬──────────────────────────┬───────────────────────────────┐
  *  │ OLED 引脚   │ STM32 引脚               │ 说明                          │
  *  ├─────────────┼──────────────────────────┼───────────────────────────────┤
  *  │ VCC         │ 板子左排第3脚 3.3V       │ 接板载 3.3V                   │
  *  │ GND         │ 板子左排第2脚 GND        │ 共地                          │
  *  │ SCL         │ 板子右排第18脚 PB8       │ 软件 I2C 时钟线               │
  *  │ SDA         │ 板子右排第19脚 PB9       │ 软件 I2C 数据线               │
  *  └─────────────┴──────────────────────────┴───────────────────────────────┘
  *
  * ============================================================
  *  使用方法
  * ============================================================
  *
  *  1. 把代码里的 WIFI_SSID 和 WIFI_PASSWORD 改成你自己的路由器信息
  *  2. 烧录后 OLED 显示 "Connecting..." 等待连接路由器
  *  3. 连接成功后 OLED 显示路由器分配的 IP 地址和端口 8080
  *  4. 手机连接同一个路由器的 WiFi
  *  5. 打开"网络调试助手"App，选择 TCP Client
  *       服务器 IP：OLED 上显示的 IP    端口：8080
  *  6. 连接后发送任意文字，OLED 立即刷新显示（最多4行×16字符）
  *
  *  Ping 测试：手机终端或电脑命令行执行 ping <OLED显示的IP>
  *
  * ============================================================
  */
#include "main.h"
#include "OLED.h"
#include <string.h>
#include <stdio.h>

/* ======================== UART 句柄 ======================== */
UART_HandleTypeDef huart1;

/* ======================== 函数声明 ======================== */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void ESP8266_SendCmd(const char *cmd);
static uint16_t ESP8266_ReadLine(uint8_t *buf, uint16_t max_len, uint32_t timeout_ms);
static uint8_t ESP8266_WaitResp(const char *keyword, uint32_t timeout_ms);
static void ESP8266_Init(void);
static void ESP8266_SendData(uint8_t link_id, const char *data);
static uint8_t ParseAndDisplay(uint8_t *line);

/* ======================== ESP8266 通信函数 ======================== */

/**
  * @brief  向 ESP8266 发送 AT 命令（字符串以 \r\n 结尾）
  */
static void ESP8266_SendCmd(const char *cmd)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)cmd, strlen(cmd), 1000);
}

/**
  * @brief  从 UART 读取一行（遇到 \n 停止），超时后返回
  * @param  buf        接收缓冲区
  * @param  max_len    缓冲区最大长度
  * @param  timeout_ms 超时时间（毫秒）
  * @retval 实际读取到的字符数
  */
static uint16_t ESP8266_ReadLine(uint8_t *buf, uint16_t max_len, uint32_t timeout_ms)
{
    uint16_t idx = 0;
    uint8_t ch;
    uint32_t start = HAL_GetTick();

    while ((HAL_GetTick() - start) < timeout_ms)
    {
        if (HAL_UART_Receive(&huart1, &ch, 1, 10) == HAL_OK)
        {
            if (ch == '\n')
            {
                break;
            }
            if (ch != '\r' && idx < max_len - 1)
            {
                buf[idx++] = ch;
            }
        }
    }
    buf[idx] = '\0';
    return idx;
}

/**
  * @brief  等待 ESP8266 响应中出现指定关键字
  * @param  keyword     期望的响应关键字，如 "OK"
  * @param  timeout_ms  总等待时间（毫秒）
  * @retval 1=找到关键字，0=超时
  */
static uint8_t ESP8266_WaitResp(const char *keyword, uint32_t timeout_ms)
{
    uint8_t line[128];
    uint32_t start = HAL_GetTick();

    while ((HAL_GetTick() - start) < timeout_ms)
    {
        uint16_t len = ESP8266_ReadLine(line, sizeof(line), 200);
        if (len > 0 && strstr((char *)line, keyword) != NULL)
        {
            return 1;
        }
    }
    return 0;
}

/**
  * @brief  初始化 ESP8266：连接路由器（Station模式），获取IP后开启 TCP Server
  *
  *  ⚠️ 修改下面两个宏为你自己的路由器 WiFi 名称和密码
  */

/* ===== 在这里填写你的路由器 WiFi 信息 ===== */
#define WIFI_SSID     "TP-LINK_1202"
#define WIFI_PASSWORD "liu88567878"
/* ========================================= */

static void ESP8266_Init(void)
{
    OLED_Clear();
    OLED_ShowString(1, 1, "ESP8266 Init...");

    /* 等待 ESP8266 上电稳定 */
    HAL_Delay(3000);

    /* 1. 测试通信 */
    ESP8266_SendCmd("AT\r\n");
    ESP8266_WaitResp("OK", 3000);

    /* 2. 关闭回显，减少干扰 */
    ESP8266_SendCmd("ATE0\r\n");
    ESP8266_WaitResp("OK", 2000);

    /* 3. 设置为 Station 模式（连接路由器） */
    ESP8266_SendCmd("AT+CWMODE=1\r\n");
    ESP8266_WaitResp("OK", 3000);

    /* 4. 连接路由器，等待最多 15 秒 */
    OLED_Clear();
    OLED_ShowString(1, 1, "Connecting...");
    {
        char cmd[128];
        /* 拼接 AT+CWJAP="SSID","PASSWORD"\r\n */
        uint16_t len = 0;
        const char *prefix = "AT+CWJAP=\"";
        const char *mid    = "\",\"";
        const char *suffix = "\"\r\n";
        memcpy(cmd + len, prefix, strlen(prefix)); len += strlen(prefix);
        memcpy(cmd + len, WIFI_SSID, strlen(WIFI_SSID)); len += strlen(WIFI_SSID);
        memcpy(cmd + len, mid, strlen(mid)); len += strlen(mid);
        memcpy(cmd + len, WIFI_PASSWORD, strlen(WIFI_PASSWORD)); len += strlen(WIFI_PASSWORD);
        memcpy(cmd + len, suffix, strlen(suffix)); len += strlen(suffix);
        cmd[len] = '\0';
        ESP8266_SendCmd(cmd);
    }

    /* 等待连接成功（WIFI GOT IP），最多30秒 */
    if (!ESP8266_WaitResp("GOT IP", 30000))
    {
        /* 连接失败，显示错误 */
        OLED_Clear();
        OLED_ShowString(1, 1, "WiFi Failed!");
        OLED_ShowString(2, 1, "Check SSID/PWD");
        while (1); /* 停在这里，检查WiFi名称和密码是否正确 */
    }

    /* 5. 查询获取到的 IP 地址（循环重试，直到拿到有效 IP） */
    OLED_Clear();
    OLED_ShowString(1, 1, "Connected!");

    {
        uint8_t line[128];
        char ip_str[16] = "Unknown";
        uint8_t retry = 0;

        /* 最多重试 10 次，每次间隔 1 秒，共等待最多 10 秒 */
        while (retry < 10)
        {
            /* 每次查询前先清空串口缓冲区残留数据 */
            HAL_Delay(1000);

            ESP8266_SendCmd("AT+CIFSR\r\n");

            uint32_t start = HAL_GetTick();
            uint8_t got_ip = 0;

            /* 读取本次 AT+CIFSR 的响应，最多等 2 秒 */
            while ((HAL_GetTick() - start) < 2000)
            {
                uint16_t len = ESP8266_ReadLine(line, sizeof(line), 300);
                if (len == 0) continue;

                /* 匹配 +CIFSR:STAIP,"x.x.x.x" 或 +CIPSTA:ip:"x.x.x.x" */
                if (strstr((char *)line, "STAIP") != NULL ||
                    strstr((char *)line, "ip:\"") != NULL)
                {
                    char *p1 = strchr((char *)line, '"');
                    char *p2 = p1 ? strchr(p1 + 1, '"') : NULL;
                    if (p1 && p2 && (p2 - p1 - 1) > 0)
                    {
                        uint8_t ip_len = (uint8_t)(p2 - p1 - 1);
                        if (ip_len > 15) ip_len = 15;
                        memcpy(ip_str, p1 + 1, ip_len);
                        ip_str[ip_len] = '\0';
                        got_ip = 1;
                        break;
                    }
                }
            }

            /* 拿到 IP 且不是 0.0.0.0，则成功退出 */
            if (got_ip && strcmp(ip_str, "0.0.0.0") != 0)
            {
                break;
            }

            /* 还没拿到，OLED 显示正在等待，继续重试 */
            retry++;
            OLED_ShowString(2, 1, "Wait IP...");
            /* 显示重试次数，方便调试 */
            {
                char retry_buf[8];
                retry_buf[0] = retry + '0';
                retry_buf[1] = '\0';
                OLED_ShowString(2, 10, retry_buf);
            }
        }

        /* 第2行显示 IP: 前缀 */
        OLED_ShowString(2, 1, "IP:             ");
        /* 第3行显示具体 IP 地址 */
        OLED_ShowString(3, 1, ip_str);
        /* 第4行显示端口 */
        OLED_ShowString(4, 1, "Port:8080");
    }

    /* 6. 开启多连接 */
    ESP8266_SendCmd("AT+CIPMUX=1\r\n");
    ESP8266_WaitResp("OK", 2000);

    /* 7. 开启 TCP Server，监听端口 8080 */
    ESP8266_SendCmd("AT+CIPSERVER=1,8080\r\n");
    ESP8266_WaitResp("OK", 3000);
}

/**
  * @brief  通过 ESP8266 TCP Server 向指定连接发送数据
  * @param  link_id  连接编号（AT+CIPMUX=1 时为 0~4）
  * @param  data     要发送的字符串
  */
static void ESP8266_SendData(uint8_t link_id, const char *data)
{
    char cmd[64];
    uint16_t data_len = strlen(data);

    /* 发送 AT+CIPSEND=<linkID>,<len> */
    snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%d,%d\r\n", link_id, data_len);
    ESP8266_SendCmd(cmd);

    /* 等待 ESP8266 返回 '>' 提示符（最多 2 秒） */
    if (ESP8266_WaitResp(">", 2000))
    {
        /* 发送实际数据 */
        HAL_UART_Transmit(&huart1, (uint8_t *)data, data_len, 1000);
        /* 等待 SEND OK */
        ESP8266_WaitResp("SEND OK", 3000);
    }
}

/**
  * @brief  解析 +IPD 数据：显示到 OLED，并返回连接编号
  *         ESP8266 透传格式：+IPD,<linkID>,<len>:<data>
  * @param  line  一行接收到的字符串
  * @retval 连接编号 link_id（0~4），解析失败返回 0xFF
  */
static uint8_t ParseAndDisplay(uint8_t *line)
{
    /* 解析 link_id：格式 +IPD,<id>,<len>:<data> */
    uint8_t link_id = 0xFF;
    char *p = strstr((char *)line, "+IPD,");
    if (p != NULL)
    {
        /* 跳过 "+IPD," 取 link_id */
        link_id = (uint8_t)(p[5] - '0');
    }
    /* 找到冒号，冒号后面才是实际数据 */
    char *colon = strchr((char *)line, ':');
    if (colon == NULL) return link_id;

    char *data = colon + 1;
    uint16_t data_len = strlen(data);
    if (data_len == 0) return link_id;

    /* 去掉末尾的换行符和回车符 */
    while (data_len > 0 &&
           (data[data_len - 1] == '\n' || data[data_len - 1] == '\r'))
    {
        data[--data_len] = '\0';
    }
    if (data_len == 0) return link_id;

    /* 清屏，将数据按行显示（每行最多16字符，共4行） */
    OLED_Clear();

    char display_buf[17];   /* 16字符 + '\0' */
    uint8_t oled_line = 1;
    uint16_t offset = 0;

    while (offset < data_len && oled_line <= 4)
    {
        uint8_t copy_len = (data_len - offset > 16) ? 16 : (uint8_t)(data_len - offset);
        memcpy(display_buf, data + offset, copy_len);
        display_buf[copy_len] = '\0';
        OLED_ShowString(oled_line, 1, display_buf);
        offset += copy_len;
        oled_line++;
    }

    return link_id;
}

/* ======================== 主函数 ======================== */

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();

    OLED_Init();

    /* 初始化 ESP8266 */
    ESP8266_Init();

    /* 主循环：持续读取 ESP8266 透传数据，显示到 OLED 并回复 "ok" */
    uint8_t rx_line[256];
    while (1)
    {
        uint16_t len = ESP8266_ReadLine(rx_line, sizeof(rx_line), 100);
        if (len > 0 && strstr((char *)rx_line, "+IPD") != NULL)
        {
            uint8_t link_id = ParseAndDisplay(rx_line);
            if (link_id != 0xFF)
            {
                /* 收到消息后向对方回复 "ok\r\n" */
                ESP8266_SendData(link_id, "ok\r\n");
            }
        }
    }
}

/* ======================== 外设初始化 ======================== */

/**
  * @brief  USART1 初始化（PA9=TX，PA10=RX，波特率 115200）
  *         用于与 ESP8266 通信，ESP8266 默认波特率也是 115200
  */
static void MX_USART1_UART_Init(void)
{
    huart1.Instance        = USART1;
    huart1.Init.BaudRate   = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits   = UART_STOPBITS_1;
    huart1.Init.Parity     = UART_PARITY_NONE;
    huart1.Init.Mode       = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief  系统时钟配置（HSE 8MHz → PLL × 9 = 72MHz）
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType  = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState        = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue  = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState        = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState    = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource   = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL      = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief  GPIO 初始化（使能 GPIOA、GPIOB、GPIOD 时钟）
  */
static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
}

/**
  * @brief  错误处理
  */
void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
