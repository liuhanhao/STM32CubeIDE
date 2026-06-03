# 单总线协议 (DHT11 / DHT22) 驱动文档

## 📋 模块概述

单总线（Single-Wire）是一种仅用 **1 根数据线** 完成双向通信的私有协议，DHT11 / DHT22 温湿度传感器采用该协议。

- 协议层面：MCU 主动发起，传感器响应；通过高低电平持续时间区分 0/1
- STM32 HAL 库无内置驱动，需用 GPIO 手动模拟时序（Bit-Banging）
- 时序精度依赖 `DWT` 计数器或 `TIM` 定时器（微秒级），不能使用 `HAL_Delay`（毫秒级）
- 通信过程中禁止中断打断，否则时序错误导致校验失败

---

## 🗺️ 使用场景速查

**DHT11 vs DHT22 怎么选**

| 对比项 | DHT11 | DHT22 |
|--------|-------|-------|
| 温度范围 | 0~50°C | -40~80°C |
| 温度精度 | ±2°C | ±0.5°C |
| 湿度范围 | 20~90%RH | 0~100%RH |
| 湿度精度 | ±5%RH | ±2%RH |
| 采样间隔 | 最小 1s | 最小 2s |
| 价格 | 更低 | 稍高 |
| 数据格式 | 整数（1 字节温度 + 1 字节湿度） | 定点小数（2 字节温度 + 2 字节湿度） |
| 适用场景 | 室内粗略监测 | 精度要求较高的场合 |

**微秒延时方案怎么选**

| 方案 | 优点 | 缺点 | 推荐 |
|------|------|------|------|
| DWT 计数器 | 无需占用 TIM，精度极高 | Cortex-M0 不支持 | ✅ STM32F1 首选 |
| TIM 定时器 | 任意平台通用 | 占用一个 TIM 外设 | 备选 |
| 汇编 NOP 循环 | 简单 | 受编译优化影响，不稳定 | ❌ 不推荐 |

---

## 📚 协议时序

### 通信流程总览

```
MCU 发起 ─→ 传感器响应 ─→ 传输 40 bit 数据 ─→ 结束
```

**40 bit 数据格式**

| 字节 | DHT11 含义 | DHT22 含义 |
|------|-----------|-----------|
| Byte0 | 湿度整数部分 | 湿度高字节 |
| Byte1 | 湿度小数部分（DHT11 恒为 0） | 湿度低字节 |
| Byte2 | 温度整数部分 | 温度高字节（最高位为符号位） |
| Byte3 | 温度小数部分（DHT11 恒为 0） | 温度低字节 |
| Byte4 | 校验和（前四字节之和的低 8 位） | 同左 |

### 关键时序参数

| 阶段 | 电平 | 持续时间 |
|------|------|---------|
| MCU 起始信号（拉低） | LOW | ≥18ms（DHT11）/ ≥1ms（DHT22） |
| MCU 释放总线 | HIGH | 20~40μs |
| 传感器响应低电平 | LOW | 80μs |
| 传感器响应高电平 | HIGH | 80μs |
| 数据位起始低电平 | LOW | 50μs |
| 数据"0"高电平 | HIGH | 26~28μs |
| 数据"1"高电平 | HIGH | 70μs |

> 判断 0/1 的核心：等待 50μs 低电平结束后，若高电平持续 **<40μs** 为 0，**>40μs** 为 1。

---

## 🔧 驱动实现

### 微秒延时（基于 DWT）

```c
/* dht_delay.h */
#include "stm32f1xx_hal.h"

/**
 * @brief 初始化 DWT 计数器，使能微秒延时
 *        在 main() 中调用一次即可
 */
void DWT_Init(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;  /* 使能 DWT */
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;             /* 启动周期计数 */
}

/**
 * @brief 微秒级忙等延时
 * @param us 延时时间（微秒）
 */
void Delay_us(uint32_t us) {
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (SystemCoreClock / 1000000U);
    while ((DWT->CYCCNT - start) < ticks);
}
```

---

### DHT11 驱动

