/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : 串口回显 + OLED 显示
  *                   PA9  = USART1_TX（接对方 RXD）
  *                   PA10 = USART1_RX（接对方 TXD）
  *                   收到数据 → 显示到 OLED → 原样发回
  ******************************************************************************
  */
#include "main.h"
#include "OLED.h"
#include <string.h>

/* ------------------------------------------------------------------ */
/*  串口句柄（在 main.h 中以 extern 声明供其他模块使用）               */
/* ------------------------------------------------------------------ */
UART_HandleTypeDef huart1;

/* ------------------------------------------------------------------ */
/*  接收缓冲区                                                          */
/*  采用 DMA + IDLE 空闲中断方案：总线安静一帧后自动判定包结束          */
/* ------------------------------------------------------------------ */
#define RX_BUF_SIZE  16        /* OLED 每行最多显示 16 个字符 */

static uint8_t  rx_dma_buf[RX_BUF_SIZE];  /* DMA 接收缓冲 */
static char     rx_buf[RX_BUF_SIZE + 1];  /* 处理后的字符串缓冲 */
static uint8_t  rx_len = 0;               /* 本包有效字节数 */
static uint8_t  oled_line = 1;            /* 当前写入 OLED 的行，1~4 */
static uint8_t  need_refresh = 0;         /* 主循环刷新标志 */

/* ------------------------------------------------------------------ */
/*  函数声明                                                            */
/* ------------------------------------------------------------------ */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);

/* ------------------------------------------------------------------ */
/*  主函数                                                              */
/* ------------------------------------------------------------------ */
int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART1_UART_Init();

  OLED_Init();
  OLED_ShowString(1, 1, "Waiting...");

  /* 使能 IDLE 空闲中断，启动 DMA 循环接收 */
  __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
  HAL_UART_Receive_DMA(&huart1, rx_dma_buf, RX_BUF_SIZE);

  while (1)
  {
    /* 有新的完整行需要刷新 OLED */
    if (need_refresh)
    {
      need_refresh = 0;
      /* 清除对应行后重新显示，避免残影 */
      OLED_ShowString(oled_line, 1, "                ");   /* 清行 */
      OLED_ShowString(oled_line, 1, rx_buf);
    }
  }
}

/* ------------------------------------------------------------------ */
/*  USART1 全局中断回调（处理 IDLE 空闲中断）                           */
/*  HAL 没有专门的 IDLE 回调，需要在 IRQHandler 里手动处理              */
/* ------------------------------------------------------------------ */
void UART1_IDLE_Handler(void)
{
  if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE) == RESET) return;

  /* 清除 IDLE 标志（先读 SR 再读 DR） */
  __HAL_UART_CLEAR_IDLEFLAG(&huart1);

  /* 停止 DMA，计算本包实际接收到的字节数 */
  HAL_UART_DMAStop(&huart1);

  uint16_t total = RX_BUF_SIZE;
  uint16_t remaining = (uint16_t)__HAL_DMA_GET_COUNTER(huart1.hdmarx);
  rx_len = total - remaining;

  if (rx_len == 0)
  {
    /* 没有数据，重启 DMA */
    HAL_UART_Receive_DMA(&huart1, rx_dma_buf, RX_BUF_SIZE);
    return;
  }

  /* 原样回显给发送方 */
  HAL_UART_Transmit(&huart1, rx_dma_buf, rx_len, 50);

  /* 过滤可见字符，复制到显示缓冲 */
  uint8_t j = 0;
  for (uint8_t i = 0; i < rx_len && j < RX_BUF_SIZE; i++)
  {
    if (rx_dma_buf[i] >= 0x20 && rx_dma_buf[i] <= 0x7E)
    {
      rx_buf[j++] = (char)rx_dma_buf[i];
    }
  }
  rx_buf[j] = '\0';

  if (j > 0)
  {
    oled_line = (oled_line % 4) + 1;
    need_refresh = 1;
  }

  /* 重启 DMA 等待下一包 */
  HAL_UART_Receive_DMA(&huart1, rx_dma_buf, RX_BUF_SIZE);
}

/* ------------------------------------------------------------------ */
/*  原单字节接收回调保留为空（不再使用）                                 */
/* ------------------------------------------------------------------ */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  (void)huart;
}

/* ------------------------------------------------------------------ */
/*  USART1 初始化（115200, 8N1）                                        */
/* ------------------------------------------------------------------ */
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

  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
}

/* ------------------------------------------------------------------ */
/*  系统时钟配置（72MHz，HSE + PLL）                                    */
/* ------------------------------------------------------------------ */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState            = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue      = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL         = RCC_PLL_MUL9;
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

/* ------------------------------------------------------------------ */
/*  GPIO 初始化                                                          */
/* ------------------------------------------------------------------ */
static void MX_GPIO_Init(void)
{
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
}

/* ------------------------------------------------------------------ */
/*  错误处理                                                             */
/* ------------------------------------------------------------------ */
void Error_Handler(void)
{
  __disable_irq();
  while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
