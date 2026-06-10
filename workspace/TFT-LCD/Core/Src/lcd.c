/**
 * @file    lcd.c
 * @brief   ILI9341 TFT-LCD HAL SPI 驱动实现
 *
 * SPI2 配置：
 *   - 模式：SPI_MODE_0（CPOL=0, CPHA=0）
 *   - 数据位：8bit，MSB First
 *   - 波特率预分频：APB1=36MHz, 预分频8 → 4.5MHz（写操作最大10MHz）
 */

#include "lcd.h"
#include "font.h"
#include <stdlib.h>
#include <string.h>

/* ===================== 全局变量 ===================== */
uint16_t  POINT_COLOR = BLACK;  /* 默认前景色 */
uint16_t  BACK_COLOR  = WHITE;  /* 默认背景色 */
LCD_Dev_t lcddev;               /* LCD参数 */

/* SPI2 句柄（在此文件内部初始化） */
static SPI_HandleTypeDef hspi2;

/* ===================== SPI2 初始化 ===================== */
static void LCD_SPI2_Init(void)
{
    /* 使能时钟 */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_SPI2_CLK_ENABLE();

    /* 配置 SPI2 GPIO：PB13=SCK, PB14=MISO, PB15=MOSI */
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin   = GPIO_PIN_13 | GPIO_PIN_15;  /* SCK + MOSI */
    gpio.Mode  = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);

    gpio.Pin  = GPIO_PIN_14;                 /* MISO */
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &gpio);

    /* 配置 SPI2 参数 */
    hspi2.Instance               = SPI2;
    hspi2.Init.Mode              = SPI_MODE_MASTER;
    hspi2.Init.Direction         = SPI_DIRECTION_2LINES;
    hspi2.Init.DataSize          = SPI_DATASIZE_8BIT;
    hspi2.Init.CLKPolarity       = SPI_POLARITY_LOW;   /* CPOL=0 */
    hspi2.Init.CLKPhase          = SPI_PHASE_1EDGE;    /* CPHA=0 */
    hspi2.Init.NSS               = SPI_NSS_SOFT;
    hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8; /* 36MHz/8 = 4.5MHz */
    hspi2.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    hspi2.Init.TIMode            = SPI_TIMODE_DISABLE;
    hspi2.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    HAL_SPI_Init(&hspi2);
    __HAL_SPI_ENABLE(&hspi2);
}

/* ===================== GPIO 初始化（CS/DC/RST/BL） ===================== */
static void LCD_GPIO_Init(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin   = LCD_CS_PIN | LCD_RST_PIN | LCD_DC_PIN | LCD_BL_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);

    /* 初始状态 */
    LCD_CS_HIGH();
    LCD_BL_OFF();
}

/* ===================== 基础 SPI 通信 ===================== */

/**
 * @brief 发送一条命令（DC=0）
 */
void LCD_WriteCmd(uint8_t cmd)
{
    LCD_DC_LOW();
    LCD_CS_LOW();
    HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY);
    LCD_CS_HIGH();
}

/**
 * @brief 发送一字节数据（DC=1）
 */
void LCD_WriteData(uint8_t data)
{
    LCD_DC_HIGH();
    LCD_CS_LOW();
    HAL_SPI_Transmit(&hspi2, &data, 1, HAL_MAX_DELAY);
    LCD_CS_HIGH();
}

/**
 * @brief 发送16位颜色数据（DC=1，高字节先发）
 */
void LCD_WriteData16(uint16_t data)
{
    uint8_t buf[2] = {(uint8_t)(data >> 8), (uint8_t)(data & 0xFF)};
    LCD_DC_HIGH();
    LCD_CS_LOW();
    HAL_SPI_Transmit(&hspi2, buf, 2, HAL_MAX_DELAY);
    LCD_CS_HIGH();
}

/* ===================== 设置参数（方向/分辨率） ===================== */
static void LCD_SetParam(void)
{
    lcddev.wramcmd = 0x2C;
    lcddev.setxcmd = 0x2A;
    lcddev.setycmd = 0x2B;

#if LCD_USE_HORIZONTAL
    /* 横屏 320x240 */
    lcddev.dir    = 1;
    lcddev.width  = 320;
    lcddev.height = 240;
    LCD_WriteCmd(0x36);
    LCD_WriteData(0x68);   /* MY=0, MX=1, MV=1, BGR=1 - 原点在右上角，X轴向左 */
#else
    /* 竖屏 240x320 */
    lcddev.dir    = 0;
    lcddev.width  = 240;
    lcddev.height = 320;
    LCD_WriteCmd(0x36);
    LCD_WriteData(0x08);   /* MY=0, MX=0, MV=0, BGR=1 - 原点在左上角 */
#endif
}

