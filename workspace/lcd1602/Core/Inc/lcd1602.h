#ifndef __LCD1602_H
#define __LCD1602_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

/* -----------------------------------------------------------------------
 * 引脚定义（4位模式，使用 PA0~PA5，避开 PB3~PB9）
 *
 *  LCD引脚  符号   STM32引脚
 *   4       RS    PA0
 *   5       RW    接GND（只写）
 *   6       E     PA1
 *  11       D4    PA2
 *  12       D5    PA3
 *  13       D6    PA4
 *  14       D7    PA5
 * ----------------------------------------------------------------------- */
#define LCD_PORT        GPIOA
#define LCD_RS_PIN      GPIO_PIN_0
#define LCD_E_PIN       GPIO_PIN_1
#define LCD_D4_PIN      GPIO_PIN_2
#define LCD_D5_PIN      GPIO_PIN_3
#define LCD_D6_PIN      GPIO_PIN_4
#define LCD_D7_PIN      GPIO_PIN_5

/* 所有 LCD 用到的引脚掩码，便于批量操作 */
#define LCD_ALL_PINS    (LCD_RS_PIN | LCD_E_PIN | \
                         LCD_D4_PIN | LCD_D5_PIN | LCD_D6_PIN | LCD_D7_PIN)

/* -----------------------------------------------------------------------
 * 公共接口
 * ----------------------------------------------------------------------- */

/**
 * @brief  初始化 LCD1602（4位模式，2行，5x8字体）
 */
void LCD_Init(void);

/**
 * @brief  清屏并将光标归位到 (0, 0)
 */
void LCD_Clear(void);

/**
 * @brief  设置光标位置
 * @param  row  行号：0 = 第一行，1 = 第二行
 * @param  col  列号：0~15
 */
void LCD_SetCursor(uint8_t row, uint8_t col);

/**
 * @brief  在当前光标位置写入一个字符
 * @param  c  ASCII 字符
 */
void LCD_WriteChar(char c);

/**
 * @brief  在当前光标位置写入字符串
 * @param  str  以 '\0' 结尾的字符串
 */
void LCD_WriteString(const char *str);

#endif /* __LCD1602_H */
