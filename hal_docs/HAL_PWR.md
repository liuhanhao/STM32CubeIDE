# PWR电源管理模块 API文档

## 📋 模块概述

PWR模块提供电源管理和低功耗模式控制,包括:
- 睡眠模式(Sleep Mode)
- 停止模式(Stop Mode)  
- 待机模式(Standby Mode)
- 电源电压检测(PVD)
- 备份域访问控制
- 唤醒引脚配置

---

## 🗺️ 使用场景速查

**三种低功耗模式怎么选**

| 模式 | 典型功耗 | 唤醒时间 | 适用场景 |
|------|---------|----------|----------|
| 睡眠（Sleep） | ~15mA | 立即（1 周期） | 等待中断，但外设（UART/ADC/DMA）需继续运行 |
| 停止（Stop） | ~2μA | 几 μs（唤醒后需重配时钟） | 长时间等待，只靠 EXTI 或 RTC 唤醒，外设可暂停 |
| 待机（Standby） | ~1μA | 几 ms（等同于复位流程） | 极低功耗，唤醒后从 main 重新执行，无需保留运行状态 |

**选择逻辑**

```
需要外设继续运行（DMA 采集、UART 接收）？
  → 睡眠模式（WFI）

外设可以暂停？恢复后需要继续之前状态？
  → 停止模式（唤醒后必须重新配置 PLL）

恢复后重新初始化无所谓？追求最低功耗？
  → 待机模式（通过 WKUP 引脚或 RTC 闹钟唤醒）
```

**停止模式唤醒后必须重配时钟**

停止模式下 PLL 和 HSE 关闭，唤醒后时钟自动切回 HSI（8MHz）：

```c
HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
// 被中断唤醒后继续执行
SystemClock_Config();  // 必须重配！否则系统运行在 8MHz
HAL_ResumeTick();
```

**PVD 使用场景**

电池供电设备 → 配置 PVD 在电压低于阈值（如 2.9V）时触发中断 → 保存关键数据到 Flash → 安全关机，防止数据因掉电丢失。

---

## 📚 相关文件

- **头文件**: `stm32f1xx_hal_pwr.h`
- **源文件**: `stm32f1xx_hal_pwr.c`

---

## 🔧 低功耗模式

### 睡眠模式 (Sleep Mode)

**特点:**
- CPU停止,所有外设继续运行
- 功耗最低的运行模式
- 任何中断或事件可唤醒

**进入方式:**
```c
HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
```

---

### 停止模式 (Stop Mode)

**特点:**
- 所有时钟停止
- PLL、HSI、HSE关闭
- SRAM和寄存器内容保持
- 功耗极低(约2μA)

**进入方式:**
```c
HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
```

---

### 待机模式 (Standby Mode)

**特点:**
- 1.8V域断电
- 仅保持备份寄存器和RTC
- 功耗最低(约1μA)
- 唤醒后相当于复位

**进入方式:**
```c
HAL_PWR_EnterSTANDBYMode();
```

---

## 🎯 核心函数

### HAL_PWR_EnableBkUpAccess()

使能备份域访问。

```c
void HAL_PWR_EnableBkUpAccess(void);
```

**说明**: 访问RTC和备份寄存器前必须调用。

---

### HAL_PWR_DisableBkUpAccess()

禁用备份域访问。

```c
void HAL_PWR_DisableBkUpAccess(void);
```

---

### HAL_PWR_EnableWakeUpPin()

使能唤醒引脚。

```c
void HAL_PWR_EnableWakeUpPin(uint32_t WakeUpPinx);
```

**参数:**
- `PWR_WAKEUP_PIN1`: PA0作为唤醒引脚

---

### HAL_PWR_ConfigPVD()

配置电源电压检测。

```c
void HAL_PWR_ConfigPVD(PWR_PVDTypeDef *sConfigPVD);
```

---

## 💡 完整应用示例

### 示例1: 睡眠模式

```c
void Enter_Sleep_Mode(void) {
    printf("进入睡眠模式...\n");
    
    // 暂停SysTick
    HAL_SuspendTick();
    
    // 进入睡眠模式
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
    
    // 唤醒后继续执行
    HAL_ResumeTick();
    
    printf("从睡眠模式唤醒\n");
}

// 配置唤醒源(外部中断)
void Wakeup_Config(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    // PA0配置为外部中断
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}
```

---

### 示例2: 停止模式

