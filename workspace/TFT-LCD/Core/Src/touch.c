/**
 * @file    touch.c
 * @brief   XPT2046/TSC2046 电阻触摸屏驱动实现（软件SPI，无EEPROM）
 *
 * 校准数据存储在 RAM 中，每次上电会弹出校准界面。
 * 校准完成后在本次运行内持续有效。
 */

#include "touch.h"
#include "lcd.h"
#include <stdlib.h>
#include <math.h>

/* ===================== 全局变量 ===================== */
TP_Dev_t tp_dev = {0};

/* ===================== GPIO 初始化 ===================== */
void TP_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};

    /* 输出引脚：CLK、DIN、CS */
    gpio.Pin   = TP_CLK_PIN | TP_DIN_PIN | TP_CS_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOC, &gpio);

    /* 输入引脚：DOUT、IRQ */
    gpio.Pin  = TP_DOUT_PIN | TP_IRQ_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOC, &gpio);

    /* 初始状态 */
    TP_CS_HIGH();
    TP_CLK_LOW();
    TP_DIN_LOW();

    /* 清除校准标志，触发校准 */
    tp_dev.adj_ok = 0;
}

/* ===================== 软件 SPI：发送一字节 ===================== */
/**
 * @brief 向 XPT2046 DIN 发送一字节，MSB 先发
 */
void TP_Write_Byte(uint8_t byte)
{
    for (uint8_t i = 0; i < 8; i++) {
        if (byte & 0x80) TP_DIN_HIGH();
        else             TP_DIN_LOW();
        byte <<= 1;
        TP_CLK_LOW();
        TP_CLK_HIGH();
    }
}

/* ===================== 读取 12 位 ADC 原始值 ===================== */
/**
 * @brief  发送控制字后读取 12 位结果（24 时钟模式）
 * @param  cmd  控制字节（如 TP_CMD_READ_X）
 * @return 12 位 ADC 值
 */
uint16_t TP_Read_AD(uint8_t cmd)
{
    uint16_t result = 0;

    TP_CS_LOW();
    TP_Write_Byte(cmd);          /* 发送控制字（8 clock） */

    /* 读高 8 位（第 9~16 clock，其中第1个为忙等） */
    for (uint8_t i = 0; i < 16; i++) {
        result <<= 1;
        TP_CLK_HIGH();
        TP_CLK_LOW();
        if (TP_DOUT_READ()) result |= 1;
    }
    result >>= 4;  /* 12 位结果，右移4位丢弃低位无效数据 */
    TP_CS_HIGH();

    return result & 0x0FFF;
}

/* ===================== 滤波读取（去极值取均值） ===================== */
/**
 * @brief  采样 TP_SAMPLE_NUM 次，去掉最大/最小值，取均值
 */
static uint16_t TP_Read_Filtered(uint8_t cmd)
{
    uint16_t buf[TP_SAMPLE_NUM];
    uint32_t sum = 0;
    uint16_t vmax = 0, vmin = 0xFFFF;

    for (uint8_t i = 0; i < TP_SAMPLE_NUM; i++) {
        buf[i] = TP_Read_AD(cmd);
        if (buf[i] > vmax) vmax = buf[i];
        if (buf[i] < vmin) vmin = buf[i];
        sum += buf[i];
    }
    sum -= vmax;
    sum -= vmin;
    return (uint16_t)(sum / (TP_SAMPLE_NUM - 2));
}

/* ===================== 读取原始 X/Y 坐标 ===================== */
/**
 * @return 1=成功，0=未触摸
 */
uint8_t TP_Read_XY(uint16_t *x, uint16_t *y)
{
    if (TP_IRQ_READ()) return 0; /* 无触摸 */

    *x = TP_Read_Filtered(TP_CMD_READ_X);
    *y = TP_Read_Filtered(TP_CMD_READ_Y);
    return 1;
}

