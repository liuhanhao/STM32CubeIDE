# CORTEX内核模块 API文档

## 📋 模块概述

CORTEX模块提供Cortex-M3内核相关功能:
- NVIC中断控制
- SysTick系统定时器
- MPU内存保护单元(部分型号)
- 系统控制

---

## 🗺️ 使用场景速查

**中断优先级怎么分配**

推荐使用 `NVIC_PRIORITYGROUP_4`（4 位抢占，0 位子优先级），子优先级全设 0，只用抢占优先级：

| 抢占优先级 | 分配给 |
|-----------|--------|
| 0 | 紧急中断：编码器高速捕获、安全保护（急停） |
| 1 | 高优先级：定时器心跳（TIM）、CAN RX |
| 2 | 中优先级：UART、SPI、I2C 传输完成 |
| 3 | 低优先级：EXTI 按键、ADC 转换完成 |
| 4 ~ 15 | 最低：DMA 完成、后台任务触发 |

> 优先级数字越小 = 越高（0 最高，15 最低）。高优先级中断可以打断低优先级中断执行。

**常见问题速查**

| 问题 | 原因 | 解决方法 |
|------|------|----------|
| `HAL_Delay` 在中断里卡住不动 | SysTick 优先级低于当前中断，SysTick 被阻塞 | 将 SysTick 设为优先级 0，或中断里改用 DWT 延时 |
| 两个中断同时触发，低优先级先响应 | 优先级数值配反了 | 检查 `PreemptPriority`，数值越小越高 |
| FreeRTOS + HAL 同时使用时系统卡死 | FreeRTOS 要求内核中断优先级最低 | 按 FreeRTOS 文档设置 `configKERNEL_INTERRUPT_PRIORITY = 15` |

**临界区保护（共享资源访问）**

```c
// 方法1：禁用全局中断（最简单，影响所有中断）
__disable_irq();
// ... 临界区代码（尽量短）...
__enable_irq();

// 方法2：只禁用特定中断（影响面小，推荐）
HAL_NVIC_DisableIRQ(USART1_IRQn);
// ... 操作共享的 UART 缓冲区 ...
HAL_NVIC_EnableIRQ(USART1_IRQn);
```

> 临界区内不能调用 `HAL_Delay`，也不要做耗时操作，否则会影响其他中断响应时间。

---

## 📚 相关文件

- **头文件**: `stm32f1xx_hal_cortex.h`
- **源文件**: `stm32f1xx_hal_cortex.c`

---

## 🎯 核心函数

### NVIC中断控制

#### HAL_NVIC_SetPriorityGrouping()

设置中断优先级分组。

```c
void HAL_NVIC_SetPriorityGrouping(uint32_t PriorityGroup);
```

**参数:**
- `NVIC_PRIORITYGROUP_0`: 0位抢占优先级,4位子优先级
- `NVIC_PRIORITYGROUP_1`: 1位抢占优先级,3位子优先级
- `NVIC_PRIORITYGROUP_2`: 2位抢占优先级,2位子优先级
- `NVIC_PRIORITYGROUP_3`: 3位抢占优先级,1位子优先级
- `NVIC_PRIORITYGROUP_4`: 4位抢占优先级,0位子优先级

---

#### HAL_NVIC_SetPriority()

设置中断优先级。

```c
void HAL_NVIC_SetPriority(IRQn_Type IRQn, 
                          uint32_t PreemptPriority,
                          uint32_t SubPriority);
```

---

#### HAL_NVIC_EnableIRQ()

使能中断。

```c
void HAL_NVIC_EnableIRQ(IRQn_Type IRQn);
```

---

#### HAL_NVIC_DisableIRQ()

禁用中断。

```c
void HAL_NVIC_DisableIRQ(IRQn_Type IRQn);
```

---

### SysTick定时器

#### HAL_SYSTICK_Config()

配置SysTick。

```c
uint32_t HAL_SYSTICK_Config(uint32_t TicksNumb);
```

---

#### HAL_SYSTICK_CLKSourceConfig()

配置SysTick时钟源。

```c
void HAL_SYSTICK_CLKSourceConfig(uint32_t CLKSource);
```

---

## 💡 应用示例

### 示例1: 中断优先级配置

```c
void NVIC_Config(void) {
    // 设置优先级分组(4位抢占优先级)
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
    
    // 配置EXTI0中断
    HAL_NVIC_SetPriority(EXTI0_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
    
    // 配置TIM2中断
    HAL_NVIC_SetPriority(TIM2_IRQn, 1, 0);  // 优先级高于EXTI0
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
    
    // 配置UART中断
    HAL_NVIC_SetPriority(USART1_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
}
```

---

### 示例2: 临时禁用中断

```c
void Critical_Section(void) {
    // 禁用全局中断
    __disable_irq();
    
    // 临界区代码
    // ...
    
    // 恢复全局中断
    __enable_irq();
}

void Disable_Specific_IRQ(void) {
    // 禁用特定中断
    HAL_NVIC_DisableIRQ(EXTI0_IRQn);
    
    // 执行操作
    // ...
    
    // 重新使能中断
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}
```

---

## ⚠️ 注意事项

### 1. 优先级分组

优先级分组在系统初始化时设置一次,之后不应更改。

### 2. 抢占优先级

数值越小,优先级越高。抢占优先级高的中断可以打断低优先级中断。

### 3. 子优先级

当抢占优先级相同时,子优先级决定响应顺序,但不能相互打断。

---

## 📖 相关文档

- [HAL核心](HAL_Core.md)
- [EXTI外部中断](HAL_EXTI.md)