/* ===================== LCD 初始化 ===================== */
void LCD_Init(void)
{
    LCD_GPIO_Init();
    LCD_SPI2_Init();

    /* 硬件复位 */
    LCD_RST_LOW();
    HAL_Delay(100);
    LCD_RST_HIGH();
    HAL_Delay(50);

    /* -------- ILI9341 初始化序列（与商家程序完全一致） -------- */
    LCD_WriteCmd(0xCF);
    LCD_WriteData(0x00); LCD_WriteData(0xC1); LCD_WriteData(0x30);

    LCD_WriteCmd(0xED);
    LCD_WriteData(0x64); LCD_WriteData(0x03);
    LCD_WriteData(0x12); LCD_WriteData(0x81);

    LCD_WriteCmd(0xE8);
    LCD_WriteData(0x85); LCD_WriteData(0x10); LCD_WriteData(0x7A);

    LCD_WriteCmd(0xCB);
    LCD_WriteData(0x39); LCD_WriteData(0x2C); LCD_WriteData(0x00);
    LCD_WriteData(0x34); LCD_WriteData(0x02);

    LCD_WriteCmd(0xF7);
    LCD_WriteData(0x20);

    LCD_WriteCmd(0xEA);
    LCD_WriteData(0x00); LCD_WriteData(0x00);

    LCD_WriteCmd(0xC0);   /* Power control 1 */
    LCD_WriteData(0x1B);  /* GVDD=4.55V */

    LCD_WriteCmd(0xC1);   /* Power control 2 */
    LCD_WriteData(0x01);

    LCD_WriteCmd(0xC5);   /* VCOM control 1 */
    LCD_WriteData(0x30); LCD_WriteData(0x30);

    LCD_WriteCmd(0xC7);   /* VCOM control 2 */
    LCD_WriteData(0xB7);

    LCD_WriteCmd(0x3A);   /* 像素格式：16bit/pixel RGB565 */
    LCD_WriteData(0x55);

    LCD_WriteCmd(0xB1);   /* 帧率：70Hz */
    LCD_WriteData(0x00); LCD_WriteData(0x1A);

    LCD_WriteCmd(0xB6);   /* 显示功能控制 */
    LCD_WriteData(0x0A); LCD_WriteData(0xA2);

    LCD_WriteCmd(0xF2);   /* 3Gamma 关闭 */
    LCD_WriteData(0x00);

    LCD_WriteCmd(0x26);   /* Gamma 曲线选择 */
    LCD_WriteData(0x01);

    LCD_WriteCmd(0xE0);   /* 正 Gamma 校正 */
    LCD_WriteData(0x0F); LCD_WriteData(0x2A); LCD_WriteData(0x28); LCD_WriteData(0x08);
    LCD_WriteData(0x0E); LCD_WriteData(0x08); LCD_WriteData(0x54); LCD_WriteData(0xA9);
    LCD_WriteData(0x43); LCD_WriteData(0x0A); LCD_WriteData(0x0F); LCD_WriteData(0x00);
    LCD_WriteData(0x00); LCD_WriteData(0x00); LCD_WriteData(0x00);

    LCD_WriteCmd(0xE1);   /* 负 Gamma 校正 */
    LCD_WriteData(0x00); LCD_WriteData(0x15); LCD_WriteData(0x17); LCD_WriteData(0x07);
    LCD_WriteData(0x11); LCD_WriteData(0x06); LCD_WriteData(0x2B); LCD_WriteData(0x56);
    LCD_WriteData(0x3C); LCD_WriteData(0x05); LCD_WriteData(0x10); LCD_WriteData(0x0F);
    LCD_WriteData(0x3F); LCD_WriteData(0x3F); LCD_WriteData(0x0F);

    /* 设置全屏显示区域 */
    LCD_WriteCmd(0x2B);
    LCD_WriteData(0x00); LCD_WriteData(0x00);
    LCD_WriteData(0x01); LCD_WriteData(0x3F);

    LCD_WriteCmd(0x2A);
    LCD_WriteData(0x00); LCD_WriteData(0x00);
    LCD_WriteData(0x00); LCD_WriteData(0xEF);

    LCD_WriteCmd(0x11);  /* Sleep Out */
    HAL_Delay(120);

    LCD_WriteCmd(0x29);  /* Display ON */

    LCD_SetParam();      /* 设置方向/分辨率 */
    LCD_BL_ON();         /* 开背光 */
}

