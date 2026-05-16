# HAL核心模块 API文档

## 📋 模块概述

HAL核心模块提供STM32F1xx HAL库的基础功能,包括:
- HAL库初始化和反初始化
- 系统时钟节拍(Tick)管理
- 延时函数
- 调试支持
- 芯片信息读取

---

## 📚 相关文件

- **头文件**: `stm32f1xx_hal.h`, `stm32f1xx_hal_def.h`
- **源文件**: `stm32f1xx_hal.c`

---

## 🔧 数据类型

### HAL_StatusTypeDef - HAL状态类型

HAL函数的返回状态枚举。

```c
typedef enum {
    HAL_OK       = 0x00U,  // 操作成功
    HAL_ERROR    = 0x01U,  // 操作失败
    HAL_BUSY     = 0x02U,  // 外设忙
    HAL_TIMEOUT  = 0x03U   // 操作超时
} HAL_StatusTypeDef;
```

| 值 | 说明 |
|---|------|
| `HAL_OK` | 操作成功完成 |
| `HAL_ERROR` | 操作失败或参数错误 |
| `HAL_BUSY` | 外设正在处理其他操作 |
| `HAL_TIMEOUT` | 操作超时 |

---

### HAL_LockTypeDef - HAL锁类型

用于外设资源锁定的枚举。

```c
typedef enum {
    HAL_UNLOCKED = 0x00U,  // 未锁定
    HAL_LOCKED   = 0x01U   // 已锁定
} HAL_LockTypeDef;
```

---

### HAL_TickFreqTypeDef - 时钟节拍频率

系统时钟节拍的频率配置。

```c
typedef enum {
    HAL_TICK_FREQ_10HZ   = 100U,  // 10Hz (100ms周期)
    HAL_TICK_FREQ_100HZ  = 10U,   // 100Hz (10ms周期)
    HAL_TICK_FREQ_1KHZ   = 1U,    // 1KHz (1ms周期) - 默认
    HAL_TICK_FREQ_DEFAULT = HAL_TICK_FREQ_1KHZ
} HAL_TickFreqTypeDef;
```

---

## 🎯 核心函数

### 初始化函数

#### HAL_Init()

初始化HAL库。

```c
HAL_StatusTypeDef HAL_Init(void);
```

**功能说明:**
- 配置Flash预取缓冲区
- 配置SysTick定时器为1ms中断
- 设置NVIC优先级分组
- 调用用户定义的`HAL_MspInit()`

**返回值:**
- `HAL_OK`: 初始化成功

**使用示例:**
```c
int main(void) {
    // HAL库初始化(必须首先调用)
    HAL_Init();
    
    // 配置系统时钟
    SystemClock_Config();
    
    // 初始化外设
    // ...
}
```

---

#### HAL_DeInit()

反初始化HAL库。

```c
HAL_StatusTypeDef HAL_DeInit(void);
```

**功能说明:**
- 复位所有外设
- 禁用所有中断
- 调用用户定义的`HAL_MspDeInit()`

**返回值:**
- `HAL_OK`: 反初始化成功

---

#### HAL_MspInit()

HAL MSP(MCU Support Package)初始化函数。

```c
void HAL_MspInit(void);
```

**功能说明:**
- 这是一个弱定义函数,用户需要在应用代码中重新实现
- 用于初始化全局的底层硬件资源(GPIO时钟、中断等)
- 在`HAL_Init()`中被调用

**使用示例:**
```c
// 用户实现
void HAL_MspInit(void) {
    // 使能PWR时钟
    __HAL_RCC_PWR_CLK_ENABLE();
    
    // 配置系统中断优先级
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
    
    // 其他全局初始化
}
```

---

#### HAL_MspDeInit()

HAL MSP反初始化函数。

```c
void HAL_MspDeInit(void);
```

**功能说明:**
- 弱定义函数,用户可重新实现
- 用于反初始化全局底层硬件资源
- 在`HAL_DeInit()`中被调用

---

### 时钟节拍(Tick)函数

#### HAL_InitTick()

初始化时钟节拍源。

```c
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority);
```

**参数:**
- `TickPriority`: 时钟节拍中断优先级

**返回值:**
- `HAL_OK`: 初始化成功

**说明:**
- 默认使用SysTick定时器
- 用户可以重新实现此函数以使用其他定时器

---

#### HAL_IncTick()

递增时钟节拍计数器。

```c
void HAL_IncTick(void);
```

**功能说明:**
- 在SysTick中断中调用
- 每次调用将`uwTick`变量加1
- 用户通常不需要直接调用此函数

**使用示例:**
```c
// SysTick中断处理函数
void SysTick_Handler(void) {
    HAL_IncTick();
}
```

