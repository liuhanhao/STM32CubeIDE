# FreeRTOS 队列（Queue）

## 目录

- [核心概念](#核心概念)
- [工作原理](#工作原理)
- [创建队列](#创建队列)
- [发送数据](#发送数据)
- [接收数据](#接收数据)
- [在中断中使用队列](#在中断中使用队列)
- [传递复杂数据](#传递复杂数据)
- [队列集（Queue Set）](#队列集queue-set)
- [邮箱（Mailbox）](#邮箱mailbox)
- [典型使用场景](#典型使用场景)
- [常见错误](#常见错误)
- [API 速查](#api-速查)

---

## 核心概念

队列是 FreeRTOS 中**最基础的任务间通信机制**，用于在任务之间（以及中断与任务之间）**传递数据**。

队列的本质是一个有容量限制的 FIFO 缓冲区：
- 发送方向队列尾部写入数据
- 接收方从队列头部取出数据
- 队列满时，发送方可以选择等待或丢弃
- 队列空时，接收方可以选择等待或立即返回

**队列 vs 全局变量：** 直接用全局变量在任务间共享数据需要加锁保护，而队列内部已经处理好了线程安全问题，使用更简单。

---

## 工作原理

### 数据存储方式

FreeRTOS 队列采用**按值复制**的方式存储数据（而不是存指针）：

```
发送方变量  →  [复制到队列缓冲区]  →  接收方变量
   data             内部存储             received
```

这意味着：
- 发送后修改原始变量不会影响队列中的数据
- 传递指针是可以的，但指针指向的内存必须在接收方使用时仍然有效

### 阻塞机制

```
队列已满时发送（指定超时 > 0）：
  发送方任务 → 进入阻塞 → 等待队列有空位 → 超时或被唤醒

队列为空时接收（指定超时 > 0）：
  接收方任务 → 进入阻塞 → 等待队列有数据 → 超时或被唤醒
```

被阻塞的任务**不占用 CPU**，调度器会在条件满足时自动将其唤醒。

---

## 创建队列

```c
QueueHandle_t xQueue;

/* 创建队列
 * 参数1：队列深度（最多存几条消息）
 * 参数2：每条消息的大小（字节），必须与实际数据类型一致
 */
xQueue = xQueueCreate(10, sizeof(uint32_t));

/* 检查创建是否成功 */
if (xQueue == NULL) {
    /* 堆内存不足，通常需要增大 configTOTAL_HEAP_SIZE */
    Error_Handler();
}
```

**队列内存占用估算：**

```
内存占用 ≈ 队列控制块（约 80 字节）+ 队列深度 × 单条消息大小
示例：深度10，uint32_t → 80 + 10×4 = 120 字节
示例：深度5，结构体20字节 → 80 + 5×20 = 180 字节
```

### 静态创建

```c
static StaticQueue_t xQueueBuffer;
static uint8_t       xQueueStorage[10 * sizeof(uint32_t)]; /* 存储空间 */

xQueue = xQueueCreateStatic(10, sizeof(uint32_t),
                             xQueueStorage, &xQueueBuffer);
```

---

## 发送数据

### 基本发送

```c
uint32_t data = 42;

/* 发送到队列尾部，等待最多 100ms（队列满时阻塞） */
BaseType_t result = xQueueSend(xQueue, &data, pdMS_TO_TICKS(100));
if (result != pdTRUE) {
    /* 等待超时，数据未发出（队列仍然满） */
}

/* 永久等待直到队列有空位 */
xQueueSend(xQueue, &data, portMAX_DELAY);

/* 不等待，立即返回（失败也不等） */
xQueueSend(xQueue, &data, 0);
```

### 发送到队列头部

```c
/* 发到队头（优先被接收），适合高优先级消息或紧急命令 */
xQueueSendToFront(xQueue, &data, portMAX_DELAY);

/* 普通发到队尾（等价于 xQueueSend） */
xQueueSendToBack(xQueue, &data, portMAX_DELAY);
```

### 覆写（仅限深度为 1 的队列）

```c
/* 无论队列是否满，强制覆写（不阻塞，不失败） */
/* 常用于只关心"最新值"的场景，如传感器当前读数 */
xQueueOverwrite(xQueue, &data);
```

---

## 接收数据

```c
uint32_t received;

/* 阻塞接收，永久等待直到有数据 */
if (xQueueReceive(xQueue, &received, portMAX_DELAY) == pdTRUE) {
    /* 成功取出数据，处理 received */
    ProcessData(received);
}

/* 超时接收（等待最多 200ms） */
if (xQueueReceive(xQueue, &received, pdMS_TO_TICKS(200)) == pdTRUE) {
    /* 有数据 */
} else {
    /* 超时，队列仍为空 */
}

/* 只查看队头数据，不取走（不影响队列内容） */
xQueuePeek(xQueue, &received, portMAX_DELAY);

/* 查询队列中当前消息数 */
UBaseType_t count = uxQueueMessagesWaiting(xQueue);

/* 查询队列剩余空位 */
UBaseType_t spaces = uxQueueSpacesAvailable(xQueue);
```

---

## 在中断中使用队列

中断中不能使用带超时的 API，必须使用 `FromISR` 版本。

```c
/* ❌ 错误：中断中调用普通版本 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    xQueueSend(xQueue, &rxData, portMAX_DELAY); /* 错误！中断不能阻塞 */
}

/* ✅ 正确：使用 FromISR 版本 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    xQueueSendFromISR(xQueue, &rxData, &xHigherPriorityTaskWoken);

    /* 如果发送唤醒了更高优先级的任务，在退出中断时立即切换 */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
```

**`xHigherPriorityTaskWoken` 的作用：**

当队列发送唤醒了一个阻塞在此队列上的高优先级任务时，`xHigherPriorityTaskWoken` 被置为 `pdTRUE`，`portYIELD_FROM_ISR` 会在中断退出时立即触发任务切换，让高优先级任务得以立刻运行，而不用等到下一次 SysTick。

```c
/* 中断中接收（从队列取数据） */
BaseType_t xHigherPriorityTaskWoken = pdFALSE;
uint32_t data;
xQueueReceiveFromISR(xQueue, &data, &xHigherPriorityTaskWoken);
portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
```

---

## 传递复杂数据

### 传递结构体（完整数据副本）

适合数据量较小（通常 < 64 字节）的场景，简单安全。

```c
typedef struct {
    uint8_t  sensor_id;
    float    temperature;
    float    humidity;
    uint32_t timestamp_ms;
} SensorData_t;

QueueHandle_t xSensorQueue;
xSensorQueue = xQueueCreate(8, sizeof(SensorData_t));

/* 发送 */
SensorData_t tx = {
    .sensor_id    = 1,
    .temperature  = 25.3f,
    .humidity     = 60.0f,
    .timestamp_ms = HAL_GetTick()
};
xQueueSend(xSensorQueue, &tx, portMAX_DELAY);

/* 接收 */
SensorData_t rx;
xQueueReceive(xSensorQueue, &rx, portMAX_DELAY);
printf("Temp: %.1f, Humi: %.1f\r\n", rx.temperature, rx.humidity);
```

### 传递指针（大数据）

适合数据量大（图片、帧缓存等）的场景，只传指针而不复制数据本身。

**注意：必须保证接收方使用时指针仍然有效。** 通常配合内存池使用。

```c
/* 简单示例：传递静态缓冲区指针 */
static uint8_t frame_buf_a[1024];
static uint8_t frame_buf_b[1024];

QueueHandle_t xFrameQueue;
xFrameQueue = xQueueCreate(2, sizeof(uint8_t *));

/* 发送方：填充数据后发送指针 */
uint8_t *pBuf = frame_buf_a;
FillFrameData(pBuf, 1024);
xQueueSend(xFrameQueue, &pBuf, portMAX_DELAY);

/* 接收方：取出指针直接使用 */
uint8_t *pReceived;
xQueueReceive(xFrameQueue, &pReceived, portMAX_DELAY);
ProcessFrame(pReceived, 1024);
/* 使用完后通知发送方该缓冲区可以复用 */
```

### 传递命令（枚举 + 联合体）

适合命令/消息路由场景，一个队列传递多种不同类型的消息。

```c
/* 命令类型 */
typedef enum {
    CMD_LED_ON,
    CMD_LED_OFF,
    CMD_MOTOR_SPEED,
    CMD_RESET
} CmdType_t;

/* 命令消息体 */
typedef struct {
    CmdType_t type;
    union {
        uint8_t  led_id;
        uint16_t motor_rpm;
    } param;
} Command_t;

QueueHandle_t xCmdQueue;
xCmdQueue = xQueueCreate(10, sizeof(Command_t));

/* 发送命令 */
Command_t cmd = {.type = CMD_MOTOR_SPEED, .param.motor_rpm = 1500};
xQueueSend(xCmdQueue, &cmd, portMAX_DELAY);

/* 接收并分派 */
Command_t recv;
xQueueReceive(xCmdQueue, &recv, portMAX_DELAY);
switch (recv.type) {
    case CMD_LED_ON:      LED_On(recv.param.led_id);        break;
    case CMD_MOTOR_SPEED: Motor_SetRPM(recv.param.motor_rpm); break;
    case CMD_RESET:       SystemReset();                    break;
}
```

---

## 队列集（Queue Set）

队列集允许一个任务同时阻塞等待**多个队列或信号量**中的任何一个有数据，避免轮询。

```c
/* 创建两个队列和一个二值信号量 */
QueueHandle_t xQueue1 = xQueueCreate(5, sizeof(uint32_t));
QueueHandle_t xQueue2 = xQueueCreate(5, sizeof(uint32_t));
SemaphoreHandle_t xSem = xSemaphoreCreateBinary();

/* 创建队列集，容量 = 所有成员的总深度 */
QueueSetHandle_t xQueueSet = xQueueCreateSet(5 + 5 + 1);

/* 将队列和信号量加入集合 */
xQueueAddToSet(xQueue1, xQueueSet);
xQueueAddToSet(xQueue2, xQueueSet);
xQueueAddToSet(xSem,    xQueueSet);

/* 等待任意成员有数据 */
void vMonitorTask(void *p) {
    QueueSetMemberHandle_t xActiveMember;

    for (;;) {
        /* 阻塞等待集合中任意一个成员就绪 */
        xActiveMember = xQueueSelectFromSet(xQueueSet, portMAX_DELAY);

        if (xActiveMember == xQueue1) {
            uint32_t val;
            xQueueReceive(xQueue1, &val, 0);
            printf("Queue1: %lu\r\n", val);
        } else if (xActiveMember == xQueue2) {
            uint32_t val;
            xQueueReceive(xQueue2, &val, 0);
            printf("Queue2: %lu\r\n", val);
        } else if (xActiveMember == xSem) {
            xSemaphoreTake(xSem, 0);
            printf("Semaphore triggered\r\n");
        }
    }
}
```

> 队列集使用限制：加入集合后的队列不能直接被不知道集合的任务接收；`xQueueOverwrite` 不能用于集合中的队列。

---

## 邮箱（Mailbox）

邮箱是深度为 1 的特殊队列，存储一个"最新值"，允许多个任务随时读取而不取走（类似共享变量，但线程安全）。

```c
QueueHandle_t xMailbox = xQueueCreate(1, sizeof(float));

/* 写入最新温度值（覆写，不管之前有没有数据） */
void vSensorTask(void *p) {
    float temp;
    for (;;) {
        temp = ReadTemperature();
        xQueueOverwrite(xMailbox, &temp); /* 覆写，永不阻塞 */
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/* 任意任务随时读取最新值（Peek 不取走，可以多个任务读） */
void vDisplayTask(void *p) {
    float temp;
    for (;;) {
        xQueuePeek(xMailbox, &temp, portMAX_DELAY);
        Display_ShowTemperature(temp);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
```

---

## 典型使用场景

### 场景 1：中断驱动数据采集（中断 → 任务）

```
ADC/UART 中断 ──xQueueSendFromISR──→ [xDataQueue] ──xQueueReceive──→ 处理任务
```

```c
/* 中断回调 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
    uint16_t adcVal = HAL_ADC_GetValue(hadc);
    BaseType_t woken = pdFALSE;
    xQueueSendFromISR(xAdcQueue, &adcVal, &woken);
    portYIELD_FROM_ISR(woken);
}

/* 处理任务 */
void vAdcProcessTask(void *p) {
    uint16_t val;
    for (;;) {
        xQueueReceive(xAdcQueue, &val, portMAX_DELAY);
        float voltage = (val / 4095.0f) * 3.3f;
        /* 进一步处理 */
    }
}
```

### 场景 2：流水线处理

```
采集任务 → [原始队列] → 滤波任务 → [结果队列] → 上报任务
```

```c
QueueHandle_t xRawQueue    = xQueueCreate(10, sizeof(uint16_t));
QueueHandle_t xResultQueue = xQueueCreate(10, sizeof(float));

void vAcquireTask(void *p) {
    for (;;) {
        uint16_t raw = ADC_Read();
        xQueueSend(xRawQueue, &raw, 0);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void vFilterTask(void *p) {
    uint16_t raw;
    float filtered;
    for (;;) {
        xQueueReceive(xRawQueue, &raw, portMAX_DELAY);
        filtered = LowPassFilter(raw);
        xQueueSend(xResultQueue, &filtered, 0);
    }
}

void vReportTask(void *p) {
    float result;
    for (;;) {
        xQueueReceive(xResultQueue, &result, portMAX_DELAY);
        printf("Result: %.2f\r\n", result);
    }
}
```

### 场景 3：命令分发

```
按键/串口/BLE ──→ [xCmdQueue] ──→ 命令处理任务 ──→ 各执行模块
```

---

## 常见错误

### 队列深度太小导致丢数据

```c
/* ❌ 危险：深度为 1，发送方比接收方快时丢数据 */
xQueue = xQueueCreate(1, sizeof(uint16_t));

/* ✅ 根据数据产生速率和消费速率确定合适深度 */
/* 深度 = 最大突发数量 × 1.5（留余量） */
xQueue = xQueueCreate(20, sizeof(uint16_t));
```

### 发送超时被忽略

```c
/* ❌ 问题：队列满时发送失败但没有处理 */
xQueueSend(xQueue, &data, 0);

/* ✅ 根据业务决定：丢弃、重试还是报警 */
if (xQueueSend(xQueue, &data, pdMS_TO_TICKS(10)) != pdTRUE) {
    g_drop_count++;  /* 统计丢包数 */
}
```

### 传递指针指向的栈变量

```c
/* ❌ 危险：局部变量在函数返回后失效 */
void vBadTask(void *p) {
    uint8_t buf[64];         /* 局部变量，在任务栈上 */
    FillData(buf);
    xQueueSend(xQueue, &buf, 0); /* 发送的是指向栈的指针（如果队列存指针） */
    /* 函数执行完后 buf 被覆盖，接收方拿到的指针已失效 */
}

/* ✅ 使用静态或堆分配的内存 */
static uint8_t buf[64]; /* 静态变量，生命周期贯穿整个程序 */
```

### 在中断中使用带超时的 API

```c
/* ❌ 错误：中断不能阻塞 */
xQueueSend(xQueue, &data, portMAX_DELAY);

/* ✅ 中断中只用 FromISR 版本 */
xQueueSendFromISR(xQueue, &data, &xHigherPriorityTaskWoken);
```

---

## API 速查

| API | 说明 | 可用场景 |
|-----|------|---------|
| `xQueueCreate(len, size)` | 创建队列 | 调度器启动前/后 |
| `xQueueCreateStatic(...)` | 静态创建 | 调度器启动前/后 |
| `vQueueDelete(q)` | 删除队列 | 任务 |
| `xQueueSend(q, &data, timeout)` | 发送到队尾 | 任务 |
| `xQueueSendToFront(q, &data, timeout)` | 发送到队头 | 任务 |
| `xQueueSendToBack(q, &data, timeout)` | 发送到队尾（同 Send） | 任务 |
| `xQueueOverwrite(q, &data)` | 覆写（深度须为1） | 任务 |
| `xQueueReceive(q, &buf, timeout)` | 取出数据 | 任务 |
| `xQueuePeek(q, &buf, timeout)` | 查看不取出 | 任务 |
| `xQueueSendFromISR(q, &data, &woken)` | 中断发送到队尾 | 中断 |
| `xQueueSendToFrontFromISR(q, &data, &woken)` | 中断发送到队头 | 中断 |
| `xQueueOverwriteFromISR(q, &data, &woken)` | 中断覆写 | 中断 |
| `xQueueReceiveFromISR(q, &buf, &woken)` | 中断接收 | 中断 |
| `uxQueueMessagesWaiting(q)` | 当前消息数 | 任务/中断 |
| `uxQueueSpacesAvailable(q)` | 剩余空位 | 任务/中断 |
| `xQueueReset(q)` | 清空队列 | 任务 |
