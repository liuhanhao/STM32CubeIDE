/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : W25Q64 软件 SPI 演示
  *                   1. 读取厂商 ID（JEDEC）
  *                   2. 读取设备 ID（0x90 指令）
  *                   3. 写入 01 02 03 04 到地址 0x000000
  *                   4. 读取并显示写入的 4 字节数据
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
#include "main.h"
#include "OLED.h"
#include "W25Q64.h"

void SystemClock_Config(void);
static void MX_GPIO_Init(void);

/**
  * @brief  应用程序入口
  * @retval int
  */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    /* 初始化 OLED 和 W25Q64 */
    OLED_Init();
    W25Q64_Init();

    /* ---- 1. 读取 JEDEC 厂商 ID ---- */
    uint8_t  manuID_jedec  = 0;
    uint16_t deviceID_jedec = 0;
    W25Q64_ReadJedecID(&manuID_jedec, &deviceID_jedec);

    /* ---- 2. 读取设备 ID（0x90 指令） ---- */
    uint8_t manuID_90  = 0;
    uint8_t deviceID_90 = 0;
    W25Q64_ReadDeviceID(&manuID_90, &deviceID_90);

    /* ---- 3. 写入 01 02 03 04 到地址 0x000000 ---- */
    /* 先擦除扇区（保证可以写入），再页编程 */
    uint8_t writeData[4] = {0x01, 0x02, 0x03, 0x04};
    W25Q64_SectorErase(0x000000);
    W25Q64_PageProgram(0x000000, writeData, 4);

    /* ---- 4. 读取刚才写入的 4 字节数据 ---- */
    uint8_t readData[4] = {0};
    W25Q64_ReadData(0x000000, readData, 4);

    /* ============ 显示到 OLED ============
     * OLED 共 4 行，每行 16 个字符（8x16 字体）
     *
     * 第1行：ManuID=EF
     * 第2行：DevID=4017
     * 第3行：W:01 02 03 04
     * 第4行：R:01 02 03 04
     * ====================================== */
    OLED_Clear();

    /* 第1行：显示 JEDEC 厂商 ID（期望 0xEF） */
    OLED_ShowString(1, 1, "ManuID=");
    OLED_ShowHexNum(1, 8, manuID_jedec, 2);

    /* 第2行：显示设备 ID（期望 0x4017） */
    OLED_ShowString(2, 1, "DevID=");
    OLED_ShowHexNum(2, 7, deviceID_jedec, 4);

    /* 第3行：显示写入的数据 */
    OLED_ShowString(3, 1, "W:");
    OLED_ShowHexNum(3, 3,  writeData[0], 2);
    OLED_ShowHexNum(3, 6,  writeData[1], 2);
    OLED_ShowHexNum(3, 9,  writeData[2], 2);
    OLED_ShowHexNum(3, 12, writeData[3], 2);

    /* 第4行：显示读取的数据 */
    OLED_ShowString(4, 1, "R:");
    OLED_ShowHexNum(4, 3,  readData[0], 2);
    OLED_ShowHexNum(4, 6,  readData[1], 2);
    OLED_ShowHexNum(4, 9,  readData[2], 2);
    OLED_ShowHexNum(4, 12, readData[3], 2);

    while (1)
    {
    }
}

/**
  * @brief  系统时钟配置
  * @retval None
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
  * @brief  GPIO 初始化（使能时钟；具体引脚由各驱动自行初始化）
  * @retval None
  */
static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
}

/**
  * @brief  错误处理函数
  * @retval None
  */
void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}

#ifdef USE_FULL_ASSERT
/**
  * @brief  断言失败时报告文件名和行号
  * @param  file: 源文件名指针
  * @param  line: 错误所在行号
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