---

#### HAL_GetTick()

获取当前时钟节拍值。

```c
uint32_t HAL_GetTick(void);
```

**返回值:**
- 当前时钟节拍计数值(单位:ms,默认配置下)

**使用示例:**
```c
uint32_t start_tick = HAL_GetTick();
// 执行某些操作
uint32_t elapsed = HAL_GetTick() - start_tick;
printf("耗时: %lu ms\n", elapsed);
```

---

#### HAL_Delay()

延时函数。

```c
void HAL_Delay(uint32_t Delay);
```

**参数:**
- `Delay`: 延时时间(单位:ms)

**功能说明:**
- 基于SysTick的阻塞延时
- 在延时期间CPU处于空闲状态

**使用示例:**
```c
// LED闪烁
while (1) {
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    HAL_Delay(500);  // 延时500ms
}
```

⚠️ **注意**: 此函数为阻塞延时,在RTOS环境下应使用操作系统的延时函数。

---

#### HAL_SuspendTick()

暂停时钟节拍。

```c
void HAL_SuspendTick(void);
```

**功能说明:**
- 禁用SysTick中断
- 用于低功耗模式

---

#### HAL_ResumeTick()

恢复时钟节拍。

```c
void HAL_ResumeTick(void);
```

**功能说明:**
- 重新使能SysTick中断
- 从低功耗模式唤醒后调用

---

#### HAL_SetTickFreq()

设置时钟节拍频率。

```c
HAL_StatusTypeDef HAL_SetTickFreq(HAL_TickFreqTypeDef Freq);
```

**参数:**
- `Freq`: 时钟节拍频率
  - `HAL_TICK_FREQ_10HZ`: 10Hz
  - `HAL_TICK_FREQ_100HZ`: 100Hz
  - `HAL_TICK_FREQ_1KHZ`: 1KHz(默认)

**返回值:**
- `HAL_OK`: 设置成功

---

#### HAL_GetTickFreq()

获取时钟节拍频率。

```c
HAL_TickFreqTypeDef HAL_GetTickFreq(void);
```

**返回值:**
- 当前时钟节拍频率

---

### 芯片信息函数

#### HAL_GetHalVersion()

获取HAL库版本。

```c
uint32_t HAL_GetHalVersion(void);
```

**返回值:**
- HAL库版本号(格式: 0xMMmmppss)
  - MM: 主版本
  - mm: 次版本
  - pp: 补丁版本
  - ss: 候选版本

---

#### HAL_GetREVID()

获取芯片修订版本ID。

```c
uint32_t HAL_GetREVID(void);
```

**返回值:**
- 芯片修订版本ID

---

#### HAL_GetDEVID()

获取芯片设备ID。

```c
uint32_t HAL_GetDEVID(void);
```

**返回值:**
- 芯片设备ID

**使用示例:**
```c
uint32_t dev_id = HAL_GetDEVID();
switch (dev_id) {
    case 0x410: printf("STM32F103 Medium-density\n"); break;
    case 0x414: printf("STM32F103 High-density\n"); break;
    case 0x418: printf("STM32F105/F107\n"); break;
    default: printf("Unknown device\n"); break;
}
```

---

#### HAL_GetUIDw0/1/2()

获取芯片唯一ID。

```c
uint32_t HAL_GetUIDw0(void);  // 获取UID第1个字(96位UID的低32位)
uint32_t HAL_GetUIDw1(void);  // 获取UID第2个字(96位UID的中32位)
uint32_t HAL_GetUIDw2(void);  // 获取UID第3个字(96位UID的高32位)
```

**返回值:**
- 芯片唯一ID的对应部分

**使用示例:**
```c
// 读取96位唯一ID
uint32_t uid[3];
uid[0] = HAL_GetUIDw0();
uid[1] = HAL_GetUIDw1();
uid[2] = HAL_GetUIDw2();

printf("芯片UID: %08lX-%08lX-%08lX\n", uid[2], uid[1], uid[0]);
```

---

### 调试支持函数

#### HAL_DBGMCU_EnableDBGSleepMode()

使能睡眠模式下的调试。

```c
void HAL_DBGMCU_EnableDBGSleepMode(void);
```

**功能说明:**
- 允许在睡眠模式下继续调试
- 会增加功耗

---

#### HAL_DBGMCU_DisableDBGSleepMode()

禁用睡眠模式下的调试。

```c
void HAL_DBGMCU_DisableDBGSleepMode(void);
```

---

#### HAL_DBGMCU_EnableDBGStopMode()

使能停止模式下的调试。

```c
void HAL_DBGMCU_EnableDBGStopMode(void);
```

---

#### HAL_DBGMCU_DisableDBGStopMode()

