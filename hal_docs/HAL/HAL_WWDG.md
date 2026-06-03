# WWDG窗口看门狗模块 API文档

## 📋 模块概述

WWDG(Window Watchdog)窗口看门狗提供时间窗口检测:
- 基于APB1时钟
- 7位递减计数器
- 可配置时间窗口
- 过早或过晚喂狗都会复位
- 可产生中断

---

## 📚 相关文件

- **头文件**: `stm32f1xx_hal_wwdg.h`
- **源文件**: `stm32f1xx_hal_wwdg.c`

---

## 🎯 核心函数

### HAL_WWDG_Init()

初始化WWDG。

```c
HAL_StatusTypeDef HAL_WWDG_Init(WWDG_HandleTypeDef *hwwdg);
```

---

### HAL_WWDG_Refresh()

喂狗。

```c
HAL_StatusTypeDef HAL_WWDG_Refresh(WWDG_HandleTypeDef *hwwdg);
```

---

## 💡 完整应用示例

### 示例1: 基本使用(带窗口时间计算)

```c
WWDG_HandleTypeDef hwwdg;

// WWDG 超时时间计算公式：
// T_wwdg(ms) = (1/PCLK1) × 4096 × 2^WDGTB × (Counter - 0x3F + 1) × 1000
//
// 以 PCLK1=36MHz，WDGTB=WWDG_PRESCALER_8 为例：
// 计数器从 127 递减到 63 的时间 = (1/36000000) × 4096 × 8 × 64 × 1000 ≈ 58ms
// 窗口值设为 80，则只有计数器在 80~63 之间才能喂狗
// 即：喂狗必须在 ~29ms ~ ~58ms 之间完成

void WWDG_Init(void) {
    hwwdg.Instance = WWDG;
    hwwdg.Init.Prescaler = WWDG_PRESCALER_8;  // 8 分频
    hwwdg.Init.Window = 80;    // 窗口上限：计数器必须 < 80 才能喂狗
    hwwdg.Init.Counter = 127;  // 初始计数值(最大 127 = 0x7F)
    hwwdg.Init.EWIMode = WWDG_EWI_ENABLE;  // 使能早期唤醒中断
    
    HAL_WWDG_Init(&hwwdg);
}

// 早期唤醒中断：计数器到达 0x40 时触发，此时还有最后一次喂狗机会
void WWDG_IRQHandler(void) {
    HAL_WWDG_IRQHandler(&hwwdg);
}

// 在早期唤醒回调中喂狗，确保不会复位
void HAL_WWDG_EarlyWakeupCallback(WWDG_HandleTypeDef *hwwdg) {
    HAL_WWDG_Refresh(hwwdg);  // 重载计数器到 127
}
```

---

### 示例2: 在主循环中定时喂狗(不使用中断)

适合任务执行时间固定、可以精确控制喂狗时机的场景。

```c
void WWDG_NoIT_Init(void) {
    hwwdg.Instance = WWDG;
    hwwdg.Init.Prescaler = WWDG_PRESCALER_8;
    hwwdg.Init.Window = 100;   // 窗口上限
    hwwdg.Init.Counter = 127;
    hwwdg.Init.EWIMode = WWDG_EWI_DISABLE;  // 不使用中断
    
    HAL_WWDG_Init(&hwwdg);
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    WWDG_NoIT_Init();
    
    while (1) {
        // 执行主任务(必须在窗口时间内完成)
        do_main_task();
        
        // 在窗口内喂狗(计数器值必须 < Window 值才有效)
        // 如果 do_main_task() 卡死或执行时间异常，这里就不会被执行，触发复位
        HAL_WWDG_Refresh(&hwwdg);
    }
}
```

---

### 示例3: WWDG 与 IWDG 对比及选择

```c
// ============ IWDG 独立看门狗 ============
// 特点：使用独立 LSI 时钟，停止/待机模式下仍运行
// 适合：检测系统完全死机（主时钟停了也能复位）
// 缺点：只能检测"太晚喂狗"，无法检测"太早喂狗"

IWDG_HandleTypeDef hiwdg;
hiwdg.Instance = IWDG;
hiwdg.Init.Prescaler = IWDG_PRESCALER_64;
hiwdg.Init.Reload = 4095;  // 超时约 6.5 秒
HAL_IWDG_Init(&hiwdg);

// 主循环中任意时刻喂狗都可以，只要不超时
while (1) {
    HAL_Delay(1000);
    HAL_IWDG_Refresh(&hiwdg);  // 只要在 6.5s 内喂就行
}

// ============ WWDG 窗口看门狗 ============
// 特点：使用 APB1 时钟，有时间窗口限制
// 适合：检测程序时序异常（太早或太晚喂狗都会复位）
// 缺点：停止模式下停止运行，超时时间较短

// 主循环中必须在特定时间窗口内喂狗
while (1) {
    HAL_Delay(30);         // 等待进入窗口期
    HAL_WWDG_Refresh(&hwwdg);  // 必须在窗口内喂
    HAL_Delay(20);         // 执行其他任务
}
```

> 💡 **选择建议：**
> - 防止系统完全死机 → 用 **IWDG**（独立时钟，更可靠）
> - 检测程序时序异常（如某个任务执行时间不对）→ 用 **WWDG**
> - 要求最高可靠性 → **两者同时使用**

---

## ⚠️ 注意事项

### 1. 窗口限制

只能在计数器值小于窗口值时喂狗,否则会复位。

### 2. 时钟源

WWDG使用APB1时钟,停止模式下会停止运行。

---

## 📖 相关文档

- [HAL核心](HAL_Core.md)
- [IWDG独立看门狗](HAL_IWDG.md)
