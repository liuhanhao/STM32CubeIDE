# FreeRTOS 任务（Task）

## 目录

- [核心概念](#核心概念)
- [任务状态机](#任务状态机)
- [创建任务](#创建任务)
- [任务函数结构](#任务函数结构)
- [优先级系统](#优先级系统)
- [栈大小估算](#栈大小估算)
- [延时与定周期执行](#延时与定周期执行)
- [任务管理 API](#任务管理-api)
- [空闲任务与钩子函数](#空闲任务与钩子函数)
- [栈溢出检测](#栈溢出检测)
- [运行时统计](#运行时统计)
- [典型使用场景](#典型使用场景)
- [常见错误](#常见错误)
- [API 速查](#api-速查)

---

## 核心概念

任务是 FreeRTOS 的基本执行单元，可以理解为一个"永不退出的独立线程"。每个任务：

- 有自己独立的**栈空间**（局部变量、函数调用链保存在这里）
- 有一个**优先级**（决定调度顺序）
- 有一个**状态**（运行/就绪/阻塞/挂起）

调度器负责在多个任务之间切换 CPU 使用权，对每个任务来说，感觉自己独占着 CPU。

---

## 任务状态机

```
  xTaskCreate()
       ↓
   就绪（Ready）──────────────────────────────────────┐
       ↓  调度器选中（最高优先级就绪任务）             │
   运行（Running）                                    │
       │                                              │
       ├──→ 阻塞（Blocked）←── vTaskDelay()           │
       │         │              等待队列/信号量        │
       │         │  超时或资源可用                     │
       │         └──────────────→ 就绪（Ready）       │
       │                                              │
       ├──→ 挂起（Suspended）←── vTaskSuspend()       │
       │         │                                    │
       │         └── vTaskResume() ──────────────→ 就绪│
       │                                              │
       └──→ 删除（Deleted）←── vTaskDelete()          │
                 被空闲任务回收资源                    │
                                                      │
   高优先级任务就绪 ←────────────────────────────────┘
   （抢占当前任务，当前任务回到就绪）
```

**四种状态说明：**

| 状态 | 含义 | 进入方式 |
|------|------|---------|
| 就绪（Ready） | 可以运行，等待调度器选中 | 刚创建、阻塞解除、被抢占 |
| 运行（Running） | 正在使用 CPU | 调度器从就绪队列中选出最高优先级 |
| 阻塞（Blocked） | 等待某个条件，不占 CPU | 主动调用延时/等待队列/信号量 |
| 挂起（Suspended） | 被暂停，不参与调度 | `vTaskSuspend()` |

---

## 创建任务

### 动态创建（推荐）

栈和任务控制块（TCB）从 FreeRTOS 堆中动态分配。

```c
TaskHandle_t xTaskHandle = NULL;

BaseType_t result = xTaskCreate(
    vMyTask,          /* 任务函数指针 */
    "MyTask",         /* 任务名（调试用，最长 configMAX_TASK_NAME_LEN 字符） */
    256,              /* 栈大小，单位 words（1 word = 4 字节，即 256×4 = 1024 字节） */
    (void *)&param,   /* 传给任务函数的参数，不需要传参数填 NULL */
    3,                /* 优先级，数值越大优先级越高 */
    &xTaskHandle      /* 保存句柄，不需要可填 NULL */
);

if (result != pdPASS) {
    /* 创建失败：堆空间不足 */
    Error_Handler();
}
```

### 静态创建（零动态内存）

栈和 TCB 由用户提供静态数组，适合安全关键系统（避免运行时内存分配失败）。

```c
/* 静态分配栈和 TCB */
static StackType_t  xTaskStack[256];
static StaticTask_t xTaskTCB;

TaskHandle_t xHandle = xTaskCreateStatic(
    vMyTask,
    "MyTask",
    256,           /* 栈大小（words），必须与数组大小一致 */
    NULL,
    3,
    xTaskStack,    /* 栈数组 */
    &xTaskTCB      /* TCB 结构体 */
);
```

> 使用静态创建需要在 `FreeRTOSConfig.h` 中设置 `configSUPPORT_STATIC_ALLOCATION = 1`，并实现 `vApplicationGetIdleTaskMemory()` 和 `vApplicationGetTimerTaskMemory()` 函数。

---

## 任务函数结构

任务函数有固定格式：返回类型 `void`，参数 `void *`，函数体内是无限循环。

```c
void vMyTask(void *pvParameters) {
    /* ① 转换参数（如有） */
    MyConfig_t *pConfig = (MyConfig_t *)pvParameters;

    /* ② 任务内部初始化（只执行一次） */
    uint32_t count = 0;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    /* ③ 无限循环 */
    for (;;) {
        /* 任务主体逻辑 */
        count++;

        /* ④ 必须有阻塞点，让出 CPU */
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    /* ⑤ 如果任务需要退出，必须调用 vTaskDelete(NULL) */
    /* 绝对不能让函数 return，否则行为未定义 */
    vTaskDelete(NULL);
}
```

**关键点：**

- 任务函数必须包含阻塞调用（`vTaskDelay`、等待队列/信号量等），否则低优先级任务永远得不到 CPU
- 不能 `return`，必须通过 `vTaskDelete(NULL)` 退出
- 局部变量存储在任务自己的栈上，不同任务的局部变量完全隔离

---

## 优先级系统

FreeRTOS 优先级数值越大越高（与 NVIC 相反），0 为空闲任务优先级，用户任务从 1 开始。

```
优先级：  5（最高用户优先级）
         4
         3
         2
         1（最低用户优先级）
         0（空闲任务，系统保留）
```

**调度规则：**

1. 多个任务就绪时，永远运行优先级最高的那个
2. 相同优先级的多个就绪任务，按时间片轮转（默认每 1ms 切换一次）
3. 高优先级任务就绪时，立即抢占正在运行的低优先级任务

**优先级规划建议（以 `MAX_PRIORITIES = 7` 为例）：**

| 优先级 | 用途 | 典型任务 |
|--------|------|---------|
| 1 | 后台/低优先级 | 状态显示、日志记录、蓝牙广播 |
| 2 | 普通业务 | 数据处理、算法计算 |
| 3 | 通信任务 | UART/CAN 消息收发 |
| 4 | 时间敏感 | 传感器采集、电机控制环 |
| 5 | 高实时 | 安全保护、紧急处理 |
| 6 | 保留 | 极少使用 |

> 不要把过多任务设为最高优先级，这样会退化成串行执行，失去多任务意义。

### 动态调整优先级

```c
/* 读取当前优先级 */
UBaseType_t prio = uxTaskPriorityGet(xTaskHandle);

/* 临时提升优先级（完成后恢复） */
vTaskPrioritySet(xTaskHandle, 5);
DoUrgentWork();
vTaskPrioritySet(xTaskHandle, prio);
```

---

## 栈大小估算

栈大小单位是 **words**（1 word = 4 字节）。栈存储：局部变量、函数调用时的返回地址和寄存器。

**估算方法：**

```
最小栈 = FreeRTOS 内核切换开销（约 26 words）
       + 函数调用链深度 × 每层帧大小
       + 最大局部变量总大小
       + printf/sprintf 缓冲区（如有，约 128 words）
```

**实用建议：**

| 任务复杂度 | 建议初始栈（words） |
|-----------|------------------|
| 简单任务（几个变量，无 printf） | 64~128 |
| 中等任务（带 printf 或较多局部变量） | 128~256 |
| 复杂任务（嵌套调用深，大型局部数组） | 256~512 |

> 先给大一点，用 `uxTaskGetStackHighWaterMark()` 测量实际使用，再按需缩减。

```c
/* 返回历史最小剩余栈大小（words），越小说明越接近溢出 */
UBaseType_t watermark = uxTaskGetStackHighWaterMark(NULL); /* NULL = 查自身 */
/* 建议保持至少 20 words 余量 */
```

---

## 延时与定周期执行

### `vTaskDelay`：相对延时

从"调用时刻"算起延时，实际周期 = 执行时间 + 延时时间。

```c
void vTaskA(void *p) {
    for (;;) {
        DoWork();                          /* 假设耗时 3ms */
        vTaskDelay(pdMS_TO_TICKS(10));     /* 延时 10ms */
        /* 实际周期：3 + 10 = 13ms，不固定 */
    }
}
```

### `vTaskDelayUntil`：绝对定周期执行

以上次唤醒时刻为基准计算下次唤醒时刻，**周期严格固定**，不受执行时间影响。

```c
void vTaskB(void *p) {
    TickType_t xLastWakeTime = xTaskGetTickCount(); /* 记录起始时刻 */

    for (;;) {
        DoWork();   /* 耗时多少无所谓，只要不超过周期 */

        /* 从 xLastWakeTime 起经过 10ms 才唤醒，xLastWakeTime 自动更新 */
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));
        /* 实际周期：严格 10ms */
    }
}
```

**选用场景对比：**

| 场景 | 推荐 |
|------|------|
| 超时等待、简单延时 | `vTaskDelay` |
| PID 控制环、ADC 定时采样、固定帧率 | `vTaskDelayUntil` |

---

## 任务管理 API

```c
/* 挂起任务（立即生效，可由其他任务或中断恢复） */
vTaskSuspend(xTaskHandle);
vTaskSuspend(NULL);         /* 挂起自身 */

/* 恢复任务 */
vTaskResume(xTaskHandle);

/* 中断中恢复任务 */
BaseType_t xWoken = pdFALSE;
xTaskResumeFromISR(xTaskHandle);  /* 返回 pdTRUE 表示需要任务切换 */

/* 删除任务（释放任务栈和 TCB，由空闲任务完成清理） */
vTaskDelete(xTaskHandle);   /* 删除指定任务 */
vTaskDelete(NULL);          /* 任务删除自身 */

/* 主动让出 CPU（让同优先级任务有机会运行） */
taskYIELD();

/* 获取任务名称 */
char *name = pcTaskGetName(xTaskHandle);

/* 获取当前系统节拍 */
TickType_t ticks = xTaskGetTickCount();        /* 任务中使用 */
TickType_t ticks = xTaskGetTickCountFromISR(); /* 中断中使用 */
```

---

## 空闲任务与钩子函数

**空闲任务（Idle Task）** 由调度器自动创建，优先级 0，在没有其他任务运行时执行。

- 负责回收已删除任务的内存
- 不能被删除，不能设置比 0 更低的优先级

**空闲任务钩子（Idle Hook）**：每次空闲任务循环时被调用，可用于低功耗或后台维护。

```c
/* 在 FreeRTOSConfig.h 中设置：configUSE_IDLE_HOOK = 1 */

/* 实现钩子函数，名称固定 */
void vApplicationIdleHook(void) {
    /* 可以在这里进入低功耗模式 */
    /* 注意：不能调用任何可能阻塞的 API */
    __WFI(); /* Wait For Interrupt，进入睡眠等待中断唤醒 */
}
```

**Tick 钩子（Tick Hook）**：每个系统节拍（默认 1ms）都会被调用。

```c
/* 在 FreeRTOSConfig.h 中设置：configUSE_TICK_HOOK = 1 */

void vApplicationTickHook(void) {
    /* 每 1ms 调用一次，不能阻塞，执行时间要极短 */
    /* 典型用途：精细计时统计、看门狗喂狗 */
}
```

---

## 栈溢出检测

栈溢出会导致数据被破坏，表现为程序随机崩溃，很难直接定位。FreeRTOS 提供两种检测方式。

**开启方式（CubeMX Config parameters 中设置）：**

| 检测模式 | 说明 | 开销 |
|---------|------|------|
| `configCHECK_FOR_STACK_OVERFLOW = 1` | 任务切换时检查栈指针是否越界 | 极小 |
| `configCHECK_FOR_STACK_OVERFLOW = 2` | 检查栈末尾的特征字节是否被覆盖 | 稍大，更可靠 |

```c
/* 实现溢出钩子，名称固定，配置开启后必须实现 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    /* 打印发生溢出的任务名 */
    /* 注意：此时栈已经损坏，不要做复杂操作 */
    printf("Stack overflow: %s\r\n", pcTaskName);
    taskDISABLE_INTERRUPTS();
    for (;;);
}
```

---

## 运行时统计

开启后可以看到每个任务占用 CPU 的百分比，便于性能分析。

**开启条件（FreeRTOSConfig.h）：**
```c
#define configGENERATE_RUN_TIME_STATS           1
#define configUSE_STATS_FORMATTING_FUNCTIONS    1
/* 还需要提供一个比 SysTick 更高精度的计时器，通常用 TIM 配置成 10μs 计一次 */
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() /* 初始化计时器 */
#define portGET_RUN_TIME_COUNTER_VALUE()          /* 读取计数值 */
```

```c
/* 打印所有任务的运行时间统计 */
char buf[512];
vTaskGetRunTimeStats(buf);
printf("%s\r\n", buf);
/* 输出示例：
Task Name       Abs Time    % Time
MyTask          12345       45%
IDLE            15000       54%
*/

/* 打印所有任务列表（名称、状态、优先级、剩余栈） */
vTaskList(buf);
printf("%s\r\n", buf);
```

---

## 典型使用场景

### 场景 1：周期采样任务

```c
void vSensorTask(void *p) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    SensorData_t data;

    for (;;) {
        /* 精确 10ms 采样一次 */
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));

        data.value = ADC_Read();
        data.timestamp = xTaskGetTickCount();

        /* 发送到处理队列 */
        xQueueSend(xProcessQueue, &data, 0); /* 不等待，丢弃比阻塞好 */
    }
}
```

### 场景 2：事件驱动任务（等待中断）

```c
void vButtonTask(void *p) {
    for (;;) {
        /* 阻塞等待按键信号量，零 CPU 占用 */
        xSemaphoreTake(xButtonSem, portMAX_DELAY);

        /* 消抖 */
        vTaskDelay(pdMS_TO_TICKS(20));

        /* 判断确实按下 */
        if (HAL_GPIO_ReadPin(BTN_GPIO_Port, BTN_Pin) == GPIO_PIN_RESET) {
            HandleButtonPress();
        }
    }
}
```

### 场景 3：带参数的多实例任务

```c
typedef struct {
    GPIO_TypeDef *port;
    uint16_t      pin;
    uint32_t      period_ms;
} LedConfig_t;

void vLedTask(void *pvParam) {
    LedConfig_t *cfg = (LedConfig_t *)pvParam;

    for (;;) {
        HAL_GPIO_TogglePin(cfg->port, cfg->pin);
        vTaskDelay(pdMS_TO_TICKS(cfg->period_ms));
    }
}

/* 创建两个 LED 任务，不同引脚不同频率 */
static LedConfig_t led1 = {GPIOC, GPIO_PIN_13, 500};
static LedConfig_t led2 = {GPIOA, GPIO_PIN_0,  200};

xTaskCreate(vLedTask, "LED1", 64, &led1, 1, NULL);
xTaskCreate(vLedTask, "LED2", 64, &led2, 1, NULL);
```

---

## 常见错误

### 任务函数意外返回

```c
/* ❌ 错误：函数正常返回，行为未定义，通常直接崩溃 */
void vBadTask(void *p) {
    DoSomething();
    /* 忘记写 for(;;) 或 vTaskDelete(NULL) */
}

/* ✅ 正确 */
void vGoodTask(void *p) {
    for (;;) {
        DoSomething();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

### 没有阻塞点

```c
/* ❌ 错误：任务一直运行不让出 CPU，低优先级任务饿死 */
void vGreedyTask(void *p) {
    for (;;) {
        DoWork();
        /* 没有任何 vTaskDelay 或等待操作 */
    }
}
```

### 栈上分配大数组

```c
/* ❌ 危险：2KB 数组在栈上，很容易溢出 */
void vBigStackTask(void *p) {
    uint8_t buffer[2048]; /* 局部变量在任务栈上 */
    for (;;) { ... }
}

/* ✅ 改为静态或全局变量 */
static uint8_t buffer[2048]; /* 静态变量在 BSS 段，不占任务栈 */
void vSafeTask(void *p) {
    for (;;) { ... }
}
```

### 在调度器启动前调用阻塞 API

```c
/* ❌ 错误：调度器没启动，vTaskDelay 没有意义 */
int main(void) {
    xTaskCreate(...);
    vTaskDelay(1000); /* 错误！ */
    osKernelStart();
}

/* ✅ 调度器启动前只能用 HAL_Delay */
int main(void) {
    HAL_Init();
    HAL_Delay(100); /* 正确，调度器启动前 */
    xTaskCreate(...);
    osKernelStart();
}
```

---

## API 速查

| API | 参数说明 | 返回值 |
|-----|---------|--------|
| `xTaskCreate(fn, name, stack, param, prio, handle)` | fn=任务函数, stack=words, prio=优先级 | `pdPASS` / `errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY` |
| `xTaskCreateStatic(fn, name, stack, param, prio, stackBuf, tcb)` | 静态创建 | `TaskHandle_t` |
| `vTaskDelete(handle)` | NULL=删自身 | void |
| `vTaskDelay(ticks)` | `pdMS_TO_TICKS(ms)` 换算 | void |
| `vTaskDelayUntil(&lastTime, ticks)` | lastTime 自动更新 | void |
| `vTaskSuspend(handle)` | NULL=挂起自身 | void |
| `vTaskResume(handle)` | — | void |
| `xTaskResumeFromISR(handle)` | — | `pdTRUE`=需要切换 |
| `vTaskPrioritySet(handle, prio)` | — | void |
| `uxTaskPriorityGet(handle)` | — | 当前优先级 |
| `uxTaskGetStackHighWaterMark(handle)` | NULL=查自身 | 最小剩余栈（words） |
| `xTaskGetTickCount()` | — | 当前节拍计数 |
| `pcTaskGetName(handle)` | — | 任务名字符串 |
| `taskYIELD()` | 主动让出 CPU | void |
| `taskENTER_CRITICAL()` | 进入临界区（关中断） | void |
| `taskEXIT_CRITICAL()` | 退出临界区（开中断） | void |
