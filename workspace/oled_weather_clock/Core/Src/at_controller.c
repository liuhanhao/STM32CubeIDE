/**
 * @file    at_controller.c
 * @brief   AT 指令控制器实现 —— 封装 USART1 收发，向上层提供同步 AT 指令接口
 */

#include "at_controller.h"
#include "main.h"
#include <string.h>

/* ------------------------------------------------------------------ */
/*  huart1 句柄由 main.c 定义，此处声明为外部引用                        */
/* ------------------------------------------------------------------ */
extern UART_HandleTypeDef huart1;

/* ------------------------------------------------------------------ */
/*  环形接收缓冲区及读写索引                                             */
/* ------------------------------------------------------------------ */
static uint8_t  rx_buf[AT_RX_BUF_SIZE];   /* 512 字节接收缓冲区 */
static volatile uint16_t rx_head = 0;     /* 写索引（ISR 写入） */
static volatile uint16_t rx_tail = 0;     /* 读索引（主循环读取） */

/* ------------------------------------------------------------------ */
/*  AT_Init                                                             */
/* ------------------------------------------------------------------ */
/**
 * @brief  初始化 AT 控制器：清空接收缓冲区，开启 RXNE 中断
 *         USART1 硬件（GPIO、UART 寄存器、NVIC）已由 MX_USART1_UART_Init()
 *         + HAL_UART_MspInit() 负责，此处不重复初始化。
 */
void AT_Init(void)
{
    AT_ClearRxBuf();
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);
}

/* ------------------------------------------------------------------ */
/*  AT_ClearRxBuf                                                       */
/* ------------------------------------------------------------------ */
/**
 * @brief  清空接收缓冲区（重置读写索引）
 */
void AT_ClearRxBuf(void)
{
    rx_head = 0;
    rx_tail = 0;
}

/* ------------------------------------------------------------------ */
/*  AT_Send                                                             */
/* ------------------------------------------------------------------ */
/**
 * @brief  发送原始字符串，末尾追加 "\r\n"，合并为一次 HAL_UART_Transmit 发送
 *         避免分两次发送时 ESP8266 误将不完整指令提前解析
 * @param  cmd  要发送的 AT 指令字符串（不含 \r\n）
 */
void AT_Send(const char *cmd)
{
    /* 使用静态缓冲区拼接指令和 \r\n，避免栈上开大数组 */
    static char tx_buf[AT_TX_BUF_SIZE + 2];  /* +2 for \r\n */
    uint16_t len = (uint16_t)strlen(cmd);

    if (len > AT_TX_BUF_SIZE - 1)
    {
        len = AT_TX_BUF_SIZE - 1;  /* 防止溢出，截断 */
    }

    memcpy(tx_buf, cmd, len);
    tx_buf[len]     = '\r';
    tx_buf[len + 1] = '\n';

    HAL_UART_Transmit(&huart1, (uint8_t *)tx_buf, len + 2U, 1000);
}

/* ------------------------------------------------------------------ */
/*  线性化快照缓冲区（供外部通过 AT_GetSnapshot 访问）                  */
/* ------------------------------------------------------------------ */
static char s_snapshot[AT_RX_BUF_SIZE + 1];

/* ------------------------------------------------------------------ */
/*  AT_Contains                                                         */
/* ------------------------------------------------------------------ */
uint8_t AT_Contains(const char *keyword)
{
    uint16_t len  = 0;
    uint16_t tail = rx_tail;
    uint16_t head = rx_head;

    while (tail != head && len < AT_RX_BUF_SIZE)
    {
        s_snapshot[len++] = (char)rx_buf[tail];
        tail = (tail + 1) % AT_RX_BUF_SIZE;
    }
    s_snapshot[len] = '\0';

    return (strstr(s_snapshot, keyword) != NULL) ? 1 : 0;
}

/* ------------------------------------------------------------------ */
/*  AT_GetSnapshot                                                      */
/* ------------------------------------------------------------------ */
/**
 * @brief  获取最近一次 AT_Contains 调用时的线性化缓冲区内容
 *         必须在 AT_Contains 返回后、下一次 AT_ClearRxBuf 之前调用
 */
const char *AT_GetSnapshot(void)
{
    return s_snapshot;
}

/**
 * @brief  将环形缓冲区内容线性化复制到外部缓冲区
 * @param  out_buf  输出缓冲区
 * @param  buf_size 输出缓冲区大小（含 '\0'）
 * @return 实际复制的字节数
 */
uint16_t AT_Linearize(char *out_buf, uint16_t buf_size)
{
    uint16_t len  = 0;
    uint16_t tail = rx_tail;
    uint16_t head = rx_head;

    while (tail != head && len < (uint16_t)(buf_size - 1U))
    {
        out_buf[len++] = (char)rx_buf[tail];
        tail = (tail + 1U) % AT_RX_BUF_SIZE;
    }
    out_buf[len] = '\0';
    return len;
}

/* ------------------------------------------------------------------ */
/*  AT_GetRxBuf                                                         */
/* ------------------------------------------------------------------ */
/**
 * @brief  获取接收缓冲区的只读指针
 * @return 指向内部接收缓冲区的常量指针
 */
const char *AT_GetRxBuf(void)
{
    return (const char *)rx_buf;
}

/* ------------------------------------------------------------------ */
/*  AT_SendWait                                                         */
/* ------------------------------------------------------------------ */
/**
 * @brief  发送指令并等待指定关键字出现
 * @param  cmd         要发送的 AT 指令字符串
 * @param  expect      期望在响应中出现的关键字
 * @param  timeout_ms  等待超时时间（毫秒）
 * @return 1 = 在超时内收到关键字；0 = 超时未收到
 */
uint8_t AT_SendWait(const char *cmd, const char *expect, uint32_t timeout_ms)
{
    AT_ClearRxBuf();
    AT_Send(cmd);

    uint32_t start = HAL_GetTick();
    while (HAL_GetTick() - start < timeout_ms)
    {
        if (AT_Contains(expect))
        {
            return 1;
        }
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/*  AT_USART1_IRQHandler                                                */
/* ------------------------------------------------------------------ */
/**
 * @brief  USART1 接收中断处理
 *         检查 RXNE 标志，读取数据寄存器，写入环形缓冲区
 *         缓冲区满时丢弃新数据
 */
void AT_USART1_IRQHandler(void)
{
    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE))
    {
        /* 读取数据寄存器（同时自动清除 RXNE 标志） */
        uint8_t data = (uint8_t)(huart1.Instance->DR & 0x00FFU);

        /* 计算写入后的下一个头指针 */
        uint16_t next_head = (rx_head + 1) % AT_RX_BUF_SIZE;

        if (next_head != rx_tail)
        {
            /* 缓冲区未满，写入数据并更新头指针 */
            rx_buf[rx_head] = data;
            rx_head = next_head;
        }
        /* 缓冲区满时丢弃本次数据，不更新 rx_head */
    }
}
