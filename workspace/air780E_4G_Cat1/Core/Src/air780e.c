/**
 * @file    air780e.c
 * @brief   YED-C780 (Air780E) 4G 模块驱动
 *          支持 TCP / UDP / HTTP 三种模式（通过 air780e.h 中的宏切换）
 *
 * ===================== 接线说明 =====================
 *  STM32F103C8T（3.3V 系统）  ←→  YED-C780 模块
 *
 *  串口通信：
 *  STM32 PA9  (USART1_TX)  ──→  模块左侧第11脚 RX
 *  STM32 PA10 (USART1_RX)  ←──  模块左侧第10脚 TX
 *
 *  复位控制：
 *  STM32 PA1  (GPIO OUT)   ──→  模块左侧第 9脚 RST（高电平≥1s 触发复位）
 *
 *  供电（二选一）：
 *  方案A：STM32 3.3V ──→ 模块左侧第 5脚 VBAT（3.3~4.2V 直接供电）
 *         适合快速验证，若模块反复重启请换方案B
 *  方案B：外部 5V 电源 ──→ 模块左侧第 1脚 VIN（5~12V 宽压输入，更稳定）
 *
 *  共地（必须）：
 *  STM32 GND ──→ 模块左侧第 2 或 4脚 GND
 *  串口信号以 GND 为参考，不共地会导致通信异常
 *
 *  其他：
 *  模块左侧第12脚 VCCIO 悬空（3.3V 系统无需接，仅 5V 系统才接 5V）
 *  OLED 已使用 PB8(SCL) / PB9(SDA)，与上述引脚无冲突
 * =====================================================
 *
 * 协议切换：修改 air780e.h 中的宏（USE_TCP / USE_UDP / USE_HTTP 三选一）
 *
 * TCP 非透传流程：
 *  AT → ATE0 → CCID → CGATT → CIPMUX=0 → CIPQSEND=1 → CIPHEAD=1
 *  → CSTT → CIICR → CIFSR → CIPSTART="TCP" → CIPSEND / +IPD
 *
 * UDP 非透传流程（同 TCP，协议改为 UDP，建立后需主动发包 NAT穿透）：
 *  AT → ATE0 → CCID → CGATT → CIPMUX=0 → CIPQSEND=1 → CIPHEAD=1
 *  → CSTT → CIICR → CIFSR → CIPSTART="UDP" → 主动发 HELLO → CIPSEND / +IPD
 *
 * HTTP 流程（使用独立的 SAPBR 承载，与 TCP/UDP 的 CSTT/CIICR 不同）：
 *  AT → ATE0 → CCID → CGATT → SAPBR(类型/APN/激活) → SAPBR查IP
 *  → HTTPINIT → HTTPPARA(CID/URL/CONTENT) → HTTPACTION → HTTPREAD → HTTPTERM
 */

#include "air780e.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>  /* atoi */

/* ==================== 硬件资源宏 ==================== */

/* ---- MQTT 参数配置（USE_MQTT 模式下填写）----
 * 在 air780e.h 的 MQTT 专用接口区域修改 ClientID/用户名/密码/topic
 * SERVER_IP 和 SERVER_PORT 在 main.c 里填写 Broker 地址和端口（通常 1883）*/

/* USART1：PA9=TX，PA10=RX */#define AIR780E_UART_CLK_EN()   __HAL_RCC_USART1_CLK_ENABLE()
#define AIR780E_UART_GPIO_CLK() __HAL_RCC_GPIOA_CLK_ENABLE()
#define AIR780E_UART_PORT       GPIOA
#define AIR780E_UART_TX_PIN     GPIO_PIN_9
#define AIR780E_UART_RX_PIN     GPIO_PIN_10
#define AIR780E_UART_IRQ        USART1_IRQn

/* RST 控制引脚：PA1，高电平≥1s 触发模块复位 */
#define AIR780E_RST_CLK()       __HAL_RCC_GPIOA_CLK_ENABLE()
#define AIR780E_RST_PORT        GPIOA
#define AIR780E_RST_PIN         GPIO_PIN_1

/* ==================== 接收缓冲区 ==================== */

#define RX_BUF_SIZE  512