禁用停止模式下的调试。

```c
void HAL_DBGMCU_DisableDBGStopMode(void);
```

---

#### HAL_DBGMCU_EnableDBGStandbyMode()

使能待机模式下的调试。

```c
void HAL_DBGMCU_EnableDBGStandbyMode(void);
```

---

#### HAL_DBGMCU_DisableDBGStandbyMode()

禁用待机模式下的调试。

```c
void HAL_DBGMCU_DisableDBGStandbyMode(void);
```

---

## 🔍 常用宏定义

### HAL_MAX_DELAY

最大延时值。

```c
#define HAL_MAX_DELAY  0xFFFFFFFFU
```

**说明**: 用于表示无限等待(阻塞模式)。

---

### HAL_IS_BIT_SET / HAL_IS_BIT_CLR

检查寄存器位状态。

```c
#define HAL_IS_BIT_SET(REG, BIT)  (((REG) & (BIT)) != 0U)
#define HAL_IS_BIT_CLR(REG, BIT)  (((REG) & (BIT)) == 0U)
```

**使用示例:**
```c
if (HAL_IS_BIT_SET(GPIOC->IDR, GPIO_PIN_13)) {
    // PC13引脚为高电平
}
```

---

### __HAL_LOCK / __HAL_UNLOCK

外设锁定/解锁宏。

```c
#define __HAL_LOCK(__HANDLE__)    // 锁定外设句柄
#define __HAL_UNLOCK(__HANDLE__)  // 解锁外设句柄
```

**说明**: 用于防止外设被多个任务同时访问。

---

### __HAL_LINKDMA

链接DMA句柄到外设句柄。

```c
#define __HAL_LINKDMA(__HANDLE__, __PPP_DMA_FIELD__, __DMA_HANDLE__)
```

**使用示例:**
```c
// 将DMA句柄链接到UART句柄
__HAL_LINKDMA(&huart1, hdmatx, hdma_usart1_tx);
```

---

### UNUSED

标记未使用的变量。

```c
#define UNUSED(X)  (void)X
```

**使用示例:**
```c
void callback(uint32_t param) {
    UNUSED(param);  // 避免编译器警告
    // 函数实现
}
```

---

## 🔧 调试宏

### 定时器调试冻结

在调试模式下冻结定时器,便于单步调试。

```c
__HAL_DBGMCU_FREEZE_TIM2()    // 冻结TIM2
__HAL_DBGMCU_UNFREEZE_TIM2()  // 解冻TIM2
// TIM3, TIM4, TIM5... 类似
```

---

### 看门狗调试冻结

```c
__HAL_DBGMCU_FREEZE_IWDG()    // 冻结独立看门狗
__HAL_DBGMCU_FREEZE_WWDG()    // 冻结窗口看门狗
```

---

### I2C调试冻结

```c
__HAL_DBGMCU_FREEZE_I2C1_TIMEOUT()  // 冻结I2C1超时
__HAL_DBGMCU_FREEZE_I2C2_TIMEOUT()  // 冻结I2C2超时
```

---

## 💡 完整应用示例

### 示例1: 非阻塞超时等待(替代 HAL_Delay)

`HAL_Delay()` 会阻塞 CPU，在需要同时处理多件事时应改用 `HAL_GetTick()`。

```c
// ❌ 阻塞写法：等待期间 CPU 什么都做不了
void bad_blink(void) {
    while (1) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        HAL_Delay(500);  // CPU 卡在这里 500ms
    }
}

// ✅ 非阻塞写法：CPU 可以同时处理其他事情
void good_blink(void) {
    static uint32_t last_tick = 0;
    
    // 每次进入函数都检查，不阻塞
    if (HAL_GetTick() - last_tick >= 500) {
        last_tick = HAL_GetTick();
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    }
}

// 主循环中可以同时处理多个定时任务
int main(void) {
    HAL_Init();
    SystemClock_Config();
    
    uint32_t led_tick = 0;
    uint32_t sensor_tick = 0;
    
    while (1) {
        // LED 每 500ms 闪烁一次
        if (HAL_GetTick() - led_tick >= 500) {
            led_tick = HAL_GetTick();
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        }
        
        // 传感器每 100ms 采样一次
        if (HAL_GetTick() - sensor_tick >= 100) {
            sensor_tick = HAL_GetTick();
            // read_sensor();
        }
        
        // 其他任务...
    }
}
```

---

### 示例2: 用芯片唯一 ID 实现设备序列号

每颗 STM32 都有一个出厂烧录的 96 位唯一 ID，可用于设备认证、序列号生成。

