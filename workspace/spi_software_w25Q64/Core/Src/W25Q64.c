/**
 * @file    W25Q64.c
 * @brief   W25Q64 软件模拟 SPI 驱动（MODE0：CPOL=0，CPHA=0）
 *          引脚：CS=PA4  SCK=PA5  MISO=PA6  MOSI=PA7
 */

#include "W25Q64.h"

/* ===================================================
 *  底层 GPIO 操作宏
 * =================================================== */
#define CS_LOW()   HAL_GPIO_WritePin(W25Q64_CS_GPIO_PORT,   W25Q64_CS_PIN,   GPIO_PIN_RESET)
#define CS_HIGH()  HAL_GPIO_WritePin(W25Q64_CS_GPIO_PORT,   W25Q64_CS_PIN,   GPIO_PIN_SET)
#define SCK_LOW()  HAL_GPIO_WritePin(W25Q64_SCK_GPIO_PORT,  W25Q64_SCK_PIN,  GPIO_PIN_RESET)
#define SCK_HIGH() HAL_GPIO_WritePin(W25Q64_SCK_GPIO_PORT,  W25Q64_SCK_PIN,  GPIO_PIN_SET)
#define MOSI_LOW()  HAL_GPIO_WritePin(W25Q64_MOSI_GPIO_PORT, W25Q64_MOSI_PIN, GPIO_PIN_RESET)
#define MOSI_HIGH() HAL_GPIO_WritePin(W25Q64_MOSI_GPIO_PORT, W25Q64_MOSI_PIN, GPIO_PIN_SET)
#define MISO_READ() HAL_GPIO_ReadPin(W25Q64_MISO_GPIO_PORT,  W25Q64_MISO_PIN)

/* ===================================================
 *  软件 SPI 初始化
 * =================================================== */
void W25Q64_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* CS / SCK / MOSI 配置为推挽输出 */
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Pin   = W25Q64_CS_PIN | W25Q64_SCK_PIN | W25Q64_MOSI_PIN;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* MISO 配置为输入（上拉） */
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Pin   = W25Q64_MISO_PIN;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* 空闲状态：CS高，SCK低 */
    CS_HIGH();
    SCK_LOW();
}

/* ===================================================
 *  软件 SPI 传输一个字节（MODE0）
 *  同时发送并接收，MSB 先行
 * =================================================== */
static uint8_t SPI_SwapByte(uint8_t txData)
{
    uint8_t rxData = 0;
    for (int i = 7; i >= 0; i--)
    {
        /* 发送位 */
        if (txData & (1 << i))
            MOSI_HIGH();
        else
            MOSI_LOW();

        SCK_HIGH();

        /* 采样 MISO */
        if (MISO_READ())
            rxData |= (1 << i);

        SCK_LOW();
    }
    return rxData;
}

/* ===================================================
 *  等待 Flash 不忙（轮询状态寄存器 BUSY 位）
 * =================================================== */
static void W25Q64_WaitBusy(void)
{
    uint8_t status;
    CS_LOW();
    SPI_SwapByte(W25Q64_CMD_READ_STATUS_REG);
    /* BUSY 位为 bit0，为 1 时表示正在操作 */
    do {
        status = SPI_SwapByte(0xFF);
    } while (status & 0x01);
    CS_HIGH();
}

/* ===================================================
 *  读取 JEDEC ID
 *  返回：厂商 ID（1 字节）+ 设备 ID（2 字节）
 *  W25Q64 期望值：ManuID=0xEF，DeviceID=0x4017
 * =================================================== */
void W25Q64_ReadJedecID(uint8_t *pManuID, uint16_t *pDeviceID)
{
    CS_LOW();
    SPI_SwapByte(W25Q64_CMD_JEDEC_ID);
    *pManuID   = SPI_SwapByte(0xFF);              /* 厂商 ID */
    *pDeviceID = (uint16_t)SPI_SwapByte(0xFF) << 8; /* 设备 ID 高字节 */
    *pDeviceID |= SPI_SwapByte(0xFF);               /* 设备 ID 低字节 */
    CS_HIGH();
}

/* ===================================================
 *  读取厂商 / 设备 ID（0x90 指令）
 *  W25Q64 期望值：ManuID=0xEF，DeviceID=0x16
 * =================================================== */
void W25Q64_ReadDeviceID(uint8_t *pManuID, uint8_t *pDeviceID)
{
    CS_LOW();
    SPI_SwapByte(W25Q64_CMD_DEVICE_ID);
    /* 发送 3 字节地址 0x000000 */
    SPI_SwapByte(0x00);
    SPI_SwapByte(0x00);
    SPI_SwapByte(0x00);
    *pManuID   = SPI_SwapByte(0xFF);
    *pDeviceID = SPI_SwapByte(0xFF);
    CS_HIGH();
}

/* ===================================================
 *  扇区擦除（4KB），写入前必须先擦除
 * =================================================== */
void W25Q64_SectorErase(uint32_t addr)
{
    /* 写使能 */
    CS_LOW();
    SPI_SwapByte(W25Q64_CMD_WRITE_ENABLE);
    CS_HIGH();

    /* 发送擦除指令 + 24 位地址 */
    CS_LOW();
    SPI_SwapByte(W25Q64_CMD_SECTOR_ERASE);
    SPI_SwapByte((addr >> 16) & 0xFF);
    SPI_SwapByte((addr >> 8)  & 0xFF);
    SPI_SwapByte( addr        & 0xFF);
    CS_HIGH();

    W25Q64_WaitBusy();
}

/* ===================================================
 *  页编程（最多 256 字节，不可跨页）
 *  写入前必须确保目标区域已擦除
 * =================================================== */
void W25Q64_PageProgram(uint32_t addr, uint8_t *pData, uint16_t len)
{
    /* 写使能 */
    CS_LOW();
    SPI_SwapByte(W25Q64_CMD_WRITE_ENABLE);
    CS_HIGH();

    /* 发送页编程指令 + 24 位地址 + 数据 */
    CS_LOW();
    SPI_SwapByte(W25Q64_CMD_PAGE_PROGRAM);
    SPI_SwapByte((addr >> 16) & 0xFF);
    SPI_SwapByte((addr >> 8)  & 0xFF);
    SPI_SwapByte( addr        & 0xFF);
    for (uint16_t i = 0; i < len; i++)
    {
        SPI_SwapByte(pData[i]);
    }
    CS_HIGH();

    W25Q64_WaitBusy();
}

/* ===================================================
 *  读取数据（支持任意长度连续读取）
 * =================================================== */
void W25Q64_ReadData(uint32_t addr, uint8_t *pData, uint32_t len)
{
    CS_LOW();
    SPI_SwapByte(W25Q64_CMD_READ_DATA);
    SPI_SwapByte((addr >> 16) & 0xFF);
    SPI_SwapByte((addr >> 8)  & 0xFF);
    SPI_SwapByte( addr        & 0xFF);
    for (uint32_t i = 0; i < len; i++)
    {
        pData[i] = SPI_SwapByte(0xFF);
    }
    CS_HIGH();
}
