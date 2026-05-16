# EXTI外部中断模块 API文档

## 📋 模块概述

EXTI(External Interrupt/Event Controller)外部中断/事件控制器:
- 23条中断/事件线
- 支持上升沿、下降沿、双边沿触发
- 可配置为中断或事件模式
- 与GPIO引脚关联

---

## 📚 相关文件

- **头文件**: `stm32f1xx_hal_exti.h`
- **源文件**: `stm32f1xx_hal_exti.c`

---

## 🎯 EXTI线分配

- **EXTI0-15**: 对应GPIO引脚Px0-Px15
- **EXTI16**: PVD输出
- **EXTI17**: RTC闹钟
- **EXTI18**: USB唤醒
- **EXTI19**: 以太网唤醒

---

## 💡 应用示例

EXTI通常通过GPIO配置使用,详见[GPIO模块文档](HAL_GPIO.md)。

### 示例1: 按键触发外部中断(最常用场景)

按键一端接 GND，另一端接 PA0，按下时产生下降沿触发中断。

```c
void EXTI_Button_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    // PA0 配置为下降沿触发中断，内部上拉
    // 按键未按下时 PA0 被上拉为高电平
    // 按键按下时 PA0 被拉到 GND，产生下降沿
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // 配置 NVIC，优先级 2
    HAL_NVIC_SetPriority(EXTI0_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

// 中断服务函数(固定写法，调用 HAL 处理函数)
void EXTI0_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

// 回调函数(用户在这里写实际逻辑)
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_0) {
        // 简单消抖：再次确认引脚状态
        HAL_Delay(10);
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET) {
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);  // 翻转 LED
        }
    }
}
```

---

### 示例2: 多个引脚同时使用外部中断

不同引脚号对应不同 EXTI 线，可以同时使用。
同一引脚号（如 PA0 和 PB0）只能选其一。

```c
void EXTI_Multi_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    // PA0 → EXTI0，下降沿触发(按键1)
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // PB1 → EXTI1，下降沿触发(按键2)
    // 注意：PA1 和 PB1 共用 EXTI1，只能选一个
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    // PA13 → EXTI13，双边沿触发(检测信号变化)
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // 配置各自的 NVIC
    HAL_NVIC_SetPriority(EXTI0_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
    
    HAL_NVIC_SetPriority(EXTI1_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(EXTI1_IRQn);
    
    // PIN10~15 共用一个中断向量 EXTI15_10
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

// 各自的中断服务函数
void EXTI0_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

void EXTI1_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
}

// PIN10~15 共用一个 IRQHandler，需要在回调中区分
void EXTI15_10_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);
}

// 统一回调，通过 GPIO_Pin 区分来源
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_0) {
        printf("按键1按下\n");
    }
    else if (GPIO_Pin == GPIO_PIN_1) {
        printf("按键2按下\n");
    }
    else if (GPIO_Pin == GPIO_PIN_13) {
        // 双边沿：读取当前电平判断是上升还是下降
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_13) == GPIO_PIN_SET) {
            printf("PA13 上升沿\n");
        } else {
            printf("PA13 下降沿\n");
        }
    }
}
```

---

### 示例3: 用外部中断检测旋转编码器

旋转编码器输出 A、B 两相方波，通过 EXTI 捕获 A 相边沿，读取 B 相判断方向。

```c
static int32_t encoder_count = 0;  // 编码器计数值

void Encoder_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    // PA0 → 编码器 A 相，双边沿触发
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // PA1 → 编码器 B 相，普通输入(只读，不触发中断)
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);  // 最高优先级，避免漏脉冲
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

void EXTI0_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_0) {
        // A 相变化时读取 B 相，判断旋转方向
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_SET) {
            encoder_count++;   // 顺时针
        } else {
            encoder_count--;   // 逆时针
        }
    }
}
```

---

## ⚠️ 注意事项

### 1. EXTI线共享

每条EXTI线只能连接到一个GPIO端口的同号引脚。
例如: EXTI0只能连接PA0、PB0、PC0等之一。

### 2. 中断优先级

合理配置中断优先级避免冲突。

---

## 📖 相关文档

- [GPIO通用IO](HAL_GPIO.md)
- [HAL核心](HAL_Core.md)
