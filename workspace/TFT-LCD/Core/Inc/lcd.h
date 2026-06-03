/**
 * @file    lcd.h
 * @brief   ILI9341 TFT-LCD HAL SPI 驱动头文件
 *
 * 引脚定义（与商家接线说明一致）：
 *   LCD CS   -> PB11
 *   LCD RST  -> PB12
 *   LCD DC   -> PB10
 *   LCD MOSI -> PB15 (SPI2)
 *   LCD SCK  -> PB13 (SPI2)
 *   LCD MISO -> PB14 (SPI2，可不接)
 *   LCD BL   -> PB9
 */

#ifndef __LCD_H
#define __LCD_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

/* ===================== 用户配置 ===================== */
/* 方向：0=竖屏(240x320)，1=横屏(320x240) */
#define LCD_USE_HORIZONTAL  1

/* ===================== 引脚定义 ===================== */
#define LCD_CS_GPIO_PORT    GPIOB
#define LCD_CS_PIN          GPIO_PIN_11

#define LCD_RST_GPIO_PORT   GPIOB
#define LCD_RST_PIN         GPIO_PIN_12

#define LCD_DC_GPIO_PORT    GPIOB
#define LCD_DC_PIN          GPIO_PIN_10

#define LCD_BL_GPIO_PORT    GPIOB
#define LCD_BL_PIN          GPIO_PIN_9

/* ===================== GPIO 操作宏 ===================== */
#define LCD_CS_LOW()    HAL_GPIO_WritePin(LCD_CS_GPIO_PORT,  LCD_CS_PIN,  GPIO_PIN_RESET)
#define LCD_CS_HIGH()   HAL_GPIO_WritePin(LCD_CS_GPIO_PORT,  LCD_CS_PIN,  GPIO_PIN_SET)
#define LCD_DC_LOW()    HAL_GPIO_WritePin(LCD_DC_GPIO_PORT,  LCD_DC_PIN,  GPIO_PIN_RESET)
#define LCD_DC_HIGH()   HAL_GPIO_WritePin(LCD_DC_GPIO_PORT,  LCD_DC_PIN,  GPIO_PIN_SET)
#define LCD_RST_LOW()   HAL_GPIO_WritePin(LCD_RST_GPIO_PORT, LCD_RST_PIN, GPIO_PIN_RESET)
#define LCD_RST_HIGH()  HAL_GPIO_WritePin(LCD_RST_GPIO_PORT, LCD_RST_PIN, GPIO_PIN_SET)
#define LCD_BL_ON()     HAL_GPIO_WritePin(LCD_BL_GPIO_PORT,  LCD_BL_PIN,  GPIO_PIN_SET)
#define LCD_BL_OFF()    HAL_GPIO_WritePin(LCD_BL_GPIO_PORT,  LCD_BL_PIN,  GPIO_PIN_RESET)

/* ===================== 屏幕尺寸 ===================== */
#if LCD_USE_HORIZONTAL
#define LCD_W   320
#define LCD_H   240
#else
#define LCD_W   240
#define LCD_H   320
#endif

/* ===================== RGB565 颜色常量 ===================== */
#define WHITE       0xFFFF
#define BLACK       0x0000
#define BLUE        0x001F
#define RED         0xF800
#define GREEN       0x07E0
#define CYAN        0x07FF
#define MAGENTA     0xF81F
#define YELLOW      0xFFE0
#define BRED        0XF81F
#define GRED        0XFFE0
#define GBLUE       0X07FF
#define BROWN       0XBC40
#define BRRED       0XFC07
#define GRAY        0X8430
#define DARKBLUE    0X01CF
#define LIGHTBLUE   0X7D7C
#define GRAYBLUE    0X5458
#define LIGHTGREEN  0X841F
#define LGRAY       0XC618
#define LGRAYBLUE   0XA651
#define LBBLUE      0X2B12

/* RGB888 转 RGB565 */
#define RGB888_TO_RGB565(r, g, b) \
    (((uint16_t)((r) & 0xF8) << 8) | ((uint16_t)((g) & 0xFC) << 3) | ((uint16_t)((b) & 0xF8) >> 3))

/* ===================== 全局变量 ===================== */
extern uint16_t POINT_COLOR; /* 当前绘图颜色 */
extern uint16_t BACK_COLOR;  /* 当前背景颜色 */

/* LCD 设备参数结构体 */
typedef struct {
    uint16_t width;     /* LCD 宽度 */
    uint16_t height;    /* LCD 高度 */
    uint8_t  dir;       /* 方向: 0=竖屏, 1=横屏 */
    uint16_t wramcmd;   /* 开始写GRAM命令 */
    uint16_t setxcmd;   /* 设置X坐标命令 */
    uint16_t setycmd;   /* 设置Y坐标命令 */
} LCD_Dev_t;

extern LCD_Dev_t lcddev;

/* ===================== 函数声明 ===================== */

/* 初始化 */
void LCD_Init(void);

/* 基础操作 */
void LCD_WriteCmd(uint8_t cmd);
void LCD_WriteData(uint8_t data);
void LCD_WriteData16(uint16_t data);

/* 显示控制 */
void LCD_DisplayOn(void);
void LCD_DisplayOff(void);

/* 绘图基础 */
void LCD_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void LCD_SetCursor(uint16_t x, uint16_t y);
void LCD_DrawPoint(uint16_t x, uint16_t y);
void LCD_DrawPoint_Color(uint16_t x, uint16_t y, uint16_t color);

/* 图形填充 */
void LCD_Clear(uint16_t color);
void LCD_Fill(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void LCD_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void LCD_DrawFillRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void LCD_Draw_Circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color, uint8_t fill);

/* 字符显示 */
void LCD_ShowChar(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc,
                  char ch, uint8_t size, uint8_t mode);
void LCD_ShowString(uint16_t x, uint16_t y, uint8_t size,
                    const char *str, uint8_t mode);
void LCD_ShowNum(uint16_t x, uint16_t y, uint32_t num,
                 uint8_t len, uint8_t size);
void LCD_Show_Chinese(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc,
                      const uint8_t *s, uint8_t size, uint8_t mode);

/* 图片显示 */
void LCD_DrawBMP(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                 const uint8_t *bmp);

#endif /* __LCD_H */
