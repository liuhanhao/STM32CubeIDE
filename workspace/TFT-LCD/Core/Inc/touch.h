/**
 * @file    touch.h
 * @brief   XPT2046/TSC2046 电阻触摸屏驱动头文件（软件SPI，参考官方例程）
 *
 * 接线（PA组引脚）：
 *   T_CLK -> PA7   SPI时钟
 *   T_CS  -> PA6   片选
 *   T_DIN -> PA5   MOSI（数据输入）
 *   T_DO  -> PA4   MISO（数据输出）
 *   T_IRQ -> PA3   触摸中断（低有效）
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

/* ===================== XPT2046 控制命令 ===================== */
#define TP_CMD_RDX      0xD0  /* 读X坐标 */
#define TP_CMD_RDY      0x90  /* 读Y坐标 */

/* ===================== 状态标志 ===================== */
#define TP_PRES_DOWN    0x80  /* 触摸屏被按下 */
#define TP_CATH_PRES    0x40  /* 捕获到按下动作 */

/* ===================== 触摸设备结构体 ===================== */
typedef struct {
    uint16_t x0;         /* 原始坐标（第一次按下时的坐标） */
    uint16_t y0;
    uint16_t x;          /* 当前触摸坐标（此次扫描时的坐标） */
    uint16_t y;
    uint16_t x_raw;      /* 原始ADC X值 */
    uint16_t y_raw;      /* 原始ADC Y值 */
    uint8_t  sta;        /* 触摸状态：
                          * b7: 按键1/松开0
                          * b6: 0没有按键按下; 1有按键按下 */

    /* 校准参数 */
    float    xfac;
    float    yfac;
    int16_t  xoff;
    int16_t  yoff;
    uint8_t  touchtype;  /* 触摸类型标记 */
    uint8_t  adj_ok;     /* 校准完成标志 */
} TP_Dev_t;

extern TP_Dev_t tp_dev;

/* ===================== 函数声明 ===================== */
void    TP_Init(void);
uint8_t TP_Scan(uint8_t tp);            /* tp: 0=屏幕坐标, 1=物理坐标 */
uint8_t TP_IsTouched(void);
void    TP_Adjust(void);                /* 四点触摸校准 */
void    TP_Draw_Cross(uint16_t x, uint16_t y, uint16_t color);
void    TP_Draw_Big_Point(uint16_t x, uint16_t y, uint16_t color);
void    TP_Draw_Touch_Point(uint16_t x, uint16_t y, uint16_t color);

/* 内部函数 */
void     TP_Write_Byte(uint8_t byte);
uint16_t TP_Read_AD(uint8_t cmd);
uint8_t  TP_Read_XY(uint16_t *x, uint16_t *y);

#endif /* __TOUCH_H */
