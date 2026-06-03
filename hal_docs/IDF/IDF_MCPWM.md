# MCPWM — 电机控制 PWM API 文档（ESP-IDF）

## 📋 模块概述

MCPWM（Motor Control PWM）是专为电机控制设计的 PWM 外设，提供：
- 互补 PWM 输出（H 桥驱动必需）
- 可配置死区时间（防止上下管同时导通）
- 旋转编码器接口（硬件计数，不丢脉冲）
- 故障检测（过流/过压保护）
- 捕获功能（测量外部信号）

---

## 🗺️ 使用场景速查

| 场景 | 功能 |
|------|------|
| 直流电机调速（单向）| LEDC 即可，无需 MCPWM |
| H 桥直流电机（双向）| MCPWM 互补 PWM + 死区 |
| 无刷电机（BLDC）| MCPWM 三相互补 PWM |
| 旋转编码器计数 | MCPWM 编码器模式 |
| 电机过流保护 | MCPWM 故障检测 |

**MCPWM vs LEDC：**

| 特性 | MCPWM | LEDC |
|------|-------|------|
| 互补输出 | ✅ | ❌ |
| 死区控制 | ✅ | ❌ |
| 编码器接口 | ✅ | ❌ |
| 故障保护 | ✅ | ❌ |
| 通道数 | 6 路（3 对互补）| 8 路独立 |
| 适用场景 | 电机驱动 | LED、舵机、普通 PWM |

---

## 📚 相关文件

- **头文件**: `driver/mcpwm_prelude.h`（IDF 5.x 新 API）

---

## 一、PWM 输出

### 核心概念

MCPWM 的层级结构：
```
MCPWM 组（Group）
└── 定时器（Timer）
    └── 操作器（Operator）
        ├── 比较器（Comparator）→ 决定占空比
        └── 发生器（Generator）→ 输出到 GPIO
```

### 初始化流程

```c
#include "driver/mcpwm_prelude.h"

mcpwm_timer_handle_t    timer    = NULL;
mcpwm_oper_handle_t     oper     = NULL;
mcpwm_cmpr_handle_t     cmpr     = NULL;
mcpwm_gen_handle_t      gen_a    = NULL;
mcpwm_gen_handle_t      gen_b    = NULL;  // 互补输出

/* 1. 创建定时器 */
mcpwm_timer_config_t timer_cfg = {
    .group_id      = 0,
    .clk_src       = MCPWM_TIMER_CLK_SRC_DEFAULT,
    .resolution_hz = 10000000,  // 10MHz，计数分辨率
    .period_ticks  = 1000,      // 周期 = 1000/10MHz = 100μs = 10kHz
    .count_mode    = MCPWM_TIMER_COUNT_MODE_UP,
};
mcpwm_new_timer(&timer_cfg, &timer);

/* 2. 创建操作器 */
mcpwm_operator_config_t oper_cfg = {
    .group_id = 0,
};
mcpwm_new_operator(&oper_cfg, &oper);

/* 3. 绑定定时器和操作器 */
mcpwm_operator_connect_timer(oper, timer);

/* 4. 创建比较器 */
mcpwm_comparator_config_t cmpr_cfg = {
    .flags.update_cmp_on_tez = true,  // 在计数器为 0 时更新比较值
};
mcpwm_new_comparator(oper, &cmpr_cfg, &cmpr);

/* 5. 创建发生器（输出到 GPIO）*/
mcpwm_generator_config_t gen_cfg_a = {
    .gen_gpio_num = GPIO_NUM_10,
};
mcpwm_new_generator(oper, &gen_cfg_a, &gen_a);

/* 6. 配置发生器动作（计数器为 0 时拉高，比较匹配时拉低）*/
mcpwm_generator_set_action_on_timer_event(gen_a,
    MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP,
                                  MCPWM_TIMER_EVENT_EMPTY,
                                  MCPWM_GEN_ACTION_HIGH));
mcpwm_generator_set_action_on_compare_event(gen_a,
    MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP,
                                    cmpr,
                                    MCPWM_GEN_ACTION_LOW));

/* 7. 启动定时器 */
mcpwm_timer_enable(timer);
mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP);
```

### 修改占空比

```c
/* 设置比较值（占空比 = compare_ticks / period_ticks）*/
/* 例：period=1000，compare=500 → 50% 占空比 */
mcpwm_comparator_set_compare_value(cmpr, 500);
```

---

## 二、互补 PWM + 死区（H 桥驱动）

