/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : W25Q64 SPI Flash 读写演示
  *                   1. 读取厂商 ID
  *                   2. 读取设备 ID
  *                   3. 写入 01 02 03 04 到地址 0x000000
  *                   4. 读取第 3 步写入的值
  *                   以上 4 项分别显示到 OLED 屏幕
  ******************************************************************************
  */
#include "main.h"
#include "OLED.h"
#include "W25Q64.h"

void SystemClock_Config(void);
static void MX_GPIO_Init(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    /* 初始化 OLED 和 W25Q64 */
    OLED_Init();
    W25Q64_Init();

    /* ---- 1. 读取厂商 ID 和设备 ID ---- */
    uint8_t MFR_ID = 0, DEV_ID = 0;
    W25Q64_ReadID(&MFR_ID, &DEV_ID);

    /* ---- 2. 写入 01 02 03 04，先擦除扇区再编程 ---- */
    uint8_t WriteData[4] = {0x01, 0x02, 0x03, 0x07};
    W25Q64_SectorErase(0x000000);                    /* 擦除第 0 扇区 */
    W25Q64_PageProgram(0x000000, WriteData, 4);      /* 写入 4 字节 */

    /* ---- 3. 读取刚写入的数据 ---- */
    uint8_t ReadData[4] = {0};
    W25Q64_ReadData(0x000000, ReadData, 4);

    /* ---- 4. 显示到 OLED ---- */

    /* 第 1 行：显示厂商 ID（Winbond = 0xEF） */
    OLED_ShowString(1, 1, "MFR ID:0x");
    OLED_ShowHexNum(1, 10, MFR_ID, 2);

    /* 第 2 行：显示设备 ID（W25Q64 = 0x16） */
    OLED_ShowString(2, 1, "DEV ID:0x");
    OLED_ShowHexNum(2, 10, DEV_ID, 2);

    /* 第 3 行：写入的值 01 02 03 04 */
    OLED_ShowString(3, 1, "W:");
    OLED_ShowHexNum(3, 3,  WriteData[0], 2);
    OLED_ShowHexNum(3, 6,  WriteData[1], 2);
    OLED_ShowHexNum(3, 9,  WriteData[2], 2);
    OLED_ShowHexNum(3, 12, WriteData[3], 2);

    /* 第 4 行：读回的值（应与写入一致） */
    OLED_ShowString(4, 1, "R:");
    OLED_ShowHexNum(4, 3,  ReadData[0], 2);
    OLED_ShowHexNum(4, 6,  ReadData[1], 2);
    OLED_ShowHexNum(4, 9,  ReadData[2], 2);
    OLED_ShowHexNum(4, 12, ReadData[3], 2);

    while (1)
    {
    }
}

/**
  * @brief  系统时钟配置（72MHz，来自 HSE + PLL）
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType    = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState          = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue    = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState          = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState      = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource     = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL        = RCC_PLL_MUL9;
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
  * @brief  GPIO 基础时钟初始化
  */
static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
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
