# BKP备份寄存器模块 API文档

## 📋 模块概述

BKP（Backup Registers）是 STM32F1xx 中由 VBAT 供电的备份寄存器域，主要功能：

- **数据保持**：主电源断电后，只要 VBAT 有电（纽扣电池），寄存器数据不丢失
- **防重复初始化**：通过"魔数"判断 RTC 是否已初始化，避免每次上电重置时间
- **篡改检测（Tamper）**：PC13 引脚电平异常时自动清除所有备份寄存器
- **RTC 校准**：通过 `RTC_CALIBR` 寄存器微调 RTC 频率

STM32F103C8T6 拥有 **10 个 16-bit 备份寄存器**（BKP_DR1 ~ BKP_DR10），共 20 字节。

> STM32F103xE / F105 / F107 等大容量型号有 42 个备份寄存器（BKP_DR1 ~ BKP_DR42）。

---

## 🗺️ 使用场景速查

| 场景 | 说明 |
|------|------|
| RTC 防重复初始化 | 写入魔数，上电检查决定是否重设时间 |
| 掉电数据保持 | 保存关键配置、运行计数、上次状态等 |
| 跨复位信息传递 | 软件复位前写入标志，复位后读取判断复位原因 |
| Tamper 防篡改 | 开盖/外力触发 PC13 变化时自动清数据 |

---

## 📚 相关文件

- **头文件**: `stm32f1xx_hal_rtc_ex.h`（BKP 读写通过 RTCEx 实现）
- **源文件**: `stm32f1xx_hal_rtc_ex.c`
- **HAL BKP 直接操作**: `stm32f1xx_hal_rtc_ex.h` 中的 `HAL_RTCEx_BKUPWrite` / `HAL_RTCEx_BKUPRead`

> STM32F1xx HAL 没有独立的 `stm32f1xx_hal_bkp.c`，备份寄存器统一通过 `HAL_RTCEx` 接口访问。

---

## 🔧 前置条件

操作备份寄存器前**必须**完成以下步骤，否则写入无效：

```c
/* 1. 使能 PWR 和 BKP 总线时钟 */
__HAL_RCC_PWR_CLK_ENABLE();
__HAL_RCC_BKP_CLK_ENABLE();

/* 2. 解除备份域写保护 */
HAL_PWR_EnableBkUpAccess();
```

> `HAL_RTC_Init()` 内部会调用上述步骤，如果项目中已经初始化了 RTC，则无需重复操作。

---

## 🎯 核心函数

### HAL_RTCEx_BKUPWrite()

向备份寄存器写入 16-bit 数据。

```c
void HAL_RTCEx_BKUPWrite(RTC_HandleTypeDef *hrtc,
                          uint32_t BackupRegister,
                          uint32_t Data);
```

| 参数 | 说明 |
|------|------|
| `hrtc` | RTC 句柄指针 |
| `BackupRegister` | 寄存器索引，`RTC_BKP_DR1` ~ `RTC_BKP_DR10` |
| `Data` | 要写入的值（实际有效位为低 16-bit） |

---

### HAL_RTCEx_BKUPRead()

从备份寄存器读取数据。

```c
uint32_t HAL_RTCEx_BKUPRead(RTC_HandleTypeDef *hrtc,
                              uint32_t BackupRegister);
```

| 参数 | 说明 |
|------|------|
| `hrtc` | RTC 句柄指针 |
| `BackupRegister` | 寄存器索引，`RTC_BKP_DR1` ~ `RTC_BKP_DR10` |

**返回值**: 寄存器当前值（低 16-bit 有效）

---

### HAL_RTCEx_SetTamper_IT()

启用 Tamper 检测中断（PC13 引脚触发）。

```c
HAL_StatusTypeDef HAL_RTCEx_SetTamper_IT(RTC_HandleTypeDef *hrtc,
                                          RTC_TamperTypeDef *sTamper);
```

触发 Tamper 时，硬件自动清除所有备份寄存器，然后进入回调 `HAL_RTCEx_TamperEventCallback()`。

---

### HAL_RTCEx_SetSmoothCalib()（仅部分型号）

设置 RTC 时钟校准值，补偿晶振误差。

