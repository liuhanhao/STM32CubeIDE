# FreeRTOS 事件组（Event Group）

## 目录

- [核心概念](#核心概念)
- [事件位与事件组](#事件位与事件组)
- [创建事件组](#创建事件组)
- [设置事件位](#设置事件位)
- [等待事件位](#等待事件位)
- [清除与查询事件位](#清除与查询事件位)
- [在中断中使用事件组](#在中断中使用事件组)
- [事件组 vs 其他同步机制](#事件组-vs-其他同步机制)
- [典型使用场景](#典型使用场景)
- [注意事项](#注意事项)
- [API 速查](#api-速查)

---

## 核心概念

事件组是一组**二进制标志位（bit flags）**，每个 bit 代表一个独立的事件。任务可以：

- **设置**某些 bit（通知事件发生了）
- **等待**一个或多个 bit 被置位（支持 AND/OR 两种等待条件）
- **清除** bit（消费事件）

相比信号量（只有一个计数值），事件组可以**一次同步多个事件**，是信号量和队列无法替代的场景。

**核心能力：**

```
一个任务等待：「事件A 发生」并且「事件B 发生」并且「事件C 发生」（AND）
或者等待：「事件A 发生」或者「事件B 发生」（OR）
```

---

## 事件位与事件组

`EventBits_t` 类型在不同配置下为 16 位或 24 位（通常是 24 位可用，高 8 位内核占用）。

```
bit23  bit22  ...  bit2  bit1  bit0
                         │     │     │
                         │     │     └── 事件 0（如：传感器采集完成）
                         │     └──────── 事件 1（如：串口收到数据）
                         └────────────── 事件 2（如：按键按下）
                         以此类推，最多 24 个独立事件
```

**定义事件位的惯用方式：**

```c
/* 使用位移宏，清晰且不易出错 */
#define EVT_SENSOR_DONE     (1UL << 0)   /* bit0 */
#define EVT_UART_RECEIVED   (1UL << 1)   /* bit1 */
#define EVT_BUTTON_PRESS    (1UL << 2)   /* bit2 */
#define EVT_TIMER_EXPIRED   (1UL << 3)   /* bit3 */
#define EVT_ALL_READY       (EVT_SENSOR_DONE | EVT_UART_RECEIVED) /* 组合 */
```

---

## 创建事件组

```c
EventGroupHandle_t xEventGroup;

xEventGroup = xEventGroupCreate();
if (xEventGroup == NULL) {
    Error_Handler(); /* 内存不足 */
}
/* 创建时所有 bit 默认为 0（无事件） */
```

**静态创建：**

```c
static StaticEventGroup_t xEventGroupBuffer;
xEventGroup = xEventGroupCreateStatic(&xEventGroupBuffer);
```

---

## 设置事件位

`xEventGroupSetBits` 将指定 bit 置 1，同时唤醒所有等待条件满足的任务。

```c
/* 设置单个 bit */
xEventGroupSetBits(xEventGroup, EVT_SENSOR_DONE);

/* 设置多个 bit（一次性，原子操作） */
xEventGroupSetBits(xEventGroup, EVT_SENSOR_DONE | EVT_UART_RECEIVED);

/* 返回值：设置后事件组的当前值（可忽略） */
EventBits_t bitsAfterSet = xEventGroupSetBits(xEventGroup, EVT_BUTTON_PRESS);
```

---

## 等待事件位

`xEventGroupWaitBits` 是事件组最核心的 API，可以等待 OR（任意一个）或 AND（全部）条件。

```c
EventBits_t xEventGroupWaitBits(
    EventGroupHandle_t xEventGroup,  /* 事件组句柄 */
    const EventBits_t uxBitsToWaitFor, /* 等待哪些 bit */
    const BaseType_t xClearOnExit,   /* 退出时是否清除这些 bit */
    const BaseType_t xWaitForAllBits,/* pdTRUE=AND（全部）, pdFALSE=OR（任意一个） */
    TickType_t xTicksToWait          /* 超时时间 */
);
/* 返回值：函数返回时事件组的值（注意：如果清除了，返回的是清除前的值） */
```

### OR 等待（任意一个事件发生就唤醒）

```c
EventBits_t bits = xEventGroupWaitBits(
    xEventGroup,
    EVT_SENSOR_DONE | EVT_UART_RECEIVED | EVT_BUTTON_PRESS, /* 等待这三个中任意一个 */
    pdTRUE,          /* 被唤醒后清除这些 bit */
    pdFALSE,         /* pdFALSE = OR，任意一个满足就返回 */
    portMAX_DELAY
);

/* 判断是哪个事件触发的 */
if (bits & EVT_SENSOR_DONE) {
    HandleSensorData();
}
if (bits & EVT_UART_RECEIVED) {
    HandleUartData();
}
if (bits & EVT_BUTTON_PRESS) {
    HandleButton();
}
```

### AND 等待（必须所有事件都发生才唤醒）

```c
/* 等待传感器采集完成 AND UART数据到达，两者都满足才继续 */
EventBits_t bits = xEventGroupWaitBits(
    xEventGroup,
    EVT_SENSOR_DONE | EVT_UART_RECEIVED,
    pdTRUE,          /* 唤醒后清除 */
    pdTRUE,          /* pdTRUE = AND，必须全部满足 */
    pdMS_TO_TICKS(5000) /* 等待最多 5 秒 */
);

/* 检查是超时返回还是条件满足返回 */
if ((bits & (EVT_SENSOR_DONE | EVT_UART_RECEIVED)) ==
            (EVT_SENSOR_DONE | EVT_UART_RECEIVED)) {
    /* 条件满足，两个事件都发生了 */
    ProcessCombinedData();
} else {
    /* 超时 */
    HandleTimeout();
}
```

### 超时处理

```c
/* 等待最多 2 秒 */
EventBits_t bits = xEventGroupWaitBits(xEventGroup, EVT_SENSOR_DONE,
                                        pdTRUE, pdFALSE,
                                        pdMS_TO_TICKS(2000));

if (bits & EVT_SENSOR_DONE) {
    /* 在超时前收到事件 */
} else {
    /* 超时，传感器没有响应 */
    HandleSensorTimeout();
}
```

---

## 清除与查询事件位

```c
/* 手动清除指定 bit（置 0） */
xEventGroupClearBits(xEventGroup, EVT_SENSOR_DONE);

/* 清除多个 bit */
xEventGroupClearBits(xEventGroup, EVT_SENSOR_DONE | EVT_UART_RECEIVED);

/* 查询当前所有 bit 的值（不清除，不阻塞） */
EventBits_t currentBits = xEventGroupGetBits(xEventGroup);

if (currentBits & EVT_SENSOR_DONE) {
    /* 传感器采集完成的 bit 当前为 1 */
}
```

---

## 在中断中使用事件组

中断中设置事件位必须使用 `xEventGroupSetBitsFromISR`。

```c
/* 中断中设置事件位 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == SENSOR_READY_Pin) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        
        xEventGroupSetBitsFromISR(xEventGroup,
                                   EVT_SENSOR_DONE,
                                   &xHigherPriorityTaskWoken);
        
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}
```

> **注意：** `xEventGroupSetBitsFromISR` 内部通过定时器任务的命令队列来实现，会有一个任务调度周期的延迟。如果需要零延迟，考虑使用任务通知（Task Notification）。

**中断中不支持的操作：**

```c
/* ❌ 不能在中断中等待事件位（会阻塞） */
xEventGroupWaitBits(...);

/* ❌ 不能在中断中清除事件位（无 FromISR 版本） */
xEventGroupClearBits(...);

/* ✅ 可以在中断中读取 */
EventBits_t bits = xEventGroupGetBitsFromISR(xEventGroup);
```

---

## 事件组 vs 其他同步机制

| 机制 | 适合场景 | 特点 |
|------|---------|------|
| **事件组** | 等待多个事件中的某些组合 | AND/OR 等待；一次可广播给多个任务 |
| 信号量 | 单个事件，精确计次 | 计数不会丢失（Give 多次，Take 多次） |
| 队列 | 传递数据（带内容的通知） | FIFO，不丢失，携带数据 |
| 任务通知 | 点对点通知，轻量 | 速度最快，只能一对一 |

**事件组的独特优势：**

1. **广播（Broadcast）**：一次 SetBits 可以同时唤醒等待同一 bit 的多个任务
2. **AND 条件**：等待多个条件同时满足，其他机制无法直接实现
3. **无需创建多个对象**：多个事件用一个事件组，节省 RAM

**事件组的局限：**

1. 不携带数据（只有 0/1 状态）
2. bit 会被覆盖，不像信号量/队列能记录事件次数
3. 中断中设置有一定延迟（经过定时器命令队列）

---

## 典型使用场景

### 场景 1：多传感器数据就绪同步

等待所有传感器数据采集完成，再一起处理（AND 等待）。

```c
#define EVT_TEMP_READY  (1UL << 0)  /* 温度传感器就绪 */
#define EVT_HUMI_READY  (1UL << 1)  /* 湿度传感器就绪 */
#define EVT_PRES_READY  (1UL << 2)  /* 气压传感器就绪 */
#define EVT_ALL_SENSORS (EVT_TEMP_READY | EVT_HUMI_READY | EVT_PRES_READY)

EventGroupHandle_t xSensorEvents;

/* 各传感器采集任务（独立运行） */
void vTempTask(void *p) {
    for (;;) {
        g_temperature = ReadTempSensor();
        xEventGroupSetBits(xSensorEvents, EVT_TEMP_READY);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void vHumiTask(void *p) {
    for (;;) {
        g_humidity = ReadHumiSensor(); /* 比温度传感器慢，耗时不一 */
        xEventGroupSetBits(xSensorEvents, EVT_HUMI_READY);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void vPresTask(void *p) {
    for (;;) {
        g_pressure = ReadPresSensor();
        xEventGroupSetBits(xSensorEvents, EVT_PRES_READY);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* 汇总处理任务：等待三个传感器全部就绪 */
void vProcessTask(void *p) {
    for (;;) {
        /* 等待所有传感器数据就绪（AND），超时 3 秒 */
        EventBits_t bits = xEventGroupWaitBits(xSensorEvents,
                                                EVT_ALL_SENSORS,
                                                pdTRUE,    /* 消费后清除 */
                                                pdTRUE,    /* AND：全部满足 */
                                                pdMS_TO_TICKS(3000));
        
        if ((bits & EVT_ALL_SENSORS) == EVT_ALL_SENSORS) {
            /* 所有传感器数据就绪，执行融合处理 */
            ProcessAndUploadData(g_temperature, g_humidity, g_pressure);
        } else {
            /* 有传感器超时未响应 */
            if (!(bits & EVT_TEMP_READY)) LogError("Temp sensor timeout");
            if (!(bits & EVT_HUMI_READY)) LogError("Humi sensor timeout");
            if (!(bits & EVT_PRES_READY)) LogError("Pres sensor timeout");
        }
    }
}
```

### 场景 2：系统初始化同步

多个模块并行初始化，主任务等待所有模块初始化完成后再开始工作。

```c
#define EVT_WIFI_INIT_DONE   (1UL << 0)
#define EVT_SENSOR_INIT_DONE (1UL << 1)
#define EVT_DISPLAY_INIT_DONE (1UL << 2)
#define EVT_ALL_INIT_DONE    (EVT_WIFI_INIT_DONE | EVT_SENSOR_INIT_DONE | EVT_DISPLAY_INIT_DONE)

EventGroupHandle_t xInitEvents;

void vWifiInitTask(void *p) {
    WiFi_Init(); /* 可能耗时 2-3 秒 */
    xEventGroupSetBits(xInitEvents, EVT_WIFI_INIT_DONE);
    vTaskDelete(NULL); /* 初始化完成，任务自删 */
}

void vSensorInitTask(void *p) {
    Sensor_Init();
    xEventGroupSetBits(xInitEvents, EVT_SENSOR_INIT_DONE);
    vTaskDelete(NULL);
}

void vMainTask(void *p) {
    /* 等待所有模块初始化完成 */
    xEventGroupWaitBits(xInitEvents, EVT_ALL_INIT_DONE,
                         pdFALSE, pdTRUE, portMAX_DELAY);
    
    /* 所有模块就绪，开始主循环 */
    printf("System ready!\r\n");
    StartMainLoop();
}
```

### 场景 3：事件广播（一对多）

一个事件通知多个任务同时执行不同操作。

```c
#define EVT_DATA_UPDATED (1UL << 0)

/* 数据更新后设置事件位 */
void DataUpdateCallback(void) {
    xEventGroupSetBits(xDataEvents, EVT_DATA_UPDATED);
    /* 一次 Set，下面等待这个 bit 的所有任务都会被唤醒 */
}

/* 显示任务：数据更新时刷新显示 */
void vDisplayTask(void *p) {
    for (;;) {
        xEventGroupWaitBits(xDataEvents, EVT_DATA_UPDATED,
                             pdFALSE,  /* 不清除（让其他任务也能看到） */
                             pdFALSE, portMAX_DELAY);
        RefreshDisplay();
        /* 手动清除自己处理过的逻辑，或者在固定时间后清除 */
    }
}

/* 日志任务：数据更新时记录日志 */
void vLogTask(void *p) {
    for (;;) {
        xEventGroupWaitBits(xDataEvents, EVT_DATA_UPDATED,
                             pdFALSE, pdFALSE, portMAX_DELAY);
        LogDataToFlash();
    }
}
```

> 广播模式中，需要小心 bit 清除时机。如果一个任务清除了 bit，其他任务可能看不到这个事件。可以考虑各任务各自维护自己的"已处理"标记。

### 场景 4：状态机事件驱动

```c
#define EVT_BTN_SHORT  (1UL << 0) /* 短按 */
#define EVT_BTN_LONG   (1UL << 1) /* 长按 */
#define EVT_TIMEOUT    (1UL << 2) /* 超时 */
#define EVT_CONN_OK    (1UL << 3) /* 连接成功 */
#define EVT_CONN_FAIL  (1UL << 4) /* 连接失败 */

void vStateMachineTask(void *p) {
    SystemState_t state = STATE_IDLE;
    EventBits_t events;

    for (;;) {
        /* 根据当前状态决定等待哪些事件 */
        switch (state) {
            case STATE_IDLE:
                events = xEventGroupWaitBits(xSystemEvents,
                    EVT_BTN_SHORT | EVT_BTN_LONG, pdTRUE, pdFALSE, portMAX_DELAY);
                if (events & EVT_BTN_SHORT) state = STATE_ACTIVE;
                if (events & EVT_BTN_LONG)  state = STATE_CONFIG;
                break;

            case STATE_ACTIVE:
                events = xEventGroupWaitBits(xSystemEvents,
                    EVT_TIMEOUT | EVT_BTN_SHORT, pdTRUE, pdFALSE,
                    pdMS_TO_TICKS(10000));
                if (events & EVT_TIMEOUT)    state = STATE_IDLE;
                if (events & EVT_BTN_SHORT)  state = STATE_IDLE;
                break;

            case STATE_CONFIG:
                events = xEventGroupWaitBits(xSystemEvents,
                    EVT_CONN_OK | EVT_CONN_FAIL, pdTRUE, pdFALSE,
                    pdMS_TO_TICKS(30000));
                if (events & EVT_CONN_OK)    state = STATE_CONNECTED;
                else                         state = STATE_IDLE;
                break;
        }
    }
}
```

---

## 注意事项

### bit 位被多任务共享时的清除问题

```c
/* 多个任务等待同一个 bit，都设置 xClearOnExit = pdTRUE */
/* 谁先被唤醒，谁就清除了 bit，其他任务可能看不到这个事件 */

/* ✅ 广播场景：设置 xClearOnExit = pdFALSE，由发送方或专门的任务清除 */
xEventGroupWaitBits(xEG, EVT_DATA, pdFALSE, pdFALSE, portMAX_DELAY);
/* 处理完后手动清除 */
xEventGroupClearBits(xEG, EVT_DATA);
```

### 事件位计数不累积

```c
/* 信号量可以记录 5 次 Give，事件组不行 */
/* 5 次 SetBits 同一个 bit，bit 值仍然是 1，只能被 Take 一次 */

/* 如果需要记录发生次数，用计数信号量，不要用事件组 */
```

### `xEventGroupSetBitsFromISR` 有调度延迟

```c
/* SetBitsFromISR 内部通过定时器命令队列，不是立即唤醒任务 */
/* 延迟约一个任务调度周期（1ms） */
/* 如果需要零延迟中断通知，用任务通知（vTaskNotifyGiveFromISR） */
```

---

## API 速查

| API | 说明 | 可用场景 |
|-----|------|---------|
| `xEventGroupCreate()` | 创建事件组 | 任何时候 |
| `xEventGroupCreateStatic(&buf)` | 静态创建 | 任何时候 |
| `vEventGroupDelete(eg)` | 删除事件组 | 任务 |
| `xEventGroupSetBits(eg, bits)` | 设置事件位 | 任务 |
| `xEventGroupClearBits(eg, bits)` | 清除事件位 | 任务 |
| `xEventGroupGetBits(eg)` | 读取当前值（宏，不阻塞） | 任务 |
| `xEventGroupWaitBits(eg, bits, clear, allBits, timeout)` | 等待事件位 | 任务 |
| `xEventGroupSetBitsFromISR(eg, bits, &woken)` | 中断中设置 | 中断 |
| `xEventGroupGetBitsFromISR(eg)` | 中断中读取 | 中断 |
| `xEventGroupSync(eg, setBits, waitBits, timeout)` | 任务间同步屏障 | 任务 |
