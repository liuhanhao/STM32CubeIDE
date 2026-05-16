/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : LED 闪烁 + 蜂鸣器同步控制
 *                   时钟配置：HSE 8MHz → PLL ×9 → SYSCLK 72MHz
 *                   PA0：LED 负极（低电平点亮）
 *                   PB12：有源蜂鸣器，低电平触发
 ******************************************************************************
 */

#include <stdint.h>

/* ============================================================
 * 寄存器基地址
 * ============================================================ */
#define RCC_BASE    0x40021000UL
#define FLASH_BASE  0x40022000UL
#define GPIOA_BASE  0x40010800UL
#define GPIOB_BASE  0x40010C00UL

/* RCC 寄存器 */
#define RCC_CR       (*(volatile uint32_t *)(RCC_BASE + 0x00))  /* 时钟控制寄存器 */
#define RCC_CFGR     (*(volatile uint32_t *)(RCC_BASE + 0x04))  /* 时钟配置寄存器 */
#define RCC_APB2ENR  (*(volatile uint32_t *)(RCC_BASE + 0x18))  /* APB2 外设时钟使能 */

/* Flash 访问控制寄存器（72MHz 需要 2 个等待周期）*/
#define FLASH_ACR    (*(volatile uint32_t *)(FLASH_BASE + 0x00))

/* GPIOA 寄存器 */
#define GPIOA_CRL    (*(volatile uint32_t *)(GPIOA_BASE + 0x00))
#define GPIOA_ODR    (*(volatile uint32_t *)(GPIOA_BASE + 0x0C))

/* GPIOB 寄存器 */
#define GPIOB_CRH    (*(volatile uint32_t *)(GPIOB_BASE + 0x04))
#define GPIOB_ODR    (*(volatile uint32_t *)(GPIOB_BASE + 0x0C))

/* ============================================================
 * RCC_CR 位定义
 * ============================================================ */
#define RCC_CR_HSEON   (1 << 16)  /* HSE 使能 */
#define RCC_CR_HSERDY  (1 << 17)  /* HSE 就绪标志 */
#define RCC_CR_PLLON   (1 << 24)  /* PLL 使能 */
#define RCC_CR_PLLRDY  (1 << 25)  /* PLL 就绪标志 */

/* ============================================================
 * RCC_CFGR 位定义
 * ============================================================ */
#define RCC_CFGR_SW_PLL     (0x2)           /* 系统时钟选择 PLL */
#define RCC_CFGR_SWS_PLL    (0x2 << 2)      /* 系统时钟状态：PLL */
#define RCC_CFGR_PPRE1_DIV2 (0x4 << 8)      /* APB1 2 分频（最高 36MHz）*/
#define RCC_CFGR_PLLSRC_HSE (1 << 16)       /* PLL 时钟源选 HSE */
#define RCC_CFGR_PLLMUL_9   (0x7 << 18)     /* PLL 倍频 ×9 → 8×9=72MHz */

/* ============================================================
 * APB2 时钟使能位
 * ============================================================ */
#define RCC_APB2ENR_IOPAEN  (1 << 2)  /* GPIOA 时钟使能 */
#define RCC_APB2ENR_IOPBEN  (1 << 3)  /* GPIOB 时钟使能 */

/* ============================================================
 * 延时：72MHz 下约 3 秒（每循环约 3 个时钟周期）
 * ============================================================ */
#define DELAY_COUNT  72000000UL

/* ============================================================
 * 函数声明
 * ============================================================ */
static void clock_init(void);
static void gpio_init(void);
static void delay(volatile uint32_t count);

/* ============================================================
 * 时钟初始化：HSE 8MHz → PLL ×9 → SYSCLK 72MHz
 * ============================================================ */
static void clock_init(void)
{
    /* 1. 设置 Flash 等待周期为 2（48~72MHz 必须设置）并开启预取缓冲 */
    FLASH_ACR = (1 << 4) | 0x2;   /* PRFTBE=1, LATENCY=2 */

    /* 2. 使能 HSE */
    RCC_CR |= RCC_CR_HSEON;

    /* 3. 等待 HSE 稳定 */
    while (!(RCC_CR & RCC_CR_HSERDY));

    /* 4. 配置 PLL：来源 HSE，倍频 ×9，APB1 2 分频 */
    RCC_CFGR = RCC_CFGR_PLLSRC_HSE |   /* PLL 源 = HSE */
               RCC_CFGR_PLLMUL_9   |   /* ×9 → 72MHz */
               RCC_CFGR_PPRE1_DIV2;    /* APB1 = 36MHz */

    /* 5. 使能 PLL */
    RCC_CR |= RCC_CR_PLLON;

    /* 6. 等待 PLL 锁定 */
    while (!(RCC_CR & RCC_CR_PLLRDY));

    /* 7. 切换系统时钟到 PLL */
    RCC_CFGR |= RCC_CFGR_SW_PLL;

    /* 8. 等待切换完成 */
    while ((RCC_CFGR & (0x3 << 2)) != RCC_CFGR_SWS_PLL);
}

/* ============================================================
 * GPIO 初始化
 * ============================================================ */
static void gpio_init(void)
{
    /* 使能 GPIOA 和 GPIOB 时钟 */
    RCC_APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN;

    /* PA0：推挽输出 2MHz（CRL bit[3:0]：MODE=10, CNF=00）*/
    GPIOA_CRL &= ~(0xF << 0);
    GPIOA_CRL |=  (0x2 << 0);

    /* PB12：推挽输出 2MHz（CRH bit[19:16]：MODE=10, CNF=00）*/
    GPIOB_CRH &= ~(0xF << 16);
    GPIOB_CRH |=  (0x2 << 16);

    /* 初始状态：LED 灭，蜂鸣器关（高电平）*/
    GPIOA_ODR |= (1 << 0);
    GPIOB_ODR |= (1 << 12);
}

/* ============================================================
 * 软件延时
 * ============================================================ */
static void delay(volatile uint32_t count)
{
    while (count--);
}

/* ============================================================
 * 主函数
 * ============================================================ */
int main(void)
{
    /* 初始化时钟（HSE 8MHz → PLL → 72MHz）*/
    clock_init();

    /* 初始化 GPIO */
    gpio_init();

    /* 主循环：LED 与蜂鸣器同步，间隔 3 秒 */
    for (;;)
    {
        /* LED 亮 + 蜂鸣器响 */
        GPIOA_ODR &= ~(1 << 0);
        GPIOB_ODR &= ~(1 << 12);
        delay(DELAY_COUNT);

        /* LED 灭 + 蜂鸣器关 */
        GPIOA_ODR |= (1 << 0);
        GPIOB_ODR |= (1 << 12);
        delay(DELAY_COUNT);
    }
}