```c
/* dht11.h */
#ifndef DHT11_H
#define DHT11_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

/* 修改为实际连接的引脚 */
#define DHT11_PORT   GPIOA
#define DHT11_PIN    GPIO_PIN_1

typedef struct {
    uint8_t  humidity_int;    /* 湿度整数部分 */
    uint8_t  humidity_dec;    /* 湿度小数部分（DHT11 恒为 0） */
    uint8_t  temp_int;        /* 温度整数部分 */
    uint8_t  temp_dec;        /* 温度小数部分（DHT11 恒为 0） */
    uint8_t  checksum;        /* 校验和 */
} DHT11_Data_t;

typedef enum {
    DHT11_OK     = 0,
    DHT11_ERR_TIMEOUT,        /* 响应超时 */
    DHT11_ERR_CHECKSUM,       /* 校验失败 */
} DHT11_Status_t;

DHT11_Status_t DHT11_Read(DHT11_Data_t *data);

#endif /* DHT11_H */
```

```c
/* dht11.c */
#include "dht11.h"

/* 内部辅助：将引脚设为输出模式 */
static void DHT11_SetOutput(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = DHT11_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;   /* 推挽输出 */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}

/* 内部辅助：将引脚设为输入模式（带上拉） */
static void DHT11_SetInput(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin  = DHT11_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}

/**
 * @brief 读取 DHT11 温湿度数据
 * @param data 数据输出结构体指针
 * @return DHT11_OK / DHT11_ERR_TIMEOUT / DHT11_ERR_CHECKSUM
 */
DHT11_Status_t DHT11_Read(DHT11_Data_t *data) {
    uint8_t  raw[5] = {0};
    uint32_t timeout;

    /* 1. MCU 发起始信号：拉低 ≥18ms，再拉高 */
    DHT11_SetOutput();
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_RESET);
    HAL_Delay(20);  /* 20ms */
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET);
    Delay_us(30);   /* 20~40μs 释放总线 */

    /* 2. 切换为输入，等待传感器拉低响应 */
    DHT11_SetInput();

    /* 等待传感器拉低（最多 100μs）*/
    timeout = 100;
    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET) {
        Delay_us(1);
        if (--timeout == 0) return DHT11_ERR_TIMEOUT;
    }

    /* 等待传感器释放（80μs 低 + 80μs 高）*/
    timeout = 100;
    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_RESET) {
        Delay_us(1);
        if (--timeout == 0) return DHT11_ERR_TIMEOUT;
    }
    timeout = 100;
    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET) {
        Delay_us(1);
        if (--timeout == 0) return DHT11_ERR_TIMEOUT;
    }

    /* 3. 接收 40 bit 数据 */
    for (int i = 0; i < 40; i++) {
        /* 等待每个数据位的 50μs 起始低电平结束 */
        timeout = 100;
        while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_RESET) {
            Delay_us(1);
            if (--timeout == 0) return DHT11_ERR_TIMEOUT;
        }

        /* 等待 40μs，通过判断此时电平高低来区分 0/1 */
        Delay_us(40);
        raw[i / 8] <<= 1;
        if (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET) {
            raw[i / 8] |= 0x01;  /* 高电平仍在 → bit=1 */
        }

        /* 等待高电平结束，准备下一 bit */
        timeout = 100;
        while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET) {
            Delay_us(1);
            if (--timeout == 0) return DHT11_ERR_TIMEOUT;
        }
    }

    /* 4. 校验和验证 */
    uint8_t sum = raw[0] + raw[1] + raw[2] + raw[3];
    if (sum != raw[4]) return DHT11_ERR_CHECKSUM;

    /* 5. 填充输出 */
    data->humidity_int = raw[0];
    data->humidity_dec = raw[1];
    data->temp_int     = raw[2];
    data->temp_dec     = raw[3];
    data->checksum     = raw[4];

    return DHT11_OK;
}
```

---

### DHT22 驱动（在 DHT11 基础上的差异）

DHT22 与 DHT11 的驱动逻辑基本相同，差异在两点：

**1. 起始信号拉低时间**

```c
/* DHT22 只需拉低 1~10ms，DHT11 需要 ≥18ms */
HAL_Delay(2);   /* 2ms，替换 DHT11 的 HAL_Delay(20) */
```

**2. 数据解析方式**

```c
/* DHT22 数据解析：两字节合并为带符号定点数 */
typedef struct {
    float temperature;   /* 摄氏度，精度 0.1°C */
    float humidity;      /* %RH，精度 0.1% */
} DHT22_Data_t;

/* 在读取成功后替换解析代码 */
int16_t raw_hum  = (int16_t)((raw[0] << 8) | raw[1]);
int16_t raw_temp = (int16_t)((raw[2] << 8) | raw[3]);

/* raw[2] 最高位为符号位（负温度），HAL 已处理为有符号强转 */
data->humidity    = raw_hum  / 10.0f;
data->temperature = raw_temp / 10.0f;
```