```c
// 生成设备序列号字符串
void Get_Device_SerialNumber(char *buf, uint32_t buf_size) {
    uint32_t uid0 = HAL_GetUIDw0();
    uint32_t uid1 = HAL_GetUIDw1();
    uint32_t uid2 = HAL_GetUIDw2();
    
    // 格式化为 24 位十六进制字符串
    snprintf(buf, buf_size, "%08lX%08lX%08lX", uid2, uid1, uid0);
}

// 简单的设备授权检查
uint8_t Check_Device_Auth(void) {
    // 将 UID 与预存的授权列表比对
    uint32_t uid0 = HAL_GetUIDw0();
    uint32_t uid1 = HAL_GetUIDw1();
    
    // 示例：只允许特定 UID 的设备运行
    if (uid0 == 0x12345678 && uid1 == 0xABCDEF01) {
        return 1;  // 授权通过
    }
    return 0;  // 未授权
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    
    char serial[25];
    Get_Device_SerialNumber(serial, sizeof(serial));
    printf("设备序列号: %s\n", serial);
    
    // 打印芯片型号
    uint32_t dev_id = HAL_GetDEVID();
    switch (dev_id) {
        case 0x410: printf("芯片: STM32F103 中容量\n"); break;
        case 0x414: printf("芯片: STM32F103 大容量\n"); break;
        case 0x418: printf("芯片: STM32F105/F107\n"); break;
        default:    printf("芯片 ID: 0x%lX\n", dev_id); break;
    }
}
```

---

### 示例3: 低功耗模式下的 Tick 管理

进入睡眠模式前暂停 Tick，唤醒后恢复，避免 SysTick 中断干扰低功耗。

```c
void Enter_LowPower_Sleep(void) {
    // 暂停 SysTick 中断，降低功耗
    HAL_SuspendTick();
    
    // 进入睡眠模式，等待任意中断唤醒
    // WFI = Wait For Interrupt
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
    
    // 被中断唤醒后，恢复 SysTick
    HAL_ResumeTick();
    
    // 此后 HAL_Delay()、HAL_GetTick() 恢复正常工作
}

// 实际使用：主循环空闲时进入睡眠
int main(void) {
    HAL_Init();
    SystemClock_Config();
    
    while (1) {
        if (has_work_to_do()) {
            process_work();
        } else {
            // 没有任务时进入睡眠，等待中断(如 EXTI 按键、定时器)唤醒
            Enter_LowPower_Sleep();
        }
    }
}
```

---

### 示例4: HAL 状态码的正确处理

```c
// 每个 HAL 函数都返回状态码，应该检查而不是忽略
void Robust_UART_Send(UART_HandleTypeDef *huart, uint8_t *data, uint16_t len) {
    HAL_StatusTypeDef ret = HAL_UART_Transmit(huart, data, len, 1000);
    
    switch (ret) {
        case HAL_OK:
            // 发送成功，继续
            break;
        case HAL_TIMEOUT:
            // 超时：可能是接收方没有准备好
            printf("UART 发送超时\n");
            HAL_UART_Abort(huart);  // 中止传输，释放外设
            break;
        case HAL_BUSY:
            // 外设忙：上一次传输还没完成
            printf("UART 忙，稍后重试\n");
            break;
        case HAL_ERROR:
            // 硬件错误：检查线路连接
            printf("UART 硬件错误\n");
            HAL_UART_DeInit(huart);
            HAL_UART_Init(huart);  // 尝试重新初始化
            break;
    }
}
```

```c
int main(void) {
    // 1. HAL库初始化
    HAL_Init();
    
    // 2. 配置系统时钟
    SystemClock_Config();
    
    // 3. 初始化外设
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    // ...
    
    // 4. 主循环
    while (1) {
        // 应用代码
    }
}
```

---

### 2. 超时处理

```c
uint32_t timeout = HAL_GetTick() + 1000;  // 1秒超时
while (condition) {
    if (HAL_GetTick() > timeout) {
        // 超时处理
        return HAL_TIMEOUT;
    }
}
```

---

### 3. 低功耗模式

```c
// 进入睡眠模式前
HAL_SuspendTick();
HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
// 唤醒后
HAL_ResumeTick();
```

---

## ⚠️ 注意事项

1. **必须首先调用** `HAL_Init()` 才能使用其他HAL函数
2. **SysTick中断优先级** 应设置为最低,避免影响其他中断
3. **HAL_Delay()** 依赖SysTick中断,确保中断已使能
4. **调试冻结宏** 仅在调试模式下有效
5. **RTOS环境** 下需要重新实现时钟节拍函数

---

## 📖 相关文档

- [RCC时钟控制](HAL_RCC.md)
- [GPIO通用IO](HAL_GPIO.md)
- [CORTEX内核](HAL_CORTEX.md)
