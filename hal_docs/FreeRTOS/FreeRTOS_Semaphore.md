# FreeRTOS 信号量（Semaphore）

## 目录

- [核心概念](#核心概念)
- [二值信号量（Binary Semaphore）](#二值信号量binary-semaphore)
- [计数信号量（Counting Semaphore）](#计数信号量counting-semaphore)
- [信号量 vs 互斥量](#信号量-vs-互斥量)
- [在中断中使用信号量](#在中断中使用信号量)
- [典型使用场景](#典型使用场景)
- [常见错误](#常见错误)
- [API 速查](#api-速查)

---

## 核心概念

信号量是一个**计数器**，用于实现任务间的**同步**——表达"某件事发生了"或"某个资源可用"。

- **Take（获取/等待）**：计数 -1；若计数为 0，则阻塞等待
- **Give（释放/通知）**：计数 +1；若有任务在等待，唤醒它

信号量有两种类型：

| 类型 | 计数范围 | 主要用途 |
|------|---------|---------|
| 二值信号量 | 0 或 1 | 任务/中断同步，通知"事件发生了" |
| 计数信号量 | 0 ~ N | 管理 N 个资源，记录事件发生次数 |

**信号量不携带数据，只有"计数值"。** 如果需要同时传递数据，应该用队列。

---

## 二值信号量（Binary Semaphore）

计数只有 0 和 1 的信号量。适合**一次性事件通知**：一端"给出"，另一端"等待"。

### 创建

```c
SemaphoreHandle_t xBinSem;

xBinSem = xSemaphoreCreateBinary();
if (xBinSem == NULL) {
    Error_Handler(); /* 内存不足 */
}
/* 注意：二值信号量创建时初始值为 0（未触发状态） */
```

### 基本用法：中断唤醒任务

这是二值信号量最典型的场景：**中断发生 → 唤醒任务处理**。

```
中断 ──xSemaphoreGiveFromISR──→ [信号量 0→1] ──xSemaphoreTake──→ 处理任务唤醒
```

```c
SemaphoreHandle_t xBtnSem;

/* 初始化 */
xBtnSem = xSemaphoreCreateBinary();

/* 中断回调：按键触发 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == BTN_Pin) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        
        /* 给出信号量，唤醒等待的任务 */
        xSemaphoreGiveFromISR(xBtnSem, &xHigherPriorityTaskWoken);
        
        /* 若唤醒的任务优先级更高，退出中断时立即切换 */
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

/* 按键处理任务 */
void vButtonTask(void *p) {
    for (;;) {
        /* 永久阻塞等待信号量，此时任务不占 CPU */
        xSemaphoreTake(xBtnSem, portMAX_DELAY);
        
        /* 被唤醒后处理按键事件 */
        vTaskDelay(pdMS_TO_TICKS(20)); /* 消抖 20ms */
        if (HAL_GPIO_ReadPin(BTN_GPIO_Port, BTN_Pin) == GPIO_PIN_RESET) {
            ToggleLED();
        }
    }
}
```

### 超时等待

```c
/* 等待最多 1000ms，超时后继续执行其他逻辑 */
if (xSemaphoreTake(xBtnSem, pdMS_TO_TICKS(1000)) == pdTRUE) {
    /* 在超时前收到信号量 */
    HandleEvent();
} else {
    /* 超时，没有事件发生 */
    HandleTimeout();
}
```

### 二值信号量的局限性

```
问题：连续两次中断，但任务还没有来得及处理第一次

时序：
  中断1 → Give（0→1）
  中断2 → Give（已经是1，再 Give 失败，被丢弃）
  任务 → Take（1→0，只处理了一次）
  ⚠️ 第二次中断的通知丢失了！
```

如果事件可能快速连发（如 UART 收到多个字节），应改用**计数信号量**或**队列**。

---

## 计数信号量（Counting Semaphore）

计数范围是 0 ~ N 的信号量，能记录多次"Give"操作。

**两种用途：**

1. **记录事件次数**：每次事件发生 Give 一次，任务每次 Take 处理一次，不丢失
2. **管理资源池**：N 个资源，Take 一个用一个，Give 一个还一个

### 创建

```c
/* xSemaphoreCreateCounting(最大计数值, 初始计数值) */

/* 用途1：记录事件次数，初始 0（没有事件待处理） */
SemaphoreHandle_t xEventSem = xSemaphoreCreateCounting(10, 0);

/* 用途2：资源池，5 个资源全部可用，初始值 = 最大值 */
SemaphoreHandle_t xResourceSem = xSemaphoreCreateCounting(5, 5);
```

### 用途 1：不丢失事件的连续通知

```c
SemaphoreHandle_t xPktSem = xSemaphoreCreateCounting(20, 0);

/* UART 每收到一个数据包，Give 一次 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    SavePacket(rxBuffer);  /* 保存数据包 */
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(xPktSem, &woken);  /* 计数 +1 */
    portYIELD_FROM_ISR(woken);
}

/* 处理任务：每次 Take 处理一个包 */
void vPacketTask(void *p) {
    for (;;) {
        xSemaphoreTake(xPktSem, portMAX_DELAY);  /* 计数 -1 */
        ProcessPacket();  /* 处理一个数据包 */
    }
}
/* 如果中断连续来了 3 个包，计数变为 3，任务会连续处理 3 次，不丢失 */
```

### 用途 2：管理资源池

```c
#define BUFFER_COUNT 4
static uint8_t buffers[BUFFER_COUNT][256];
static uint8_t buf_used[BUFFER_COUNT] = {0};

SemaphoreHandle_t xBufSem = xSemaphoreCreateCounting(BUFFER_COUNT, BUFFER_COUNT);

/* 申请一个缓冲区 */
uint8_t *AllocBuffer(void) {
    /* 等待有可用缓冲区（计数 > 0 时立即返回，否则阻塞） */
    xSemaphoreTake(xBufSem, portMAX_DELAY);  /* 计数 -1 */
    
    /* 找一个空闲缓冲区 */
    for (int i = 0; i < BUFFER_COUNT; i++) {
        if (!buf_used[i]) {
            buf_used[i] = 1;
            return buffers[i];
        }
    }
    return NULL; /* 不应该到这里 */
}

/* 释放缓冲区 */
void FreeBuffer(uint8_t *buf) {
    for (int i = 0; i < BUFFER_COUNT; i++) {
        if (buffers[i] == buf) {
            buf_used[i] = 0;
            xSemaphoreGive(xBufSem);  /* 计数 +1，通知等待者可用了 */
            return;
        }
    }
}
```

### 查询计数值

```c
/* 查询信号量当前计数值（有多少次 Give 在等待被 Take） */
UBaseType_t count = uxSemaphoreGetCount(xEventSem);
```

---

## 信号量 vs 互斥量

这是一个常见混淆点，本质区别如下：

| 特性 | 二值信号量 | 互斥量 |
|------|-----------|--------|
| 用途 | **同步**（通知事件） | **互斥**（保护资源） |
| Give 限制 | 任何任务/中断都可以 Give | 只有持有者能 Give |
| 优先级继承 | ❌ 没有 | ✅ 有（防止优先级反转） |
| 中断中使用 | ✅ 可以（FromISR） | ❌ 不可以 |
| 初始状态 | 0（未触发） | 1（可用） |
| 典型用法 | 中断通知任务 | 保护共享资源（UART/SPI/全局变量） |

**优先级反转问题（信号量 vs 互斥量）：**

```
场景：任务L（低优先级）持有锁，任务H（高优先级）等待锁
      任务M（中优先级）抢占了任务L，导致任务L长期不能释放锁
      任务H等着任务L，任务L被任务M挡住 → 高优先级被中优先级间接阻塞

互斥量的解决：任务H等待互斥量时，临时将任务L的优先级提升到与H相同
             这样任务L能抢占M，尽快释放互斥量
```

**选择原则：**
- 通知某件事发生了 → **二值信号量**
- 保护共享资源（UART、SPI、全局变量） → **互斥量**
- 计数有限资源 → **计数信号量**

---

## 在中断中使用信号量

```c
/* ✅ 中断中只能用 FromISR 版本 */
void SomeIRQHandler(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    xSemaphoreGiveFromISR(xSem, &xHigherPriorityTaskWoken);
    
    /* 这一行不能省略，确保高优先级任务能及时响应 */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
```

**多个中断触发同一个信号量：**

```c
/* 多个传感器中断都 Give 同一个信号量 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    BaseType_t woken = pdFALSE;
    if (GPIO_Pin == SENSOR1_Pin || GPIO_Pin == SENSOR2_Pin) {
        xSemaphoreGiveFromISR(xSensorSem, &woken);
    }
    portYIELD_FROM_ISR(woken);
}
```

---

## 典型使用场景

### 场景 1：DMA 传输完成通知任务

```c
SemaphoreHandle_t xDmaSem;

/* DMA 完成回调（中断上下文） */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(xDmaSem, &woken);
    portYIELD_FROM_ISR(woken);
}

/* 任务：启动 DMA 后等待完成 */
void vSpiTask(void *p) {
    for (;;) {
        /* 启动 DMA 发送 */
        HAL_SPI_Transmit_DMA(&hspi1, txBuf, sizeof(txBuf));
        
        /* 等待 DMA 完成（阻塞，不占 CPU） */
        xSemaphoreTake(xDmaSem, portMAX_DELAY);
        
        /* DMA 传输完成，处理下一帧数据 */
        PrepareNextFrame();
    }
}
```

### 场景 2：任务同步（A 完成后 B 才开始）

```c
SemaphoreHandle_t xSyncSem;

/* 任务 A：完成初始化后通知任务 B */
void vTaskA(void *p) {
    InitHardware();
    CalibrateSensor();
    
    /* 通知任务 B 可以开始了 */
    xSemaphoreGive(xSyncSem);
    
    /* A 继续自己的工作 */
    for (;;) {
        DoMainWork();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/* 任务 B：等待任务 A 初始化完成 */
void vTaskB(void *p) {
    /* 等待 A 的通知 */
    xSemaphoreTake(xSyncSem, portMAX_DELAY);
    
    /* A 已经完成初始化，B 开始工作 */
    for (;;) {
        ReadSensorData();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
```

### 场景 3：限制并发访问数量

```c
/* 允许最多 3 个任务同时访问网络（模拟） */
SemaphoreHandle_t xNetSem = xSemaphoreCreateCounting(3, 3);

void vNetworkTask(void *p) {
    for (;;) {
        /* 申请网络访问权限（最多 3 个任务同时进入） */
        xSemaphoreTake(xNetSem, portMAX_DELAY);
        
        /* 使用网络资源 */
        DoNetworkRequest();
        
        /* 释放，允许其他任务进入 */
        xSemaphoreGive(xNetSem);
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
```

---

## 常见错误

### 把互斥量当二值信号量用（缺少优先级继承）

```c
/* ❌ 用二值信号量保护共享资源 */
SemaphoreHandle_t xLock = xSemaphoreCreateBinary();
xSemaphoreGive(xLock); /* 手动初始化为1 */

/* 这样用可以工作，但没有优先级继承，可能导致优先级反转 */

/* ✅ 保护共享资源应该用互斥量 */
SemaphoreHandle_t xLock = xSemaphoreCreateMutex();
```

### 在中断中用普通版本 API

```c
/* ❌ 中断中不能使用会阻塞的 API */
void IRQHandler(void) {
    xSemaphoreGive(xSem);  /* 错误：中断中不能调用带任务切换的 API */
}

/* ✅ 使用 FromISR 版本 */
void IRQHandler(void) {
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(xSem, &woken);
    portYIELD_FROM_ISR(woken);
}
```

### 忘记初始化二值信号量的初始值

```c
/* 注意：xSemaphoreCreateBinary() 创建的信号量初始值为 0 */
/* 如果你想让任务一开始就能 Take，需要先 Give 一次 */

xBinSem = xSemaphoreCreateBinary();
xSemaphoreGive(xBinSem); /* 手动设置初始值为 1 */
```

---

## API 速查

| API | 说明 | 可用场景 |
|-----|------|---------|
| `xSemaphoreCreateBinary()` | 创建二值信号量（初始值 0） | 任何时候 |
| `xSemaphoreCreateCounting(max, init)` | 创建计数信号量 | 任何时候 |
| `xSemaphoreCreateBinaryStatic(&buf)` | 静态创建二值信号量 | 任何时候 |
| `xSemaphoreTake(sem, timeout)` | 获取信号量（Take），计数 -1 | 任务 |
| `xSemaphoreGive(sem)` | 释放信号量（Give），计数 +1 | 任务 |
| `xSemaphoreGiveFromISR(sem, &woken)` | 中断中 Give | 中断 |
| `xSemaphoreTakeFromISR(sem, &woken)` | 中断中 Take（较少用） | 中断 |
| `uxSemaphoreGetCount(sem)` | 查询当前计数值 | 任务/中断 |
| `vSemaphoreDelete(sem)` | 删除信号量 | 任务 |
