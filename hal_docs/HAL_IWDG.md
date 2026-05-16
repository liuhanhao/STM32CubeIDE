# IWDG独立看门狗模块 API文档

## 📋 模块概述

IWDG(Independent Watchdog)独立看门狗是一个硬件看门狗定时器,用于检测和解决软件故障:
- 独立的LSI时钟源(~40KHz)
- 12位递减计数器
- 可编程预分频器(4~256)
- 超时时间: 0.1ms ~ 26s
- 一旦启动无法停止

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