```c
/* 在上面基础上，添加互补输出和死区 */

/* 创建互补发生器（gen_b）*/
mcpwm_generator_config_t gen_cfg_b = {
    .gen_gpio_num = GPIO_NUM_11,  // 互补输出引脚
};
mcpwm_new_generator(oper, &gen_cfg_b, &gen_b);

/* gen_b 与 gen_a 相反 */
mcpwm_generator_set_action_on_timer_event(gen_b,
    MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP,
                                  MCPWM_TIMER_EVENT_EMPTY,
                                  MCPWM_GEN_ACTION_LOW));
mcpwm_generator_set_action_on_compare_event(gen_b,
    MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP,
                                    cmpr,
                                    MCPWM_GEN_ACTION_HIGH));

/* 配置死区（防止上下管同时导通）*/
mcpwm_dead_time_config_t dt_cfg = {
    .posedge_delay_ticks = 50,   // 上升沿死区：50 × (1/10MHz) = 5μs
    .negedge_delay_ticks = 50,   // 下降沿死区：5μs
};
mcpwm_generator_set_dead_time(gen_a, gen_a, &dt_cfg);

mcpwm_dead_time_config_t dt_cfg_b = {
    .posedge_delay_ticks = 50,
    .negedge_delay_ticks = 50,
    .flags.invert_output = true,  // 互补输出需要反相
};
mcpwm_generator_set_dead_time(gen_b, gen_b, &dt_cfg_b);
```

---

## 三、旋转编码器（Encoder）

### 核心概念

MCPWM 编码器模式通过硬件自动计数 A/B 相脉冲，支持：
- 正交解码（4 倍频）
- 自动判断方向（正转/反转）
- 不丢脉冲（硬件计数，不受中断延迟影响）

### 初始化

```c
#include "driver/mcpwm_prelude.h"

mcpwm_cap_timer_handle_t cap_timer = NULL;
mcpwm_cap_channel_handle_t cap_chan = NULL;

/* 方法：使用 PCNT（脉冲计数器）模块，更适合编码器 */
/* MCPWM 主要用于 PWM 输出，编码器推荐用 PCNT */
```

> **推荐：** 旋转编码器优先使用 **PCNT（脉冲计数器）**模块，专为编码器设计，API 更简洁。

### PCNT 编码器示例

```c
#include "driver/pulse_cnt.h"
#include "esp_log.h"

#define ENC_A_PIN  GPIO_NUM_12
#define ENC_B_PIN  GPIO_NUM_13

static const char *TAG = "ENCODER";
static pcnt_unit_handle_t pcnt_unit = NULL;

void encoder_init(void)
{
    /* 1. 创建 PCNT 单元 */
    pcnt_unit_config_t unit_cfg = {
        .high_limit =  32767,
        .low_limit  = -32768,
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_cfg, &pcnt_unit));

    /* 2. 配置滤波器（去抖）*/
    pcnt_glitch_filter_config_t filter_cfg = {
        .max_glitch_ns = 1000,  // 过滤 1μs 以下的毛刺
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filter_cfg));

    /* 3. 配置 A 相通道 */
    pcnt_chan_config_t chan_a_cfg = {
        .edge_gpio_num  = ENC_A_PIN,
        .level_gpio_num = ENC_B_PIN,
    };
    pcnt_channel_handle_t chan_a = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_a_cfg, &chan_a));

    /* A 相上升沿：B=高→+1，B=低→-1 */
    pcnt_channel_set_edge_action(chan_a,
        PCNT_CHANNEL_EDGE_ACTION_INCREASE,  // 上升沿
        PCNT_CHANNEL_EDGE_ACTION_DECREASE); // 下降沿
    pcnt_channel_set_level_action(chan_a,
        PCNT_CHANNEL_LEVEL_ACTION_KEEP,     // B=高：保持方向
        PCNT_CHANNEL_LEVEL_ACTION_INVERSE); // B=低：反向

    /* 4. 配置 B 相通道（实现 4 倍频）*/
    pcnt_chan_config_t chan_b_cfg = {
        .edge_gpio_num  = ENC_B_PIN,
        .level_gpio_num = ENC_A_PIN,
    };
    pcnt_channel_handle_t chan_b = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_b_cfg, &chan_b));

    pcnt_channel_set_edge_action(chan_b,
        PCNT_CHANNEL_EDGE_ACTION_DECREASE,
        PCNT_CHANNEL_EDGE_ACTION_INCREASE);
    pcnt_channel_set_level_action(chan_b,
        PCNT_CHANNEL_LEVEL_ACTION_KEEP,
        PCNT_CHANNEL_LEVEL_ACTION_INVERSE);

    /* 5. 启动 */
    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));

    ESP_LOGI(TAG, "编码器初始化完成（4倍频）");
}

/* 读取编码器计数值 */
int encoder_get_count(void)
{
    int count = 0;
    pcnt_unit_get_count(pcnt_unit, &count);
    return count;
}

/* 清零计数 */
void encoder_reset(void)
{
    pcnt_unit_clear_count(pcnt_unit);
}
```

---

## 💡 完整应用示例

### H 桥电机驱动（完整）

