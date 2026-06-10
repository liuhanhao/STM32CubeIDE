/**
 * @file    touch.c
 * @brief   XPT2046/TSC2046 电阻触摸屏驱动实现（软件SPI，参考官方例程改写）
 *
 * 接线（PA组引脚）：
 *   T_CLK -> PA7
 *   T_CS  -> PA6
 *   T_DIN -> PA5
 *   T_DO  -> PA4
 *   T_IRQ -> PA3
 */

#include "touch.h"
#include "lcd.h"
#include <stdlib.h>
#include <math.h>

/* ===================== 全局变量 ===================== */
TP_Dev_t tp_dev = {0};

/* XPT2046 读取命令 - 参考官方例程（静态变量，可动态调整方向） */
static uint8_t CMD_RDX = TP_CMD_RDX;  /* 读X坐标 */
static uint8_t CMD_RDY = TP_CMD_RDY;  /* 读Y坐标 */

/* ===================== 软件延时（微秒级） ===================== */
/**
 * @brief 微秒级延时（粗略）
 */
static void delay_us(uint16_t us)
{
    /* 72MHz下，9次循环约1μs */
    for (volatile uint32_t i = 0; i < us * 9; i++);
}

/* ===================== GPIO 初始化 ===================== */
void TP_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};

    /* 输出引脚：CLK(PA7)、DIN(PA5)、CS(PA6) */
    gpio.Pin   = TP_CLK_PIN | TP_DIN_PIN | TP_CS_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    /* 输入引脚：IRQ(PA3)、DOUT(PA4) - 使用上拉 */
    gpio.Pin  = TP_IRQ_PIN | TP_DOUT_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &gpio);

    /* 读取一次初始化触摸芯片 */
    TP_Read_XY(&tp_dev.x_raw, &tp_dev.y_raw);

    /* 清除校准标志 - 不在TP_Init中执行校准 */
    tp_dev.adj_ok = 0;
}

/* ===================== 软件 SPI：发送一字节 ===================== */
/**
 * @brief 向 XPT2046 发送一字节（参考官方例程实现）
 */
void TP_Write_Byte(uint8_t byte)
{
    uint8_t count = 0;
    for (count = 0; count < 8; count++) {
        if (byte & 0x80) TP_DIN_HIGH();
        else             TP_DIN_LOW();
        byte <<= 1;
        TP_CLK_LOW();
        TP_CLK_HIGH();  /* 上升沿有效 */
    }
}

/* ===================== 读取 12 位 ADC 原始值 ===================== */
/**
 * @brief 从触摸IC读取ADC值（完全参考官方例程）
 */
uint16_t TP_Read_AD(uint8_t cmd)
{
    uint8_t count = 0;
    uint16_t num = 0;

    TP_CLK_LOW();    /* 先拉低时钟 */
    TP_DIN_LOW();    /* 拉低数据线 */
    TP_CS_LOW();     /* 选中触摸IC */

    TP_Write_Byte(cmd); /* 发送控制命令 */

    delay_us(6);     /* ADS7846转换时间为6us */

    TP_CLK_LOW();
    delay_us(1);
    TP_CLK_HIGH();   /* 给1个时钟，清除BUSY */
    TP_CLK_LOW();

    for (count = 0; count < 16; count++) {  /* 读取16位数据，只有高12位有效 */
        num <<= 1;
        TP_CLK_LOW();   /* 下降沿有效 */
        TP_CLK_HIGH();
        if (TP_DOUT_READ()) num++;
    }

    num >>= 4;       /* 只有高12位有效 */
    TP_CS_HIGH();    /* 释放片选 */

    return num;
}

/* ===================== 滤波读取（参考官方例程） ===================== */
#define READ_TIMES 5   /* 读取次数 */
#define LOST_VAL   1   /* 丢弃值 */

/**
 * @brief  读取一个坐标值(x或y)，进行滤波处理
 *         读取READ_TIMES次数据，去掉最大最小值后取平均
 */
static uint16_t TP_Read_XOY(uint8_t xy)
{
    uint16_t i, j;
    uint16_t buf[READ_TIMES];
    uint16_t sum = 0;
    uint16_t temp;

    for (i = 0; i < READ_TIMES; i++) {
        buf[i] = TP_Read_AD(xy);
    }

    /* 排序 */
    for (i = 0; i < READ_TIMES - 1; i++) {
        for (j = i + 1; j < READ_TIMES; j++) {
            if (buf[i] > buf[j]) {  /* 升序排列 */
                temp = buf[i];
                buf[i] = buf[j];
                buf[j] = temp;
            }
        }
    }

    sum = 0;
    for (i = LOST_VAL; i < READ_TIMES - LOST_VAL; i++) {
        sum += buf[i];
    }

    temp = sum / (READ_TIMES - 2 * LOST_VAL);
    return temp;
}

