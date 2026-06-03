/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : TFT-LCD HAL 驱动测试程序
  *
  * 功能：
  *   1. 颜色填充测试（红/绿/蓝/白/黑）
  *   2. GUI图形测试（矩形、圆形）
  *   3. ASCII字体显示（6x12、8x16）
  *   4. 触摸校准 + 触摸画板
  *
  * ============================================================
  *  接线说明 —— STM32F103C8T6 ("蓝色小板") <-> ILI9341 TFT-LCD
  * ============================================================
  *
  *  【LCD 显示部分（硬件 SPI2）】
  *  ┌─────────────┬──────────────┬─────────────────────────────┐
  *  │  LCD 引脚   │  STM32 引脚  │  说明                       │
  *  ├─────────────┼──────────────┼─────────────────────────────┤
  *  │  VCC        │  3.3V        │  供电（勿接5V）             │
  *  │  GND        │  GND         │  地                         │
  *  │  CS         │  PB11        │  片选（低有效）             │
  *  │  RST        │  PB12        │  复位（低有效）             │
  *  │  DC / RS    │  PB10        │  命令/数据切换              │
  *  │  MOSI / SDA │  PB15        │  SPI2 数据输入              │
  *  │  SCK / CLK  │  PB13        │  SPI2 时钟                  │
  *  │  BL / LED   │  PB9         │  背光（高电平点亮）         │
  *  │  MISO / SDO │  PB14        │  SPI2 数据输出（可不接）    │
  *  └─────────────┴──────────────┴─────────────────────────────┘
  *
  *  【触摸部分（软件 SPI，XPT2046）】
  *  ┌─────────────┬──────────────┬─────────────────────────────┐
  *  │  触摸引脚   │  STM32 引脚  │  说明                       │
  *  ├─────────────┼──────────────┼─────────────────────────────┤
  *  │  T_IRQ      │  PA3         │  触摸中断（低有效，可不接） │
  *  │  T_DO / OUT │  PA4         │  软件SPI 数据输出（MISO）   │
  *  │  T_DIN      │  PA5         │  软件SPI 数据输入（MOSI）   │
  *  │  T_CS       │  PA6         │  触摸片选（低有效）         │
  *  │  T_CLK      │  PA7         │  软件SPI 时钟               │
  *  └─────────────┴──────────────┴─────────────────────────────┘
  *  （排针顺序与屏幕物理顺序一致，可直排连接，无需交叉）
  *
  *  注意事项：
  *   1. STM32F103C8T6 IO 电平为 3.3V，与屏幕直连即可，无需电平转换
  *   2. Blue Pill 的 PC0~PC3 未引出排针，触摸引脚改用 PA3~PA7
  *   3. SPI2 时钟频率配置为 4.5MHz（APB1 36MHz / 8），可稳定驱动 ILI9341
  *   4. 若屏幕显示花屏，检查 RST 和 DC 引脚是否接反
  * ============================================================
  ******************************************************************************
  */
#include "main.h"
#include "lcd.h"
#include "touch.h"

void SystemClock_Config(void);
static void MX_GPIO_Init(void);

/* ==================== 测试用颜色列表 ==================== */
static const uint16_t color_tab[] = {RED, GREEN, BLUE, YELLOW, CYAN};

/* ==================== 测试函数声明 ==================== */
static void Test_Color(void);
static void Test_GUI(void);
static void Test_Font(void);
static void Test_Touch(void);

/* ==================== 主函数 ==================== */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    /* 初始化 LCD（含 SPI2 和 GPIO 初始化） */
    LCD_Init();

    /* 初始化触摸（含触摸引脚 GPIO 初始化） */
    TP_Init();

    /* 开机欢迎界面 */
    LCD_Clear(BLACK);
    POINT_COLOR = WHITE; BACK_COLOR = BLACK;
    LCD_ShowString(10, 30, 16, "ILI9341 HAL Driver", 0);
    LCD_ShowString(10, 55, 16, "STM32F103C8T6", 0);
    LCD_ShowString(10, 80, 16, "240x320 / 320x240", 0);
    HAL_Delay(2000);

    while (1)
    {
        Test_Color();    /* 测试1：全屏颜色 */
        Test_GUI();      /* 测试2：图形绘制 */
        Test_Font();     /* 测试3：字体显示 */
        Test_Touch();    /* 测试4：触摸画板（需校准，触摸持续60秒后退出） */
    }
}

/* ==================== 测试1：颜色填充 ==================== */
static void Test_Color(void)
{
    const uint16_t colors[]  = {WHITE, BLACK, RED, GREEN, BLUE};
    const char    *names[]   = {"White", "Black", "Red", "Green", "Blue"};
    const uint16_t fg_colors[] = {BLACK, WHITE, WHITE, BLACK, WHITE};

    for (uint8_t i = 0; i < 5; i++) {
        LCD_Clear(colors[i]);
        POINT_COLOR = fg_colors[i];
        BACK_COLOR  = colors[i];
        LCD_ShowString(10, 10, 16, names[i], 0);
        HAL_Delay(800);
    }
}

