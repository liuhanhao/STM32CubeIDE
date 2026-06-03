/**
 * @file    uart.c
 * @brief   USART1 驱动 — 接收 SU-03T 语音识别串口命令
 *
 * 接线：
 *   STM32 PA9  (USART1_TX) → SU-03T B6 (UART1_RXD)
 *   STM32 PA10 (USART1_RX) → SU-03T B7 (UART1_TXD)
 *   共地
 *
 * SU-03T 串口帧格式（平台默认帧，4字节）：
 *   0xFD  CMD_H  CMD_L  0xDF
 * 帧头 0xFD，帧尾 0xDF，中间两字节为命令 ID。
 *
 * 本驱动使用环形缓冲 + RXNE 中断方式接收，
 * 主循环调用 UART_GetFrame() 取数据。
 */

#include "uart.h"

/* ------------------------------------------------------------------ */
/*  环形缓冲                                                              */
/* ------------------------------------------------------------------ */
#define RX_BUF_SIZE  64

static uint8_t  s_rx_buf[RX_BUF_SIZE];
static uint8_t  s_rx_head = 0;
static uint8_t  s_rx_tail = 0;

static UART_HandleTypeDef s_huart1;

/* ------------------------------------------------------------------ */
/*  内部辅助                                                              */
/* ------------------------------------------------------------------ */
static inline uint8_t _buf_empty(void) { return s_rx_head == s_rx_tail; }
static inline uint8_t _buf_count(void)
{
    return (uint8_t)((s_rx_tail - s_rx_head) & (RX_BUF_SIZE - 1));
}
static inline uint8_t _buf_pop(void)
{
    uint8_t v = s_rx_buf[s_rx_head];
    s_rx_head = (s_rx_head + 1) & (RX_BUF_SIZE - 1);
    return v;
}

/* ------------------------------------------------------------------ */
/*  初始化                                                                */
/* ------------------------------------------------------------------ */

void UART_Init(void)
{
    /* 1. 使能时钟 */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    /* 2. PA9=TX(复用推挽), PA10=RX(浮空输入) */
    GPIO_InitTypeDef gpio = {0};

    gpio.Pin   = GPIO_PIN_9;
    gpio.Mode  = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin  = GPIO_PIN_10;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);

    /* 3. USART1 配置：9600-8-N-1 */
    s_huart1.Instance          = USART1;
    s_huart1.Init.BaudRate     = 9600;
    s_huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    s_huart1.Init.StopBits     = UART_STOPBITS_1;
    s_huart1.Init.Parity       = UART_PARITY_NONE;
    s_huart1.Init.Mode         = UART_MODE_TX_RX;
    s_huart1.Init.HwFlowCtl   = UART_HWCONTROL_NONE;
    s_huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&s_huart1);

    /* 4. 使能 RXNE 中断 */
    __HAL_UART_ENABLE_IT(&s_huart1, UART_IT_RXNE);

    /* 5. 配置 NVIC */
    HAL_NVIC_SetPriority(USART1_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
}

/* ------------------------------------------------------------------ */
/*  中断处理（从 stm32f1xx_it.c 的 USART1_IRQHandler 调用）               */
/* ------------------------------------------------------------------ */

void USART1_IRQ_Process(void)
{
    if (__HAL_UART_GET_FLAG(&s_huart1, UART_FLAG_RXNE)) {
        uint8_t byte = (uint8_t)(s_huart1.Instance->DR & 0xFF);
        uint8_t next_tail = (s_rx_tail + 1) & (RX_BUF_SIZE - 1);
        if (next_tail != s_rx_head) {   /* 缓冲未满则存入 */
            s_rx_buf[s_rx_tail] = byte;
            s_rx_tail = next_tail;
        }
    }
}

/* ------------------------------------------------------------------ */
/*  取帧：寻找 0xFD...0xDF 四字节帧                                        */
/* ------------------------------------------------------------------ */

uint8_t UART_GetFrame(uint8_t *data, uint8_t *len)
{
    /* 缓冲至少需要 4 字节 */
    while (_buf_count() >= 4) {
        /* 扫描帧头 */
        if (s_rx_buf[s_rx_head] != 0xFD) {
            _buf_pop();   /* 丢弃非帧头字节 */
            continue;
        }

        /* 检查第 4 个字节是否为帧尾 0xDF */
        uint8_t tail_pos = (s_rx_head + 3) & (RX_BUF_SIZE - 1);
        if (s_rx_buf[tail_pos] != 0xDF) {
            _buf_pop();   /* 帧尾不对，丢弃帧头，继续扫 */
            continue;
        }

        /* 合法帧：取出 4 字节 */
        data[0] = _buf_pop();   /* 0xFD */
        data[1] = _buf_pop();   /* CMD_H */
        data[2] = _buf_pop();   /* CMD_L */
        data[3] = _buf_pop();   /* 0xDF */
        *len = 4;
        return 1;
    }
    return 0;
}
