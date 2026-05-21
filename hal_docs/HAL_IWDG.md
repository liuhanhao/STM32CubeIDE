# IWDG独立看门狗模块 API文档

## 📋 模块概述

IWDG(Independent Watchdog)独立看门狗是一个硬件看门狗定时器,用于检测和解决软件故障:
- 独立的LSI时钟源(~40KHz)
- 12位递减计数器
- 可编程预分频器(4~256)
- 超时时间: 0.1ms ~ 26s
- 一旦启动无法停止

---

## 🗺️ 使用场景速查

**为什么需要看门狗**

看门狗解决的核心问题：**程序跑飞或死循环后无法自动恢复**，必须有人手动断电。

| 场景 | 说明 |
|------|------|
| 正式产品（无人值守设备） | 程序异常后自动复位，无需人工干预 |
| 通信等待超时保护 | 长时间等不到回包，看门狗超时后重新初始化 |
| 防止中断嵌套/栈溢出卡死 | 主循环跑不到喂狗就复位 |
| 低功耗设备（睡眠中也要防止异常） | IWDG 使用独立 LSI 时钟，停止/待机模式下继续运行 |

**IWDG vs WWDG 怎么选**

| | IWDG | WWDG |
|--|------|------|
| 检测"太晚喂狗"（程序死掉） | ✅ | ✅ |
| 检测"太早喂狗"（程序时序异常） | ❌ | ✅ |
| 低功耗模式下工作 | ✅（独立 LSI 时钟） | ❌（依赖 APB1） |
| 超时时间范围 | 0.1ms ~ 26s | 较短（毫秒级） |
| 推荐场景 | 通用防死机，首选 | 实时任务时序监控 |
| 两者同时使用 | 最高可靠性 | 同左 |

**超时时间设置建议**

- 超时时间 = 主循环最坏情况执行时间的 **2~3 倍**
- 喂狗位置放在主循环末尾，每圈喂一次
- 调试阶段必须加冻结，否则单步时触发复位：

```c
// 在 SystemClock_Config 之后、IWDG_Init 之前加
__HAL_DBGMCU_FREEZE_IWDG();
```

---

## 📚 相关文件

- **头文件**: `stm32f1xx_hal_iwdg.h`
- **源文件**: `stm32f1xx_hal_iwdg.c`

---

## 🎯 核心函数

### HAL_IWDG_Init()

初始化IWDG。

```c
HAL_StatusTypeDef HAL_IWDG_Init(IWDG_HandleTypeDef *hiwdg);
```

---

### HAL_IWDG_Refresh()

喂狗(重载计数器)。

```c
HAL_StatusTypeDef HAL_IWDG_Refresh(IWDG_HandleTypeDef *hiwdg);
```

---

## 💡 完整应用示例

### 示例1: 基本使用

```c
IWDG_HandleTypeDef hiwdg;

void IWDG_Init(void) {
    hiwdg.Instance = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_64;  // 64分频
    hiwdg.Init.Reload = 4095;  // 最大重载值
    // 超时时间 = (4095+1) * 64 / 40000 ≈ 6.5秒
    
    HAL_IWDG_Init(&hiwdg);
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    IWDG_Init();
    
    while (1) {
        // 正常工作
        HAL_Delay(1000);
        
        // 喂狗
        HAL_IWDG_Refresh(&hiwdg);
    }
}
```

---

### 示例2: 检测复位原因

```c
int main(void) {
    HAL_Init();
    
    // 检查是否由看门狗复位
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != RESET) {
        printf("看门狗复位!\n");
        __HAL_RCC_CLEAR_RESET_FLAGS();
    }
    
    SystemClock_Config();
    IWDG_Init();
    
    while (1) {
        // 主循环
        HAL_IWDG_Refresh(&hiwdg);
        HAL_Delay(1000);
    }
}
```

---

## ⚠️ 注意事项

### 1. 超时时间计算

```
超时时间(s) = (Reload + 1) × Prescaler / LSI频率(40000Hz)
```

### 2. 一旦启动无法停止

IWDG启动后即使进入调试模式也会继续运行(可配置调试时冻结)。

### 3. 喂狗频率

喂狗间隔必须小于超时时间,建议为超时时间的1/2。

---

## 📖 相关文档

- [HAL核心](HAL_Core.md)
- [WWDG窗口看门狗](HAL_WWDG.md)