```c
void Enter_Stop_Mode(void) {
    printf("进入停止模式...\n");
    HAL_Delay(100);  // 等待串口发送完成
    
    // 暂停SysTick
    HAL_SuspendTick();
    
    // 进入停止模式
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
    
    // 唤醒后重新配置时钟(HSI 8MHz)
    SystemClock_Config();
    
    // 恢复SysTick
    HAL_ResumeTick();
    
    printf("从停止模式唤醒\n");
}

// 停止模式唤醒后的时钟配置
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    
    // 配置HSE和PLL
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);
    
    // 配置系统时钟
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}
```

---

### 示例3: 待机模式

```c
void Enter_Standby_Mode(void) {
    printf("进入待机模式...\n");
    HAL_Delay(100);
    
    // 使能唤醒引脚(PA0)
    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);
    
    // 清除唤醒标志
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
    
    // 进入待机模式
    HAL_PWR_EnterSTANDBYMode();
    
    // 注意: 此后代码不会执行,唤醒后从复位开始
}

// 在main函数开始检查复位原因
int main(void) {
    HAL_Init();
    SystemClock_Config();
    
    // 检查是否从待机模式唤醒
    if (__HAL_PWR_GET_FLAG(PWR_FLAG_SB) != RESET) {
        printf("从待机模式唤醒\n");
        __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);
        __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
    } else {
        printf("正常启动\n");
    }
    
    // 其他初始化...
    
    while (1) {
        // 主循环
    }
}
```

---

### 示例4: 电源电压检测(PVD)

```c
void PVD_Config(void) {
    PWR_PVDTypeDef sConfigPVD = {0};
    
    // 使能PWR时钟
    __HAL_RCC_PWR_CLK_ENABLE();
    
    // 配置PVD
    sConfigPVD.PVDLevel = PWR_PVDLEVEL_6;  // 2.9V
    sConfigPVD.Mode = PWR_PVD_MODE_IT_RISING_FALLING;
    HAL_PWR_ConfigPVD(&sConfigPVD);
    
    // 使能PVD
    HAL_PWR_EnablePVD();
    
    // 配置NVIC
    HAL_NVIC_SetPriority(PVD_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(PVD_IRQn);
}

// PVD中断服务函数
void PVD_IRQHandler(void) {
    HAL_PWR_PVD_IRQHandler();
}

// PVD回调函数
void HAL_PWR_PVDCallback(void) {
    // 电压低于阈值
    printf("警告: 电源电压过低!\n");
    // 采取保护措施,如保存数据、关闭外设等
}
```

---

### 示例5: 定时唤醒(RTC+待机)

```c
void Standby_With_RTC_Wakeup(uint32_t seconds) {
    // 使能PWR时钟
    __HAL_RCC_PWR_CLK_ENABLE();
    
    // 使能备份域访问
    HAL_PWR_EnableBkUpAccess();
    
    // 配置RTC闹钟唤醒
    // (需要先初始化RTC,见RTC模块文档)
    RTC_AlarmTypeDef sAlarm = {0};
    // ... 配置闹钟时间 ...
    HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN);
    
    // 清除唤醒标志
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
    
    // 进入待机模式
    HAL_PWR_EnterSTANDBYMode();
}
```

---

## 📊 功耗对比

| 模式 | 功耗 | 唤醒时间 | 保持内容 |
|------|------|----------|----------|
| 运行模式 | ~30mA | - | 全部 |
| 睡眠模式 | ~15mA | 立即 | 全部 |
| 停止模式 | ~2μA | 几μs | SRAM+寄存器 |
| 待机模式 | ~1μA | 几ms | 备份寄存器+RTC |

---

## ⚠️ 注意事项

### 1. 唤醒源

**睡眠模式**: 任何中断或事件
**停止模式**: EXTI线、RTC闹钟、IWDG、USART等
**待机模式**: WKUP引脚、RTC闹钟、IWDG、外部复位

### 2. 时钟恢复

停止模式唤醒后,系统时钟为HSI(8MHz),需要重新配置PLL。

### 3. 调试限制

待机模式下无法调试,调试器会断开连接。

### 4. 备份域

访问RTC和备份寄存器前必须:
```c
__HAL_RCC_PWR_CLK_ENABLE();
HAL_PWR_EnableBkUpAccess();
```

### 5. 唤醒延时

待机模式唤醒需要几ms,不适合需要快速响应的场景。

---

## 📖 相关文档

- [HAL核心](HAL_Core.md)
- [RCC时钟控制](HAL_RCC.md)
- [RTC实时时钟](HAL_RTC.md)
- [GPIO通用IO](HAL_GPIO.md)
