# 触控屏接口文档（I2C 触摸控制器）

## 📋 模块概述

触控屏由两个独立子系统构成：

- **显示子系统**：通过 SPI 或并行总线（FSMC）传输像素数据 → 参考 [HAL_SPI.md](HAL_SPI.md)
- **触摸子系统**：独立 IC（如 FT6206、GT911）通过 **I2C** 回传触摸坐标，并通过 **INT 中断引脚** 通知 MCU 有触摸事件

本文档聚焦触摸控制器的 I2C 通信驱动，以最常见的 **FT6206**（电容单点）和 **GT911**（电容多点）为例。

---

## 🗺️ 使用场景速查

**触摸控制器选型**

| 控制器 | 协议 | 触点数 | 分辨率 | 典型屏幕 | 常见模块 |
|--------|------|--------|--------|---------|---------|
| FT6206 | I2C | 2 点 | 480×272 以内 | 2.8"~3.5" 小屏 | 正点原子、淘宝小屏模块 |
| FT6236 | I2C | 5 点 | 800×480 以内 | 4"~5" 中屏 | 常见 5" 屏模块 |
| GT911 | I2C | 10 点 | 1280×720 以内 | 4"~7" 屏 | 7" 工业屏 |
| CST816S | I2C | 1 点 | 240×240 以内 | 圆形小屏 | TTGO、手表屏 |
| XPT2046 | SPI | 电阻屏 | — | 2.4"~3.5" 电阻屏 | 廉价模块 |

**软件 I2C vs 硬件 I2C 用于触摸**

| 对比项 | 软件 I2C | 硬件 I2C |
|--------|----------|----------|
| 引脚限制 | ✅ 任意 GPIO | ❌ 固定引脚 |
| 驱动复杂度 | ✅ 透明易调试 | 中等 |
| 稳定性 | ❌ 中断可能干扰时序 | ✅ 硬件保证 |
| 推荐场景 | 快速原型 | 正式产品 |

---

## 📚 相关文件

触摸驱动用到的头文件和 HAL 依赖：

- **触摸 IC 通信**：`stm32f1xx_hal_i2c.h`
- **中断引脚**：`stm32f1xx_hal_gpio.h` + `stm32f1xx_hal_exti.h`
- **坐标映射**：用户层软件逻辑

---

## 🔧 硬件连接

### FT6206 典型接线

| 触摸 IC 引脚 | MCU 引脚 | 说明 |
|-------------|---------|------|
| SDA | PB7（I2C1_SDA）| 数据线，需 4.7kΩ 上拉到 3.3V |
| SCL | PB6（I2C1_SCL）| 时钟线，需 4.7kΩ 上拉到 3.3V |
| INT | PA0（EXTI0）| 触摸事件中断，下降沿触发 |
| RST | PB0（GPIO输出）| 复位引脚，低电平复位 |
| VCC | 3.3V | — |
| GND | GND | — |

> FT6206 的 I2C 地址固定为 **0x38**（7 位），发送时左移 1 位为 `0x70`（写）/ `0x71`（读）。

### GT911 典型接线

| 触摸 IC 引脚 | MCU 引脚 | 说明 |
|-------------|---------|------|
| SDA | PB7 | 同上 |
| SCL | PB6 | 同上 |
| INT | PA1 | 兼作地址配置引脚（上电时序决定地址） |
| RST | PB1 | 复位引脚 |

> GT911 地址由上电时序决定：复位时 INT 拉低 → 地址 **0x5D**；INT 拉高 → 地址 **0x14**。

---

## 🎯 FT6206 驱动

### 寄存器速查

| 寄存器地址 | 说明 |
|-----------|------|
| 0x00 | 设备模式 |
| 0x01 | 手势 ID |
| 0x02 | 当前触点数量 |
| 0x03~0x04 | 触点 1 X 坐标高低字节（含事件标志）|
| 0x05~0x06 | 触点 1 Y 坐标高低字节（含触点 ID）|
| 0x09~0x0A | 触点 2 X 坐标 |
| 0x0B~0x0C | 触点 2 Y 坐标 |
| 0xA1 | 固件版本 |
| 0xA8 | 屏幕 X 分辨率高字节 |
| 0xA9 | 屏幕 X 分辨率低字节 |

---

### 驱动代码

