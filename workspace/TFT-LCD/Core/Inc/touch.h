/**
 * @file    touch.h
 * @brief   XPT2046/TSC2046 电阻触摸屏驱动头文件（软件SPI）
 *
 * 引脚定义（按屏幕排针物理顺序，直排连接无需交叉）：
 *   T_IRQ  -> PA3  触摸中断（低有效）
 *   T_DO   -> PA4  软件SPI MISO（数据输出）
 *   T_DIN  -> PA5  软件SPI MOSI（数据输入）
 *   T_CS   -> PA6  片选（低有效）
 *   T_CLK  -> PA7  软件SPI时钟
 */

#ifndef __TOUCH_H
#define __TOUCH_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

/* ===================== 引脚定义 ===================== */
#define TP_CLK_GPIO_PORT    GPIOA
#define TP_CLK_PIN          GPIO_PIN_7   /* PA7  SPI时钟 */

#define TP_IRQ_GPIO_PORT    GPIOA
#define TP_IRQ_PIN          GPIO_PIN_3   /* PA3  触摸中断 */

#define TP_DOUT_GPIO_PORT   GPIOA
#define TP_DOUT_PIN         GPIO_PIN_4   /* PA4  MISO */

#define TP_DIN_GPIO_PORT    GPIOA
#define TP_DIN_PIN          GPIO_PIN_5   /* PA5  MOSI */

#define TP_CS_GPIO_PORT     GPIOA
#define TP_CS_PIN           GPIO_PIN_6   /* PA6  片选 */

/* ===================== GPIO 操作宏 ===================== */
#define TP_CS_LOW()   HAL_GPIO_WritePin(TP_CS_GPIO_PORT,  TP_CS_PIN,  GPIO_PIN_RESET)
#define TP_CS_HIGH()  HAL_GPIO_WritePin(TP_CS_GPIO_PORT,  TP_CS_PIN,  GPIO_PIN_SET)
#define TP_CLK_LOW()  HAL_GPIO_WritePin(TP_CLK_GPIO_PORT, TP_CLK_PIN, GPIO_PIN_RESET)
#define TP_CLK_HIGH() HAL_GPIO_WritePin(TP_CLK_GPIO_PORT, TP_CLK_PIN, GPIO_PIN_SET)
#define TP_DIN_LOW()  HAL_GPIO_WritePin(TP_DIN_GPIO_PORT, TP_DIN_PIN, GPIO_PIN_RESET)
#define TP_DIN_HIGH() HAL_GPIO_WritePin(TP_DIN_GPIO_PORT, TP_DIN_PIN, GPIO_PIN_SET)
#define TP_DOUT_READ() HAL_GPIO_ReadPin(TP_DOUT_GPIO_PORT, TP_DOUT_PIN)
#define TP_IRQ_READ()  HAL_GPIO_ReadPin(TP_IRQ_GPIO_PORT,  TP_IRQ_PIN)

/* ===================== XPT2046 控制字 ===================== */
#define TP_CMD_READ_X   0xD0  /* 读X坐标：S=1, A=101, MODE=12bit, DFR, PD=00 */
#define TP_CMD_READ_Y   0x90  /* 读Y坐标：S=1, A=001, MODE=12bit, DFR, PD=00 */
#define TP_CMD_READ_Z1  0xB0  /* 读Z1压力 */
#define TP_CMD_READ_Z2  0xC0  /* 读Z2压力 */

/* ===================== 状态标志 ===================== */
#define TP_PRES_DOWN    0x80  /* 触摸屏被按下 */
#define TP_CATH_PRES    0x40  /* 捕获到按下动作 */

/* 采样次数（滤波用） */
#define TP_SAMPLE_NUM   5

/* ===================== 触摸设备结构体 ===================== */
typedef struct {
    uint16_t x;         /* 当前触摸X坐标（屏幕坐标） */
    uint16_t y;         /* 当前触摸Y坐标（屏幕坐标） */
    uint16_t x_raw;     /* 原始ADC X值 */
    uint16_t y_raw;     /* 原始ADC Y值 */
    uint8_t  sta;       /* 触摸状态：bit7=是否按下，bit6=是否捕获到按下 */

    /* 校准参数 */
    float    xfac;      /* X轴校准因子 */
    float    yfac;      /* Y轴校准因子 */
    int16_t  xoff;      /* X轴偏移 */
    int16_t  yoff;      /* Y轴偏移 */
    uint8_t  adj_ok;    /* 是否已完成校准 */
} TP_Dev_t;

extern TP_Dev_t tp_dev;

/* ===================== 函数声明 ===================== */
void    TP_Init(void);
uint8_t TP_Scan(uint8_t mode);
uint8_t TP_IsTouched(void);
void    TP_Adjust(void);                     /* 四点触摸校准 */
void    TP_Draw_Cross(uint16_t x, uint16_t y, uint16_t color); /* 绘制校准十字 */
void    TP_Draw_Big_Point(uint16_t x, uint16_t y, uint16_t color);

/* 内部函数（外部可访问以便调试） */
void     TP_Write_Byte(uint8_t byte);
uint16_t TP_Read_AD(uint8_t cmd);
uint8_t  TP_Read_XY(uint16_t *x, uint16_t *y);

#endif /* __TOUCH_H */
