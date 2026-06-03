/**
 ******************************************************************************
 * @file    lcd1602.c
 * @brief   LCD1602 驱动（4位模式）
 *
 * 接线（只写模式，RW 接 GND）：
 *
 *   编号  符号   接到
 *   ───────────────────────────────────────
 *    1    VSS   GND（电源地）
 *    2    VDD   3.3V（电源正）
 *    3    V0    GND（对比度，如太深可改接电位器）
 *    4    RS    PA0
 *    5    RW    GND（只写，不需接 STM32）
 *    6    E     PA1
 *    7~10 D0~D3 不接（4位模式不使用）
 *    11   D4    PA2
 *    12   D5    PA3
 *    13   D6    PA4
 *    14   D7    PA5
 *    15   A     3.3V（背光 LED 阳极）
 *    16   K     GND（背光 LED 阴极）
 ******************************************************************************
 */

#include "lcd1602.h"
#include "delay.h"

/* -----------------------------------------------------------------------
 * 底层 GPIO 操作
 * ----------------------------------------------------------------------- */

/** 将 D4~D7 四根数据线按 nibble 输出（高4位） */
static void lcd_write_nibble(uint8_t nibble)
{
    /* 先清除 D4~D7 对应引脚 */
    HAL_GPIO_WritePin(LCD_PORT, LCD_D4_PIN | LCD_D5_PIN | LCD_D6_PIN | LCD_D7_PIN,
                      GPIO_PIN_RESET);

    if (nibble & 0x10) HAL_GPIO_WritePin(LCD_PORT, LCD_D4_PIN, GPIO_PIN_SET);
    if (nibble & 0x20) HAL_GPIO_WritePin(LCD_PORT, LCD_D5_PIN, GPIO_PIN_SET);
    if (nibble & 0x40) HAL_GPIO_WritePin(LCD_PORT, LCD_D6_PIN, GPIO_PIN_SET);
    if (nibble & 0x80) HAL_GPIO_WritePin(LCD_PORT, LCD_D7_PIN, GPIO_PIN_SET);

    /* 产生 E 使能脉冲（宽度 > 450ns，取 2us 安全裕量） */
    HAL_GPIO_WritePin(LCD_PORT, LCD_E_PIN, GPIO_PIN_SET);
    Delay_us(2);
    HAL_GPIO_WritePin(LCD_PORT, LCD_E_PIN, GPIO_PIN_RESET);
    Delay_us(2);
}

/**
 * @brief  向 LCD 发送一个字节（分两次 nibble 发送）
 * @param  byte  要发送的数据
 * @param  rs    0 = 指令，1 = 数据
 */
static void lcd_send_byte(uint8_t byte, uint8_t rs)
{
    HAL_GPIO_WritePin(LCD_PORT, LCD_RS_PIN,
                      rs ? GPIO_PIN_SET : GPIO_PIN_RESET);

    /* 先发高4位 */
    lcd_write_nibble(byte & 0xF0);
    /* 再发低4位（左移4位对齐到高位） */
    lcd_write_nibble((uint8_t)(byte << 4));

    /* 等待 LCD 执行完毕（普通指令 37us，清屏/归位 1.52ms） */
    Delay_us(50);
}

/* -----------------------------------------------------------------------
 * 公共接口实现
 * ----------------------------------------------------------------------- */

void LCD_Init(void)
{
    /* 初始化 GPIO */
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();

    gpio.Pin   = LCD_ALL_PINS;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;   /* 推挽输出 */
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LCD_PORT, &gpio);

    /* 所有引脚初始为低电平 */
    HAL_GPIO_WritePin(LCD_PORT, LCD_ALL_PINS, GPIO_PIN_RESET);

    /* 上电等待 LCD 稳定（> 40ms） */
    Delay_ms(50);

    /* --- 按 HD44780 数据手册规定的 4位初始化序列 --- */

    /* 第1次：发送 0x30（8位模式指令），等待 > 4.1ms */
    HAL_GPIO_WritePin(LCD_PORT, LCD_RS_PIN, GPIO_PIN_RESET);
    lcd_write_nibble(0x30);
    Delay_ms(5);

    /* 第2次：发送 0x30，等待 > 100us */
    lcd_write_nibble(0x30);
    Delay_us(150);

    /* 第3次：发送 0x30 */
    lcd_write_nibble(0x30);
    Delay_us(150);

    /* 第4次：发送 0x20 → 切换到 4位模式 */
    lcd_write_nibble(0x20);
    Delay_us(150);

    /* 功能设置：4位模式，2行，5x8字体  0x28 */
    lcd_send_byte(0x28, 0);

    /* 关闭显示 0x08 */
    lcd_send_byte(0x08, 0);

    /* 清屏 0x01（需要等待约 1.52ms，lcd_send_byte 内 50us 不够，额外等） */
    lcd_send_byte(0x01, 0);
    Delay_ms(2);

    /* 进入模式设置：光标右移，屏幕不移动  0x06 */
    lcd_send_byte(0x06, 0);

    /* 打开显示，光标不显示，不闪烁  0x0C */
    lcd_send_byte(0x0C, 0);
}

void LCD_Clear(void)
{
    lcd_send_byte(0x01, 0);
    Delay_ms(2);
}

void LCD_SetCursor(uint8_t row, uint8_t col)
{
    /* HD44780 DDRAM 地址：第一行从 0x00，第二行从 0x40 */
    uint8_t addr = (row == 0) ? (0x00 + col) : (0x40 + col);
    lcd_send_byte((uint8_t)(0x80 | addr), 0);
}

void LCD_WriteChar(char c)
{
    lcd_send_byte((uint8_t)c, 1);
}

void LCD_WriteString(const char *str)
{
    while (*str)
    {
        LCD_WriteChar(*str++);
    }
}
