#ifndef __W25Q64_H
#define __W25Q64_H

#include "stm32f1xx_hal.h"

/* W25Q64 指令定义 */
#define W25Q64_WRITE_ENABLE          0x06
#define W25Q64_READ_STATUS_REG1      0x05
#define W25Q64_PAGE_PROGRAM          0x02
#define W25Q64_SECTOR_ERASE_4KB      0x20
#define W25Q64_READ_DATA             0x03
#define W25Q64_READ_MANUFACTURER_ID  0x90  /* 读取厂商ID + 设备ID */
#define W25Q64_DUMMY_BYTE            0xFF

/**
 * @brief 初始化 SPI1 及相关 GPIO
 *        PA4=CS  PA5=SCK  PA6=MISO  PA7=MOSI
 */
void W25Q64_Init(void);

/**
 * @brief 读取厂商 ID 和设备 ID
 * @param MFR_ID 输出：厂商 ID（Winbond = 0xEF）
 * @param DEV_ID 输出：设备 ID（W25Q64 = 0x16）
 */
void W25Q64_ReadID(uint8_t *MFR_ID, uint8_t *DEV_ID);

/**
 * @brief 擦除指定地址所在的 4KB 扇区
 * @param Address 目标地址（会自动对齐到扇区首地址）
 */
void W25Q64_SectorErase(uint32_t Address);

/**
 * @brief 页编程（一次最多写 256 字节，不可跨页）
 * @param Address   起始地址
 * @param DataArray 数据缓冲区指针
 * @param Count     写入字节数
 */
void W25Q64_PageProgram(uint32_t Address, uint8_t *DataArray, uint16_t Count);

/**
 * @brief 从指定地址连续读取数据
 * @param Address   起始地址
 * @param DataArray 数据缓冲区指针
 * @param Count     读取字节数
 */
void W25Q64_ReadData(uint32_t Address, uint8_t *DataArray, uint32_t Count);

#endif /* __W25Q64_H */