/* ===================== 显示控制 ===================== */
void LCD_DisplayOn(void)  { LCD_WriteCmd(0x29); }
void LCD_DisplayOff(void) { LCD_WriteCmd(0x28); }

/* ===================== 绘图窗口 ===================== */
void LCD_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    LCD_WriteCmd(lcddev.setxcmd);
    LCD_WriteData(x0 >> 8); LCD_WriteData(x0 & 0xFF);
    LCD_WriteData(x1 >> 8); LCD_WriteData(x1 & 0xFF);

    LCD_WriteCmd(lcddev.setycmd);
    LCD_WriteData(y0 >> 8); LCD_WriteData(y0 & 0xFF);
    LCD_WriteData(y1 >> 8); LCD_WriteData(y1 & 0xFF);

    LCD_WriteCmd(lcddev.wramcmd);  /* 准备写GRAM */
}

void LCD_SetCursor(uint16_t x, uint16_t y)
{
    LCD_SetWindow(x, y, x, y);
}

/* ===================== 画点 ===================== */
void LCD_DrawPoint(uint16_t x, uint16_t y)
{
    LCD_SetCursor(x, y);
    LCD_WriteData16(POINT_COLOR);
}

void LCD_DrawPoint_Color(uint16_t x, uint16_t y, uint16_t color)
{
    LCD_SetCursor(x, y);
    LCD_WriteData16(color);
}

/* ===================== 全屏清除 ===================== */
void LCD_Clear(uint16_t color)
{
    uint8_t  buf[2] = {(uint8_t)(color >> 8), (uint8_t)(color & 0xFF)};
    uint32_t total  = (uint32_t)lcddev.width * lcddev.height;

    LCD_SetWindow(0, 0, lcddev.width - 1, lcddev.height - 1);
    LCD_DC_HIGH();
    LCD_CS_LOW();
    for (uint32_t i = 0; i < total; i++) {
        HAL_SPI_Transmit(&hspi2, buf, 2, HAL_MAX_DELAY);
    }
    LCD_CS_HIGH();
}

/* ===================== 矩形填充 ===================== */
void LCD_Fill(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
    if (x0 > x1 || y0 > y1) return;
    uint8_t  buf[2] = {(uint8_t)(color >> 8), (uint8_t)(color & 0xFF)};
    uint32_t total  = (uint32_t)(x1 - x0 + 1) * (y1 - y0 + 1);

    LCD_SetWindow(x0, y0, x1, y1);
    LCD_DC_HIGH();
    LCD_CS_LOW();
    for (uint32_t i = 0; i < total; i++) {
        HAL_SPI_Transmit(&hspi2, buf, 2, HAL_MAX_DELAY);
    }
    LCD_CS_HIGH();
}

/* ===================== 画线（Bresenham） ===================== */
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    int dx = (int)x2 - (int)x1;
    int dy = (int)y2 - (int)y1;
    int ux = (dx >= 0) ? 1 : -1;
    int uy = (dy >= 0) ? 1 : -1;
    int x  = x1, y = y1;
    int eps;

    dx = abs(dx); dy = abs(dy);
    if (dx > dy) {
        eps = -dx / 2;
        for (; x != x2; x += ux) {
            LCD_DrawPoint(x, y);
            eps += dy;
            if (eps >= 0) { y += uy; eps -= dx; }
        }
    } else {
        eps = -dy / 2;
        for (; y != y2; y += uy) {
            LCD_DrawPoint(x, y);
            eps += dx;
            if (eps >= 0) { x += ux; eps -= dy; }
        }
    }
    LCD_DrawPoint(x2, y2);
}

/* ===================== 空心矩形 ===================== */
void LCD_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    LCD_DrawLine(x1, y1, x2, y1);
    LCD_DrawLine(x1, y2, x2, y2);
    LCD_DrawLine(x1, y1, x1, y2);
    LCD_DrawLine(x2, y1, x2, y2);
}

/* ===================== 实心矩形 ===================== */
void LCD_DrawFillRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    LCD_Fill(x1, y1, x2, y2, POINT_COLOR);
}

