# RTC实时时钟模块 API文档

## 📋 模块概述

RTC(Real-Time Clock)实时时钟模块提供日历和时间功能,包括:
- 日历功能(年月日时分秒)
- 可编程闹钟
- 自动唤醒功能
- 备份寄存器(20字节)
- 32.768KHz晶振或LSI时钟源
- 低功耗运行

---

## 🗺️ 使用场景速查

**什么项目需要 RTC**

| 场景 | 说明 |
|------|------|
| 数据记录仪（带时间戳） | 每条数据附加采集时间，回放时可还原时序 |
| 定时开关机 / 定时执行任务 | 设置闹钟，到时间触发中断唤醒并执行 |
| 低功耗设备定时唤醒 | 待机模式下 RTC 继续运行，省电的同时保持定时能力 |
| 实时时钟显示（时钟类项目） | 周期读取时间刷新 LCD / OLED |
| 设备运行时长 / 开机次数统计 | 配合备份寄存器记录关机时间和计数 |

**时钟源怎么选**

| 时钟源 | 精度 | 硬件要求 | 适用 |
|--------|------|----------|------|
| LSE（32.768kHz 晶振） | 高（<±20ppm，误差约 1min/月） | 需外接晶振（PC14/PC15） | 正式产品，时间精度有要求 |
| LSI（内部 RC，~40kHz） | 低（±5%，误差约 3min/天） | 无需外部器件 | 原型验证，时间精度不重要 |

**备份寄存器防时间被重置**

上电后先读 `BKP_DR1`：
- 等于预设魔数 → RTC 已初始化，跳过时间设置直接用
- 不等于魔数 → 首次上电，初始化 RTC 并写入当前时间，再写入魔数

```c
if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1) != 0xA5A5) {
    RTC_SetDateTime();                            // 设置初始时间
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0xA5A5);
}
```

> 备份寄存器需要 VBAT 引脚接纽扣电池，才能在主电源断开后保持 RTC 运行。

---

## 📚 相关文件

- **头文件**: `stm32f1xx_hal_rtc.h`, `stm32f1xx_hal_rtc_ex.h`
- **源文件**: `stm32f1xx_hal_rtc.c`, `stm32f1xx_hal_rtc_ex.c`

---

## 🔧 数据类型

### RTC_TimeTypeDef - 时间结构体

```c
typedef struct {
    uint8_t Hours;    // 小时 (0-23)
    uint8_t Minutes;  // 分钟 (0-59)
    uint8_t Seconds;  // 秒 (0-59)
} RTC_TimeTypeDef;
```

### RTC_DateTypeDef - 日期结构体

```c
typedef struct {
    uint8_t WeekDay;  // 星期 (1-7)
    uint8_t Month;    // 月份 (1-12)
    uint8_t Date;     // 日期 (1-31)
    uint8_t Year;     // 年份 (0-99)
} RTC_DateTypeDef;
```

### RTC_AlarmTypeDef - 闹钟结构体

```c
typedef struct {
    RTC_TimeTypeDef AlarmTime;  // 闹钟时间
} RTC_AlarmTypeDef;
```

---

## 🎯 核心函数

### HAL_RTC_Init()

初始化RTC。

```c
HAL_StatusTypeDef HAL_RTC_Init(RTC_HandleTypeDef *hrtc);
```

---

### HAL_RTC_SetTime()

设置时间。

```c
HAL_StatusTypeDef HAL_RTC_SetTime(RTC_HandleTypeDef *hrtc,
                                  RTC_TimeTypeDef *sTime,
                                  uint32_t Format);
```

---

### HAL_RTC_GetTime()

获取时间。

```c
HAL_StatusTypeDef HAL_RTC_GetTime(RTC_HandleTypeDef *hrtc,
                                  RTC_TimeTypeDef *sTime,
                                  uint32_t Format);
```

---

### HAL_RTC_SetDate()

设置日期。

```c
HAL_StatusTypeDef HAL_RTC_SetDate(RTC_HandleTypeDef *hrtc,
                                  RTC_DateTypeDef *sDate,
                                  uint32_t Format);
```

