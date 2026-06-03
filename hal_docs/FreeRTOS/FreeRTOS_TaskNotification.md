# FreeRTOS 任务通知（Task Notification）

## 目录

- [核心概念](#核心概念)
- [通知值与通知状态](#通知值与通知状态)
- [发送通知（xTaskNotify）](#发送通知xtasknotify)
- [等待通知（xTaskNotifyWait）](#等待通知xtasknotifywait)
- [轻量版 API（NotifyGive / NotifyTake）](#轻量版-apinotifygive--notifytake)
- [在中断中使用任务通知](#在中断中使用任务通知)
- [五种发送模式（eAction）](#五种发送模式eaction)
- [任务通知 vs 其他同步机制](#任务通知-vs-其他同步机制)
- [典型使用场景](#典型使用场景)
- [多通知索引（FreeRTOS 10.4+）](#多通知索引freertos-104)
- [常见错误](#常见错误)
- [API 速查](#api-速查)

---

## 核心概念

任务通知是 FreeRTOS 中**最轻量的任务间通知机制**。每个任务内置一个 32 位的通知值和一个通知状态，无需创建任何额外对象（信号量、队列等）。

**核心优势：**
- 零额外 RAM（通知值内置在任务控制块中）
- 速度比信号量快约 45%
- API 简洁

**核心限制：**
- **只能点对点**：一个发送者，一个固定的接收任务
- 无法被多个任务等待（不像信号量/事件组）

---

## 通知值与通知状态

每个任务有两个内置字段：

```c
/* 任务控制块（TCB）内部字段（简化表示） */
typedef struct {
    /* ... 其他字段 ... */
    volatile uint32_t ulNotifiedValue; /* 32 位通知值 */
    volatile uint8_t  ucNotifyState;   /* 通知状态：等待中 / 有通知待处理 / 无通知 */
} TCB_t;
```

**通知状态：**

```
taskNOT_WAITING_NOTIFICATION  → 任务没有在等待通知
taskWAITING_NOTIFICATION      → 任务正在 xTaskNotifyWait 中等待
taskNOTIFICATION_RECEIVED     → 通知已到达（任务还没读取）
```

---

## 发送通知（xTaskNotify）

```c
/* 原型 */
BaseType_t xTaskNotify(
    TaskHandle_t xTaskToNotify,  /* 目标任务句柄 */
    uint32_t ulValue,            /* 通知值（含义由 eAction 决定） */
    eNotifyAction eAction        /* 发送模式 */
);
/* 返回值：通常为 pdPASS，仅 eSetValueWithoutOverwrite 在有未读通知时返回 pdFAIL */
```

**五种发送模式（eAction）的简单示例：**

```c
/* 1. 按位置 1（常用于事件标志） */
xTaskNotify(xTaskHandle, 0x01, eSetBits);
xTaskNotify(xTaskHandle, 0x04, eSetBits); /* 不覆盖，OR 叠加 */

/* 2. 增量（计数，等同于计数信号量） */
xTaskNotify(xTaskHandle, 0, eIncrement);

/* 3. 覆写（直接设置新值，不管之前有没有未读通知） */
xTaskNotify(xTaskHandle, 100, eSetValueWithOverwrite);

/* 4. 不覆写（若有未读通知则失败，保护之前的数据） */
if (xTaskNotify(xTaskHandle, 200, eSetValueWithoutOverwrite) != pdPASS) {
    /* 目标任务有未读通知，未能发送 */
}

/* 5. 仅发出通知，不修改值 */
xTaskNotify(xTaskHandle, 0, eNoAction);
```

---

## 等待通知（xTaskNotifyWait）

```c
/* 原型 */
BaseType_t xTaskNotifyWait(
    uint32_t ulBitsToClearOnEntry,  /* 进入等待时先清除哪些 bit */
    uint32_t ulBitsToClearOnExit,   /* 退出等待（收到通知）时清除哪些 bit */
    uint32_t *pulNotificationValue, /* 输出：收到的通知值 */
    TickType_t xTicksToWait         /* 超时 */
);
```

```c
void vReceiverTask(void *p) {
    uint32_t ulValue;
    for (;;) {
        /* 进入等待：清除所有 bit（重置之前的残留通知）
         * 退出等待：清除所有 bit（消费通知）
         * 获取通知值 */
        BaseType_t got = xTaskNotifyWait(
            0xFFFFFFFF,   /* 进入时清除所有 bit */
            0xFFFFFFFF,   /* 退出时清除所有 bit */
            &ulValue,
            portMAX_DELAY
        );

        if (got == pdTRUE) {
            /* 处理通知值 */
            if (ulValue & 0x01) HandleEvent1();
            if (ulValue & 0x02) HandleEvent2();
        }
    }
}
```

**`ulBitsToClearOnEntry` 和 `ulBitsToClearOnExit` 的作用：**

```
ulBitsToClearOnEntry = 0x00000000  → 进入时不清除任何 bit，保留历史通知
ulBitsToClearOnEntry = 0xFFFFFFFF  → 进入时清除所有 bit，从干净状态开始等待

ulBitsToClearOnExit  = 0x00000000  → 退出时不清除，通知值保留供下次 Wait 读取
ulBitsToClearOnExit  = 0xFFFFFFFF  → 退出时清除所有 bit，消费掉通知
ulBitsToClearOnExit  = 0x00000001  → 只清除 bit0，其他 bit 保留
```

---

## 轻量版 API（NotifyGive / NotifyTake）

这是任务通知的简化版本，行为等价于二值/计数信号量，代码更简洁。

### `ulTaskNotifyTake`（接收端，等价于信号量 Take）

```c
/* 等待通知（等价于 xSemaphoreTake） */
ulTaskNotifyTake(
    pdTRUE,          /* pdTRUE：收到通知后将通知值清零（等价于二值信号量）
                        pdFALSE：收到通知后将通知值减一（等价于计数信号量）*/
    portMAX_DELAY    /* 超时 */
);
```

```c
void vWorkerTask(void *p) {
    for (;;) {
        /* 等待工作通知（阻塞） */
        uint32_t count = ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
        /* count 是到来的通知次数（积累未处理的次数） */
        
        while (count > 0) {
            ProcessOneItem();
            count--;
        }
    }
}
```

### `vTaskNotifyGive`（发送端，等价于信号量 Give）

```c
/* 发送通知（等价于 xSemaphoreGive），通知值 +1 */
vTaskNotifyGive(xWorkerTaskHandle);
```

**与计数信号量的对比：**

```c
/* 使用计数信号量 */
SemaphoreHandle_t xSem = xSemaphoreCreateCounting(10, 0);
xSemaphoreGive(xSem);           /* 生产端 */
xSemaphoreTake(xSem, portMAX_DELAY); /* 消费端 */

/* 使用任务通知（等价，但更轻量，省 ~80 字节 RAM） */
vTaskNotifyGive(xWorkerHandle);                      /* 生产端 */
ulTaskNotifyTake(pdFALSE, portMAX_DELAY);            /* 消费端 */
```

---

## 在中断中使用任务通知

```c
/* 中断中发送通知（FromISR 版本） */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    SaveRxData(rxBuf);
    
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    /* 方式1：按位设置（常用） */
    xTaskNotifyFromISR(xUartTask, 0x01, eSetBits, &xHigherPriorityTaskWoken);
    
    /* 方式2：轻量版（等价于信号量 Give） */
    vTaskNotifyGiveFromISR(xUartTask, &xHigherPriorityTaskWoken);
    
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/* 接收任务 */
void vUartTask(void *p) {
    for (;;) {
        /* 等待 UART 通知 */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
        /* 处理接收到的数据 */
        ProcessUartData();
        
        /* 重启下一次接收 */
        HAL_UART_Receive_IT(&huart1, rxBuf, sizeof(rxBuf));
    }
}
```

---

## 五种发送模式（eAction）

| eAction | 行为 | 适合替代 |
|---------|------|---------|
| `eNoAction` | 仅唤醒目标任务，不修改通知值 | 二值信号量（仅唤醒） |
| `eSetBits` | 通知值 OR 上发送值（按位置 1） | 事件组（轻量版） |
| `eIncrement` | 通知值 +1（忽略 ulValue） | 计数信号量 |
| `eSetValueWithOverwrite` | 直接覆写通知值 | 邮箱（最新值） |
| `eSetValueWithoutOverwrite` | 若有未读通知则失败，否则写入 | 保护旧数据的场景 |

```c
/* eSetBits 示例：多个事件用不同 bit 区分 */
#define NOTIF_ADC_DONE  (1UL << 0)
#define NOTIF_UART_RX   (1UL << 1)
#define NOTIF_BTN_PRESS (1UL << 2)

/* 不同来源设置不同 bit */
xTaskNotify(xProcessTask, NOTIF_ADC_DONE, eSetBits);
xTaskNotify(xProcessTask, NOTIF_UART_RX,  eSetBits);

/* 接收任务 */
uint32_t bits;
xTaskNotifyWait(0, 0xFFFFFFFF, &bits, portMAX_DELAY);
if (bits & NOTIF_ADC_DONE)  HandleADC();
if (bits & NOTIF_UART_RX)   HandleUART();
if (bits & NOTIF_BTN_PRESS) HandleButton();
```

---

## 任务通知 vs 其他同步机制

| 特性 | 任务通知 | 二值信号量 | 计数信号量 | 事件组 | 队列 |
|------|---------|-----------|-----------|--------|------|
| RAM 开销 | **0**（内置） | ~80 字节 | ~80 字节 | ~72 字节 | 可变 |
| 速度 | **最快** | 较快 | 较快 | 中 | 较慢 |
| 接收者 | **只能一个** | 多个可等待 | 多个可等待 | 多个可等待 | 多个可等待 |
| 发送者 | 任意（含中断） | 任意 | 任意 | 任意 | 任意 |
| 携带数据 | ✅ 32 位值 | ❌ 无 | ❌ 无 | ❌ 无 | ✅ 任意 |
| AND 等待 | ❌ | ❌ | ❌ | ✅ | ❌ |
| 计数不丢失 | ✅（Increment 模式） | ❌ | ✅ | ❌ | ✅ |

**选择建议：**

```
单一固定接收者 + 追求最小 RAM/最快速度 → 任务通知
多个任务等待同一事件 → 信号量 / 事件组
需要 AND 等待多个事件 → 事件组
传递数据（不只是通知） → 队列
```

---

## 典型使用场景

### 场景 1：替代二值信号量（DMA 完成通知）

```c
TaskHandle_t xSpiTaskHandle;

/* DMA 传输完成中断 */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
    BaseType_t woken = pdFALSE;
    vTaskNotifyGiveFromISR(xSpiTaskHandle, &woken); /* 通知值 +1 */
    portYIELD_FROM_ISR(woken);
}

/* SPI 任务 */
void vSpiTask(void *p) {
    xSpiTaskHandle = xTaskGetCurrentTaskHandle(); /* 保存自己的句柄 */
    
    for (;;) {
        /* 准备数据 */
        PrepareTxData(txBuf, sizeof(txBuf));
        
        /* 启动 DMA */
        HAL_SPI_Transmit_DMA(&hspi1, txBuf, sizeof(txBuf));
        
        /* 等待 DMA 完成（阻塞，不占 CPU） */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
        /* DMA 完成，处理下一帧 */
    }
}
```

### 场景 2：替代计数信号量（生产者-消费者）

```c
TaskHandle_t xConsumerHandle;

/* 生产者：每产生一个数据，通知消费者一次 */
void vProducerTask(void *p) {
    for (;;) {
        uint16_t sample = ADC_Read();
        StoreInBuffer(sample);
        
        /* 通知消费者有一个新数据（通知值 +1） */
        vTaskNotifyGive(xConsumerHandle);
        
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

/* 消费者：每次处理一个数据 */
void vConsumerTask(void *p) {
    xConsumerHandle = xTaskGetCurrentTaskHandle();
    
    for (;;) {
        /* 等待通知，pdFALSE = 计数减一（而非清零），不丢失多次通知 */
        uint32_t pending = ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
        
        /* pending 是积累的待处理数量 */
        for (uint32_t i = 0; i < pending; i++) {
            ProcessOneData();
        }
    }
}
```

### 场景 3：向任务传递一个值

```c
TaskHandle_t xWorkerHandle;

/* 发送方：将电机目标转速传给控制任务 */
void SetMotorTarget(uint16_t rpm) {
    /* 覆写模式：只关心最新值 */
    xTaskNotify(xWorkerHandle, rpm, eSetValueWithOverwrite);
}

/* 控制任务 */
void vMotorTask(void *p) {
    xWorkerHandle = xTaskGetCurrentTaskHandle();
    uint32_t targetRpm;

    for (;;) {
        /* 等待新的转速指令 */
        xTaskNotifyWait(0, 0xFFFFFFFF, &targetRpm, portMAX_DELAY);
        Motor_SetSpeed((uint16_t)targetRpm);
    }
}
```

### 场景 4：多中断共用一个任务（用 bit 区分来源）

```c
TaskHandle_t xEventTask;
#define NOTIF_UART1  (1UL << 0)
#define NOTIF_UART2  (1UL << 1)
#define NOTIF_ADC    (1UL << 2)

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    BaseType_t woken = pdFALSE;
    if (huart == &huart1) {
        xTaskNotifyFromISR(xEventTask, NOTIF_UART1, eSetBits, &woken);
    } else if (huart == &huart2) {
        xTaskNotifyFromISR(xEventTask, NOTIF_UART2, eSetBits, &woken);
    }
    portYIELD_FROM_ISR(woken);
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
    BaseType_t woken = pdFALSE;
    xTaskNotifyFromISR(xEventTask, NOTIF_ADC, eSetBits, &woken);
    portYIELD_FROM_ISR(woken);
}

void vEventTask(void *p) {
    xEventTask = xTaskGetCurrentTaskHandle();
    uint32_t bits;

    for (;;) {
        xTaskNotifyWait(0, 0xFFFFFFFF, &bits, portMAX_DELAY);
        
        if (bits & NOTIF_UART1) ProcessUart1();
        if (bits & NOTIF_UART2) ProcessUart2();
        if (bits & NOTIF_ADC)   ProcessAdc();
    }
}
```

---

## 多通知索引（FreeRTOS 10.4+）

FreeRTOS 10.4 起，每个任务支持多个（默认 1 个，可配置）通知索引，互不干扰。

```c
/* 在 FreeRTOSConfig.h 中配置：
   #define configTASK_NOTIFICATION_ARRAY_ENTRIES 3  */

/* 使用索引 0 的通知（用于 UART 数据就绪） */
xTaskNotifyIndexed(xTask, 0, NOTIF_UART_RX, eSetBits);

/* 使用索引 1 的通知（用于优先级别的紧急事件） */
xTaskNotifyIndexed(xTask, 1, 0x01, eSetBits);

/* 接收索引 0 的通知 */
xTaskNotifyWaitIndexed(0, 0, 0xFFFFFFFF, &value, portMAX_DELAY);

/* 接收索引 1 的通知 */
xTaskNotifyWaitIndexed(1, 0, 0xFFFFFFFF, &value, portMAX_DELAY);
```

> STM32CubeMX 自带的 FreeRTOS 版本通常支持此特性，需确认版本号。

---

## 常见错误

### 忘记保存任务句柄

```c
/* ❌ 没有保存句柄，无法从其他地方通知这个任务 */
xTaskCreate(vMyTask, "Task", 128, NULL, 2, NULL);

/* ✅ 保存句柄供通知使用 */
TaskHandle_t xMyTaskHandle;
xTaskCreate(vMyTask, "Task", 128, NULL, 2, &xMyTaskHandle);
/* 其他任务或中断可以用 xMyTaskHandle 通知它 */
```

### 多个任务等待同一通知

```c
/* ❌ 错误：两个任务都在等待 xSharedTask 的通知 */
/* 任务通知是点对点的，只有持有句柄的一方能被通知 */
/* 多任务等待场景应使用信号量或事件组 */

/* 如果需要多个任务被通知，改用信号量/事件组 */
xEventGroupSetBits(xEG, MY_EVENT_BIT); /* 可以唤醒多个等待者 */
```

### eSetValueWithoutOverwrite 失败未处理

```c
/* eSetValueWithoutOverwrite 在目标任务有未读通知时会失败 */
BaseType_t result = xTaskNotify(xTask, value, eSetValueWithoutOverwrite);
if (result == pdFAIL) {
    /* 通知未发送，决定是丢弃还是等待重试 */
    /* 若频繁失败，说明接收任务处理速度跟不上 */
}
```

---

## API 速查

| API | 说明 | 可用场景 |
|-----|------|---------|
| `xTaskNotify(handle, value, action)` | 发送通知（带值和模式） | 任务 |
| `xTaskNotifyGive(handle)` | 轻量通知（通知值 +1） | 任务 |
| `xTaskNotifyFromISR(handle, value, action, &woken)` | 中断中发送 | 中断 |
| `vTaskNotifyGiveFromISR(handle, &woken)` | 中断中轻量通知 | 中断 |
| `xTaskNotifyWait(clearEntry, clearExit, &value, timeout)` | 等待通知（获取值） | 任务 |
| `ulTaskNotifyTake(clearOnExit, timeout)` | 轻量等待（pdTRUE清零/pdFALSE减一） | 任务 |
| `xTaskNotifyStateClear(handle)` | 清除待处理通知状态 | 任务 |
| `ulTaskNotifyValueClear(handle, bitsToClear)` | 清除通知值指定 bit | 任务 |
| `xTaskNotifyIndexed(handle, idx, value, action)` | 指定索引发送（10.4+） | 任务 |
| `xTaskNotifyWaitIndexed(idx, clearEntry, clearExit, &val, timeout)` | 指定索引等待（10.4+） | 任务 |