static uint8_t  rx_buf[RX_BUF_SIZE];
static uint16_t rx_head = 0;   /* 读指针 */
static uint16_t rx_tail = 0;   /* 写指针（中断内操作） */

/* ==================== 模块状态 ==================== */

static Air780E_State g_state     = AIR780E_STATE_IDLE;
static uint8_t       g_fail_step = 0;  /* 失败步骤，0=未失败 */

/* ==================== 内部函数声明 ==================== */

static void     uart_init(void);
static void     rst_pin_init(void);
static int      send_at(const char *cmd, const char *expect, uint32_t timeout_ms);
static uint16_t rx_available(void);
static void     rx_flush(void);
static int      rx_wait_string(const char *target, uint32_t timeout_ms);

/* ==================== 通用对外接口 ==================== */

Air780E_State Air780E_GetState(void)   { return g_state; }
uint8_t       Air780E_GetFailStep(void){ return g_fail_step; }

/**
 * @brief  硬件复位模块：RST 拉高 1.2s，释放后等待 3s 重启完成
 */
void Air780E_HardReset(void)
{
    HAL_GPIO_WritePin(AIR780E_RST_PORT, AIR780E_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(1200);
    HAL_GPIO_WritePin(AIR780E_RST_PORT, AIR780E_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(3000);
    rx_flush();
}

/**
 * @brief  USART1 接收中断处理，每字节写入环形缓冲区
 *         在 stm32f1xx_it.c 的 USART1_IRQHandler 中调用
 */
void Air780E_UART_IRQHandler(void)
{
    if (USART1->SR & USART_SR_RXNE)
    {
        uint8_t byte = (uint8_t)(USART1->DR & 0xFF);
        uint16_t next = (rx_tail + 1) % RX_BUF_SIZE;
        if (next != rx_head)
        {
            rx_buf[rx_tail] = byte;
            rx_tail = next;
        }
    }
    /* 清除帧错误 / 溢出错误（读 SR 再读 DR 即可清除） */
    if (USART1->SR & (USART_SR_FE | USART_SR_ORE))
    {
        (void)USART1->SR;
        (void)USART1->DR;
    }
}

/* ================================================================
 *  公共初始化部分（三种模式共用：上电 → 确认通信 → 确认 SIM → 附着网络）
 * ================================================================ */
static int common_init(void)
{
    g_state = AIR780E_STATE_INIT;
    rst_pin_init();
    uart_init();
    Air780E_HardReset();

    /* 步骤1：测试串口通信，最多重试 5 次（自适应波特率） */
    for (int i = 0; i < 5; i++)
    {
        if (send_at("AT\r", "OK", 2000) == 0) break;
        if (i == 4) { g_fail_step = 1; g_state = AIR780E_STATE_ERROR; return -1; }
        HAL_Delay(500);
    }

    /* 步骤2：关闭回显，避免回显干扰应答解析 */
    if (send_at("ATE0\r", "OK", 2000) != 0)
    { g_fail_step = 2; g_state = AIR780E_STATE_ERROR; return -1; }

    /* 步骤3：查询 SIM 卡 ICCID，确认卡已插好 */
    if (send_at("AT+CCID\r", "OK", 5000) != 0)
    { g_fail_step = 3; g_state = AIR780E_STATE_ERROR; return -1; }

    /* 步骤4：等待 GPRS 附着网络（最多 30 秒） */
    for (int retry = 0; retry < 30; retry++)
    {
        if (send_at("AT+CGATT?\r", "+CGATT: 1", 2000) == 0) break;
        HAL_Delay(1000);
        if (retry == 29) { g_fail_step = 4; g_state = AIR780E_STATE_ERROR; return -1; }
    }
    return 0;
}

/* ================================================================
 *  Air780E_Init 入口：根据宏选择初始化路径
 * ================================================================ */
int Air780E_Init(const char *server_ip, uint16_t server_port)
{
    if (common_init() != 0) return -1;

/* ---------- TCP / UDP 初始化路径 ---------- */
#if defined(USE_TCP) || defined(USE_UDP)
    {
        char cmd_buf[80];

        /* 步骤5：单链接模式 */
        if (send_at("AT+CIPMUX=0\r", "OK", 3000) != 0)
        { g_fail_step = 5; g_state = AIR780E_STATE_ERROR; return -1; }

        /* 步骤6：快发模式（减少等待时间） */
        if (send_at("AT+CIPQSEND=1\r", "OK", 3000) != 0)
        { g_fail_step = 6; g_state = AIR780E_STATE_ERROR; return -1; }

        /* 步骤7：开启接收数据头 +IPD,N:<data>，不开启则裸数据无法判断边界 */
        send_at("AT+CIPHEAD=1\r", "OK", 3000); /* 失败不致命，继续执行 */

        /* 步骤7：设置 APN（大部分国内卡用空 APN 即可） */
        if (send_at("AT+CSTT=\"\",\"\",\"\"\r", "OK", 5000) != 0)
        { g_fail_step = 7; g_state = AIR780E_STATE_ERROR; return -1; }

        /* 步骤8：激活 PDP，等待分配 IP */
        if (send_at("AT+CIICR\r", "OK", 15000) != 0)
        { g_fail_step = 8; g_state = AIR780E_STATE_ERROR; return -1; }

        /* 步骤9：查询本机 IP，确认网络已激活（IP 中必含"."） */
        if (send_at("AT+CIFSR\r", ".", 5000) != 0)
        { g_fail_step = 9; g_state = AIR780E_STATE_ERROR; return -1; }

        g_state = AIR780E_STATE_READY;

        /* 步骤10：建立连接 */
#ifdef USE_UDP
        snprintf(cmd_buf, sizeof(cmd_buf),
                 "AT+CIPSTART=\"UDP\",\"%s\",%u\r", server_ip, server_port);
#else
        snprintf(cmd_buf, sizeof(cmd_buf),
                 "AT+CIPSTART=\"TCP\",\"%s\",%u\r", server_ip, server_port);
#endif
        if (send_at(cmd_buf, "CONNECT OK", 15000) != 0)
        { g_fail_step = 10; g_state = AIR780E_STATE_ERROR; return -1; }

        g_state = AIR780E_STATE_CONNECTED;

#ifdef USE_UDP
        /* UDP 建立后必须主动发一包，服务器才能获知设备地址（NAT穿透） */
        HAL_Delay(200);
        Air780E_Send("HELLO", 5);
#endif
    }

/* ---------- HTTP 初始化路径 ---------- */
#elif defined(USE_HTTP)
    {
        /* HTTP 使用独立的 SAPBR 承载，不使用 CSTT/CIICR
         *
         * 步骤5：设置 PDP 承载类型为 GPRS */
        if (send_at("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"\r", "OK", 5000) != 0)
        { g_fail_step = 5; g_state = AIR780E_STATE_ERROR; return -1; }

        /* 步骤6：设置 APN（大部分国内卡用空 APN） */
        if (send_at("AT+SAPBR=3,1,\"APN\",\"\"\r", "OK", 5000) != 0)
        { g_fail_step = 6; g_state = AIR780E_STATE_ERROR; return -1; }

        /* 步骤7：激活 PDP 承载（cid=1） */
        if (send_at("AT+SAPBR=1,1\r", "OK", 15000) != 0)
        { g_fail_step = 7; g_state = AIR780E_STATE_ERROR; return -1; }

        /* 步骤8：查询 PDP 承载 IP，确认激活成功 */
        if (send_at("AT+SAPBR=2,1\r", ".", 5000) != 0)
        { g_fail_step = 8; g_state = AIR780E_STATE_ERROR; return -1; }

        g_state = AIR780E_STATE_READY;
        /* HTTP 模式不建立持久连接，每次请求时临时建立，Init 到此结束 */
        (void)server_ip;
        (void)server_port;
    }

/* ---------- MQTT 初始化路径 ---------- */
#elif defined(USE_MQTT)
    {
        char cmd_buf[128];

        /* 步骤5：设置 APN（MQTT 使用 CSTT/CIICR，与 TCP/UDP 相同） */
        if (send_at("AT+CSTT=\"\",\"\",\"\"\r", "OK", 5000) != 0)
        { g_fail_step = 5; g_state = AIR780E_STATE_ERROR; return -1; }

        /* 步骤6：激活 PDP，等待分配 IP */
        if (send_at("AT+CIICR\r", "OK", 15000) != 0)
        { g_fail_step = 6; g_state = AIR780E_STATE_ERROR; return -1; }

        /* 步骤7：查询本机 IP（IP 中必含"."） */
        if (send_at("AT+CIFSR\r", ".", 5000) != 0)
        { g_fail_step = 7; g_state = AIR780E_STATE_ERROR; return -1; }

        /* 步骤8：配置 MQTT 参数（客户端ID、用户名、密码）
         * 银尔达测试服务器：ClientID/用户名/密码 均从测试工具页面复制 */
        snprintf(cmd_buf, sizeof(cmd_buf),
                 "AT+MCONFIG=\"%s\",\"%s\",\"%s\"\r",
                 MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD);
        if (send_at(cmd_buf, "OK", 5000) != 0)
        { g_fail_step = 8; g_state = AIR780E_STATE_ERROR; return -1; }

        /* 步骤9：连接 MQTT Broker（IP/域名 + 端口） */
        snprintf(cmd_buf, sizeof(cmd_buf),
                 "AT+MIPSTART=\"%s\",%u\r", server_ip, server_port);
        if (send_at(cmd_buf, "CONNECT OK", 15000) != 0)
        { g_fail_step = 9; g_state = AIR780E_STATE_ERROR; return -1; }

        /* 步骤10：发起 MQTT 会话（clean_session=1，心跳 120 秒） */
        if (send_at("AT+MCONNECT=1,120\r", "CONNACK OK", 10000) != 0)
        { g_fail_step = 10; g_state = AIR780E_STATE_ERROR; return -1; }

        /* 步骤11：订阅接收 topic，QoS=0 */
        snprintf(cmd_buf, sizeof(cmd_buf),
                 "AT+MSUB=\"%s\",0\r", MQTT_SUB_TOPIC);
        if (send_at(cmd_buf, "SUBACK", 10000) != 0)
        { g_fail_step = 11; g_state = AIR780E_STATE_ERROR; return -1; }

        g_state = AIR780E_STATE_CONNECTED;
    }
#endif /* USE_TCP/USE_UDP/USE_HTTP */

    return 0;
}

/* ================================================================
 *  TCP / UDP 专用：发送 / 接收
 * ================================================================ */
#if defined(USE_TCP) || defined(USE_UDP)

/**
 * @brief  发送数据（AT+CIPSEND 非透传方式）
 *  → AT+CIPSEND=N  ← >  → 实际数据  ← DATAACCEPT:N
 */
int Air780E_Send(const char *data, uint16_t len)
{
    char cmd_buf[32];
    if (g_state != AIR780E_STATE_CONNECTED || len == 0) return -1;

    snprintf(cmd_buf, sizeof(cmd_buf), "AT+CIPSEND=%u\r", len);
    rx_flush();
    for (uint16_t i = 0; i < (uint16_t)strlen(cmd_buf); i++)
    {
        while (!(USART1->SR & USART_SR_TXE));
        USART1->DR = cmd_buf[i];
    }
    if (rx_wait_string(">", 3000) != 0) return -1;

    for (uint16_t i = 0; i < len; i++)
    {
        while (!(USART1->SR & USART_SR_TXE));
        USART1->DR = (uint8_t)data[i];
    }
    if (rx_wait_string("DATAACCEPT", 5000) != 0) return -1;
    return 0;
}

/**
 * @brief  解析接收缓冲区中的 +IPD,N:<data> 数据
 *         同时检测 TCP 断连（CLOSED）
 */
uint16_t Air780E_Recv(char *buf, uint16_t max_len)
{
    static char parse_buf[RX_BUF_SIZE];
    uint16_t avail = rx_available();
    if (avail == 0) return 0;

    uint16_t copy_len = (avail < sizeof(parse_buf) - 1) ? avail : sizeof(parse_buf) - 1;
    for (uint16_t i = 0; i < copy_len; i++)
        parse_buf[i] = rx_buf[(rx_head + i) % RX_BUF_SIZE];
    parse_buf[copy_len] = '\0';

    char *ipd = strstr(parse_buf, "+IPD,");
    if (ipd == NULL)
    {
        /* TCP 断连检测：UDP 无连接不会收到 CLOSED */
#ifdef USE_TCP
        if (strstr(parse_buf, "CLOSED") != NULL)
            g_state = AIR780E_STATE_READY;
#endif
        rx_head = rx_tail;  /* 丢弃 AT 残留数据 */
        return 0;
    }

    char *colon = strchr(ipd, ':');
    if (colon == NULL) return 0;

    uint16_t data_len = (uint16_t)atoi(ipd + 5);
    if (data_len == 0) return 0;

    char    *data_start    = colon + 1;
    uint16_t header_offset = (uint16_t)(data_start - parse_buf);
    if (header_offset + data_len > copy_len) return 0;

    uint16_t read_len = (data_len < max_len) ? data_len : max_len;
    memcpy(buf, data_start, read_len);
    buf[read_len] = '\0';

    uint16_t garbage = (uint16_t)(ipd - parse_buf);
    rx_head = (rx_head + garbage + header_offset + data_len) % RX_BUF_SIZE;
    return read_len;
}

#endif /* USE_TCP || USE_UDP */

/* ================================================================
 *  HTTP 专用：GET / POST
 * ================================================================ */
#ifdef USE_HTTP

/**
 * @brief  发起 HTTP GET 请求并读取响应体
 *
 *  流程：
 *   HTTPINIT → HTTPPARA(CID) → HTTPPARA(URL)
 *   → HTTPACTION=0（GET）→ 等待 +HTTPACTION:0,<code>,<len>
 *   → HTTPREAD → HTTPTERM
 *
 * @param  url      完整 URL（含 http://）
 * @param  resp_buf 响应体输出缓冲区
 * @param  buf_len  缓冲区大小
 * @retval HTTP 状态码（200/404等），-1 表示失败
 */
int Air780E_HttpGet(const char *url, char *resp_buf, uint16_t buf_len)
{
    char cmd_buf[256];
    static char tmp[RX_BUF_SIZE];

    /* 初始化 HTTP */
    if (send_at("AT+HTTPINIT\r", "OK", 5000) != 0) return -1;

    /* 绑定 SAPBR 承载（cid=1） */
    if (send_at("AT+HTTPPARA=\"CID\",1\r", "OK", 3000) != 0) goto fail;

    /* 设置目标 URL */
    snprintf(cmd_buf, sizeof(cmd_buf), "AT+HTTPPARA=\"URL\",\"%s\"\r", url);
    if (send_at(cmd_buf, "OK", 5000) != 0) goto fail;

    /* 发起 GET 请求（HTTPACTION=0），等待 +HTTPACTION:0,<code>,<len> */
    rx_flush();
    if (send_at("AT+HTTPACTION=0\r", "+HTTPACTION", 30000) != 0) goto fail;

    /* 从应答中解析 HTTP 状态码 */
    uint16_t avail = rx_available();
    uint16_t copy  = (avail < sizeof(tmp) - 1) ? avail : sizeof(tmp) - 1;
    for (uint16_t i = 0; i < copy; i++)
        tmp[i] = rx_buf[(rx_head + i) % RX_BUF_SIZE];
    tmp[copy] = '\0';

    char *action = strstr(tmp, "+HTTPACTION:");
    int   http_code = -1;
    if (action)
    {
        /* 格式：+HTTPACTION:0,200,1234 */
        char *p = strchr(action, ',');
        if (p) http_code = atoi(p + 1);
    }

    /* 读取响应体 */
    if (send_at("AT+HTTPREAD\r", "+HTTPREAD:", 10000) != 0) goto fail;

    /* 从缓冲区里提取 +HTTPREAD: 之后的内容 */
    avail = rx_available();
    copy  = (avail < sizeof(tmp) - 1) ? avail : sizeof(tmp) - 1;
    for (uint16_t i = 0; i < copy; i++)
        tmp[i] = rx_buf[(rx_head + i) % RX_BUF_SIZE];
    tmp[copy] = '\0';
    rx_head = rx_tail;

    char *body = strstr(tmp, "+HTTPREAD:");
    if (body)
    {
        body = strchr(body, '\n');  /* 跳过 +HTTPREAD:len\r\n 这行 */
        if (body)
        {
            body++;  /* 跳过 \n */
            uint16_t body_len = (uint16_t)strlen(body);
            uint16_t read_len = (body_len < buf_len - 1) ? body_len : buf_len - 1;
            memcpy(resp_buf, body, read_len);
            resp_buf[read_len] = '\0';
        }
    }

    /* 关闭 HTTP 会话 */
    send_at("AT+HTTPTERM\r", "OK", 3000);
    return http_code;

fail:
    send_at("AT+HTTPTERM\r", "OK", 3000);
    return -1;
}

/**
 * @brief  发起 HTTP POST 请求并读取响应体
 *
 *  流程：
 *   HTTPINIT → HTTPPARA(CID) → HTTPPARA(URL) → HTTPPARA(CONTENT)
 *   → HTTPDATA=<len>,<timeout>  等 DOWNLOAD 后发 body
 *   → HTTPACTION=1（POST）→ 等待 +HTTPACTION:1,<code>,<len>
 *   → HTTPREAD → HTTPTERM
 *
 * @param  url          完整 URL
 * @param  content_type Content-Type，如 "application/json"
 * @param  body         请求体数据
 * @param  body_len     请求体字节数
 * @param  resp_buf     响应体输出缓冲区
 * @param  buf_len      缓冲区大小
 * @retval HTTP 状态码，-1 表示失败
 */
int Air780E_HttpPost(const char *url, const char *content_type,
                     const char *body, uint16_t body_len,
                     char *resp_buf, uint16_t buf_len)
{
    char cmd_buf[256];
    static char tmp[RX_BUF_SIZE];

    if (send_at("AT+HTTPINIT\r", "OK", 5000) != 0) return -1;

    if (send_at("AT+HTTPPARA=\"CID\",1\r", "OK", 3000) != 0) goto fail;

    snprintf(cmd_buf, sizeof(cmd_buf), "AT+HTTPPARA=\"URL\",\"%s\"\r", url);
    if (send_at(cmd_buf, "OK", 5000) != 0) goto fail;

    /* 设置 Content-Type（可选，根据服务器要求） */
    snprintf(cmd_buf, sizeof(cmd_buf),
             "AT+HTTPPARA=\"CONTENT\",\"%s\"\r", content_type);
    if (send_at(cmd_buf, "OK", 3000) != 0) goto fail;

    /* 准备 POST 数据：AT+HTTPDATA=<len>,<最大等待ms> */
    snprintf(cmd_buf, sizeof(cmd_buf), "AT+HTTPDATA=%u,10000\r", body_len);
    if (send_at(cmd_buf, "DOWNLOAD", 5000) != 0) goto fail;

    /* 收到 DOWNLOAD 提示后发送请求体 */
    for (uint16_t i = 0; i < body_len; i++)
    {
        while (!(USART1->SR & USART_SR_TXE));
        USART1->DR = (uint8_t)body[i];
    }
    /* 等待模块确认数据接收完毕 */
    if (rx_wait_string("OK", 5000) != 0) goto fail;

    /* 发起 POST 请求（HTTPACTION=1） */
    rx_flush();
    if (send_at("AT+HTTPACTION=1\r", "+HTTPACTION", 30000) != 0) goto fail;

    /* 解析 HTTP 状态码 */
    uint16_t avail = rx_available();
    uint16_t copy  = (avail < sizeof(tmp) - 1) ? avail : sizeof(tmp) - 1;
    for (uint16_t i = 0; i < copy; i++)
        tmp[i] = rx_buf[(rx_head + i) % RX_BUF_SIZE];
    tmp[copy] = '\0';

    char *action = strstr(tmp, "+HTTPACTION:");
    int   http_code = -1;
    if (action)
    {
        char *p = strchr(action, ',');
        if (p) http_code = atoi(p + 1);
    }

    /* 读取响应体 */
    if (send_at("AT+HTTPREAD\r", "+HTTPREAD:", 10000) != 0) goto fail;

    avail = rx_available();
    copy  = (avail < sizeof(tmp) - 1) ? avail : sizeof(tmp) - 1;
    for (uint16_t i = 0; i < copy; i++)
        tmp[i] = rx_buf[(rx_head + i) % RX_BUF_SIZE];
    tmp[copy] = '\0';
    rx_head = rx_tail;

    char *resp_body = strstr(tmp, "+HTTPREAD:");
    if (resp_body)
    {
        resp_body = strchr(resp_body, '\n');
        if (resp_body)
        {
            resp_body++;
            uint16_t rlen    = (uint16_t)strlen(resp_body);
            uint16_t read_len = (rlen < buf_len - 1) ? rlen : buf_len - 1;
            memcpy(resp_buf, resp_body, read_len);
            resp_buf[read_len] = '\0';
        }
    }

    send_at("AT+HTTPTERM\r", "OK", 3000);
    return http_code;

fail:
    send_at("AT+HTTPTERM\r", "OK", 3000);
    return -1;
}

#endif /* USE_HTTP */

/* ================================================================
 *  内部辅助函数
 * ================================================================ */

/**
 * @brief  初始化 USART1（115200 8N1，开启接收中断）
 *         直接操作寄存器，不依赖 HAL_UART_MODULE
 *         BRR = 72MHz / 115200 = 625 = 0x271
 */
static void uart_init(void)
{
    AIR780E_UART_GPIO_CLK();
    AIR780E_UART_CLK_EN();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin   = AIR780E_UART_TX_PIN;
    gpio.Mode  = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(AIR780E_UART_PORT, &gpio);

    gpio.Pin  = AIR780E_UART_RX_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(AIR780E_UART_PORT, &gpio);

    USART1->BRR = 0x0271U;
    USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;
    USART1->CR2 = 0x0000U;
    USART1->CR3 = 0x0000U;

    HAL_NVIC_SetPriority(AIR780E_UART_IRQ, 1, 0);
    HAL_NVIC_EnableIRQ(AIR780E_UART_IRQ);
}

/**
 * @brief  初始化 RST 引脚（PA1，推挽输出，初始低电平）
 */
static void rst_pin_init(void)
{
    AIR780E_RST_CLK();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin   = AIR780E_RST_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(AIR780E_RST_PORT, &gpio);
    HAL_GPIO_WritePin(AIR780E_RST_PORT, AIR780E_RST_PIN, GPIO_PIN_RESET);
}

/**
 * @brief  发送 AT 命令并等待期望应答
 */
static int send_at(const char *cmd, const char *expect, uint32_t timeout_ms)
{
    rx_flush();
    for (uint16_t i = 0; i < (uint16_t)strlen(cmd); i++)
    {
        while (!(USART1->SR & USART_SR_TXE));
        USART1->DR = (uint8_t)cmd[i];
    }
    return rx_wait_string(expect, timeout_ms);
}

/**
 * @brief  等待缓冲区中出现目标字符串，超时返回 -1
 */
static int rx_wait_string(const char *target, uint32_t timeout_ms)
{
    static char tmp[RX_BUF_SIZE];
    uint32_t start      = HAL_GetTick();
    uint16_t target_len = (uint16_t)strlen(target);

    while ((HAL_GetTick() - start) < timeout_ms)
    {
        uint16_t avail = rx_available();
        if (avail >= target_len)
        {
            uint16_t len = (avail < sizeof(tmp) - 1) ? avail : sizeof(tmp) - 1;
            for (uint16_t i = 0; i < len; i++)
                tmp[i] = rx_buf[(rx_head + i) % RX_BUF_SIZE];
            tmp[len] = '\0';
            if (strstr(tmp, target) != NULL) return 0;
        }
        HAL_Delay(10);
    }
    return -1;
}

static uint16_t rx_available(void)
{
    return (rx_tail - rx_head + RX_BUF_SIZE) % RX_BUF_SIZE;
}

static void rx_flush(void)
{
    rx_head = rx_tail;
}

/* ================================================================
 *  MQTT 专用：发布 / 接收
 * ================================================================ */
#ifdef USE_MQTT

/**
 * @brief  发布 MQTT 消息（AT+MPUBEX 定长方式）
 *  → AT+MPUBEX="topic",qos,retain,len  ← >  → payload  ← OK
 *  使用 MPUBEX 而非 MPUB，原因：
 *    1. 支持任意内容（含 JSON、二进制），无需对 " 做转义
 *    2. 发送流程与 TCP CIPSEND 一致，方便统一处理
 */
int Air780E_MqttPublish(const char *topic, const char *payload, uint16_t len)
{
    char cmd_buf[128];
    if (g_state != AIR780E_STATE_CONNECTED || len == 0) return -1;

    /* AT+MPUBEX="topic",qos=0,retain=0,len */
    snprintf(cmd_buf, sizeof(cmd_buf),
             "AT+MPUBEX=\"%s\",0,0,%u\r", topic, len);
    rx_flush();
    for (uint16_t i = 0; i < (uint16_t)strlen(cmd_buf); i++)
    {
        while (!(USART1->SR & USART_SR_TXE));
        USART1->DR = (uint8_t)cmd_buf[i];
    }

    /* 等待 > 提示符 */
    if (rx_wait_string(">", 3000) != 0) return -1;

    /* 发送 payload */
    for (uint16_t i = 0; i < len; i++)
    {
        while (!(USART1->SR & USART_SR_TXE));
        USART1->DR = (uint8_t)payload[i];
    }

    /* 等待 OK 确认 */
    if (rx_wait_string("OK", 5000) != 0) return -1;
    return 0;
}

/**
 * @brief  轮询接收订阅消息，解析 +MSUB:"topic",len,data 格式
 *
 *  模块收到订阅消息后主动推送：
 *    +MSUB:"device/receive",5,hello\r\n
 *
 * @param  topic_buf   输出 topic 字符串（至少 64 字节）
 * @param  payload_buf 输出消息内容
 * @param  buf_len     payload_buf 缓冲区大小
 * @retval 消息字节数，0 表示暂无新消息
 */
uint16_t Air780E_MqttRecv(char *topic_buf, char *payload_buf, uint16_t buf_len)
{
    static char parse_buf[RX_BUF_SIZE];
    uint16_t avail = rx_available();
    if (avail == 0) return 0;

    uint16_t copy_len = (avail < sizeof(parse_buf) - 1) ? avail : sizeof(parse_buf) - 1;
    for (uint16_t i = 0; i < copy_len; i++)
        parse_buf[i] = rx_buf[(rx_head + i) % RX_BUF_SIZE];
    parse_buf[copy_len] = '\0';

    /* 查找 +MSUB: 标头 */
    char *msub = strstr(parse_buf, "+MSUB:");
    if (msub == NULL)
    {
        rx_head = rx_tail;  /* 丢弃无关数据 */
        return 0;
    }

    /* 格式：+MSUB:"topic",len,data
     * 先找第一个逗号定位 topic 结束，再找第二个逗号定位 len */
    char *p = msub + 6;  /* 跳过 "+MSUB:" */

    /* 解析 topic（带引号，如 "device/recv"） */
    char *topic_start = strchr(p, '"');
    if (!topic_start) { rx_head = rx_tail; return 0; }
    topic_start++;  /* 跳过开引号 */
    char *topic_end = strchr(topic_start, '"');
    if (!topic_end) return 0;  /* 引号未收完，等待更多数据 */

    uint16_t topic_len = (uint16_t)(topic_end - topic_start);
    if (topic_len > 63) topic_len = 63;
    memcpy(topic_buf, topic_start, topic_len);
    topic_buf[topic_len] = '\0';

    /* 跳过 topic 后的 , 找数据长度 */
    char *len_start = topic_end + 2;  /* 跳过 ", */
    char *comma2    = strchr(len_start, ',');
    if (!comma2) return 0;

    uint16_t data_len = (uint16_t)atoi(len_start);
    if (data_len == 0) { rx_head = rx_tail; return 0; }

    char    *data_start    = comma2 + 1;
    uint16_t header_offset = (uint16_t)(data_start - parse_buf);

    /* 检查完整数据是否已到达 */
    if (header_offset + data_len > copy_len) return 0;

    uint16_t read_len = (data_len < buf_len - 1) ? data_len : buf_len - 1;
    memcpy(payload_buf, data_start, read_len);
    payload_buf[read_len] = '\0';

    /* 消费已解析的数据 */
    uint16_t garbage = (uint16_t)(msub - parse_buf);
    rx_head = (rx_head + garbage + header_offset + data_len) % RX_BUF_SIZE;
    return read_len;
}

#endif /* USE_MQTT */