/* ===================== 坐标转换（应用校准参数） ===================== */
static void TP_ConvertXY(uint16_t raw_x, uint16_t raw_y,
                         uint16_t *scr_x, uint16_t *scr_y)
{
    int32_t tx = (int32_t)(tp_dev.xfac * raw_x) + tp_dev.xoff;
    int32_t ty = (int32_t)(tp_dev.yfac * raw_y) + tp_dev.yoff;

    if (tx < 0) tx = 0;
    if (ty < 0) ty = 0;
    if (tx >= lcddev.width)  tx = lcddev.width  - 1;
    if (ty >= lcddev.height) ty = lcddev.height - 1;

    *scr_x = (uint16_t)tx;
    *scr_y = (uint16_t)ty;
}

/* ===================== 触摸扫描 ===================== */
/**
 * @brief  检测当前触摸状态，更新 tp_dev.x / tp_dev.y / tp_dev.sta
 * @param  mode  0=正常（有状态变化时更新），1=强制读取
 * @return sta 中 TP_PRES_DOWN 位
 */
uint8_t TP_Scan(uint8_t mode)
{
    uint16_t raw_x, raw_y;

    if (TP_IRQ_READ() == GPIO_PIN_RESET) {
        /* 检测到触摸 */
        if (TP_Read_XY(&raw_x, &raw_y)) {
            tp_dev.x_raw = raw_x;
            tp_dev.y_raw = raw_y;

            if (tp_dev.adj_ok) {
                TP_ConvertXY(raw_x, raw_y, &tp_dev.x, &tp_dev.y);
            } else {
                /* 未校准时直接映射 */
                tp_dev.x = (uint16_t)((float)raw_x * lcddev.width  / 4096.0f);
                tp_dev.y = (uint16_t)((float)raw_y * lcddev.height / 4096.0f);
            }

            if (!(tp_dev.sta & TP_PRES_DOWN)) {
                /* 刚按下 */
                tp_dev.sta |= TP_PRES_DOWN | TP_CATH_PRES;
            }
        }
    } else {
        /* 无触摸 */
        if (tp_dev.sta & TP_PRES_DOWN) {
            tp_dev.sta &= ~TP_PRES_DOWN; /* 清除按下标志 */
        }
    }

    return tp_dev.sta & TP_PRES_DOWN;
}

uint8_t TP_IsTouched(void)
{
    return (TP_IRQ_READ() == GPIO_PIN_RESET);
}

/* ===================== 校准辅助：绘制十字准心 ===================== */
void TP_Draw_Cross(uint16_t x, uint16_t y, uint16_t color)
{
    LCD_DrawLine(x - 12, y,  x + 12, y);   /* 水平线 */
    LCD_DrawLine(x,  y - 12, x,  y + 12);  /* 垂直线 */
    LCD_DrawPoint_Color(x, y, color);
    LCD_DrawPoint_Color(x + 1, y, color);
    LCD_DrawPoint_Color(x - 1, y, color);
    LCD_DrawPoint_Color(x, y + 1, color);
    LCD_DrawPoint_Color(x, y - 1, color);
}

/* ===================== 画大点（触摸画板用） ===================== */
void TP_Draw_Big_Point(uint16_t x, uint16_t y, uint16_t color)
{
    uint16_t save = POINT_COLOR;
    POINT_COLOR = color;
    LCD_Fill(x - 2, y - 2, x + 2, y + 2, color);
    POINT_COLOR = save;
}

/* ===================== 等待稳定触摸（校准用） ===================== */
static uint8_t TP_Wait_Press(uint16_t *x, uint16_t *y, uint32_t timeout_ms)
{
    uint32_t tick = HAL_GetTick();
    while (HAL_GetTick() - tick < timeout_ms) {
        if (TP_IRQ_READ() == GPIO_PIN_RESET) {
            HAL_Delay(20); /* 去抖 */
            if (TP_IRQ_READ() == GPIO_PIN_RESET) {
                if (TP_Read_XY(x, y)) {
                    /* 等待松开 */
                    while (TP_IRQ_READ() == GPIO_PIN_RESET) HAL_Delay(5);
                    HAL_Delay(20);
                    return 1;
                }
            }
        }
        HAL_Delay(5);
    }
    return 0;
}

/* ===================== 四点触摸校准 ===================== */
/**
 * @brief  在屏幕四个角落显示十字，引导用户依次点击，计算校准参数。
 *         校准参数保存在 tp_dev.xfac/yfac/xoff/yoff，本次上电内有效。
 */
