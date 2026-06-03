/**
 * @file    dht11.c
 * @brief   DHT11 温湿度传感器驱动实现
 *
 * 单总线协议时序：
 *   主机拉低 ≥18ms → 拉高 20~40μs → 切换输入 →
 *   等待 DHT11 响应（低~80μs + 高~80μs）→
 *   接收 40 位（每位：低~50μs + 高宽度判 0/1）→
 *   校验 & 范围验证 → 写入输出结构体
 */

#include "dht11.h"
#include "delay.h"    /* Delay_us() */

/* ------------------------------------------------------------------ */
/*  内部辅助函数：引脚模式动态切换                                       */
/* ------------------------------------------------------------------ */

/**
 * @brief  将 DHT11 数据引脚配置为推挽输出模式
 *         GPIO_NOPULL：模块已内置上拉电阻，无需 STM32 内部上下拉
 */
static void dht11_set_output(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = DHT11_GPIO_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStruct);
}

/**
 * @brief  将 DHT11 数据引脚配置为浮空输入模式
 *         GPIO_NOPULL：模块已内置上拉电阻，无需 STM32 内部上拉
 */
static void dht11_set_input(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = DHT11_GPIO_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStruct);
}

/* ------------------------------------------------------------------ */
/*  公共接口实现                                                        */
/* ------------------------------------------------------------------ */

/**
 * @brief  初始化 DHT11 数据引脚
 *         - 使能 GPIOA 时钟
 *         - 配置 PA0 为推挽输出
 *         - 将总线拉高，进入空闲状态
 */
void DHT11_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    dht11_set_output();
    HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN, GPIO_PIN_SET);
}

/**
 * @brief  执行一次 DHT11 单总线读取（含起始信号、等待响应、接收 40 位、校验）
 *         温度有效范围：-20~60 °C；湿度有效范围：0~100%。
 *         超出范围或校验失败时 out->valid = 0。
 * @param  out  指向输出结构体的指针，不可为 NULL
 * @return 1 = 读取成功且校验通过；0 = 超时或校验错误
 */
uint8_t DHT11_Read(DHT11Data_t *out)
{
    uint8_t  data[5] = {0};
    uint32_t timeout;

    /* ---- 阶段 1：主机发起起始信号 ---- */

    dht11_set_output();

    /* 拉低总线 ≥18ms（使用 20ms 确保裕量） */
    HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN, GPIO_PIN_RESET);
    HAL_Delay(20);

    /* 拉高总线 20~40μs（取中间值 30μs） */
    HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN, GPIO_PIN_SET);
    Delay_us(30);

    /* ---- 阶段 2：切换为输入，等待 DHT11 响应 ---- */

    dht11_set_input();

    /* 等待 DHT11 拉低总线（响应低电平约 80μs），超时保护 100μs */
    timeout = 0;
    while (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == GPIO_PIN_SET)
    {
        Delay_us(1);
        if (++timeout > 100)
            return 0;
    }

    /* 等待 DHT11 拉高总线（响应高电平约 80μs），超时保护 100μs */
    timeout = 0;
    while (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == GPIO_PIN_RESET)
    {
        Delay_us(1);
        if (++timeout > 100)
            return 0;
    }

    /* 等待 DHT11 再次拉低（数据传输开始前的低电平），超时保护 100μs */
    timeout = 0;
    while (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == GPIO_PIN_SET)
    {
        Delay_us(1);
        if (++timeout > 100)
            return 0;
    }

    /* ---- 阶段 3：接收 40 位数据（5 字节 × 8 位） ---- */

    for (uint8_t byte_idx = 0; byte_idx < 5; byte_idx++)
    {
        for (uint8_t bit_idx = 0; bit_idx < 8; bit_idx++)
        {
            /* 等待每位起始低电平结束（约 50μs），超时保护 80μs */
            timeout = 0;
            while (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == GPIO_PIN_RESET)
            {
                Delay_us(1);
                if (++timeout > 80)
                    return 0;
            }

            /*
             * 延时 40μs 后采样：
             *   仍为高电平 → 高脉冲 > 40μs → bit = 1（约 70μs）
             *   已变低电平 → 高脉冲 < 40μs → bit = 0（约 26~28μs）
             */
            Delay_us(40);

            data[byte_idx] <<= 1;

            if (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == GPIO_PIN_SET)
            {
                data[byte_idx] |= 0x01;

                /* 等待当前位高电平结束，超时保护 60μs */
                timeout = 0;
                while (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == GPIO_PIN_SET)
                {
                    Delay_us(1);
                    if (++timeout > 60)
                        return 0;
                }
            }
        }
    }

    /* ---- 阶段 4：校验和验证 ---- */
    /* data[0]=湿度整数, data[1]=湿度小数, data[2]=温度整数, data[3]=温度小数, data[4]=校验 */
    if (((uint8_t)(data[0] + data[1] + data[2] + data[3])) != data[4])
        return 0;

    /* ---- 阶段 5：解析温度（含符号位）---- */
    /* DHT11 温度：data[2] bit7=1 表示负温度，低 7 位为绝对值 */
    int8_t raw_temp;
    if (data[2] & 0x80)
        raw_temp = -(int8_t)(data[2] & 0x7F);
    else
        raw_temp = (int8_t)data[2];

    uint8_t raw_humi = data[0];

    /* ---- 阶段 6：范围验证 ---- */
    if (raw_temp < -20 || raw_temp > 60 || raw_humi > 100)
    {
        out->valid = 0;
        return 0;
    }

    /* ---- 阶段 7：写入输出结构体 ---- */
    out->temperature = raw_temp;
    out->humidity    = raw_humi;
    out->valid       = 1;

    return 1;
}