```c
/* ft6206.h */
#ifndef FT6206_H
#define FT6206_H

#include "stm32f1xx_hal.h"

#define FT6206_I2C_ADDR     (0x38 << 1)  /* HAL 要求左移 1 位 */
#define FT6206_MAX_POINTS   2

/* 触点事件类型 */
#define FT6206_EVT_PRESS_DOWN   0x00
#define FT6206_EVT_LIFT_UP      0x01
#define FT6206_EVT_CONTACT      0x02
#define FT6206_EVT_NO_EVENT     0x03

typedef struct {
    uint8_t  event;    /* 事件类型（按下/抬起/接触）*/
    uint16_t x;        /* X 坐标 */
    uint16_t y;        /* Y 坐标 */
    uint8_t  id;       /* 触点 ID（多点区分用）*/
} FT6206_Point_t;

typedef struct {
    uint8_t        touch_num;                  /* 当前触点数 */
    FT6206_Point_t points[FT6206_MAX_POINTS];  /* 各触点数据 */
} FT6206_TouchData_t;

/* 外部依赖：hi2c 句柄 */
extern I2C_HandleTypeDef hi2c1;

HAL_StatusTypeDef FT6206_Init(void);
HAL_StatusTypeDef FT6206_ReadTouchData(FT6206_TouchData_t *touch);

#endif /* FT6206_H */
```

```c
/* ft6206.c */
#include "ft6206.h"

/* 复位引脚定义，修改为实际引脚 */
#define FT6206_RST_PORT  GPIOB
#define FT6206_RST_PIN   GPIO_PIN_0

/**
 * @brief 读取 FT6206 寄存器
 */
static HAL_StatusTypeDef FT6206_ReadReg(uint8_t reg, uint8_t *buf, uint8_t len) {
    return HAL_I2C_Mem_Read(&hi2c1, FT6206_I2C_ADDR,
                            reg, I2C_MEMADD_SIZE_8BIT,
                            buf, len, 100);
}

/**
 * @brief 写入 FT6206 寄存器
 */
static HAL_StatusTypeDef FT6206_WriteReg(uint8_t reg, uint8_t val) {
    return HAL_I2C_Mem_Write(&hi2c1, FT6206_I2C_ADDR,
                             reg, I2C_MEMADD_SIZE_8BIT,
                             &val, 1, 100);
}

/**
 * @brief 初始化 FT6206
 *        需在 I2C 和 GPIO 初始化完成后调用
 */
HAL_StatusTypeDef FT6206_Init(void) {
    /* 硬件复位 */
    HAL_GPIO_WritePin(FT6206_RST_PORT, FT6206_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(FT6206_RST_PORT, FT6206_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(200);  /* 等待 FT6206 启动完成 */

    /* 验证设备是否就绪 */
    if (HAL_I2C_IsDeviceReady(&hi2c1, FT6206_I2C_ADDR, 3, 100) != HAL_OK) {
        return HAL_ERROR;
    }

    /* 配置工作模式为正常模式 */
    FT6206_WriteReg(0x00, 0x00);

    return HAL_OK;
}

/**
 * @brief 读取触摸数据（可在中断或轮询中调用）
 * @param touch 输出触摸数据结构体
 */
HAL_StatusTypeDef FT6206_ReadTouchData(FT6206_TouchData_t *touch) {
    uint8_t buf[13] = {0};

    /* 一次性读取触点数量 + 两个触点的全部原始数据 */
    if (FT6206_ReadReg(0x02, buf, 13) != HAL_OK) {
        return HAL_ERROR;
    }

    touch->touch_num = buf[0] & 0x0F;
    if (touch->touch_num > FT6206_MAX_POINTS) {
        touch->touch_num = FT6206_MAX_POINTS;
    }

    /* 解析每个触点 */
    for (uint8_t i = 0; i < touch->touch_num; i++) {
        uint8_t offset = i * 6;  /* 每个触点占 6 字节，从 buf[1] 开始 */
        touch->points[i].event = (buf[1 + offset] >> 6) & 0x03;
        touch->points[i].x     = ((buf[1 + offset] & 0x0F) << 8) | buf[2 + offset];
        touch->points[i].id    = (buf[3 + offset] >> 4) & 0x0F;
        touch->points[i].y     = ((buf[3 + offset] & 0x0F) << 8) | buf[4 + offset];
    }

    return HAL_OK;
}
```

---

## 🎯 GT911 驱动

### 地址配置时序

GT911 上电时通过 INT/RST 时序选择 I2C 地址：