```c
#include "driver/mcpwm_prelude.h"
#include "esp_log.h"

#define MOTOR_PWM_A  GPIO_NUM_10  // 上桥臂 A
#define MOTOR_PWM_B  GPIO_NUM_11  // 下桥臂 A（互补）
#define MOTOR_PWM_C  GPIO_NUM_12  // 上桥臂 B
#define MOTOR_PWM_D  GPIO_NUM_13  // 下桥臂 B（互补）

#define PWM_FREQ_HZ   10000   // 10kHz
#define PWM_RES_HZ    10000000  // 10MHz 分辨率
#define PWM_PERIOD    (PWM_RES_HZ / PWM_FREQ_HZ)  // 1000 ticks

static mcpwm_cmpr_handle_t cmpr_a = NULL;
static mcpwm_cmpr_handle_t cmpr_b = NULL;

static const char *TAG = "MOTOR";

void motor_init(void)
{
    mcpwm_timer_handle_t timer;
    mcpwm_timer_config_t timer_cfg = {
        .group_id      = 0,
        .clk_src       = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = PWM_RES_HZ,
        .period_ticks  = PWM_PERIOD,
        .count_mode    = MCPWM_TIMER_COUNT_MODE_UP,
    };
    mcpwm_new_timer(&timer_cfg, &timer);

    /* 操作器 A（控制电机一侧）*/
    mcpwm_oper_handle_t oper_a;
    mcpwm_operator_config_t oper_cfg = { .group_id = 0 };
    mcpwm_new_operator(&oper_cfg, &oper_a);
    mcpwm_operator_connect_timer(oper_a, timer);

    mcpwm_comparator_config_t cmpr_cfg = {
        .flags.update_cmp_on_tez = true,
    };
    mcpwm_new_comparator(oper_a, &cmpr_cfg, &cmpr_a);

    /* 发生器 A+ 和 A-（互补）*/
    mcpwm_gen_handle_t gen_ap, gen_an;
    mcpwm_generator_config_t gen_cfg = { .gen_gpio_num = MOTOR_PWM_A };
    mcpwm_new_generator(oper_a, &gen_cfg, &gen_ap);
    gen_cfg.gen_gpio_num = MOTOR_PWM_B;
    mcpwm_new_generator(oper_a, &gen_cfg, &gen_an);

    /* 配置互补动作 */
    mcpwm_generator_set_action_on_timer_event(gen_ap,
        MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP,
                                      MCPWM_TIMER_EVENT_EMPTY,
                                      MCPWM_GEN_ACTION_HIGH));
    mcpwm_generator_set_action_on_compare_event(gen_ap,
        MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP,
                                        cmpr_a, MCPWM_GEN_ACTION_LOW));
    mcpwm_generator_set_action_on_timer_event(gen_an,
        MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP,
                                      MCPWM_TIMER_EVENT_EMPTY,
                                      MCPWM_GEN_ACTION_LOW));
    mcpwm_generator_set_action_on_compare_event(gen_an,
        MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP,
                                        cmpr_a, MCPWM_GEN_ACTION_HIGH));

    /* 死区配置（5μs）*/
    mcpwm_dead_time_config_t dt = {
        .posedge_delay_ticks = 50,
        .negedge_delay_ticks = 50,
    };
    mcpwm_generator_set_dead_time(gen_ap, gen_ap, &dt);
    dt.flags.invert_output = true;
    mcpwm_generator_set_dead_time(gen_an, gen_an, &dt);

    mcpwm_timer_enable(timer);
    mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP);

    ESP_LOGI(TAG, "电机驱动初始化完成");
}

/* 设置电机速度（-100~+100，负值反转）*/
void motor_set_speed(int speed)
{
    if (speed > 100)  speed = 100;
    if (speed < -100) speed = -100;

    uint32_t duty = (uint32_t)(abs(speed) * PWM_PERIOD / 100);

    if (speed >= 0) {
        /* 正转：A+ 有 PWM，B+ 全低 */
        mcpwm_comparator_set_compare_value(cmpr_a, duty);
        if (cmpr_b) mcpwm_comparator_set_compare_value(cmpr_b, 0);
    } else {
        /* 反转：A+ 全低，B+ 有 PWM */
        mcpwm_comparator_set_compare_value(cmpr_a, 0);
        if (cmpr_b) mcpwm_comparator_set_compare_value(cmpr_b, duty);
    }
}
```

---

## ⚠️ 注意事项

### 1. 死区时间必须配置

H 桥驱动中，上下管同时导通会造成短路（直通）。
死区时间确保一个管关断后，另一个管才开通。
典型死区：MOSFET 驱动 1~5μs，IGBT 驱动 2~10μs。

### 2. 编码器推荐用 PCNT

MCPWM 的捕获功能主要用于脉宽测量，旋转编码器用 PCNT 模块更合适：
- PCNT 专为计数设计，API 更简洁
- 支持 4 倍频（A/B 双相各双边沿）
- 硬件滤波器去抖

### 3. 分辨率与频率

`period_ticks = resolution_hz / freq_hz`，分辨率越高，占空比精度越高：
- 10MHz 分辨率，10kHz 频率 → period=1000，占空比精度 0.1%
- 10MHz 分辨率，50Hz 频率 → period=200000，占空比精度 0.0005%

---

## 📖 相关文档

- [IDF_LEDC.md](IDF_LEDC.md) — 普通 PWM（LED、舵机）
- [IDF_GPTimer.md](IDF_GPTimer.md) — 通用定时器
- [README.md](README.md) — 模块导航