---

### HAL_RTC_GetDate()

获取日期。

```c
HAL_StatusTypeDef HAL_RTC_GetDate(RTC_HandleTypeDef *hrtc,
                                  RTC_DateTypeDef *sDate,
                                  uint32_t Format);
```

---

### HAL_RTC_SetAlarm_IT()

设置闹钟(中断模式)。

```c
HAL_StatusTypeDef HAL_RTC_SetAlarm_IT(RTC_HandleTypeDef *hrtc,
                                      RTC_AlarmTypeDef *sAlarm,
                                      uint32_t Format);
```

---

## 💡 完整应用示例

### 示例1: RTC初始化和时间设置

```c
RTC_HandleTypeDef hrtc;

void RTC_Init(void) {
    // 使能PWR和BKP时钟
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_RCC_BKP_CLK_ENABLE();
    
    // 使能备份域访问
    HAL_PWR_EnableBkUpAccess();
    
    // 配置RTC
    hrtc.Instance = RTC;
    hrtc.Init.AsynchPrediv = RTC_AUTO_1_SECOND;
    
    if (HAL_RTC_Init(&hrtc) != HAL_OK) {
        Error_Handler();
    }
}

void RTC_SetDateTime(void) {
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};
    
    // 设置时间: 14:30:00
    sTime.Hours = 14;
    sTime.Minutes = 30;
    sTime.Seconds = 0;
    HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    
    // 设置日期: 2026年5月11日 星期一
    sDate.Year = 26;
    sDate.Month = 5;
    sDate.Date = 11;
    sDate.WeekDay = RTC_WEEKDAY_MONDAY;
    HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
}

void RTC_GetDateTime(void) {
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;
    
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
    
    printf("20%02d-%02d-%02d %02d:%02d:%02d\n",
           sDate.Year, sDate.Month, sDate.Date,
           sTime.Hours, sTime.Minutes, sTime.Seconds);
}
```

---

### 示例2: RTC闹钟

```c
void RTC_SetAlarm(uint8_t hour, uint8_t minute, uint8_t second) {
    RTC_AlarmTypeDef sAlarm = {0};
    
    sAlarm.AlarmTime.Hours = hour;
    sAlarm.AlarmTime.Minutes = minute;
    sAlarm.AlarmTime.Seconds = second;
    
    HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN);
}

// RTC闹钟中断服务函数
void RTC_Alarm_IRQHandler(void) {
    HAL_RTC_AlarmIRQHandler(&hrtc);
}

// 闹钟回调函数
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc) {
    printf("闹钟触发!\n");
    // 执行闹钟任务
}
```

---

### 示例3: 备份寄存器

```c
// 写入备份寄存器
void BKP_WriteData(uint32_t index, uint32_t data) {
    HAL_PWR_EnableBkUpAccess();
    HAL_RTCEx_BKUPWrite(&hrtc, index, data);
}

// 读取备份寄存器
uint32_t BKP_ReadData(uint32_t index) {
    return HAL_RTCEx_BKUPRead(&hrtc, index);
}

// 使用示例: 保存运行次数
void SaveBootCount(void) {
    uint32_t count = BKP_ReadData(RTC_BKP_DR1);
    count++;
    BKP_WriteData(RTC_BKP_DR1, count);
    printf("启动次数: %lu\n", count);
}
```

---

## ⚠️ 注意事项

### 1. 时钟源选择

RTC可使用LSE(32.768KHz晶振)或LSI(~40KHz内部RC):
- LSE精度高,推荐使用
- LSI精度低,但无需外部晶振

### 2. 备份域访问

操作RTC前必须:
```c
__HAL_RCC_PWR_CLK_ENABLE();
HAL_PWR_EnableBkUpAccess();
```

### 3. 首次配置

首次使用需要配置RTC时钟源和分频器。

### 4. 低功耗

RTC在待机模式下继续运行,适合低功耗应用。

---

## 📖 相关文档

- [HAL核心](HAL_Core.md)
- [PWR电源管理](HAL_PWR.md)
- [RCC时钟控制](HAL_RCC.md)