```c
/* 地址 0x5D：复位时 INT 引脚保持低电平 */
void GT911_SetAddr_0x5D(void) {
    /* INT 拉低 */
    HAL_GPIO_WritePin(GT911_INT_PORT, GT911_INT_PIN, GPIO_PIN_RESET);
    /* RST 拉低复位 */
    HAL_GPIO_WritePin(GT911_RST_PORT, GT911_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(10);
    /* 释放 RST */
    HAL_GPIO_WritePin(GT911_RST_PORT, GT911_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(10);
    /* 释放 INT，切换为输入 */
    /* 此后将 INT 配置为浮空输入或中断输入 */
    HAL_Delay(50);
}
```

### 关键寄存器

| 寄存器地址 | 说明 |
|-----------|------|
| 0x8047 | 状态寄存器（触点数 + 有无新数据）|
| 0x8048~0x814F | 最多 10 个触点坐标数据（每个 8 字节）|
| 0x8040~0x8046 | 产品 ID |
| 0x8047 | 读完后必须清零（否则不更新新数据）|

### GT911 读取触摸数据

```c
#define GT911_I2C_ADDR      (0x5D << 1)
#define GT911_STATUS_REG    0x8047
#define GT911_DATA_REG      0x8048
#define GT911_MAX_POINTS    5   /* 实际使用最多 5 点即可 */

typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t size;   /* 触点面积（可用于区分手指/手掌）*/
    uint8_t  id;
} GT911_Point_t;

typedef struct {
    uint8_t        touch_num;
    GT911_Point_t  points[GT911_MAX_POINTS];
} GT911_TouchData_t;

/**
 * @brief 读取 GT911 触摸数据
 */
HAL_StatusTypeDef GT911_ReadTouchData(GT911_TouchData_t *touch) {
    uint8_t status;
    uint8_t buf[8];

    /* 读状态寄存器 */
    if (HAL_I2C_Mem_Read(&hi2c1, GT911_I2C_ADDR,
                         GT911_STATUS_REG, I2C_MEMADD_SIZE_16BIT,
                         &status, 1, 100) != HAL_OK) {
        return HAL_ERROR;
    }

    /* bit7 为 1 表示有新数据 */
    if (!(status & 0x80)) {
        touch->touch_num = 0;
        return HAL_OK;
    }

    touch->touch_num = status & 0x0F;
    if (touch->touch_num > GT911_MAX_POINTS) {
        touch->touch_num = GT911_MAX_POINTS;
    }

    /* 读取各触点坐标（每点 8 字节：id,x_l,x_h,y_l,y_h,size_l,size_h,reserved）*/
    for (uint8_t i = 0; i < touch->touch_num; i++) {
        uint16_t reg = GT911_DATA_REG + i * 8;
        HAL_I2C_Mem_Read(&hi2c1, GT911_I2C_ADDR,
                         reg, I2C_MEMADD_SIZE_16BIT,
                         buf, 8, 100);

        touch->points[i].id   = buf[0];
        touch->points[i].x    = (uint16_t)(buf[1] | (buf[2] << 8));
        touch->points[i].y    = (uint16_t)(buf[3] | (buf[4] << 8));
        touch->points[i].size = (uint16_t)(buf[5] | (buf[6] << 8));
    }

    /* 清除状态寄存器，通知 IC 可以更新下一帧数据 */
    status = 0x00;
    HAL_I2C_Mem_Write(&hi2c1, GT911_I2C_ADDR,
                      GT911_STATUS_REG, I2C_MEMADD_SIZE_16BIT,
                      &status, 1, 100);

    return HAL_OK;
}
```

---

## 💡 完整应用示例

### 示例1：中断驱动的触摸检测（FT6206）

```c
/* 中断回调：INT 引脚下降沿触发 */
volatile uint8_t touch_flag = 0;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_0) {  /* INT 引脚 PA0 */
        touch_flag = 1;
    }
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_USART1_UART_Init();

    if (FT6206_Init() != HAL_OK) {
        printf("FT6206 初始化失败\r\n");
        Error_Handler();
    }
    printf("FT6206 初始化成功\r\n");

    FT6206_TouchData_t touch;

    while (1) {
        if (touch_flag) {
            touch_flag = 0;
            FT6206_ReadTouchData(&touch);

            if (touch.touch_num > 0) {
                printf("触点数: %d  X=%d Y=%d\r\n",
                       touch.touch_num,
                       touch.points[0].x,
                       touch.points[0].y);
            }
        }
    }
}
```

---

### 示例2：轮询模式（适合简单项目）

```c
void Touch_Poll(void) {
    FT6206_TouchData_t touch;

    /* 通过检查 INT 引脚电平代替中断 */
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET) {
        FT6206_ReadTouchData(&touch);

        if (touch.touch_num > 0 &&
            touch.points[0].event == FT6206_EVT_PRESS_DOWN) {
            /* 按下事件处理 */
            Handle_TouchDown(touch.points[0].x, touch.points[0].y);
        }
    }
}
```

