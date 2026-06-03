/**
  ******************************************************************************
  * @file    ble_hc08.c
  * @brief   HC-08 蓝牙模块驱动
  *
  * ============================================================
  *  完整接线说明
  * ============================================================
  *
  *  【OLED — I2C 软件模拟】
  *  ┌─────────────┬───────────────┬──────────────────────┐
  *  │ OLED 引脚   │ STM32 引脚    │ 说明                 │
  *  ├─────────────┼───────────────┼──────────────────────┤
  *  │ VCC         │ 3.3V          │ 供电                 │
  *  │ GND         │ GND           │ 共地                 │
  *  │ SCL         │ PB8           │ 时钟线（开漏输出）   │
  *  │ SDA         │ PB9           │ 数据线（开漏输出）   │
  *  └─────────────┴───────────────┴──────────────────────┘
  *
  *  【HC-08 蓝牙模块 — USART1】
  *  ┌─────────────┬───────────────┬──────────────────────────────┐
  *  │ HC-08 引脚  │ STM32 引脚    │ 说明                         │
  *  ├─────────────┼───────────────┼──────────────────────────────┤
  *  │ VCC         │ 3.3V          │ 供电，⚠️ 只能接 3.3V！       │
  *  │ GND         │ GND           │ 共地                         │
  *  │ TXD         │ PA10(USART1_RX)│ 模块发送 → 单片机接收      │
  *  │ RXD         │ PA9 (USART1_TX)│ 单片机发送 → 模块接收      │
  *  └─────────────┴───────────────┴──────────────────────────────┘
  *
  *  ⚠️ 注意：
  *  1. OLED 与 HC-08 的 VCC 均只能接 3.3V，不可接 5V
  *  2. HC-08 为交叉连接：模块 TXD → PA10，模块 RXD → PA9
  *  3. 3.3V / GND 可多个模块共用同一组引脚
  *
  ******************************************************************************
  */

#include "ble_hc08.h"
#include <string.h>

/* USART1 句柄 */
UART_HandleTypeDef huart1;

/* 单字节中断接收暂存 */
static uint8_t g_rxByte;

/* 完整消息缓冲区 */
static char    g_rxBuf[BLE_RX_BUFFER_SIZE];
static uint8_t g_rxIndex        = 0;
static uint8_t g_msgReady       = 0;   /* 1=收到一条完整消息 */
static uint32_t g_lastRxTick    = 0;   /* 最后一次收到字节的 HAL tick */
static uint32_t g_rxByteCount   = 0;   /* 调试用：累计收到的原始字节数 */

/* AT 模式临时缓冲区（只在 BLE_SendAT 内使用）*/
static char    g_atRespBuf[64];
static uint8_t g_atRespIdx    = 0;
static uint8_t g_atMode       = 0;   /* 1=AT 模式接收中，屏蔽普通消息处理 */

/**
  * @brief  初始化 USART1（9600,8N1）并通过 AT 指令测试模块
  * @retval HAL_OK=成功，HAL_ERROR=失败
  */
HAL_StatusTypeDef BLE_Init(void)
{
    HAL_StatusTypeDef ret;

    /* ---- 配置 USART1 ---- */
    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = 9600;   /* HC-08 默认波特率 9600 */
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;

    ret = HAL_UART_Init(&huart1);
    if (ret != HAL_OK)
        return ret;

    /* ---- 启动首次单字节中断接收 ---- */
    ret = HAL_UART_Receive_IT(&huart1, &g_rxByte, 1);
    if (ret != HAL_OK)
        return ret;

    /* ---- 发送 AT 指令测试模块是否就绪 ----
     * HC-08 未连接时会响应 "OK"；已连接时无响应（正常，不影响透传）
     * 手册要求：模块启动约 150ms，建议上电 200ms 后再发 AT 指令       */
    HAL_Delay(200);
    BLE_SendAT("AT", NULL, 0);   /* 测试通信，响应结果忽略 */

    return HAL_OK;
}

/**
  * @brief  发送 AT 指令并等待响应（仅在模块未连接时有效）
  * @param  cmd      AT 指令字符串（无需 \r\n，函数自动追加）
  * @param  respBuf  存放响应的缓冲区，NULL 表示忽略
  * @param  bufLen   缓冲区长度
  * @retval 1=收到响应，0=超时
  */