/* ===================== 读取原始 X/Y 坐标 ===================== */
/**
 * @brief  读取x,y坐标（参考官方例程）
 * @return 0=失败, 1=成功
 */
uint8_t TP_Read_XY(uint16_t *x, uint16_t *y)
{
    uint16_t xtemp, ytemp;

    xtemp = TP_Read_XOY(CMD_RDX);
    ytemp = TP_Read_XOY(CMD_RDY);

    /* 调试输出：观察ADC原始值 */
    // 如果需要调试，可以在这里添加串口输出或LCD显示

    *x = xtemp;
    *y = ytemp;
    return 1;  /* 读数成功 */
}

/* ===================== 连续2次读取，验证数据可靠性 ===================== */
#define ERR_RANGE 50  /* 误差范围 */

/**
 * @brief  连续2次读取触摸IC，过滤掉偏差超出ERR_RANGE的数据
 * @return 0=失败, 1=成功
 */
static uint8_t TP_Read_XY2(uint16_t *x, uint16_t *y)
{
    uint16_t x1, y1;
    uint16_t x2, y2;
    uint8_t flag;

    flag = TP_Read_XY(&x1, &y1);
    if (flag == 0) return 0;

    flag = TP_Read_XY(&x2, &y2);
    if (flag == 0) return 0;

    /* 判断两次读数的偏差是否在ERR_RANGE内 */
    if (((x2 <= x1 && x1 < x2 + ERR_RANGE) || (x1 <= x2 && x2 < x1 + ERR_RANGE)) &&
        ((y2 <= y1 && y1 < y2 + ERR_RANGE) || (y1 <= y2 && y2 < y1 + ERR_RANGE))) {
        *x = (x1 + x2) / 2;
        *y = (y1 + y2) / 2;
        return 1;
    } else {
        return 0;
    }
}

/* ===================== 辅助绘图函数 ===================== */
/**
 * @brief 画一个触摸点（校准用十字）
 */
void TP_Draw_Touch_Point(uint16_t x, uint16_t y, uint16_t color)
{
    uint16_t save_color = POINT_COLOR;
    POINT_COLOR = color;
    LCD_DrawLine(x - 12, y, x + 13, y);    /* 横线 */
    LCD_DrawLine(x, y - 12, x, y + 13);    /* 竖线 */
    LCD_DrawPoint_Color(x + 1, y + 1, color);
    LCD_DrawPoint_Color(x - 1, y + 1, color);
    LCD_DrawPoint_Color(x + 1, y - 1, color);
    LCD_DrawPoint_Color(x - 1, y - 1, color);
    POINT_COLOR = save_color;
}

/**
 * @brief 画一个大点（2×2的点）
 */
void TP_Draw_Big_Point(uint16_t x, uint16_t y, uint16_t color)
{
    uint16_t save_color = POINT_COLOR;
    POINT_COLOR = color;
    LCD_DrawPoint_Color(x, y, color);
    LCD_DrawPoint_Color(x + 1, y, color);
    LCD_DrawPoint_Color(x, y + 1, color);
    LCD_DrawPoint_Color(x + 1, y + 1, color);
    POINT_COLOR = save_color;
}

/* ===================== 触摸扫描（参考官方例程） ===================== */
/**
 * @brief  触摸屏扫描
 * @param  tp  0=屏幕坐标; 1=物理坐标(校准和画图场合用)
 * @return 当前触摸状态
 */
uint8_t TP_Scan(uint8_t tp)
{
    if (TP_IRQ_READ() == GPIO_PIN_RESET) {  /* 有按键按下 */
        if (tp) {
            TP_Read_XY2(&tp_dev.x, &tp_dev.y);  /* 读取物理坐标 */
        } else if (TP_Read_XY2(&tp_dev.x, &tp_dev.y)) {  /* 读取屏幕坐标 */
            tp_dev.x = (uint16_t)(tp_dev.xfac * tp_dev.x + tp_dev.xoff); /* 转换为屏幕坐标 */
            tp_dev.y = (uint16_t)(tp_dev.yfac * tp_dev.y + tp_dev.yoff);
        }

        if ((tp_dev.sta & TP_PRES_DOWN) == 0) {  /* 之前没有被按下 */
            tp_dev.sta = TP_PRES_DOWN | TP_CATH_PRES;  /* 按键按下 */
            tp_dev.x0 = tp_dev.x;  /* 记录第一次按下时的坐标 */
            tp_dev.y0 = tp_dev.y;
        }
    } else {
        if (tp_dev.sta & TP_PRES_DOWN) {  /* 之前是被按下的 */
            tp_dev.sta &= ~(1 << 7);      /* 标记按键松开 */
        } else {  /* 之前就没有被按下 */
            tp_dev.x0 = 0;
            tp_dev.y0 = 0;
            tp_dev.x = 0xffff;
            tp_dev.y = 0xffff;
        }
    }

    return tp_dev.sta & TP_PRES_DOWN;  /* 返回当前的触摸状态 */
}

