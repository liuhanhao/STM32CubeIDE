#ifndef __W25Q64_H
#define __W25Q64_H

#include "stm32f1xx_hal.h"

/* ===== 引脚定义 =====
 * DI  (MOSI) -> PA7
 * CLK (SCK)  -> PA5
 * DO  (MISO) -> PA6
 * CS         -> PA4
 */
#define W25Q64_CS_GPIO_PORT   GPIOA
#define W25Q64_CS_PIN         GPIO_PIN_4
#define W25Q64_SCK_GPIO_PORT  GPIOA
#define W25Q64_SCK_PIN        GPIO_PIN_5
#define W25Q64_MISO_GPIO_PORT GPIOA
#define W25Q64_MISO_PIN       GPIO_PIN_6
#define W25Q64_MOSI_GPIO_PORT GPIOA
#define W25Q64_MOSI_PIN       GPIO_PIN_7

/* ===== W25Q64 指令集 ===== */
#define W25Q64_CMD_JEDEC_ID        0x9F  /* 读取 JEDEC ID（厂商ID + 设备ID） */
#define W25Q64_CMD_DEVICE_ID       0x90  /* 读取厂商/设备 ID */
#define W25Q64_CMD_WRITE_ENABLE    0x06  /* 写使能 */
#define W25Q64_CMD_PAGE_PROGRAM    0x02  /* 页编程 */
#define W25Q64_CMD_READ_DATA       0x03  /* 读数据 */
#define W25Q64_CMD_SECTOR_ERASE    0x20  /* 扇区擦除（4KB） */
#define W25Q64_CMD_READ_STATUS_REG 0x05  /* 读状态寄存器 */

/* ===== 对外接口 ===== */
void    W25Q64_Init(void);
void    W25Q64_ReadJedecID(uint8_t *pManuID, uint16_t *pDeviceID);
void    W25Q64_ReadDeviceID(uint8_t *pManuID, uint8_t *pDeviceID);
void    W25Q64_SectorErase(uint32_t addr);
void    W25Q64_PageProgram(uint32_t addr, uint8_t *pData, uint16_t len);
void    W25Q64_ReadData(uint32_t addr, uint8_t *pData, uint32_t len);

#endif /* __W25Q64_H */