uint8_t BLE_SendAT(const char *cmd, char *respBuf, uint8_t bufLen)
{
    uint32_t startTick;
    uint8_t  received = 0;

    /* 清空 AT 响应缓冲区，进入 AT 模式 */
    memset(g_atRespBuf, 0, sizeof(g_atRespBuf));
    g_atRespIdx = 0;
    g_atMode    = 1;

    /* 发送指令（HC-08 AT 指令无需 \r\n，直接发 "AT"、"AT+NAME?" 等即可）*/
    HAL_UART_Transmit(&huart1, (uint8_t *)cmd, (uint16_t)strlen(cmd), 1000);

    /* 等待响应，最长 BLE_AT_TIMEOUT_MS
     * HC-08 响应格式为 "OK" 或 "OK+xxx"（无\r\n），
     * 检测到 'K' 后再等 10ms 收完剩余字符即可退出            */
    startTick = HAL_GetTick();
    while ((HAL_GetTick() - startTick) < BLE_AT_TIMEOUT_MS)
    {
        if (g_atRespIdx > 0 &&
            g_atRespBuf[g_atRespIdx - 1] == 'K')
        {
            HAL_Delay(10);   /* 等后续可能还有的字符（如 "OKsetNAME"）*/
            received = 1;
            break;
        }
    }

    /* 退出 AT 模式，恢复普通透传接收 */
    g_atMode = 0;

    if (respBuf != NULL && bufLen > 0)
    {
        strncpy(respBuf, g_atRespBuf, bufLen - 1);
        respBuf[bufLen - 1] = '\0';
    }

    return received;
}

/**
  * @brief  发送字符串（阻塞，超时 1000 ms）
  */
void BLE_SendString(const char *str)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)str, (uint16_t)strlen(str), 1000);
}

uint32_t BLE_GetRxByteCount(void)
{
    return g_rxByteCount;
}

/**
  * @brief  查询是否收到完整消息
  */
uint8_t BLE_IsMessageReady(void)
{
    return g_msgReady;
}

/**
  * @brief  读取消息内容
  */
void BLE_GetMessage(char *buf, uint8_t maxLen)
{
    strncpy(buf, g_rxBuf, maxLen - 1);
    buf[maxLen - 1] = '\0';
}

/**
  * @brief  清除消息标志，接收下一条
  */
void BLE_ClearMessage(void)
{
    g_rxIndex    = 0;
    g_msgReady   = 0;
    g_lastRxTick = 0;
    memset(g_rxBuf, 0, sizeof(g_rxBuf));
}

/**
  * @brief  超时检测，需在主循环中轮询调用
  *         App 发送数据不带 \r\n 时，靠此函数触发消息就绪
  */
void BLE_Poll(void)
{
    if (g_msgReady || g_rxIndex == 0)
        return;

    /* 距离最后一个字节超过 BLE_RX_TIMEOUT_MS，认为一帧结束 */
    if ((HAL_GetTick() - g_lastRxTick) >= BLE_RX_TIMEOUT_MS)
    {
        g_rxBuf[g_rxIndex] = '\0';
        g_msgReady = 1;
    }
}

/* ------------------------------------------------------------------ */
/*  HAL 回调：每接收到一个字节触发一次                                  */
/* ------------------------------------------------------------------ */

/**
  * @brief  UART 接收完成回调（由 HAL 调用）
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != USART1)
        return;

    g_rxByteCount++;   /* 每收到一个字节就计数，用于调试 */

    char c = (char)g_rxByte;

    /* AT 模式：把字节存入 AT 响应缓冲区，不做普通消息处理 */
    if (g_atMode)
    {
        if (g_atRespIdx < sizeof(g_atRespBuf) - 1)
            g_atRespBuf[g_atRespIdx++] = c;
        HAL_UART_Receive_IT(&huart1, &g_rxByte, 1);
        return;
    }

    if (g_msgReady)
    {
        /* 上一条消息还未处理，丢弃新字节，继续等待 */
        HAL_UART_Receive_IT(&huart1, &g_rxByte, 1);
        return;
    }

    /* '\r' 或 '\n' 视为消息结束符 */
    if (c == '\r' || c == '\n')
    {
        if (g_rxIndex > 0)
        {
            g_rxBuf[g_rxIndex] = '\0';
            g_msgReady = 1;
        }
        /* 若只收到换行符（空行），不置标志，继续接收 */
    }
    else
    {
        if (g_rxIndex < BLE_RX_BUFFER_SIZE - 1)
        {
            g_rxBuf[g_rxIndex++] = c;
        }
        /* 缓冲区满时自动截断，防止溢出 */
        g_lastRxTick = HAL_GetTick();   /* 记录最新字节时间，用于超时检测 */
    }
    /* 重新挂起单字节接收，等待下一个字节 */
    HAL_UART_Receive_IT(&huart1, &g_rxByte, 1);
}
