# FreeRTOS 工程创建与使用指南

> **平台支持：** FreeRTOS 同时支持 **STM32（HAL 库）** 和 **ESP32（ESP-IDF）**。
> - STM32：通过 STM32CubeMX 集成，本指南以此为主
> - ESP32（IDF）：IDF 内核本身就是 FreeRTOS，`freertos/FreeRTOS.h` 直接可用，任务/队列/信号量等 API 完全一致，无需额外配置

## 目录

- [FreeRTOS 核心概念](#freertos-核心概念)
- [STM32CubeMX 配置 FreeRTOS](#stm32cubemx-配置-freertos)
- [任务（Task）](#任务task) → 详细文档：[FreeRTOS_Task.md](FreeRTOS_Task.md)
- [队列（Queue）](#队列queue) → 详细文档：[FreeRTOS_Queue.md](FreeRTOS_Queue.md)
- [信号量（Semaphore）](#信号量semaphore) → 详细文档：[FreeRTOS_Semaphore.md](FreeRTOS_Semaphore.md)
- [互斥量（Mutex）](#互斥量mutex) → 详细文档：[FreeRTOS_Mutex.md](FreeRTOS_Mutex.md)
- [软件定时器（Software Timer）](#软件定时器software-timer) → 详细文档：[FreeRTOS_SoftwareTimer.md](FreeRTOS_SoftwareTimer.md)
- [事件组（Event Group）](#事件组event-group) → 详细文档：[FreeRTOS_EventGroup.md](FreeRTOS_EventGroup.md)
- [任务通知（Task Notification）](#任务通知task-notification) → 详细文档：[FreeRTOS_TaskNotification.md](FreeRTOS_TaskNotification.md)
- [内存管理](#内存管理) → 详细文档：[FreeRTOS_Memory.md](FreeRTOS_Memory.md)
- [常见问题与注意事项](#常见问题与注意事项)
- [典型工程结构](#典型工程结构)

---

## FreeRTOS 核心概念

FreeRTOS 是一个轻量级实时操作系统内核，核心提供任务调度器。在 STM32F103 上，资源占用约 5~10KB Flash、2~4KB RAM，裕量充足。

**引入 FreeRTOS 的时机**：任务超过 3~4 个且有优先级差异、需要任务间同步通信、裸机 `while(1)` 循环逻辑越来越难维护时。

### 调度机制

FreeRTOS 使用**抢占式优先级调度**，优先级高的任务就绪后立即抢占低优先级任务。数值越大，优先级越高（与 NVIC 相反）。

| 调度条件 | 触发时机 |
|---------|---------|
| 高优先级就绪 | 立即抢占当前任务 |
| 同优先级竞争 | 时间片轮转（默认 1ms 一次） |
| 任务主动让出 | 调用阻塞 API（Delay、等待队列等） |

### 任务状态机

```
           创建
            ↓
         就绪（Ready）
            ↓ 调度器选中
         运行（Running）
            ↓ 主动阻塞或被抢占
     ┌──阻塞（Blocked）  ←── 等待延时/队列/信号量
     │   挂起（Suspended）←── 手动 vTaskSuspend
     └──就绪（Ready）   ←── 超时/资源可用
```

### 关键数据类型

| 类型 | 说明 | 典型用途 |
|------|------|---------|
| `TaskHandle_t` | 任务句柄 | 引用/操作某个任务 |
| `QueueHandle_t` | 队列句柄 | 操作队列 |
| `SemaphoreHandle_t` | 信号量/互斥量句柄 | 同步与互斥 |
| `TimerHandle_t` | 软件定时器句柄 | 操作定时器 |
| `EventGroupHandle_t` | 事件组句柄 | 多事件同步 |
| `TickType_t` | 节拍计数（毫秒） | 超时参数 |
| `BaseType_t` | 整型，用于返回值 | `pdTRUE` / `pdFALSE` |
| `UBaseType_t` | 无符号整型 | 优先级、计数值 |

---

## STM32CubeMX 配置 FreeRTOS

### 第一步：启用 FreeRTOS

左侧 `Middleware` → `FREERTOS`：
- **Interface** 选 `CMSIS_V2`（更新的 API，推荐）
- 或 `CMSIS_V1`（旧版 API，兼容性好）

> **CMSIS_V1 vs CMSIS_V2：** 两者都是对 FreeRTOS 原生 API 的封装。V2 提供更丰富的属性控制，与 FreeRTOS 原生 API 更接近。新工程建议选 V2。实际上两者最终都调用同一套 FreeRTOS 内核。

### 第二步：配置 SYS 的时基源

**必须将 SysTick 改成其他定时器**。原因：FreeRTOS 自己使用 SysTick 作为节拍源，如果 HAL_Delay 也用 SysTick 会冲突。

左侧 `System Core` → `SYS`：
- **Timebase Source** 改为 `TIM4`（或任意空闲的基础/通用定时器）

> 生成代码后，`HAL_Delay` 会自动切换为使用 TIM4 中断计时，与 FreeRTOS 完全独立。

### 第三步：NVIC 优先级分组（重要）

左侧 `System Core` → `NVIC`：
- **Priority Group** 确认为 `4 bits for pre-emption priority`（对应 `NVIC_PRIORITYGROUP_4`）

FreeRTOS 要求所有管理中断的优先级必须低于 `configMAX_SYSCALL_INTERRUPT_PRIORITY`（默认值 5，即中断优先级 5~15 可以调用带 `FromISR` 后缀的 API）。

**在 ISR 中调用 FreeRTOS API 的规则：**

| 中断优先级 | 可用 API |
|-----------|---------|
| 0 ~ 4（高优先级） | ❌ 不允许调用任何 FreeRTOS API |
| 5 ~ 15（低优先级） | ✅ 只能调用 `xxxFromISR` 系列 API |
| 任务上下文 | ✅ 可以调用普通 API |

### 第四步：FreeRTOS 关键参数配置

在 FreeRTOS 配置页 `Config parameters` 标签中，常用参数：

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `TOTAL_HEAP_SIZE` | 3072 字节 | 堆总大小，任务栈、队列等都从这里分配 |
| `MAX_PRIORITIES` | 56 | 最大优先级数量，STM32F1 资源有限建议改为 `7` |
| `TICK_RATE_HZ` | 1000 | 节拍频率（1ms/tick），不建议改 |
| `MAX_TASK_NAME_LEN` | 16 | 任务名最大长度 |
| `USE_TRACE_FACILITY` | 0 | 调试追踪，一般关闭 |
| `USE_STATS_FORMATTING_FUNCTIONS` | 0 | 运行时统计，调试时可开 |

**TOTAL_HEAP_SIZE 估算：**

```
每个任务约占用：128字节固定开销 + 任务栈大小（字节）
STM32F103C8T6 总 SRAM 20KB，建议 FreeRTOS 堆不超过 12KB
```

### 第五步：添加默认任务（CubeMX 自动生成）

在 `Tasks and Queues` 标签，CubeMX 已默认生成一个 `defaultTask`，优先级 `normal`，栈大小 128 words（512 字节）。

可以在这里直接添加更多任务、队列、信号量（生成桩代码），也可以之后手动在代码中创建。

### 第六步：生成代码

点击 **GENERATE CODE**，CubeMX 会生成：

| 文件 | 内容 |
|------|------|
| `freertos.c` | 任务入口函数，`MX_FREERTOS_Init()` |
| `FreeRTOS/` | FreeRTOS 内核源码 |
| `FreeRTOSConfig.h` | 所有配置参数（由 CubeMX 参数自动生成） |
| `main.c` | `MX_FREERTOS_Init()` 调用后 `osKernelStart()` 启动调度器 |

> **重要：** 用户代码写在 `/* USER CODE BEGIN */` 和 `/* USER CODE END */` 之间，CubeMX 重新生成代码时会保留这些区域的内容。

---

## 任务（Task）

任务是 FreeRTOS 的基本执行单元，每个任务有独立的栈空间。

### 创建任务

```c
TaskHandle_t xTask1Handle = NULL;

/* 任务函数，必须是 void 返回值，void* 参数，且不能返回 */
void vTask1(void *pvParameters) {
    /* 初始化代码（只执行一次） */
    
    for (;;) {
        /* 周期性逻辑 */
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        
        /* 阻塞延时，释放 CPU 给其他任务 */
        vTaskDelay(pdMS_TO_TICKS(500));  /* 延时 500ms */
    }
    /* 理论上不应到达这里，但如果任务要退出必须调用 vTaskDelete(NULL) */
}

/* 在 MX_FREERTOS_Init() 或 main() 的 USER CODE 区域创建任务 */
xTaskCreate(
    vTask1,           /* 任务函数指针 */
    "Task1",          /* 任务名称（调试用） */
    128,              /* 栈大小，单位 words（1 word = 4 字节） */
    NULL,             /* 传给任务函数的参数 */
    2,                /* 优先级（数值越大越高） */
    &xTask1Handle     /* 任务句柄，不需要可填 NULL */
);
```

### 任务优先级规划建议

| 优先级 | 用途 |
|--------|------|
| 1 | 后台低优先级任务（日志、状态显示） |
| 2 | 普通业务任务（数据处理、控制逻辑） |
| 3 | 通信任务（UART 收发、CAN 处理） |
| 4 | 时间敏感任务（电机控制、传感器采集） |
| 5+ | 紧急任务（保留，极少使用） |

> `configMAX_PRIORITIES` 设为 7 时，优先级范围 0~6，0 为空闲任务（不可用），用户任务从 1 开始。

### 任务管理 API

```c
vTaskDelay(pdMS_TO_TICKS(100));          /* 相对延时 100ms */
vTaskDelayUntil(&xLastTime, pdMS_TO_TICKS(10)); /* 绝对延时，适合固定频率任务 */

vTaskSuspend(xTask1Handle);              /* 挂起任务 */
vTaskResume(xTask1Handle);               /* 恢复任务 */
vTaskDelete(xTask1Handle);               /* 删除任务（NULL 表示删除自身） */

UBaseType_t uxPriority = uxTaskPriorityGet(xTask1Handle);  /* 读取优先级 */
vTaskPrioritySet(xTask1Handle, 3);                          /* 动态修改优先级 */

UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark(xTask1Handle); /* 栈最小剩余 */
```

### `vTaskDelay` vs `vTaskDelayUntil`

```c
/* vTaskDelay：从当前时刻延时，执行时间不固定 */
void vTaskA(void *p) {
    for (;;) {
        DoWork();        /* 假设耗时 3ms */
        vTaskDelay(pdMS_TO_TICKS(10)); /* 等 10ms，实际周期 = 3 + 10 = 13ms */
    }
}

/* vTaskDelayUntil：固定周期，适合控制环、采样等 */
void vTaskB(void *p) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    for (;;) {
        DoWork();        /* 耗时不影响周期 */
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10)); /* 严格 10ms 周期 */
    }
}
```

---

## 队列（Queue）

队列用于任务间传递数据，是最常用的任务间通信机制。先进先出（FIFO），支持多个任务同时读写。

### 创建队列

```c
QueueHandle_t xQueue;

/* 创建可存放 10 个 uint32_t 的队列 */
xQueue = xQueueCreate(10, sizeof(uint32_t));
if (xQueue == NULL) {
    /* 内存不足，创建失败 */
    Error_Handler();
}
```

### 发送数据

```c
uint32_t data = 1234;

/* 任务中发送，等待 100ms；队列满时阻塞 */
if (xQueueSend(xQueue, &data, pdMS_TO_TICKS(100)) != pdTRUE) {
    /* 超时，队列仍然满 */
}

/* 发到队列头部（高优先级数据） */
xQueueSendToFront(xQueue, &data, portMAX_DELAY);

/* 中断中发送，不能使用延时版本 */
BaseType_t xHigherPriorityTaskWoken = pdFALSE;
xQueueSendFromISR(xQueue, &data, &xHigherPriorityTaskWoken);
portYIELD_FROM_ISR(xHigherPriorityTaskWoken);  /* 必须加这一行 */
```

### 接收数据

```c
uint32_t received;

/* 阻塞等待，永久等待直到有数据 */
if (xQueueReceive(xQueue, &received, portMAX_DELAY) == pdTRUE) {
    /* 处理 received */
}

/* 只查看不取走 */
xQueuePeek(xQueue, &received, 0);

/* 查询队列中已有的消息数 */
UBaseType_t count = uxQueueMessagesWaiting(xQueue);
```

### 传递结构体（复杂数据）

```c
typedef struct {
    uint16_t sensor_id;
    float    value;
    uint32_t timestamp;
} SensorData_t;

QueueHandle_t xSensorQueue;
xSensorQueue = xQueueCreate(5, sizeof(SensorData_t));

/* 发送 */
SensorData_t data = {.sensor_id = 1, .value = 25.6f, .timestamp = HAL_GetTick()};
xQueueSend(xSensorQueue, &data, portMAX_DELAY);

/* 接收 */
SensorData_t received;
xQueueReceive(xSensorQueue, &received, portMAX_DELAY);
```

---

## 信号量（Semaphore）

信号量用于**任务同步**，表示"某件事发生了"。不携带数据，只有计数值。

### 二值信号量（Binary Semaphore）

适合任务与中断同步——中断发生后通知任务处理。

```c
SemaphoreHandle_t xSemaphore;

/* 创建 */
xSemaphore = xSemaphoreCreateBinary();

/* 在中断中"给出"信号量（释放） */
void EXTI0_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_0) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(xSemaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

/* 在任务中"获取"信号量（等待事件） */
void vButtonTask(void *p) {
    for (;;) {
        /* 永久阻塞等待按键中断触发 */
        if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE) {
            /* 处理按键事件 */
            ProcessButton();
        }
    }
}
```

### 计数信号量（Counting Semaphore）

适合管理资源池（如有 N 个缓冲区，每用一个减一，释放后加一）。

```c
/* 创建计数信号量，最大值 5，初始值 5（5 个资源全可用） */
SemaphoreHandle_t xCountSem = xSemaphoreCreateCounting(5, 5);

/* 申请资源 */
xSemaphoreTake(xCountSem, portMAX_DELAY);  /* 计数 -1 */
UseResource();

/* 释放资源 */
xSemaphoreGive(xCountSem);  /* 计数 +1 */
```

---

## 互斥量（Mutex）

互斥量用于**资源互斥保护**，同一时刻只有一个任务能持有。内置**优先级继承**，可防止优先级反转问题。

> **信号量 vs 互斥量：** 信号量用于同步（谁都可以 Give/Take）；互斥量用于互斥（只有持有者才能 Give）。

```c
SemaphoreHandle_t xUartMutex;

/* 创建互斥量 */
xUartMutex = xSemaphoreCreateMutex();

/* 任务 A：带互斥保护的串口打印 */
void vTaskA(void *p) {
    for (;;) {
        /* 获取互斥量，最多等 100ms */
        if (xSemaphoreTake(xUartMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            /* 临界区：独占使用 UART */
            printf("Task A: %d\r\n", counter_a);
            
            /* 释放互斥量，必须由持有者释放 */
            xSemaphoreGive(xUartMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

/* 任务 B 同样使用 xUartMutex 保护，与 A 互斥 */
```

> **注意：互斥量不能在中断中使用**（因为优先级继承机制需要任务上下文）。中断中保护共享资源应用**临界区**或**挂起调度器**。

---

## 软件定时器（Software Timer）

软件定时器在定时器任务（Timer Service Task）中执行回调，不占用独立任务栈，但回调函数**不能阻塞**。

```c
TimerHandle_t xTimer;

/* 定时器回调函数，不能调用任何阻塞 API */
void vTimerCallback(TimerHandle_t xTimer) {
    /* 快速处理，不能阻塞 */
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
}

/* 创建单次定时器（pdFALSE）或周期定时器（pdTRUE） */
xTimer = xTimerCreate(
    "LED_Timer",               /* 名称 */
    pdMS_TO_TICKS(1000),       /* 超时时间 1s */
    pdTRUE,                    /* pdTRUE=周期, pdFALSE=单次 */
    (void *)0,                 /* 定时器 ID，回调中可读取 */
    vTimerCallback             /* 回调函数 */
);

/* 启动 */
xTimerStart(xTimer, 0);

/* 停止 */
xTimerStop(xTimer, 0);

/* 复位（重新计时） */
xTimerReset(xTimer, 0);

/* 修改周期 */
xTimerChangePeriod(xTimer, pdMS_TO_TICKS(500), 0);
```

**软件定时器 vs 硬件定时器**

| 对比项 | 软件定时器 | 硬件定时器（TIM） |
|--------|-----------|-----------------|
| 精度 | ±1 tick（±1ms）| 微秒级 |
| 数量 | 几乎无限制 | F103 最多 7 个 |
| 回调延迟 | 受定时器任务优先级影响 | 立即（中断） |
| 适用场景 | 超时检测、周期 UI 刷新 | 精确 PWM、输入捕获 |

---

## 事件组（Event Group）

事件组提供 24 个独立的事件位（bit），一个任务可以等待多个事件同时满足，常用于**多事件同步**。

```c
EventGroupHandle_t xEventGroup;

/* 定义事件位 */
#define EVT_SENSOR_DONE   (1 << 0)  /* bit0：传感器采集完成 */
#define EVT_UART_RECEIVED (1 << 1)  /* bit1：串口收到数据 */
#define EVT_BUTTON_PRESS  (1 << 2)  /* bit2：按键按下 */

/* 创建 */
xEventGroup = xEventGroupCreate();

/* 任务/中断设置事件位 */
xEventGroupSetBits(xEventGroup, EVT_SENSOR_DONE);

/* 中断中设置 */
BaseType_t xHigherPriorityTaskWoken = pdFALSE;
xEventGroupSetBitsFromISR(xEventGroup, EVT_UART_RECEIVED, &xHigherPriorityTaskWoken);
portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

/* 等待任意一个事件（OR） */
EventBits_t bits = xEventGroupWaitBits(
    xEventGroup,
    EVT_SENSOR_DONE | EVT_UART_RECEIVED,  /* 等待的位 */
    pdTRUE,    /* 等到后清除这些位 */
    pdFALSE,   /* pdFALSE = 任意一个，pdTRUE = 必须全部 */
    portMAX_DELAY
);

if (bits & EVT_SENSOR_DONE) {
    /* 传感器采集完成 */
}
if (bits & EVT_UART_RECEIVED) {
    /* 串口有数据 */
}

/* 等待全部事件同时满足（AND） */
xEventGroupWaitBits(xEventGroup,
    EVT_SENSOR_DONE | EVT_UART_RECEIVED | EVT_BUTTON_PRESS,
    pdTRUE, pdTRUE, portMAX_DELAY);
```

---

## 任务通知（Task Notification）

每个任务内置一个 32 位通知值，比信号量和队列开销更小（无需创建对象），适合点对点通知。

```c
/* 接收通知（阻塞等待） */
void vReceiverTask(void *p) {
    uint32_t ulValue;
    for (;;) {
        /* 等待通知，同时清除所有位，获取通知值 */
        xTaskNotifyWait(0x00, 0xFFFFFFFF, &ulValue, portMAX_DELAY);
        /* 处理 ulValue */
    }
}

/* 发送通知（普通设置值） */
xTaskNotify(xReceiverHandle, 0x01, eSetBits);       /* 按位置 1 */
xTaskNotify(xReceiverHandle, 100, eSetValueWithOverwrite); /* 直接设置值 */

/* 中断中发送 */
BaseType_t xHigherPriorityTaskWoken = pdFALSE;
vTaskNotifyGiveFromISR(xReceiverHandle, &xHigherPriorityTaskWoken);
portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

/* 对应接收（简化版，等价于计数信号量） */
ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  /* pdTRUE=清零, pdFALSE=减一 */
```

**通知 vs 二值信号量对比**

| 特性 | 任务通知 | 二值信号量 |
|------|---------|-----------|
| RAM 开销 | 0（任务内置） | ~80 字节 |
| 速度 | 更快 | 较快 |
| 限制 | 只能点对点（一个接收者） | 多任务可等待同一信号量 |

---

## 内存管理

FreeRTOS 提供 5 种堆管理方案（heap_1 ~ heap_5），CubeMX 默认使用 **heap_4**。

| 方案 | 支持 free | 碎片整理 | 适用场景 |
|------|-----------|----------|---------|
| heap_1 | ❌ | ❌ | 只分配不释放的简单系统 |
| heap_2 | ✅ | ❌ | 大小固定的重复分配 |
| heap_3 | ✅ | — | 使用标准库 malloc/free |
| **heap_4** | ✅ | ✅ 合并相邻块 | **推荐，通用场景** |
| heap_5 | ✅ | ✅ | 内存跨多个非连续区域 |

```c
/* 查询剩余堆大小（调试用） */
size_t freeHeap = xPortGetFreeHeapSize();

/* 查询历史最小堆大小（判断是否快耗尽） */
size_t minFreeHeap = xPortGetMinimumEverFreeHeapSize();
```

**堆空间不足的症状：** `xTaskCreate`、`xQueueCreate` 返回 NULL；系统在启动阶段崩溃。解决方法是增大 `configTOTAL_HEAP_SIZE` 或减少任务栈大小。

---

## 常见问题与注意事项

### 1. 中断中必须用 `FromISR` 版本的 API

```c
/* ❌ 错误：在中断中调用普通版本 */
void UART_RxCpltCallback() {
    xQueueSend(xQueue, &data, 0);  /* 不能在中断里用这个 */
}

/* ✅ 正确：使用 FromISR 版本并处理任务切换 */
void UART_RxCpltCallback() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(xQueue, &data, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
```

### 2. 调用 FreeRTOS API 的中断优先级限制

NVIC 优先级 0~4 禁止调用任何 FreeRTOS API。需要非常快响应的中断（如电机紧急停止）分配优先级 0~4，在中断中直接操作寄存器，不调用 FreeRTOS。

### 3. `HAL_Delay` 在 FreeRTOS 中的问题

`HAL_Delay` 是阻塞忙等待，会占用 CPU 不让出给其他任务。FreeRTOS 中应改用 `vTaskDelay`。

```c
/* ❌ 不要在任务中用这个 */
HAL_Delay(100);

/* ✅ 应该用这个 */
vTaskDelay(pdMS_TO_TICKS(100));
```

> `HAL_Delay` 只在 FreeRTOS 启动前（初始化阶段）或无法使用 FreeRTOS 的极少数场景使用。

### 4. 任务栈溢出检测

在 `FreeRTOSConfig.h` 中开启栈溢出检测（CubeMX 里在 Config parameters 中设置 `STACK_OVERFLOW_DETECTION` 为 2）：

```c
/* 实现栈溢出钩子函数 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    /* 溢出时会调用这里，打印任务名定位问题 */
    printf("Stack overflow in task: %s\r\n", pcTaskName);
    taskDISABLE_INTERRUPTS();
    for (;;);
}
```

### 5. 死锁（Deadlock）

两个任务互相等待对方持有的互斥量时发生死锁。避免方法：

- 所有任务以相同顺序获取多个互斥量
- 使用超时版本的 `xSemaphoreTake`，超时后放弃

### 6. 开机调度器启动前不能调用 FreeRTOS API

```c
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    
    MX_FREERTOS_Init();   /* 创建任务、队列、信号量 */
    osKernelStart();       /* 启动调度器，此后不再返回 */
    
    /* 这里的代码永远不会执行 */
    for (;;);
}
```

### 7. 全局变量的多任务访问保护

多个任务都会读写的全局变量必须加保护：

```c
/* 方案 1：互斥量（推荐，有优先级继承） */
xSemaphoreTake(xMutex, portMAX_DELAY);
g_shared_var = new_value;
xSemaphoreGive(xMutex);

/* 方案 2：临界区（屏蔽中断，时间要极短） */
taskENTER_CRITICAL();
g_shared_var = new_value;
taskEXIT_CRITICAL();
```

---

## 典型工程结构

以下是一个包含传感器采集、数据处理、串口上报三个任务的典型架构：

```
main()
├── MX_FREERTOS_Init()
│   ├── 创建队列：xSensorQueue（传感器数据）
│   ├── 创建互斥量：xUartMutex（串口保护）
│   ├── 创建任务：vSensorTask（优先级 4）
│   ├── 创建任务：vProcessTask（优先级 3）
│   └── 创建任务：vReportTask（优先级 2）
└── osKernelStart()

vSensorTask（最高优先级）
    └── 每 10ms 采集一次 ADC/I2C 传感器
    └── xQueueSend 到 xSensorQueue
    
vProcessTask（中等优先级）
    └── xQueueReceive 从 xSensorQueue 获取数据
    └── 滤波、换算
    └── xQueueSend 到 xResultQueue
    
vReportTask（低优先级）
    └── xQueueReceive 从 xResultQueue
    └── 加互斥量保护
    └── printf/UART 输出
```

**任务通信数据流示意：**

```
传感器中断/轮询 → vSensorTask → [xSensorQueue] → vProcessTask → [xResultQueue] → vReportTask → UART
```

---

## 快速 API 速查表

### 任务

| API | 说明 |
|-----|------|
| `xTaskCreate(fn, name, stack, param, prio, handle)` | 创建任务 |
| `vTaskDelete(handle)` | 删除任务（NULL 删自身） |
| `vTaskDelay(ticks)` | 相对延时 |
| `vTaskDelayUntil(&lastTime, ticks)` | 固定周期延时 |
| `vTaskSuspend(handle)` | 挂起任务 |
| `vTaskResume(handle)` | 恢复任务 |
| `uxTaskGetStackHighWaterMark(handle)` | 栈最小剩余 words |

### 队列

| API | 说明 |
|-----|------|
| `xQueueCreate(length, itemSize)` | 创建队列 |
| `xQueueSend(q, &data, timeout)` | 发送到队尾 |
| `xQueueSendToFront(q, &data, timeout)` | 发送到队头 |
| `xQueueReceive(q, &buf, timeout)` | 取出数据 |
| `xQueuePeek(q, &buf, timeout)` | 查看不取出 |
| `xQueueSendFromISR(q, &data, &woken)` | 中断中发送 |
| `uxQueueMessagesWaiting(q)` | 队列中消息数 |

### 信号量 / 互斥量

| API | 说明 |
|-----|------|
| `xSemaphoreCreateBinary()` | 创建二值信号量 |
| `xSemaphoreCreateCounting(max, init)` | 创建计数信号量 |
| `xSemaphoreCreateMutex()` | 创建互斥量 |
| `xSemaphoreTake(sem, timeout)` | 获取（等待） |
| `xSemaphoreGive(sem)` | 释放 |
| `xSemaphoreGiveFromISR(sem, &woken)` | 中断中释放 |

### 时间换算

```c
pdMS_TO_TICKS(100)   /* 100ms → ticks，推荐用这个宏，不要硬编码 */
portMAX_DELAY        /* 永久等待 */
0                    /* 不等待，立即返回 */
```
