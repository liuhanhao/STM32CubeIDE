/**
 * @file    at_controller.h
 * @brief   AT 指令控制器接口 —— 封装 USART1 收发，向上层提供同步 AT 指令接口
 */

#ifndef AT_CONTROLLER_H
#define AT_CONTROLLER_H

#include <stdint.h>

/* ------------------------------------------------------------------ */
/*  缓冲区大小宏                                                        */
/* ------------------------------------------------------------------ */
#define AT_RX_BUF_SIZE  1024  /* 接收缓冲区大小（字节），需容纳完整 HTTP 响应 */
#define AT_TX_BUF_SIZE  256   /* 发送缓冲区大小（字节） */

/* ------------------------------------------------------------------ */
/*  函数声明                                                            */
/* ------------------------------------------------------------------ */

/**
 * @brief  初始化 USART1（115200/8N1），开启 RXNE 接收中断
 */
void AT_Init(void);

/**
 * @brief  清空接收缓冲区
 */
void AT_ClearRxBuf(void);

/**
 * @brief  发送原始字符串，自动在末尾追加 \r\n
 * @param  cmd  要发送的 AT 指令字符串（不含 \r\n）
 */
void AT_Send(const char *cmd);

/**
 * @brief  发送指令并等待指定关键字出现
 * @param  cmd         要发送的 AT 指令字符串
 * @param  expect      期望在响应中出现的关键字
 * @param  timeout_ms  等待超时时间（毫秒）
 * @return 1 = 在超时内收到关键字；0 = 超时未收到
 */
uint8_t AT_SendWait(const char *cmd, const char *expect, uint32_t timeout_ms);

/**
 * @brief  检查接收缓冲区中是否包含指定关键字
 * @param  keyword  要搜索的关键字字符串
 * @return 1 = 包含；0 = 不包含
 */
uint8_t AT_Contains(const char *keyword);

/**
 * @brief  获取最近一次 AT_Contains 调用时的线性化缓冲区内容
 */
const char *AT_GetSnapshot(void);

/**
 * @brief  将环形缓冲区内容线性化复制到外部缓冲区
 * @param  out_buf  输出缓冲区
 * @param  buf_size 输出缓冲区大小（含 '\0'）
 * @return 实际复制的字节数
 */
uint16_t AT_Linearize(char *out_buf, uint16_t buf_size);

/**
 * @brief  获取接收缓冲区的只读指针
 * @return 指向内部接收缓冲区的常量指针
 */
const char *AT_GetRxBuf(void);

/**
 * @brief  USART1 中断服务例程
 *         在 stm32f1xx_it.c 的 USART1_IRQHandler() 中调用
 */
void AT_USART1_IRQHandler(void);

#endif /* AT_CONTROLLER_H */