/* ==================== 测试2：GUI 图形 ==================== */
static void Test_GUI(void)
{
    /* 顶/底蓝色标题栏 */
    LCD_Fill(0, 0, lcddev.width, 20, BLUE);
    LCD_Fill(0, lcddev.height - 20, lcddev.width, lcddev.height, BLUE);
    POINT_COLOR = WHITE; BACK_COLOR = BLUE;
    LCD_ShowString(10, 2,  16, "Test2: GUI", 0);
    LCD_ShowString(10, lcddev.height - 18, 16, "HAL LCD Driver", 0);

    /* 中间白色区域 */
    LCD_Fill(0, 20, lcddev.width, lcddev.height - 20, WHITE);

    /* 绘制5个嵌套空心矩形 */
    for (uint8_t i = 0; i < 5; i++) {
        POINT_COLOR = color_tab[i];
        uint16_t x0 = lcddev.width / 2 - 80 + i * 15;
        uint16_t y0 = lcddev.height / 2 - 60 + i * 12;
        LCD_DrawRectangle(x0, y0, x0 + 60, y0 + 50);
    }
    HAL_Delay(1500);

    /* 绘制5个实心矩形 */
    LCD_Fill(0, 20, lcddev.width, lcddev.height - 20, WHITE);
    for (uint8_t i = 0; i < 5; i++) {
        POINT_COLOR = color_tab[i];
        uint16_t x0 = lcddev.width / 2 - 80 + i * 15;
        uint16_t y0 = lcddev.height / 2 - 60 + i * 12;
        LCD_DrawFillRectangle(x0, y0, x0 + 60, y0 + 50);
    }
    HAL_Delay(1500);

    /* 绘制空心圆和实心圆 */
    LCD_Fill(0, 20, lcddev.width, lcddev.height - 20, WHITE);
    for (uint8_t i = 0; i < 5; i++) {
        LCD_Draw_Circle(lcddev.width / 2 - 60 + i * 30,
                        lcddev.height / 2 - 20 + i * 15,
                        25, color_tab[i], 0);
    }
    HAL_Delay(1500);

    LCD_Fill(0, 20, lcddev.width, lcddev.height - 20, WHITE);
    for (uint8_t i = 0; i < 5; i++) {
        LCD_Draw_Circle(lcddev.width / 2 - 60 + i * 30,
                        lcddev.height / 2 - 20 + i * 15,
                        25, color_tab[i], 1);
    }
    HAL_Delay(1500);
}

/* ==================== 测试3：字体显示 ==================== */
static void Test_Font(void)
{
    LCD_Fill(0, 0, lcddev.width, 20, BLUE);
    LCD_Fill(0, lcddev.height - 20, lcddev.width, lcddev.height, BLUE);
    POINT_COLOR = WHITE; BACK_COLOR = BLUE;
    LCD_ShowString(10, 2, 16, "Test3: Font", 0);
    LCD_Fill(0, 20, lcddev.width, lcddev.height - 20, BLACK);

    POINT_COLOR = YELLOW; BACK_COLOR = BLACK;
    LCD_ShowString(5, 30,  12, "6x12:ABCDEFGHIJKLMNOPQRSTUVWXYZ", 0);
    LCD_ShowString(5, 45,  12, "6x12:abcdefghijklmnopqrstuvwxyz", 0);
    LCD_ShowString(5, 60,  12, "6x12:0123456789!@#$%^&*()", 0);

    POINT_COLOR = CYAN;
    LCD_ShowString(5, 80,  16, "8x16:ABCDEFGHIJKLMNOPQRST", 0);
    LCD_ShowString(5, 100, 16, "8x16:abcdefghijklmnopqrst", 0);
    LCD_ShowString(5, 120, 16, "8x16:0123456789+-*/=<>?", 0);

    POINT_COLOR = GREEN;
    LCD_ShowString(5, 145, 16, "ILI9341 + XPT2046", 0);
    LCD_ShowString(5, 165, 16, "STM32F103C8T6 HAL", 0);

    HAL_Delay(2000);
}

/* ==================== 测试4：触摸画板 ==================== */
static void Test_Touch(void)
{
    uint8_t  color_idx = 0;
    uint16_t pen_colors[] = {RED, GREEN, BLUE, YELLOW, CYAN, MAGENTA, WHITE};
    uint32_t start_tick;

    /* 先校准 */
    TP_Adjust();

    /* 绘制画板界面 */
    LCD_Clear(BLACK);
    LCD_Fill(0, 0, lcddev.width, 20, BLUE);
    POINT_COLOR = WHITE; BACK_COLOR = BLUE;
    LCD_ShowString(5, 2, 16, "Test4: TouchPad", 0);

    /* 右上角颜色切换按钮 */
    LCD_Fill(lcddev.width - 52, 2, lcddev.width - 2, 18, pen_colors[color_idx]);
    LCD_ShowString(lcddev.width - 50, 2, 12, "CLR", 0);

    POINT_COLOR = pen_colors[color_idx];
    start_tick  = HAL_GetTick();

    while (HAL_GetTick() - start_tick < 60000) { /* 60秒后自动退出 */
        TP_Scan(0);

        if (tp_dev.sta & TP_PRES_DOWN) {
            tp_dev.sta &= ~TP_CATH_PRES;

            if (tp_dev.x < lcddev.width && tp_dev.y < lcddev.height && tp_dev.y > 20) {
                /* 检测颜色切换按钮（右上角） */
                if (tp_dev.x > (lcddev.width - 55) && tp_dev.y < 20) {
                    color_idx = (color_idx + 1) % (sizeof(pen_colors) / sizeof(pen_colors[0]));
                    POINT_COLOR = pen_colors[color_idx];
                    LCD_Fill(lcddev.width - 52, 2, lcddev.width - 2, 18, pen_colors[color_idx]);
                } else {
                    /* 正常绘图 */
                    TP_Draw_Big_Point(tp_dev.x, tp_dev.y, POINT_COLOR);
                    start_tick = HAL_GetTick(); /* 有触摸操作则重置超时 */
                }
            }
        } else {
            HAL_Delay(10);
        }
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
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;   /* HCLK  = 72MHz */
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;     /* APB1  = 36MHz */
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;     /* APB2  = 72MHz */
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
#endif /* USE_FULL_ASSERT */