```c
HAL_StatusTypeDef HAL_RTCEx_SetSmoothCalib(RTC_HandleTypeDef *hrtc,
                                            uint32_t SmoothCalibPeriod,
                                            uint32_t SmoothCalibPlusPulses,
                                            uint32_t SmouthCalibMinusPulsesValue);
```

---

## 📌 备份寄存器索引速查

| 宏定义 | 寄存器 | 偏移地址 |
|--------|--------|----------|
| `RTC_BKP_DR1`  | BKP_DR1  | 0x04 |
| `RTC_BKP_DR2`  | BKP_DR2  | 0x08 |
| `RTC_BKP_DR3`  | BKP_DR3  | 0x0C |
| `RTC_BKP_DR4`  | BKP_DR4  | 0x10 |
| `RTC_BKP_DR5`  | BKP_DR5  | 0x14 |
| `RTC_BKP_DR6`  | BKP_DR6  | 0x28 |
| `RTC_BKP_DR7`  | BKP_DR7  | 0x2C |
| `RTC_BKP_DR8`  | BKP_DR8  | 0x30 |
| `RTC_BKP_DR9`  | BKP_DR9  | 0x34 |
| `RTC_BKP_DR10` | BKP_DR10 | 0x38 |

> DR6 ~ DR10 的地址不连续，HAL 已封装好，直接用宏名即可。

---

## 💡 完整应用示例

### 示例 1：RTC 防重复初始化（最常用）

上电检查魔数，避免每次复位都重置 RTC 时间：

```c
#define RTC_MAGIC_KEY   0xA5A5  /* 自定义魔数，任意非 0 值 */
#define RTC_MAGIC_REG   RTC_BKP_DR1

extern RTC_HandleTypeDef hrtc;

void App_RTC_Init(void) {
    /* MX_RTC_Init() 已由 CubeMX 生成，此处只做业务层判断 */
    if (HAL_RTCEx_BKUPRead(&hrtc, RTC_MAGIC_REG) != RTC_MAGIC_KEY) {
        /* 首次上电或电池耗尽：设置初始时间 */
        RTC_TimeTypeDef sTime = {14, 30, 0};
        RTC_DateTypeDef sDate = {RTC_WEEKDAY_THURSDAY, RTC_MONTH_MAY, 21, 26};

        HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
        HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

        /* 写入魔数，下次上电跳过初始化 */
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_MAGIC_REG, RTC_MAGIC_KEY);
    }
    /* 已有魔数：RTC 继续计时，无需任何操作 */
}
```

---

### 示例 2：保存运行计数（掉电不丢失）

```c
#define BOOT_COUNT_REG  RTC_BKP_DR2

void BKP_IncBootCount(void) {
    uint16_t count = (uint16_t)HAL_RTCEx_BKUPRead(&hrtc, BOOT_COUNT_REG);
    count++;
    HAL_RTCEx_BKUPWrite(&hrtc, BOOT_COUNT_REG, count);
    printf("第 %u 次启动\n", count);
}
```

---

### 示例 3：跨复位传递标志

软件复位前保存状态，复位后读取判断来源：

```c
#define RESET_FLAG_REG   RTC_BKP_DR3
#define FLAG_SOFT_RESET  0x1234
#define FLAG_WATCHDOG    0x5678

/* 触发软件复位前调用 */
void App_RequestSoftReset(void) {
    HAL_RTCEx_BKUPWrite(&hrtc, RESET_FLAG_REG, FLAG_SOFT_RESET);
    HAL_NVIC_SystemReset();
}

/* 上电时检查复位原因 */
void App_CheckResetCause(void) {
    uint16_t flag = (uint16_t)HAL_RTCEx_BKUPRead(&hrtc, RESET_FLAG_REG);
    if (flag == FLAG_SOFT_RESET) {
        printf("软件复位\n");
        HAL_RTCEx_BKUPWrite(&hrtc, RESET_FLAG_REG, 0); /* 清除标志 */
    } else if (flag == FLAG_WATCHDOG) {
        printf("看门狗复位\n");
        HAL_RTCEx_BKUPWrite(&hrtc, RESET_FLAG_REG, 0);
    } else {
        printf("正常上电\n");
    }
}
```

---

### 示例 4：Tamper 防篡改检测