void TP_Adjust(void)
{
    uint16_t px[4], py[4]; /* 四个校准点的原始ADC值 */
    uint16_t x_margin = 20, y_margin = 20;

    /* 校准点屏幕坐标（四角） */
    uint16_t cx[4] = {x_margin,
                      (uint16_t)(lcddev.width  - x_margin),
                      x_margin,
                      (uint16_t)(lcddev.width  - x_margin)};
    uint16_t cy[4] = {y_margin,
                      y_margin,
                      (uint16_t)(lcddev.height - y_margin),
                      (uint16_t)(lcddev.height - y_margin)};

    /* 界面提示 */
    LCD_Clear(WHITE);
    uint16_t save_fc = POINT_COLOR, save_bc = BACK_COLOR;
    POINT_COLOR = BLACK; BACK_COLOR = WHITE;
    LCD_ShowString(10, lcddev.height / 2 - 32, 16,
                   "Touch Calibration", 0);
    LCD_ShowString(10, lcddev.height / 2 - 12, 16,
                   "Please click the cross", 0);
    POINT_COLOR = save_fc; BACK_COLOR = save_bc;

    for (uint8_t i = 0; i < 4; i++) {
        /* 擦除上一个十字 */
        if (i > 0) TP_Draw_Cross(cx[i-1], cy[i-1], WHITE);

        TP_Draw_Cross(cx[i], cy[i], RED);

        if (!TP_Wait_Press(&px[i], &py[i], 10000)) {
            /* 超时：使用默认值 */
            tp_dev.xfac = (float)lcddev.width  / 4096.0f;
            tp_dev.yfac = (float)lcddev.height / 4096.0f;
            tp_dev.xoff = 0;
            tp_dev.yoff = 0;
            tp_dev.adj_ok = 1;
            LCD_Clear(WHITE);
            return;
        }
    }

    /* 计算X方向校准因子和偏移 */
    /* 使用 点0(左上) 和 点1(右上)，以及 点2(左下) 和 点3(右下) */
    float xfac1 = (float)(cx[1] - cx[0]) / (float)((int16_t)px[1] - (int16_t)px[0]);
    float xfac2 = (float)(cx[3] - cx[2]) / (float)((int16_t)px[3] - (int16_t)px[2]);
    tp_dev.xfac = (xfac1 + xfac2) / 2.0f;

    float xoff1 = cx[0] - tp_dev.xfac * px[0];
    float xoff2 = cx[2] - tp_dev.xfac * px[2];
    tp_dev.xoff = (int16_t)((xoff1 + xoff2) / 2.0f);

    /* 计算Y方向校准因子和偏移 */
    /* 使用 点0(左上) 和 点2(左下)，以及 点1(右上) 和 点3(右下) */
    float yfac1 = (float)(cy[2] - cy[0]) / (float)((int16_t)py[2] - (int16_t)py[0]);
    float yfac2 = (float)(cy[3] - cy[1]) / (float)((int16_t)py[3] - (int16_t)py[1]);
    tp_dev.yfac = (yfac1 + yfac2) / 2.0f;

    float yoff1 = cy[0] - tp_dev.yfac * py[0];
    float yoff2 = cy[1] - tp_dev.yfac * py[1];
    tp_dev.yoff = (int16_t)((yoff1 + yoff2) / 2.0f);

    /* 校准质量检查：因子应在合理范围内 */
    if (fabsf(tp_dev.xfac) > 2.0f || fabsf(tp_dev.yfac) > 2.0f) {
        /* 校准失败，使用默认值 */
        tp_dev.xfac = (float)lcddev.width  / 4096.0f;
        tp_dev.yfac = (float)lcddev.height / 4096.0f;
        tp_dev.xoff = 0;
        tp_dev.yoff = 0;
    }

    tp_dev.adj_ok = 1;

    /* 显示校准完成 */
    LCD_Clear(WHITE);
    POINT_COLOR = GREEN; BACK_COLOR = WHITE;
    LCD_ShowString(lcddev.width / 2 - 60, lcddev.height / 2 - 8, 16,
                   "Calibration OK!", 0);
    POINT_COLOR = save_fc; BACK_COLOR = save_bc;
    HAL_Delay(1000);
    LCD_Clear(WHITE);
}