/* ===================== 画圆（Midpoint） ===================== */
void LCD_Draw_Circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color, uint8_t fill)
{
    int x = 0, y = r;
    int d = 1 - r;

    while (x <= y) {
        if (fill) {
            LCD_DrawLine(x0 - x, y0 + y, x0 + x, y0 + y);
            LCD_DrawLine(x0 - y, y0 + x, x0 + y, y0 + x);
            LCD_DrawLine(x0 - x, y0 - y, x0 + x, y0 - y);
            LCD_DrawLine(x0 - y, y0 - x, x0 + y, y0 - x);
        } else {
            LCD_DrawPoint_Color(x0 + x, y0 + y, color);
            LCD_DrawPoint_Color(x0 - x, y0 + y, color);
            LCD_DrawPoint_Color(x0 + x, y0 - y, color);
            LCD_DrawPoint_Color(x0 - x, y0 - y, color);
            LCD_DrawPoint_Color(x0 + y, y0 + x, color);
            LCD_DrawPoint_Color(x0 - y, y0 + x, color);
            LCD_DrawPoint_Color(x0 + y, y0 - x, color);
            LCD_DrawPoint_Color(x0 - y, y0 - x, color);
        }
        if (d < 0) {
            d += 2 * x + 3;
        } else {
            d += 2 * (x - y) + 5;
            y--;
        }
        x++;
    }
}

/* ===================== 显示单个 ASCII 字符 ===================== */
/**
 * @param x,y   左上角坐标
 * @param fc    前景色
 * @param bc    背景色
 * @param ch    字符（ASCII）
 * @param size  字体大小：12 或 16
 * @param mode  0=非透明（背景色填充），1=透明（仅绘制有笔迹的像素）
 */
void LCD_ShowChar(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc,
                  char ch, uint8_t size, uint8_t mode)
{
    uint8_t  w = size / 2;     /* 字符宽度：12号字体=6px，16号字体=8px */
    uint8_t  rows = size;      /* 字符高度 */
    const uint8_t *font;
    uint16_t chr_offset = (uint8_t)(ch - ' '); /* 字符在字体表中的偏移 */

    if (size == 12) {
        /* asc2_1206：每字符12字节，每字节对应1行，每行6像素（bit5为最左像素） */
        font = asc2_1206[chr_offset];
    } else {
        /* asc2_1608：每字符16行×2字节，每行16像素，但字符宽8px（高字节有效） */
        font = asc2_1608[chr_offset];
    }

    if (!mode) {
        /* 非透明模式：设置窗口后逐行逐像素输出 */
        LCD_SetWindow(x, y, x + w - 1, y + rows - 1);
        LCD_DC_HIGH();
        LCD_CS_LOW();
        for (uint8_t row = 0; row < rows; row++) {
            /* 取当前行的字节（1206每行1字节，1608每行取高字节即第一个字节） */
            uint8_t byte = font[row];
            for (uint8_t col = 0; col < w; col++) {
                /* 最低位对应最左像素（反转像素顺序以适应右到左的X轴） */
                uint16_t color = (byte & 0x01) ? fc : bc;
                uint8_t  buf[2] = {(uint8_t)(color >> 8), (uint8_t)(color & 0xFF)};
                HAL_SPI_Transmit(&hspi2, buf, 2, HAL_MAX_DELAY);
                byte >>= 1;
            }
        }
        LCD_CS_HIGH();
    } else {
        /* 透明模式：只画有笔迹的像素 */
        for (uint8_t row = 0; row < rows; row++) {
            uint8_t byte = font[row];
            for (uint8_t col = 0; col < w; col++) {
                if (byte & 0x01) {
                    LCD_DrawPoint_Color(x + col, y + row, fc);
                }
                byte >>= 1;
            }
        }
    }
}

/* ===================== 显示字符串 ===================== */
/**
 * @param mode  0=非透明，1=透明
 */
void LCD_ShowString(uint16_t x, uint16_t y, uint8_t size,
                    const char *str, uint8_t mode)
{
    uint8_t w = size / 2;
    while (*str && x < lcddev.width) {
        LCD_ShowChar(x, y, POINT_COLOR, BACK_COLOR, *str, size, mode);
        x += w;
        str++;
    }
}

/* ===================== 显示无符号整数 ===================== */
static uint32_t lcd_pow10(uint8_t n)
{
    uint32_t r = 1;
    while (n--) r *= 10;
    return r;
}

void LCD_ShowNum(uint16_t x, uint16_t y, uint32_t num,
                 uint8_t len, uint8_t size)
{
    uint8_t  w = size / 2;
    uint8_t  enshow = 0;
    uint8_t  t, digit;

    for (t = 0; t < len; t++) {
        digit = (uint8_t)((num / lcd_pow10(len - t - 1)) % 10);
        if (!enshow && t < (len - 1)) {
            if (digit == 0) {
                LCD_ShowChar(x + t * w, y, POINT_COLOR, BACK_COLOR, ' ', size, 0);
                continue;
            } else {
                enshow = 1;
            }
        }
        LCD_ShowChar(x + t * w, y, POINT_COLOR, BACK_COLOR, '0' + digit, size, 0);
    }
}

