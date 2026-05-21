/**
  ******************************************************************************
  * @file    W25Q64.c
  * @brief   W25Q64 SPI Flash 驱动（硬件 SPI1）
  *
  * 引脚连接：
  *   PA4 = CS（片选，软件控制）
  *   PA5 = SCK（SPI1 时钟）
  *   PA6 = MISO（SPI1 主入从出）
  *   PA7 = MOSI（SPI1 主出从入）
  ******************************************************************************
  */
#include "W25Q64.h"

/* SPI 句柄 */
static SPI_HandleTypeDef hspi1;

/* CS 引脚控制宏 */
#define W25Q64_CS_LOW()   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET)
#define W25Q64_CS_HIGH()  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET)

/* ===================== 静态内部函数 ===================== */

/**
 * @brief 通过 SPI 收发 1 字节
 */
static uint8_t SPI_SwapByte(uint8_t TxData)
{
    uint8_t RxData = 0;
    HAL_SPI_TransmitReceive(&hspi1, &TxData, &RxData, 1, HAL_MAX_DELAY);
    return RxData;
}

/**
 * @brief 等待 Flash 内部操作完成（轮询 BUSY 位）
 */
static void W25Q64_WaitBusy(void)
{
    uint8_t status;
    W25Q64_CS_LOW();
    SPI_SwapByte(W25Q64_READ_STATUS_REG1);
    do {
        status = SPI_SwapByte(W25Q64_DUMMY_BYTE);
    } while (status & 0x01);   /* Bit0 = BUSY */
    W25Q64_CS_HIGH();
}

/**
 * @brief 发送写使能命令
 */
static void W25Q64_WriteEnable(void)
{
    W25Q64_CS_LOW();
    SPI_SwapByte(W25Q64_WRITE_ENABLE);
    W25Q64_CS_HIGH();
}

/* ===================== 公开函数 ===================== */

/**
 * @brief 初始化 SPI1 及相关 GPIO
 */
void W25Q64_Init(void)
{
    /* 使能时钟 */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_SPI1_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* PA4 = CS：推挽输出，初始高电平（不选中） */
    GPIO_InitStruct.Pin   = GPIO_PIN_4;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    W25Q64_CS_HIGH();

    /* PA5(SCK) PA7(MOSI)：复用推挽输出 */
    GPIO_InitStruct.Pin   = GPIO_PIN_5 | GPIO_PIN_7;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PA6(MISO)：浮空输入 */
    GPIO_InitStruct.Pin  = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* 配置 SPI1：主模式，CPOL=0 CPHA=0（Mode 0），8位，MSB，波特率=PCLK2/8 */
    hspi1.Instance               = SPI1;
    hspi1.Init.Mode              = SPI_MODE_MASTER;
    hspi1.Init.Direction         = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize          = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity       = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase          = SPI_PHASE_1EDGE;
    hspi1.Init.NSS               = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;  /* 72MHz/8 = 9MHz */
    hspi1.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode            = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    HAL_SPI_Init(&hspi1);
}

/**
 * @brief 读取厂商 ID 和设备 ID
 *        指令 0x90 + 24位地址(0x000000) -> 返回 MFR_ID + DEV_ID
 */
void W25Q64_ReadID(uint8_t *MFR_ID, uint8_t *DEV_ID)
{
    W25Q64_CS_LOW();
    SPI_SwapByte(W25Q64_READ_MANUFACTURER_ID);
    SPI_SwapByte(0x00);   /* 24位地址高字节 */
    SPI_SwapByte(0x00);   /* 24位地址中字节 */
    SPI_SwapByte(0x00);   /* 24位地址低字节 */
    *MFR_ID = SPI_SwapByte(W25Q64_DUMMY_BYTE);   /* 厂商 ID */
    *DEV_ID = SPI_SwapByte(W25Q64_DUMMY_BYTE);   /* 设备 ID */
    W25Q64_CS_HIGH();
}

/**
 * @brief 擦除 4KB 扇区（擦除后全为 0xFF，写入前必须先擦除）
 */
void W25Q64_SectorErase(uint32_t Address)
{
    W25Q64_WriteEnable();

    W25Q64_CS_LOW();
    SPI_SwapByte(W25Q64_SECTOR_ERASE_4KB);
    SPI_SwapByte((Address >> 16) & 0xFF);  /* 地址高字节 */
    SPI_SwapByte((Address >>  8) & 0xFF);  /* 地址中字节 */
    SPI_SwapByte( Address        & 0xFF);  /* 地址低字节 */
    W25Q64_CS_HIGH();

    W25Q64_WaitBusy();   /* 等待擦除完成（约 40~400ms） */
}

/**
 * @brief 页编程：向指定地址写入最多 256 字节
 */
void W25Q64_PageProgram(uint32_t Address, uint8_t *DataArray, uint16_t Count)
{
    W25Q64_WriteEnable();

    W25Q64_CS_LOW();
    SPI_SwapByte(W25Q64_PAGE_PROGRAM);
    SPI_SwapByte((Address >> 16) & 0xFF);
    SPI_SwapByte((Address >>  8) & 0xFF);
    SPI_SwapByte( Address        & 0xFF);
    for (uint16_t i = 0; i < Count; i++)
    {
        SPI_SwapByte(DataArray[i]);
    }
    W25Q64_CS_HIGH();

    W25Q64_WaitBusy();   /* 等待编程完成（约 0.4~3ms） */
}

/**
 * @brief 从指定地址连续读取数据
 */
void W25Q64_ReadData(uint32_t Address, uint8_t *DataArray, uint32_t Count)
{
    W25Q64_CS_LOW();
    SPI_SwapByte(W25Q64_READ_DATA);
    SPI_SwapByte((Address >> 16) & 0xFF);
    SPI_SwapByte((Address >>  8) & 0xFF);
    SPI_SwapByte( Address        & 0xFF);
    for (uint32_t i = 0; i < Count; i++)
    {
        DataArray[i] = SPI_SwapByte(W25Q64_DUMMY_BYTE);
    }
    W25Q64_CS_HIGH();
}