uint8_t TP_IsTouched(void)
{
    return (TP_IRQ_READ() == GPIO_PIN_RESET);
}

/* ===================== 校准辅助：绘制十字准心 ===================== */
void TP_Draw_Cross(uint16_t x, uint16_t y, uint16_t color)
{
    TP_Draw_Touch_Point(x, y, color);
}

/* ===================== 四点触摸校准（参考官方例程简化版） ===================== */
/**
 * @brief  触摸屏校准函数（参考官方例程）
 */
void TP_Adjust(void)
{
    uint16_t pos_temp[4][2];  /* 坐标缓存值 */
    uint8_t cnt = 0;
    uint16_t d1, d2;
    uint32_t tem1, tem2;
    float fac;
    uint16_t outtime = 0;

    uint16_t save_pc = POINT_COLOR;
    uint16_t save_bc = BACK_COLOR;

    cnt = 0;
    POINT_COLOR = BLUE;
    BACK_COLOR = WHITE;
    LCD_Clear(WHITE);  /* 清屏 */

    POINT_COLOR = RED;   /* 红色 */
    LCD_Clear(WHITE);    /* 清屏 */

    POINT_COLOR = BLACK;
    LCD_ShowString(10, 40, 16, "Please use the stylus click the", 1);
    LCD_ShowString(10, 56, 16, "cross on the screen.The cross will", 1);
    LCD_ShowString(10, 72, 16, "always move until the screen ", 1);
    LCD_ShowString(10, 88, 16, "adjustment is completed.", 1);

    TP_Draw_Touch_Point(20, 20, RED);  /* 画点1 */

    tp_dev.sta = 0;      /* 消除触发信号 */
    tp_dev.xfac = 0;     /* xfac用来标记是否校准过 */

    while (1) {  /* 如果连续10秒钟没有按下，则自动退出 */
        TP_Scan(1);  /* 扫描物理坐标 */

        if ((tp_dev.sta & 0xc0) == TP_CATH_PRES) {  /* 按键按下了一次(此时按键松开了) */
            outtime = 0;
            tp_dev.sta &= ~(1 << 6);  /* 标记按键已经被处理过了 */

            pos_temp[cnt][0] = tp_dev.x;
            pos_temp[cnt][1] = tp_dev.y;
            cnt++;

            switch (cnt) {
                case 1:
                    TP_Draw_Touch_Point(20, 20, WHITE);                    /* 擦除点1 */
                    TP_Draw_Touch_Point(lcddev.width - 20, 20, RED);      /* 画点2 */
                    break;
                case 2:
                    TP_Draw_Touch_Point(lcddev.width - 20, 20, WHITE);    /* 擦除点2 */
                    TP_Draw_Touch_Point(20, lcddev.height - 20, RED);     /* 画点3 */
                    break;
                case 3:
                    TP_Draw_Touch_Point(20, lcddev.height - 20, WHITE);               /* 擦除点3 */
                    TP_Draw_Touch_Point(lcddev.width - 20, lcddev.height - 20, RED); /* 画点4 */
                    break;
                case 4:  /* 全部四个点已经得到 */
                    /* 对边测试 */
                    tem1 = abs(pos_temp[0][0] - pos_temp[1][0]);  /* x1-x2 */
                    tem2 = abs(pos_temp[0][1] - pos_temp[1][1]);  /* y1-y2 */
                    tem1 *= tem1;
                    tem2 *= tem2;
                    d1 = sqrt(tem1 + tem2);  /* 得到1,2的距离 */

                    tem1 = abs(pos_temp[2][0] - pos_temp[3][0]);  /* x3-x4 */
                    tem2 = abs(pos_temp[2][1] - pos_temp[3][1]);  /* y3-y4 */
                    tem1 *= tem1;
                    tem2 *= tem2;
                    d2 = sqrt(tem1 + tem2);  /* 得到3,4的距离 */

                    fac = (float)d1 / d2;
                    if (fac < 0.95 || fac > 1.05 || d1 == 0 || d2 == 0) {  /* 不合格 */
                        cnt = 0;
                        TP_Draw_Touch_Point(lcddev.width - 20, lcddev.height - 20, WHITE);  /* 擦除点4 */
                        TP_Draw_Touch_Point(20, 20, RED);                                    /* 画点1 */
                        continue;
                    }

                    tem1 = abs(pos_temp[0][0] - pos_temp[2][0]);  /* x1-x3 */
                    tem2 = abs(pos_temp[0][1] - pos_temp[2][1]);  /* y1-y3 */
                    tem1 *= tem1;
                    tem2 *= tem2;
                    d1 = sqrt(tem1 + tem2);  /* 得到1,3的距离 */

                    tem1 = abs(pos_temp[1][0] - pos_temp[3][0]);  /* x2-x4 */
                    tem2 = abs(pos_temp[1][1] - pos_temp[3][1]);  /* y2-y4 */
                    tem1 *= tem1;
                    tem2 *= tem2;
                    d2 = sqrt(tem1 + tem2);  /* 得到2,4的距离 */

                    fac = (float)d1 / d2;
                    if (fac < 0.95 || fac > 1.05) {  /* 不合格 */
                        cnt = 0;
                        TP_Draw_Touch_Point(lcddev.width - 20, lcddev.height - 20, WHITE);  /* 擦除点4 */
                        TP_Draw_Touch_Point(20, 20, RED);                                    /* 画点1 */
                        continue;
                    }

                    /* 对角测试 */
                    tem1 = abs(pos_temp[1][0] - pos_temp[2][0]);  /* x1-x3 */
                    tem2 = abs(pos_temp[1][1] - pos_temp[2][1]);  /* y1-y3 */
                    tem1 *= tem1;
                    tem2 *= tem2;
                    d1 = sqrt(tem1 + tem2);  /* 得到1,4的距离 */

                    tem1 = abs(pos_temp[0][0] - pos_temp[3][0]);  /* x2-x4 */
                    tem2 = abs(pos_temp[0][1] - pos_temp[3][1]);  /* y2-y4 */
                    tem1 *= tem1;
                    tem2 *= tem2;
                    d2 = sqrt(tem1 + tem2);  /* 得到2,3的距离 */

                    fac = (float)d1 / d2;
                    if (fac < 0.95 || fac > 1.05) {  /* 不合格 */
                        cnt = 0;
                        TP_Draw_Touch_Point(lcddev.width - 20, lcddev.height - 20, WHITE);  /* 擦除点4 */
                        TP_Draw_Touch_Point(20, 20, RED);                                    /* 画点1 */
                        continue;
                    }

                    /* 计算校准参数 */
                    tp_dev.xfac = (float)(lcddev.width - 40) / (pos_temp[1][0] - pos_temp[0][0]);   /* 得到xfac */
                    tp_dev.xoff = (lcddev.width - tp_dev.xfac * (pos_temp[1][0] + pos_temp[0][0])) / 2;  /* 得到xoff */

                    tp_dev.yfac = (float)(lcddev.height - 40) / (pos_temp[2][1] - pos_temp[0][1]);  /* 得到yfac */
                    tp_dev.yoff = (lcddev.height - tp_dev.yfac * (pos_temp[2][1] + pos_temp[0][1])) / 2;  /* 得到yoff */

                    if (fabs(tp_dev.xfac) > 2 || fabs(tp_dev.yfac) > 2) {  /* 触摸屏和预设的相反了 */
                        cnt = 0;
                        TP_Draw_Touch_Point(lcddev.width - 20, lcddev.height - 20, WHITE);  /* 擦除点4 */
                        TP_Draw_Touch_Point(20, 20, RED);                                    /* 画点1 */
                        LCD_ShowString(40, 26, 16, "TP Need readjust!", 1);
                        tp_dev.touchtype = !tp_dev.touchtype;  /* 修改触摸类型 */
                        if (tp_dev.touchtype) {  /* X,Y方向与屏幕相反 */
                            CMD_RDX = 0X90;
                            CMD_RDY = 0XD0;
                        } else {  /* X,Y方向与屏幕相同 */
                            CMD_RDX = 0XD0;
                            CMD_RDY = 0X90;
                        }
                        continue;
                    }

                    POINT_COLOR = BLUE;
                    LCD_Clear(WHITE);  /* 清屏 */
                    LCD_ShowString(35, 110, 16, "Touch Screen Adjust OK!", 1);  /* 校准完成 */
                    HAL_Delay(1000);
                    LCD_Clear(WHITE);  /* 清屏 */

                    POINT_COLOR = save_pc;
                    BACK_COLOR = save_bc;
                    return;  /* 校准完成 */
            }
        }

        HAL_Delay(10);
        outtime++;
        if (outtime > 1000) {
            break;
        }
    }

    POINT_COLOR = save_pc;
    BACK_COLOR = save_bc;
}