/* ===================== 显示中文字符（16x16） ===================== */
/**
 * @param s     指向 GBK 编码双字节汉字
 * @param size  16 / 24 / 32
 */
void LCD_Show_Chinese(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc,
                      const uint8_t *s, uint8_t size, uint8_t mode)
{
    uint16_t num;
    const char *pFont;     /* 与 font.h 中 Msk 字段类型一致（char[]） */
    uint8_t byte_per_char; /* 每个汉字的字节数 */

    if (size == 32) {
        num = sizeof(tfont32) / sizeof(typFNT_GB32);
        byte_per_char = 128; /* 32*4 bytes */
        for (uint16_t k = 0; k < num; k++) {
            if (tfont32[k].Index[0] == s[0] && tfont32[k].Index[1] == s[1]) {
                pFont = tfont32[k].Msk;
                LCD_SetWindow(x, y, x + 31, y + 31);
                LCD_DC_HIGH(); LCD_CS_LOW();
                for (uint8_t i = 0; i < 32 * 4; i++) {
                    uint8_t byte = (uint8_t)pFont[i];
                    for (uint8_t j = 0; j < 8; j++) {
                        uint16_t color = (byte & 0x80) ? fc : bc;
                        uint8_t buf[2] = {(uint8_t)(color >> 8), (uint8_t)(color & 0xFF)};
                        HAL_SPI_Transmit(&hspi2, buf, 2, HAL_MAX_DELAY);
                        byte <<= 1;
                    }
                }
                LCD_CS_HIGH();
                return;
            }
        }
    } else if (size == 24) {
        num = sizeof(tfont24) / sizeof(typFNT_GB24);
        for (uint16_t k = 0; k < num; k++) {
            if (tfont24[k].Index[0] == s[0] && tfont24[k].Index[1] == s[1]) {
                pFont = tfont24[k].Msk;
                LCD_SetWindow(x, y, x + 23, y + 23);
                LCD_DC_HIGH(); LCD_CS_LOW();
                for (uint8_t i = 0; i < 24 * 3; i++) {
                    uint8_t byte = (uint8_t)pFont[i];
                    for (uint8_t j = 0; j < 8; j++) {
                        uint16_t color = (byte & 0x80) ? fc : bc;
                        uint8_t buf[2] = {(uint8_t)(color >> 8), (uint8_t)(color & 0xFF)};
                        HAL_SPI_Transmit(&hspi2, buf, 2, HAL_MAX_DELAY);
                        byte <<= 1;
                    }
                }
                LCD_CS_HIGH();
                return;
            }
        }
    } else { /* 16 */
        num = sizeof(tfont16) / sizeof(typFNT_GB16);
        for (uint16_t k = 0; k < num; k++) {
            if (tfont16[k].Index[0] == s[0] && tfont16[k].Index[1] == s[1]) {
                pFont = tfont16[k].Msk;
                LCD_SetWindow(x, y, x + 15, y + 15);
                LCD_DC_HIGH(); LCD_CS_LOW();
                for (uint8_t i = 0; i < 16 * 2; i++) {
                    uint8_t byte = (uint8_t)pFont[i];
                    for (uint8_t j = 0; j < 8; j++) {
                        uint16_t color = (byte & 0x80) ? fc : bc;
                        uint8_t buf[2] = {(uint8_t)(color >> 8), (uint8_t)(color & 0xFF)};
                        HAL_SPI_Transmit(&hspi2, buf, 2, HAL_MAX_DELAY);
                        byte <<= 1;
                    }
                }
                LCD_CS_HIGH();
                return;
            }
        }
    }
}

/* ===================== 显示 BMP 图片（RGB565） ===================== */
/**
 * @param bmp  图片数据，格式：低字节在前的 RGB565
 */
void LCD_DrawBMP(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                 const uint8_t *bmp)
{
    LCD_SetWindow(x, y, x + w - 1, y + h - 1);
    LCD_DC_HIGH();
    LCD_CS_LOW();
    for (uint32_t i = 0; i < (uint32_t)w * h; i++) {
        /* BMP 数据低字节在前，需要交换 */
        uint8_t buf[2] = {bmp[i * 2 + 1], bmp[i * 2]};
        HAL_SPI_Transmit(&hspi2, buf, 2, HAL_MAX_DELAY);
    }
    LCD_CS_HIGH();
    /* 恢复全屏窗口 */
    LCD_SetWindow(0, 0, lcddev.width - 1, lcddev.height - 1);
}
