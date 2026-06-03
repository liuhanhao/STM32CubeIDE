#ifndef __UART_H
#define __UART_H

#include "stm32f1xx_hal.h"

/**
 * @brief 初始化 USART1（PA9=TX, PA10=RX，9600bps）
 *        并使能 RXNE 中断，用于接收 SU-03T 串口命令
 */
void UART_Init(void);

/**
 * @brief 从中断接收缓冲取出一帧数据
 * @param data 输出缓冲区指针（至少 4 字节）
 * @param len  实际读到的字节数
 * @return 1 = 有新帧，0 = 无数据
 */
uint8_t UART_GetFrame(uint8_t *data, uint8_t *len);

/* 供 stm32f1xx_it.c 中 USART1_IRQHandler 调用 */
void USART1_IRQ_Process(void);

#endif /* __UART_H */