---

### 示例3：坐标映射（旋转 / 镜像）

实际屏幕安装方向可能与触摸 IC 坐标系不一致，需要软件映射：

```c
#define LCD_WIDTH   240
#define LCD_HEIGHT  320

typedef enum {
    ROTATION_0   = 0,   /* 默认方向 */
    ROTATION_90  = 1,   /* 顺时针 90° */
    ROTATION_180 = 2,   /* 180° */
    ROTATION_270 = 3,   /* 顺时针 270° */
} LCD_Rotation_t;

/**
 * @brief 将触摸原始坐标转换为屏幕坐标
 */
void Touch_MapCoord(uint16_t raw_x, uint16_t raw_y,
                    uint16_t *scr_x, uint16_t *scr_y,
                    LCD_Rotation_t rotation) {
    switch (rotation) {
        case ROTATION_0:
            *scr_x = raw_x;
            *scr_y = raw_y;
            break;
        case ROTATION_90:
            *scr_x = LCD_HEIGHT - raw_y;
            *scr_y = raw_x;
            break;
        case ROTATION_180:
            *scr_x = LCD_WIDTH  - raw_x;
            *scr_y = LCD_HEIGHT - raw_y;
            break;
        case ROTATION_270:
            *scr_x = raw_y;
            *scr_y = LCD_WIDTH - raw_x;
            break;
    }
}
```

---

### 示例4：配合 LVGL 使用

LVGL 需要注册一个输入设备读取回调：

```c
#include "lvgl.h"

static void ft6206_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    FT6206_TouchData_t touch;
    FT6206_ReadTouchData(&touch);

    if (touch.touch_num > 0 &&
        touch.points[0].event != FT6206_EVT_LIFT_UP) {
        data->point.x = touch.points[0].x;
        data->point.y = touch.points[0].y;
        data->state   = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void LVGL_TouchInit(void) {
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = ft6206_read_cb;
    lv_indev_drv_register(&indev_drv);
}
```

---

## ⚠️ 注意事项

### 1. I2C 上拉电阻

触摸 IC 的 I2C 总线需外接 4.7kΩ 上拉电阻到 3.3V，不能依赖 MCU 内部上拉（驱动能力不足）。

---

### 2. GT911 状态寄存器必须手动清零

GT911 每次读取完触摸数据后，**必须将 0x8047 写 0x00**，否则触摸 IC 不会刷新下一帧坐标数据，坐标将一直卡在上次读到的值。

---

### 3. INT 引脚双重作用（GT911）

GT911 的 INT 引脚在上电复位期间用于配置 I2C 地址，复位完成后才能改配为中断输入。驱动初始化顺序：

```
1. INT 配置为推挽输出（配置地址）
2. 执行复位时序（拉低/拉高 RST，INT 保持目标电平）
3. 等待 50ms 稳定
4. 将 INT 重新配置为输入（可接中断）
```

---

### 4. 坐标超出屏幕范围

触摸 IC 报告的坐标可能超过屏幕分辨率（边缘触摸或贴膜导致），需做边界裁剪：

```c
if (x >= LCD_WIDTH)  x = LCD_WIDTH  - 1;
if (y >= LCD_HEIGHT) y = LCD_HEIGHT - 1;
```

---

### 5. 防抖与消抖

电容屏在手指快速滑动时可能产生噪点坐标，可用简单平均滤波：

```c
#define FILTER_SIZE 3
static uint16_t x_buf[FILTER_SIZE], y_buf[FILTER_SIZE];
static uint8_t  buf_idx = 0;

void Touch_Filter(uint16_t raw_x, uint16_t raw_y,
                  uint16_t *out_x, uint16_t *out_y) {
    x_buf[buf_idx] = raw_x;
    y_buf[buf_idx] = raw_y;
    buf_idx = (buf_idx + 1) % FILTER_SIZE;

    uint32_t sx = 0, sy = 0;
    for (int i = 0; i < FILTER_SIZE; i++) {
        sx += x_buf[i];
        sy += y_buf[i];
    }
    *out_x = (uint16_t)(sx / FILTER_SIZE);
    *out_y = (uint16_t)(sy / FILTER_SIZE);
}
```

---

## 📖 相关文档

- [I2C 总线](HAL_I2C.md)
- [EXTI 外部中断](HAL_EXTI.md)
- [GPIO 通用 IO](HAL_GPIO.md)
- [SPI 总线（显示子系统）](HAL_SPI.md)
