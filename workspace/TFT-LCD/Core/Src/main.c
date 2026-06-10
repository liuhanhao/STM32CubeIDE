/**
  ******************************************************************************
  * @file    main.c
  * @brief   TFT-LCD 触摸画板（参考官方例程修改版）
  *
  * 流程：上电 → LCD初始化 → 进入触摸测试
  *
  * ============================================================================
  *                           完整接线说明
  * ============================================================================
  *
  * 【供电部分】
  * ─────────────────────────────────────────────────────────────
  * LCD模块VCC   → STM32 3.3V引脚  (电源正极，必须接！)
  * LCD模块GND   → STM32 GND引脚   (电源地，必须接！)
  *
  * 【LCD显示部分 - SPI接口】
  * ─────────────────────────────────────────────────────────────
  * LCD_CS       → PB11  (LCD片选，低电平有效)
  * LCD_RST      → PB12  (LCD复位，低电平复位)
  * LCD_DC (RS)  → PB10  (数据/命令选择：1=数据，0=命令)
  * LCD_MOSI     → PB15  (SPI数据输入，主→从)
  * LCD_SCK      → PB13  (SPI时钟)
  * LCD_BL (LED) → PB9   (背光控制，高电平点亮)
  * LCD_MISO     → PB14  (SPI数据输出，从→主，可选不接)
  *
  * 【触摸屏部分 - 软件SPI】
  * ─────────────────────────────────────────────────────────────
  * 触摸模块VCC  → STM32 3.3V引脚  (触摸IC供电，必须接！)
  * 触摸模块GND  → STM32 GND引脚   (触摸IC地，必须接！)
  * T_IRQ        → PA3   (触摸中断，低电平表示有触摸)
  * T_DO (MISO)  → PA4   (触摸SPI数据输出，触摸IC→MCU)
  * T_DIN (MOSI) → PA5   (触摸SPI数据输入，MCU→触摸IC)
  * T_CS         → PA6   (触摸片选，低电平有效)
  * T_CLK        → PA7   (触摸SPI时钟)
  *
  * 【注意事项】
  * ─────────────────────────────────────────────────────────────
  * 1. VCC必须接3.3V，不能接5V！STM32和触摸IC都是3.3V系统
  * 2. LCD和触摸如果是一体模块，VCC/GND可以共用
  * 3. LCD的SPI和触摸的SPI是独立的，不共用总线
  * 4. 触摸使用软件模拟SPI，所以引脚可以任意GPIO
  * 5. 背光LED(BL)如果不需要控制，可以直接接3.3V常亮
  * 6. LCD_MISO如果不需要读取LCD数据，可以不接
  *
  * 【模块型号】
  * ─────────────────────────────────────────────────────────────
  * LCD控制器：  ILI9341（240×320分辨率，RGB565色彩）
  * 触摸芯片：   XPT2046/TSC2046/ADS7843（12位ADC，4线电阻式）
  * 开发板：     STM32F103C8T6（72MHz，蓝色药丸板或类似）
  *
  ******************************************************************************
  */
#include "main.h"
#include "lcd.h"
#include "touch.h"
#include <stdio.h>

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void Touch_Test(void);

/* ==================== 主函数 ==================== */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    LCD_Init();  /* LCD初始化 */

    /* 进入触摸测试（参考官方例程流程） */
    while (1) {
        Touch_Test();
    }
}