```c
void BKP_Tamper_Init(void) {
    RTC_TamperTypeDef sTamper = {0};

    /* PC13 下降沿触发（主动低有效） */
    sTamper.Tamper        = RTC_TAMPER_1;
    sTamper.Trigger       = RTC_TAMPERTRIGGER_FALLINGEDGE;
    sTamper.NoErase       = RTC_TAMPER_ERASE_BACKUP_ENABLE;  /* 触发时自动清除备份寄存器 */

    HAL_RTCEx_SetTamper_IT(&hrtc, &sTamper);
}

/* Tamper 中断回调 */
void HAL_RTCEx_TamperEventCallback(RTC_HandleTypeDef *hrtc) {
    /* 此时备份寄存器已被硬件自动清零 */
    printf("Tamper 触发！备份数据已清除\n");
    /* 可在此记录日志、触发报警等 */
}

/* 中断服务函数 */
void TAMPER_IRQHandler(void) {
    HAL_RTCEx_TamperIRQHandler(&hrtc);
}
```

> **注意**：STM32F103C8T6（Blue Pill）的 PC13 同时接了板载 LED，Tamper 功能与 LED 不能同时使用。

---

### 示例 5：封装通用读写工具函数

```c
/**
 * @brief  向备份寄存器写入 16-bit 数据
 * @param  reg   寄存器宏，如 RTC_BKP_DR1
 * @param  data  要写入的值（高 16-bit 将被忽略）
 */
void BKP_Write(uint32_t reg, uint16_t data) {
    HAL_RTCEx_BKUPWrite(&hrtc, reg, (uint32_t)data);
}

/**
 * @brief  从备份寄存器读取 16-bit 数据
 * @param  reg  寄存器宏，如 RTC_BKP_DR1
 * @return 寄存器值（低 16-bit）
 */
uint16_t BKP_Read(uint32_t reg) {
    return (uint16_t)HAL_RTCEx_BKUPRead(&hrtc, reg);
}

/* 使用示例 */
BKP_Write(RTC_BKP_DR4, 0x1234);
uint16_t val = BKP_Read(RTC_BKP_DR4);  /* val == 0x1234 */
```

---

## ⚠️ 注意事项

### 1. 必须解除写保护

写操作前若缺少 `HAL_PWR_EnableBkUpAccess()`，写入会被静默丢弃，读取仍返回旧值。

### 2. 只有 16-bit 有效

`HAL_RTCEx_BKUPWrite()` 的 `Data` 参数虽为 `uint32_t`，但寄存器只有 16-bit，高 16-bit 自动丢弃。

### 3. VBAT 供电是关键

- VBAT 不接电池时，主电源断电后 BKP 域掉电，数据和 RTC 计时都会丢失。
- 实际产品中 VBAT 通常接一颗 CR2032 纽扣电池，通过 100Ω 电阻隔离主 3.3V。

### 4. Tamper 与 PC13 冲突

Blue Pill 的 PC13 已连接板载 LED，启用 Tamper 后 LED 会导致误触发。生产板应将 Tamper 引脚独立走线。

### 5. CubeMX 中的配置

在 CubeMX 的 **RTC → Tamper** 选项卡中可以启用 Tamper 检测；备份寄存器读写无需额外配置，初始化 RTC 后直接调用读写函数即可。

---

## 📐 与 RTC 的关系

```
BKP 域
├── BKP_DR1 ~ BKP_DR10   ← 用户数据（HAL_RTCEx_BKUPRead/Write）
├── RTC_CALIBR            ← RTC 时钟校准（HAL_RTCEx_SetSmoothCalib）
├── TAMPER 引脚检测        ← HAL_RTCEx_SetTamper_IT
└── RTC 计数器            ← HAL_RTC_GetTime / HAL_RTC_SetTime
```

BKP 和 RTC 共用同一个备份域供电，由 VBAT 维持。两者同时被 `HAL_PWR_EnableBkUpAccess()` 解除写保护。

---

## 📖 相关文档

- [HAL_RTC.md](HAL_RTC.md) - 实时时钟（与 BKP 紧密配合）
- [HAL_PWR.md](HAL_PWR.md) - 电源控制（备份域访问权限）
- [HAL_RCC.md](HAL_RCC.md) - 时钟使能（`__HAL_RCC_BKP_CLK_ENABLE`）