---

## 💡 完整应用示例

### 示例1：定时采集温湿度并串口打印

```c
#include "main.h"
#include "dht11.h"
#include <stdio.h>

extern UART_HandleTypeDef huart1;

/* 重定向 printf */
int fputc(int ch, FILE *f) {
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 100);
    return ch;
}

void DHT11_Task(void) {
    DHT11_Data_t data;
    DHT11_Status_t status;

    status = DHT11_Read(&data);

    if (status == DHT11_OK) {
        printf("温度: %d.%d°C  湿度: %d.%d%%RH\r\n",
               data.temp_int, data.temp_dec,
               data.humidity_int, data.humidity_dec);
    } else if (status == DHT11_ERR_TIMEOUT) {
        printf("DHT11: 读取超时\r\n");
    } else {
        printf("DHT11: 校验失败\r\n");
    }
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();

    DWT_Init();  /* 初始化微秒延时 */

    /* DHT11 上电后需等待 1s 稳定 */
    HAL_Delay(1000);

    while (1) {
        DHT11_Task();
        HAL_Delay(2000);  /* 每 2s 采集一次，DHT11 最快 1s */
    }
}
```

---

### 示例2：配合 FreeRTOS 任务使用

```c
/* 注意：FreeRTOS 中不能在任务间共享 GPIO 时序操作，DHT11 读取需放在独立任务中 */
void vDHT11Task(void *pvParameters) {
    DHT11_Data_t data;
    DHT11_Status_t status;
    uint8_t retry;

    for (;;) {
        retry = 3;
        status = DHT11_ERR_TIMEOUT;

        /* 最多重试 3 次 */
        while (retry-- && status != DHT11_OK) {
            /* 读取前暂时关闭调度（保护时序，≤5ms 内完成） */
            taskENTER_CRITICAL();
            status = DHT11_Read(&data);
            taskEXIT_CRITICAL();

            if (status != DHT11_OK) vTaskDelay(pdMS_TO_TICKS(10));
        }

        if (status == DHT11_OK) {
            printf("T=%d°C H=%d%%\r\n", data.temp_int, data.humidity_int);
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
```

---

## ⚠️ 注意事项

### 1. 中断保护

DHT11 通信全程约 4ms，期间任何中断（UART、SysTick 等）都可能导致时序错误：

```c
/* 裸机环境：读取前关闭全局中断 */
__disable_irq();
DHT11_Read(&data);
__enable_irq();

/* FreeRTOS 环境：使用临界区（只关闭受管理的中断）*/
taskENTER_CRITICAL();
DHT11_Read(&data);
taskEXIT_CRITICAL();
```

---

### 2. 上拉电阻

数据线需要接上拉电阻到 VCC（3.3V 或 5V）：

- 通常 4.7kΩ~10kΩ
- 线路较长时选 4.7kΩ，短线可用 10kΩ
- 若使用内部上拉，GPIO 配置需设 `GPIO_PULLUP`

> DHT11 供电 3.3V~5V 均可，但数据线电平需与 MCU IO 电压匹配。

---

### 3. 采样频率限制

```c
/* DHT11：两次读取间隔不得小于 1s */
HAL_Delay(1000);

/* DHT22：两次读取间隔不得小于 2s */
HAL_Delay(2000);
```

频繁读取会导致传感器响应异常，返回全 0 或校验失败。

---

### 4. GPIO 引脚速度

数据线 GPIO 配置推挽输出时，速度必须设为 `GPIO_SPEED_FREQ_HIGH`，否则上升沿过慢，传感器无法识别起始信号。

---

### 5. 上电稳定时间

传感器上电后需等待 **1s** 才能首次读取：

```c
/* main() 初始化完成后 */
HAL_Delay(1000);  /* 等待 DHT11 稳定 */
```

---

## 📖 相关文档

- [GPIO 通用 IO](HAL_GPIO.md)
- [TIM 定时器](HAL_TIM.md)（可替代 DWT 实现微秒延时）
- [HAL 核心](HAL_Core.md)
