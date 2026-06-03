#ifndef __OLED_H
#define __OLED_H

#include "stm32f1xx_hal.h"

void OLED_Init(void);
void OLED_Clear(void);
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String);
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length);
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);

/**
 * @brief  在指定位置绘制位图图像（列主序，SSD1306 页格式）
 * @param  page    起始页（0~7），每页 8 行像素
 * @param  col     起始列（0~127）
 * @param  width   图像宽度（像素列数）
 * @param  pages   图像高度（页数，1页=8像素行）
 * @param  data    字模数据，长度 = width * pages 字节
 */
void OLED_ShowImage(uint8_t page, uint8_t col,
                    uint8_t width, uint8_t pages,
                    const uint8_t *data);

#endif
