# FreeRTOS 软件定时器（Software Timer）

## 目录

- [核心概念](#核心概念)
- [定时器任务（Timer Service Task）](#定时器任务timer-service-task)
- [单次定时器 vs 周期定时器](#单次定时器-vs-周期定时器)
- [创建定时器](#创建定时器)
- [启动与控制](#启动与控制)
- [回调函数中的限制](#回调函数中的限制)
- [定时器 ID](#定时器-id)
- [软件定时器 vs 硬件定时器](#软件定时器-vs-硬件定时器)
- [典型使用场景](#典型使用场景)
- [常见错误](#常见错误)
- [API 速查](#api-速查)

---

## 核心概念

软件定时器是 FreeRTOS 提供的**基于 SysTick 节拍的定时回调机制**，到期后自动调用用户指定的回调函数。

**核心特点：**
- 数量几乎无限制（受 RAM 约束）
- 精度 ±1 tick（默认 ±1ms）
- 回调在专用的"定时器任务"中执行，不是在中断中
- 回调函数**不能阻塞**

---

## 定时器任务（Timer Service Task）

FreeRTOS 在调度器启动时自动创建一个定时器任务（Timer Daemon Task），所有软件定时器的回调函数都在这个任务中执行。

```
SysTick 中断（每 1ms）
    ↓
FreeRTOS 内核检查定时器链表
    ↓
到期的定时器 → 将"执行回调"命令发到定时器命令队列
    ↓
定时器任务（后台运行）从队列取命令 → 执行回调函数
```

**定时器任务配置（FreeRTOSConfig.h）：**

```c
/* 定时器任务优先级，建议设置稍高一些，确保回调及时执行 */
#define configTIMER_TASK_PRIORITY      (configMAX_PRIORITIES - 1)

/* 定时器命令队列深度，太小会导致命令丢失 */
#define configTIMER_QUEUE_LENGTH       10

/* 定时器任务栈大小（words） */
#define configTIMER_TASK_STACK_DEPTH   128
```

---

## 单次定时器 vs 周期定时器

| 类型 | 创建参数 | 行为 |
|------|---------|------|
| 单次（One-shot） | `pdFALSE` | 到期后执行一次回调，然后停止；可以用 `xTimerReset` 重新启动 |
| 周期（Auto-reload） | `pdTRUE` | 到期后执行回调，自动重新计时，持续循环 |

```
单次定时器时序：
  启动 ──等待N ms──→ 回调执行 ──→ 停止

周期定时器时序：
  启动 ──等待N ms──→ 回调执行 ──等待N ms──→ 回调执行 ──→ (持续循环)
```

---

## 创建定时器

```c
TimerHandle_t xTimer;

/* 回调函数（不能阻塞，要快速完成） */
void vTimerCallback(TimerHandle_t xTimer) {
    /* 通过 ID 区分是哪个定时器触发的 */
    uint32_t id = (uint32_t)pvTimerGetTimerID(xTimer);
    
    switch (id) {
        case TIMER_ID_LED:
            HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
            break;
        case TIMER_ID_WATCHDOG:
            HAL_IWDG_Refresh(&hiwdg);
            break;
    }
}

/* 创建周期定时器 */
xTimer = xTimerCreate(
    "LEDTimer",              /* 名称（调试用） */
    pdMS_TO_TICKS(500),      /* 超时时间：500ms */
    pdTRUE,                  /* pdTRUE = 周期，pdFALSE = 单次 */
    (void *)TIMER_ID_LED,    /* 定时器 ID（pvTimerID），可以是任何值 */
    vTimerCallback           /* 回调函数 */
);

if (xTimer == NULL) {
    Error_Handler(); /* 内存不足 */
}
```

---

## 启动与控制

### 基本控制

```c
/* 启动定时器（从创建或停止状态开始计时） */
xTimerStart(xTimer, 0); /* 第二个参数：命令队列等待时间（0 = 不等待） */

/* 停止定时器（回调不再触发） */
xTimerStop(xTimer, 0);

/* 复位定时器（重新开始计时，无论当前状态如何） */
/* 常用于超时检测：每次活动就 Reset，没有活动则到期触发 */
xTimerReset(xTimer, 0);

/* 修改定时器周期（立即生效，同时重启定时器） */
xTimerChangePeriod(xTimer, pdMS_TO_TICKS(1000), 0);

/* 删除定时器 */
xTimerDelete(xTimer, 0);
```

### 在中断中控制

```c
/* 中断中控制定时器，使用 FromISR 版本 */
BaseType_t xHigherPriorityTaskWoken = pdFALSE;

xTimerStartFromISR(xTimer, &xHigherPriorityTaskWoken);
xTimerStopFromISR(xTimer, &xHigherPriorityTaskWoken);
xTimerResetFromISR(xTimer, &xHigherPriorityTaskWoken);
xTimerChangePeriodFromISR(xTimer, pdMS_TO_TICKS(200), &xHigherPriorityTaskWoken);

portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
```

### 查询状态

```c
/* 查询定时器是否正在运行 */
if (xTimerIsTimerActive(xTimer) == pdTRUE) {
    /* 定时器正在运行 */
} else {
    /* 定时器已停止 */
}

/* 获取定时器名称 */
const char *name = pcTimerGetName(xTimer);

/* 获取定时器周期 */
TickType_t period = xTimerGetPeriod(xTimer);

/* 获取定时器到期时刻（绝对节拍值） */
TickType_t expiry = xTimerGetExpiryTime(xTimer);
```

---

## 回调函数中的限制

定时器回调运行在定时器任务上下文中，有以下限制：

```c
/* ❌ 不允许的操作：任何可能阻塞的 API */
void vBadCallback(TimerHandle_t xTimer) {
    vTaskDelay(100);                          /* 不可以：阻塞 */
    xQueueSend(xQueue, &data, portMAX_DELAY); /* 不可以：可能阻塞 */
    xSemaphoreTake(xSem, portMAX_DELAY);      /* 不可以：可能阻塞 */
    HAL_Delay(10);                            /* 不可以：阻塞 */
}

/* ✅ 允许的操作 */
void vGoodCallback(TimerHandle_t xTimer) {
    /* 1. 快速的硬件操作 */
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    
    /* 2. 发送到队列（超时为 0，不阻塞） */
    xQueueSend(xQueue, &data, 0);
    
    /* 3. 给出信号量（不阻塞） */
    xSemaphoreGive(xSem);
    
    /* 4. 向任务发送通知 */
    xTaskNotify(xTaskHandle, 0x01, eSetBits);
    
    /* 5. 控制其他定时器 */
    xTimerStop(xTimer, 0);
}
```

**为什么不能阻塞：** 所有定时器回调共用一个定时器任务。一个回调阻塞，其他所有定时器的回调都无法执行。

---

## 定时器 ID

定时器 ID 是一个 `void *` 指针，可以存储任何值，用于在共享回调函数中区分不同定时器。

```c
/* 方式1：存整数 ID */
#define LED1_TIMER_ID  1
#define LED2_TIMER_ID  2

TimerHandle_t xLed1Timer = xTimerCreate("LED1", pdMS_TO_TICKS(500), pdTRUE,
                                         (void *)LED1_TIMER_ID, vLedCallback);
TimerHandle_t xLed2Timer = xTimerCreate("LED2", pdMS_TO_TICKS(200), pdTRUE,
                                         (void *)LED2_TIMER_ID, vLedCallback);

void vLedCallback(TimerHandle_t xTimer) {
    uint32_t id = (uint32_t)pvTimerGetTimerID(xTimer);
    if (id == LED1_TIMER_ID) {
        HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
    } else if (id == LED2_TIMER_ID) {
        HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
    }
}

/* 方式2：存结构体指针（运行时传递配置数据） */
typedef struct {
    GPIO_TypeDef *port;
    uint16_t      pin;
} LedInfo_t;

static LedInfo_t led1_info = {GPIOC, GPIO_PIN_13};
static LedInfo_t led2_info = {GPIOA, GPIO_PIN_0};

TimerHandle_t xLed1Timer = xTimerCreate("LED1", pdMS_TO_TICKS(500), pdTRUE,
                                         &led1_info, vLedCallback2);

void vLedCallback2(TimerHandle_t xTimer) {
    LedInfo_t *info = (LedInfo_t *)pvTimerGetTimerID(xTimer);
    HAL_GPIO_TogglePin(info->port, info->pin);
}

/* 运行时更新 ID */
vTimerSetTimerID(xTimer, (void *)new_id);
```

---

## 软件定时器 vs 硬件定时器

| 对比项 | 软件定时器 | 硬件定时器（TIM） |
|--------|-----------|-----------------|
| 精度 | ±1 tick（默认 ±1ms） | 微秒甚至纳秒级 |
| 数量限制 | 几乎无限制（仅受 RAM） | STM32F103 共 7 个 |
| 回调执行上下文 | 定时器任务（可调度延迟） | 中断（立即） |
| 最大延迟 | 受定时器任务优先级影响 | 无调度延迟 |
| 使用复杂度 | 简单 | 需要配置寄存器/CubeMX |
| 适用场景 | 超时检测、周期刷新、延时关闭 | 精确 PWM、输入捕获、精确采样 |

**选择原则：**

```
需要精确微秒级时序（PWM、超声波测距、精确采样）→ 硬件定时器
需要定时但精度 ±1ms 可接受（UI 刷新、超时保护、LED 闪烁）→ 软件定时器
定时器数量多（10+ 个）→ 软件定时器
```

---

## 典型使用场景

### 场景 1：按键长按检测

```c
TimerHandle_t xBtnTimer;
static uint8_t btn_pressed = 0;

void vBtnTimerCallback(TimerHandle_t xTimer) {
    /* 定时器到期（500ms），按键仍被按下则认为是长按 */
    if (HAL_GPIO_ReadPin(BTN_GPIO_Port, BTN_Pin) == GPIO_PIN_RESET) {
        HandleLongPress();
    }
}

/* 初始化 */
xBtnTimer = xTimerCreate("BtnTimer", pdMS_TO_TICKS(500), pdFALSE, 0, vBtnTimerCallback);

/* 按键中断 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == BTN_Pin) {
        if (HAL_GPIO_ReadPin(BTN_GPIO_Port, BTN_Pin) == GPIO_PIN_RESET) {
            /* 按下：启动单次定时器 */
            BaseType_t woken = pdFALSE;
            xTimerStartFromISR(xBtnTimer, &woken);
            portYIELD_FROM_ISR(woken);
        } else {
            /* 松开：停止定时器，未到期则是短按 */
            BaseType_t woken = pdFALSE;
            if (xTimerIsTimerActive(xBtnTimer)) {
                xTimerStopFromISR(xBtnTimer, &woken);
                /* 短按处理 */
                HandleShortPress();
            }
            portYIELD_FROM_ISR(woken);
        }
    }
}
```

### 场景 2：通信超时检测（看门狗模式）

```c
TimerHandle_t xCommTimer;

void vCommTimeoutCallback(TimerHandle_t xTimer) {
    /* 超过 5 秒没收到数据 → 通信超时 */
    SetCommError(COMM_ERROR_TIMEOUT);
    TryReconnect();
}

/* 初始化 */
xCommTimer = xTimerCreate("CommWDT", pdMS_TO_TICKS(5000), pdFALSE, 0, vCommTimeoutCallback);
xTimerStart(xCommTimer, 0);

/* 每次收到数据就 Reset（重新计时） */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    BaseType_t woken = pdFALSE;
    xTimerResetFromISR(xCommTimer, &woken); /* 重置，5秒倒计时重新开始 */
    portYIELD_FROM_ISR(woken);
    
    /* 处理数据 */
    ProcessReceivedData(rxBuf);
}
```

### 场景 3：多路 LED 不同频率闪烁

```c
typedef struct {
    GPIO_TypeDef *port;
    uint16_t      pin;
    uint32_t      period_ms;
} LedConfig_t;

#define LED_COUNT 3
static LedConfig_t leds[LED_COUNT] = {
    {GPIOC, GPIO_PIN_13, 100},  /* LED1: 100ms */
    {GPIOA, GPIO_PIN_0,  500},  /* LED2: 500ms */
    {GPIOB, GPIO_PIN_1,  1000}, /* LED3: 1000ms */
};

void vLedToggleCallback(TimerHandle_t xTimer) {
    LedConfig_t *cfg = (LedConfig_t *)pvTimerGetTimerID(xTimer);
    HAL_GPIO_TogglePin(cfg->port, cfg->pin);
}

/* 批量创建 */
for (int i = 0; i < LED_COUNT; i++) {
    TimerHandle_t t = xTimerCreate("LED",
                                    pdMS_TO_TICKS(leds[i].period_ms),
                                    pdTRUE,
                                    &leds[i],
                                    vLedToggleCallback);
    xTimerStart(t, 0);
}
```

### 场景 4：延时关闭背光

```c
TimerHandle_t xBacklightTimer;

void vBacklightOffCallback(TimerHandle_t xTimer) {
    LCD_SetBacklight(0); /* 关闭背光 */
}

xBacklightTimer = xTimerCreate("BL", pdMS_TO_TICKS(30000), pdFALSE, 0, vBacklightOffCallback);
xTimerStart(xBacklightTimer, 0);

/* 任何触摸操作都刷新背光延时 */
void OnTouchEvent(void) {
    LCD_SetBacklight(1);             /* 打开背光 */
    xTimerReset(xBacklightTimer, 0); /* 重置 30 秒倒计时 */
}
```

---

## 常见错误

### 回调函数中阻塞

```c
/* ❌ 严重错误：阻塞定时器任务，导致所有定时器停止运行 */
void vBadCallback(TimerHandle_t xTimer) {
    HAL_Delay(100);                     /* 阻塞定时器任务 100ms */
    xSemaphoreTake(xSem, portMAX_DELAY); /* 可能永久阻塞 */
}

/* ✅ 回调中通知任务来做耗时操作 */
void vGoodCallback(TimerHandle_t xTimer) {
    /* 仅通知任务 */
    xTaskNotify(xWorkerTask, TIMER_EXPIRED_BIT, eSetBits);
}
```

### 定时器命令队列满

```c
/* xTimerStart 第二个参数是命令队列等待时间 */
/* 传 0 时，如果队列满，命令被丢弃，定时器操作失败 */
BaseType_t result = xTimerStart(xTimer, 0);
if (result != pdPASS) {
    /* 命令队列满，定时器未启动 */
    /* 解决：增大 configTIMER_QUEUE_LENGTH，或传入超时时间 */
}

/* 如果可以等待，传入超时时间 */
xTimerStart(xTimer, pdMS_TO_TICKS(10));
```

### 误以为回调在中断中执行

```c
/* 软件定时器回调不在中断中，而在定时器任务中 */
void vMyCallback(TimerHandle_t xTimer) {
    /* 不需要 FromISR 版本 */
    xSemaphoreGive(xSem);   /* ✅ 可以用普通版本（但不能阻塞） */
    xQueueSend(xQ, &d, 0);  /* ✅ 可以用普通版本（超时为 0） */
}
```

---

## API 速查

| API | 说明 | 可用场景 |
|-----|------|---------|
| `xTimerCreate(name, period, reload, id, callback)` | 创建定时器 | 调度器启动前/后 |
| `xTimerCreateStatic(...)` | 静态创建 | — |
| `xTimerStart(timer, timeout)` | 启动定时器 | 任务 |
| `xTimerStop(timer, timeout)` | 停止定时器 | 任务 |
| `xTimerReset(timer, timeout)` | 复位（重新计时） | 任务 |
| `xTimerChangePeriod(timer, newPeriod, timeout)` | 修改周期并重启 | 任务 |
| `xTimerDelete(timer, timeout)` | 删除定时器 | 任务 |
| `xTimerStartFromISR(timer, &woken)` | 中断中启动 | 中断 |
| `xTimerStopFromISR(timer, &woken)` | 中断中停止 | 中断 |
| `xTimerResetFromISR(timer, &woken)` | 中断中复位 | 中断 |
| `xTimerChangePeriodFromISR(timer, period, &woken)` | 中断中修改周期 | 中断 |
| `xTimerIsTimerActive(timer)` | 查询是否运行中 | 任务/中断 |
| `pvTimerGetTimerID(timer)` | 获取定时器 ID | 任务/中断 |
| `vTimerSetTimerID(timer, id)` | 设置定时器 ID | 任务/中断 |
| `pcTimerGetName(timer)` | 获取名称 | 任务/中断 |
| `xTimerGetPeriod(timer)` | 获取周期（ticks） | 任务/中断 |
| `xTimerGetExpiryTime(timer)` | 获取到期时刻 | 任务/中断 |
