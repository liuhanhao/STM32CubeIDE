/**
  ******************************************************************************
  * @file    dht11.c
  * @brief   DHT11 温湿度传感器驱动
  *
  * ── 模块规格参数 ────────────────────────────────────────────────────
  * 工作电压        : 3.3 ~ 5V
  * 工作电流        : < 2.5mA
  * 输出形式        : 数字输出，单总线串行数据
  * 模块上拉电阻    : 已集成，无需外接
  *
  * 温度检测范围    : 0 ~ 50 °C
  * 温度分辨率      : 1 °C
  * 温度精度        : ±2 °C
  *
  * 湿度检测范围    : 20% ~ 90% RH
  * 湿度分辨率      : 1% RH
  * 湿度精度        : ±5% RH
  *
  * 推荐存储环境    : 温度 10~40°C，湿度 60%RH 以下
  * 安装孔径        : Φ4mm
  * ────────────────────────────────────────────────────────────────────
  *
  * 接线说明：
  *   DHT11 VCC  → STM32 3.3V 或 5V
  *   DHT11 GND  → STM32 GND
  *   DHT11 DATA → STM32 PA0
  *
  * 通信协议：单总线，主机先拉低 ≥18ms 启动，再拉高等待应答，
  * 随后读取 40bit 数据（湿度高/低 + 温度高/低 + 校验）。
  ******************************************************************************
  */
#include "dht11.h"
#include "delay.h"

/* ------------------------------------------------------------------ */
/* 引脚方向切换辅助函数                                                */
/* ------------------------------------------------------------------ */

/** 将 DATA 引脚配置为推挽输出 */
static void DHT11_SetOutput(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = DHT11_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}

/** 将 DATA 引脚配置为上拉输入 */
static void DHT11_SetInput(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin  = DHT11_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}

/** 写 DATA 引脚电平 */
static inline void DHT11_WritePin(uint8_t val)
{
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, (GPIO_PinState)val);
}

/** 读 DATA 引脚电平 */
static inline uint8_t DHT11_ReadPin(void)
{
    return (uint8_t)HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN);
}

/* ------------------------------------------------------------------ */
/* 初始化                                                              */
/* ------------------------------------------------------------------ */

void DHT11_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    DHT11_SetInput();   /* 空闲状态：上拉输入 */
}

/* ------------------------------------------------------------------ */
/* 内部函数：读取一个字节（MSB first）                                  */
/* ------------------------------------------------------------------ */

static uint8_t DHT11_ReadByte(void)
{
    uint8_t byte = 0;
    uint8_t i;

    for (i = 0; i < 8; i++)
    {
        /* 等待每 bit 起始的低电平结束 */
        uint16_t timeout = 0;
        while (DHT11_ReadPin() == 0)
        {
            Delay_us(1);
            if (++timeout > 100) break;   /* 防死锁 */
        }

        /* 采样时刻：延时 40us
           高电平持续 ~26us → '0'，持续 ~70us → '1' */
        Delay_us(40);

        byte <<= 1;
        if (DHT11_ReadPin() == 1)
        {
            byte |= 0x01;
            /* 等待本 bit 高电平结束 */
            timeout = 0;
            while (DHT11_ReadPin() == 1)
            {
                Delay_us(1);
                if (++timeout > 100) break;
            }
        }
    }
    return byte;
}

/* ------------------------------------------------------------------ */
/* 对外接口：读取温湿度                                                 */
/* ------------------------------------------------------------------ */

/**
  * @brief  读取 DHT11 一次完整数据
  * @param  data  指向结果结构体的指针
  * @retval DHT11_OK(0) 成功，DHT11_ERROR(1) 失败
  */
uint8_t DHT11_Read(DHT11_Data_t *data)
{
    uint8_t buf[5];
    uint16_t timeout;

    /* -------- 主机发起信号：拉低 ≥18ms -------- */
    DHT11_SetOutput();
    DHT11_WritePin(0);
    Delay_ms(20);
    DHT11_WritePin(1);
    Delay_us(30);

    /* -------- 切换为输入，等待 DHT11 应答 -------- */
    DHT11_SetInput();

    /* 等待 DHT11 拉低（应答低电平 80us）*/
    timeout = 0;
    while (DHT11_ReadPin() == 1)
    {
        Delay_us(1);
        if (++timeout > 100) return DHT11_ERROR;
    }

    /* 等待 DHT11 拉高（应答高电平 80us）*/
    timeout = 0;
    while (DHT11_ReadPin() == 0)
    {
        Delay_us(1);
        if (++timeout > 100) return DHT11_ERROR;
    }

    /* 等待应答高电平结束，进入数据传输 */
    timeout = 0;
    while (DHT11_ReadPin() == 1)
    {
        Delay_us(1);
        if (++timeout > 100) return DHT11_ERROR;
    }

    /* -------- 读取 5 字节数据 -------- */
    buf[0] = DHT11_ReadByte();   /* 湿度整数 */
    buf[1] = DHT11_ReadByte();   /* 湿度小数 */
    buf[2] = DHT11_ReadByte();   /* 温度整数 */
    buf[3] = DHT11_ReadByte();   /* 温度小数 */
    buf[4] = DHT11_ReadByte();   /* 校验和   */

    /* -------- 校验和验证 -------- */
    if (buf[4] != (uint8_t)(buf[0] + buf[1] + buf[2] + buf[3]))
    {
        return DHT11_ERROR;
    }

    /* -------- 填充结果 -------- */
    data->humi_int = buf[0];
    data->humi_dec = buf[1];
    data->temp_int = buf[2];
    data->temp_dec = buf[3];

    return DHT11_OK;
}