/* ==================== 触摸画板测试 ==================== */
static void Touch_Test(void)
{
    char debug_buf[80];
    uint32_t last_tick = 0;
    uint16_t pen_color = RED;
    uint8_t color_idx = 0;
    uint16_t colors[] = {RED, GREEN, BLUE, YELLOW, CYAN, MAGENTA, WHITE};

    /* 初始化触摸 */
    TP_Init();

    /* ========== 每次上电执行四点校准（学习阶段） ========== */
    /* Flash保存功能已禁用,每次启动都需要重新校准 */
    TP_Adjust();  /* 四点触摸校准 */
    
    /* ========== Flash保存代码（已注释，学习阶段避免频繁擦写） ========== */
    #if 0  /* 如需启用Flash保存，将0改为1 */
    /* Flash存储地址（STM32F103C8T6最后1KB） */
    #define FLASH_CALIBRATION_ADDR  0x0800FC00
    #define CALIBRATION_MAGIC       0x5AA5A55A
    
    typedef struct {
        uint32_t magic;
        float    xfac;
        float    yfac;
        int16_t  xoff;
        int16_t  yoff;
        uint8_t  touchtype;
        uint32_t checksum;
    } CalibData_t;
    
    /* 保存校准数据到Flash */
    HAL_FLASH_Unlock();
    
    FLASH_EraseInitTypeDef erase = {0};
    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.PageAddress = FLASH_CALIBRATION_ADDR;
    erase.NbPages = 1;
    uint32_t page_error = 0;
    HAL_FLASHEx_Erase(&erase, &page_error);
    
    CalibData_t calib_data;
    calib_data.magic = CALIBRATION_MAGIC;
    calib_data.xfac = tp_dev.xfac;
    calib_data.yfac = tp_dev.yfac;
    calib_data.xoff = tp_dev.xoff;
    calib_data.yoff = tp_dev.yoff;
    calib_data.touchtype = tp_dev.touchtype;
    calib_data.checksum = calib_data.magic + 
                          *(uint32_t*)&calib_data.xfac + 
                          *(uint32_t*)&calib_data.yfac + 
                          (uint32_t)calib_data.xoff + 
                          (uint32_t)calib_data.yoff;
    
    uint32_t *data_ptr = (uint32_t *)&calib_data;
    for (uint8_t i = 0; i < sizeof(CalibData_t) / 4; i++) {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, 
                         FLASH_CALIBRATION_ADDR + i * 4, 
                         data_ptr[i]);
    }
    
    HAL_FLASH_Lock();
    
    LCD_Clear(WHITE);
    POINT_COLOR = GREEN;
    LCD_ShowString(10, 140, 16, "Calibration Saved!", 0);
    HAL_Delay(800);
    #endif  /* Flash保存功能结束 */

    /* 绘制画板界面 */
    LCD_Clear(BLACK);
    
    /* 顶部标题栏 */
    LCD_Fill(0, 0, lcddev.width, 22, BLUE);
    POINT_COLOR = WHITE;
    BACK_COLOR = BLUE;
    LCD_ShowString(10, 4, 16, "Touch Draw Board", 0);
    
    /* 底部工具栏 */
    uint16_t bar_y = lcddev.height - 40;
    LCD_Fill(0, bar_y, lcddev.width, lcddev.height, DARKBLUE);
    
    /* 颜色按钮 */
    for (uint8_t i = 0; i < 7; i++) {
        uint16_t bx = 2 + i * 33;
        LCD_Fill(bx, bar_y + 3, bx + 28, bar_y + 20, colors[i]);
        if (i == 0) {
            POINT_COLOR = WHITE;
            LCD_DrawRectangle(bx - 1, bar_y + 2, bx + 29, bar_y + 21);
        }
    }
    
    /* CLEAR按钮 */
    LCD_Fill(lcddev.width - 50, bar_y + 3, lcddev.width - 3, bar_y + 20, RED);
    POINT_COLOR = WHITE;
    BACK_COLOR = RED;
    LCD_ShowString(lcddev.width - 46, bar_y + 8, 12, "CLR", 0);
    
    /* RECAL（重新校准）按钮 */
    LCD_Fill(lcddev.width - 105, bar_y + 3, lcddev.width - 55, bar_y + 20, BLUE);
    POINT_COLOR = WHITE;
    BACK_COLOR = BLUE;
    LCD_ShowString(lcddev.width - 98, bar_y + 8, 12, "CAL", 0);

    /* 绘图区 */
    uint16_t draw_top = 24;
    uint16_t draw_bottom = bar_y - 2;
    LCD_Fill(0, draw_top, lcddev.width, draw_bottom, WHITE);

    /* 主循环 */
    uint16_t last_x = 0xFFFF, last_y = 0xFFFF;  /* 上一次的坐标 */
    
    while (1) {
        /* 调用触摸扫描 - 更快的扫描速率 */
        TP_Scan(0);  /* 0=屏幕坐标模式 */
        
        /* 调试信息（每300ms刷新） */
        if (HAL_GetTick() - last_tick > 300) {
            last_tick = HAL_GetTick();
            
            POINT_COLOR = WHITE;
            BACK_COLOR = DARKBLUE;
            
            /* 显示校准参数和坐标 */
            sprintf(debug_buf, "X:%03d Y:%03d (%dx%d)", 
                    tp_dev.x, tp_dev.y, lcddev.width, lcddev.height);
            LCD_ShowString(2, bar_y + 30, 12, debug_buf, 0);
        }
        
        if (tp_dev.sta & TP_PRES_DOWN) {
            /* 有触摸 - 强制范围限制 */
            uint16_t safe_x = tp_dev.x;
            uint16_t safe_y = tp_dev.y;
            
            if (safe_x >= lcddev.width) safe_x = lcddev.width - 1;
            if (safe_y >= lcddev.height) safe_y = lcddev.height - 1;
            
            /* 点击底部工具栏 */
            if (safe_y >= bar_y) {
                /* 颜色按钮 */
                uint8_t found = 0;
                for (uint8_t i = 0; i < 7; i++) {
                    uint16_t bx = 2 + i * 33;
                    if (safe_x >= bx && safe_x <= bx + 28 && safe_y <= bar_y + 20) {
                        /* 清除旧选框 */
                        uint16_t old_bx = 2 + color_idx * 33;
                        LCD_Fill(old_bx - 1, bar_y + 2, old_bx + 29, bar_y + 21, DARKBLUE);
                        LCD_Fill(old_bx, bar_y + 3, old_bx + 28, bar_y + 20, colors[color_idx]);
                        
                        /* 新选框 */
                        color_idx = i;
                        pen_color = colors[i];
                        POINT_COLOR = WHITE;
                        LCD_DrawRectangle(bx - 1, bar_y + 2, bx + 29, bar_y + 21);
                        
                        /* 等待松手 */
                        while (TP_IRQ_READ() == GPIO_PIN_RESET) HAL_Delay(5);
                        found = 1;
                        last_x = 0xFFFF; last_y = 0xFFFF;  /* 重置轨迹 */
                        break;
                    }
                }
                
                /* CLEAR按钮 */
                if (!found && safe_x >= lcddev.width - 50 && safe_y <= bar_y + 20) {
                    LCD_Fill(0, draw_top, lcddev.width, draw_bottom, WHITE);
                    while (TP_IRQ_READ() == GPIO_PIN_RESET) HAL_Delay(5);
                    last_x = 0xFFFF; last_y = 0xFFFF;  /* 重置轨迹 */
                }
                
                /* RECAL（重新校准）按钮 */
                if (!found && safe_x >= lcddev.width - 105 && safe_x < lcddev.width - 55 && safe_y <= bar_y + 20) {
                    while (TP_IRQ_READ() == GPIO_PIN_RESET) HAL_Delay(5);
                    /* 重新校准（不保存到Flash） */
                    TP_Adjust();
                    /* 重新绘制界面 */
                    return;  /* 退出当前Touch_Test，main会重新调用 */
                }
            }
            /* 绘图区画点/线 */
            else if (safe_y >= draw_top && safe_y < draw_bottom) {
                /* 如果有上一次的坐标，画线；否则画点 */
                if (last_x != 0xFFFF && last_y != 0xFFFF) {
                    /* 画线连接上一点和当前点 */
                    LCD_DrawLine(last_x, last_y, safe_x, safe_y);
                    POINT_COLOR = pen_color;  /* 恢复画笔颜色 */
                } else {
                    /* 第一个点，只画点 */
                    TP_Draw_Big_Point(safe_x, safe_y, pen_color);
                }
                
                /* 更新上一次坐标 */
                last_x = safe_x;
                last_y = safe_y;
            }
        } else {
            /* 松手时重置轨迹 */
            last_x = 0xFFFF;
            last_y = 0xFFFF;
        }
        
        /* 减少延时，提高采样率 */
        HAL_Delay(2);  /* 2ms = 500Hz采样率，画线更流畅 */
    }
}

/* ==================== 时钟配置 ==================== */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType  = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState        = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue  = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState        = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState    = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource   = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL      = RCC_PLL_MUL9;  /* 8MHz x9 = 72MHz */
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) Error_Handler();
}

static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {}
#endif
