# FreeRTOS 互斥量（Mutex）

## 目录

- [核心概念](#核心概念)
- [优先级继承机制](#优先级继承机制)
- [普通互斥量](#普通互斥量)
- [递归互斥量（Recursive Mutex）](#递归互斥量recursive-mutex)
- [互斥量 vs 二值信号量 vs 临界区](#互斥量-vs-二值信号量-vs-临界区)
- [典型使用场景](#典型使用场景)
- [死锁与预防](#死锁与预防)
- [常见错误](#常见错误)
- [API 速查](#api-速查)

---

## 核心概念

互斥量（Mutex，Mutual Exclusion）用于**保护共享资源**，确保同一时刻只有一个任务能访问该资源。

互斥量的本质是一个特殊的二值信号量，但有两个关键区别：

1. **所有权约束**：只有"持有"互斥量的任务才能"释放"它（信号量任何任务都可以 Give）
2. **优先级继承**：内置机制防止优先级反转

**使用模式固定为：**

```
获取互斥量（Take）→ 访问共享资源（临界区）→ 释放互斥量（Give）
```

---

## 优先级继承机制

### 问题：优先级反转

没有优先级继承时，高优先级任务可能被低优先级任务"间接阻塞"很久。

```
任务优先级：H（高）> M（中）> L（低）

时序：
  1. 任务 L 获取互斥量，进入临界区
  2. 任务 H 就绪，尝试获取互斥量 → 被阻塞（互斥量被 L 持有）
  3. 任务 M 就绪 → 抢占 L（M 优先级 > L），L 无法执行
  4. 任务 M 执行很长时间
  5. M 执行完，L 才能继续，释放互斥量
  6. H 才能运行

结果：H 等待了 M 的全部执行时间，高优先级任务被低优先级任务拖延 ← 优先级反转
```

### 解决：优先级继承

FreeRTOS 互斥量自动处理：

```
同样场景：
  1. 任务 L 获取互斥量
  2. 任务 H 尝试获取互斥量 → 被阻塞
  ★ FreeRTOS：临时将 L 的优先级提升到与 H 相同
  3. L（现在优先级 = H）能抢占 M，迅速完成临界区
  4. L 释放互斥量 → L 的优先级恢复为原来的低优先级
  5. H 立即获得互斥量，开始运行

结果：H 仅等待 L 完成临界区所需的时间（通常很短）
```

> 优先级继承**不能完全消除**优先级反转，只能减少影响。如果临界区执行时间很长，还是会有影响。

---

## 普通互斥量

### 创建与使用

```c
SemaphoreHandle_t xMutex;

/* 创建互斥量（初始状态：可用，等同于 Give 了一次） */
xMutex = xSemaphoreCreateMutex();
if (xMutex == NULL) {
    Error_Handler();
}
```

```c
/* 标准使用模式 */
void vTaskA(void *p) {
    for (;;) {
        /* ① 尝试获取互斥量，等待最多 100ms */
        if (xSemaphoreTake(xMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            
            /* ② 临界区：独占访问共享资源 */
            SharedResource_Write(data);
            
            /* ③ 释放互斥量（必须由持有者释放） */
            xSemaphoreGive(xMutex);
        } else {
            /* 超时未能获取互斥量，处理超时逻辑 */
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

**注意：互斥量不能在中断中使用。** 中断中保护共享数据应使用临界区。

---

## 递归互斥量（Recursive Mutex）

普通互斥量不允许同一个任务重复 Take（会死锁）。递归互斥量允许同一个任务嵌套获取，但每次 Take 必须对应一次 Give。

### 使用场景

函数 A 持有互斥量，调用了函数 B，B 也需要获取同一个互斥量。使用普通互斥量会死锁，递归互斥量可以正常工作。

```c
SemaphoreHandle_t xRecMutex;

/* 需要在 FreeRTOSConfig.h 中开启：configUSE_RECURSIVE_MUTEXES = 1 */
xRecMutex = xSemaphoreCreateRecursiveMutex();

void FunctionB(void) {
    /* 同一任务再次获取互斥量（嵌套第 2 层） */
    xSemaphoreTakeRecursive(xRecMutex, portMAX_DELAY);
    DoWork();
    xSemaphoreGiveRecursive(xRecMutex); /* 对应释放 */
}

void FunctionA(void) {
    /* 第 1 次获取 */
    xSemaphoreTakeRecursive(xRecMutex, portMAX_DELAY);
    
    DoSomething();
    FunctionB(); /* FunctionB 内部也会 Take/Give，普通互斥量会死锁，递归不会 */
    
    xSemaphoreGiveRecursive(xRecMutex); /* 对应释放 */
}

void vTask(void *p) {
    for (;;) {
        FunctionA();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

**内部计数原理：**

```
Task Take → 内部计数 = 1
  FunctionA Take → 内部计数 = 2
    FunctionB Take → 内部计数 = 3
    FunctionB Give → 内部计数 = 2
  FunctionA Give → 内部计数 = 1
Task Give → 内部计数 = 0 → 真正释放，其他任务可以获取
```

---

## 互斥量 vs 二值信号量 vs 临界区

| 对比项 | 互斥量 | 二值信号量 | 临界区 |
|--------|--------|-----------|--------|
| 主要用途 | 保护共享资源 | 通知事件发生 | 短暂保护（极快） |
| 优先级继承 | ✅ 有 | ❌ 无 | — |
| 中断中使用 | ❌ 不可以 | ✅ FromISR 可以 | ✅ FromISR 版本可以 |
| 持有者限制 | ✅ 只有持有者可 Give | ❌ 任何人可 Give | — |
| 阻塞任务 | ✅ 可等待 | ✅ 可等待 | ❌ 不阻塞，屏蔽中断 |
| 开销 | 中 | 中 | 最小 |
| 适用时长 | 毫秒级 | — | 微秒级（极短） |

**选择原则：**

```
需要等待资源且资源被其他任务占用（阻塞可接受）→ 互斥量
纯粹通知"某事发生了"→ 二值信号量
共享数据的修改极快（几条指令）→ 临界区
在中断中保护数据 → 临界区（FromISR 版本）
```

### 临界区用法

```c
/* 临界区：挂起中断，保护极短的操作 */
taskENTER_CRITICAL();
/* ← 中断被屏蔽，只能执行极快的代码（<几十微秒）*/
g_counter++;
taskEXIT_CRITICAL();

/* 中断中的临界区 */
UBaseType_t uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
g_irq_count++;
taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
```

---

## 典型使用场景

### 场景 1：多任务共享 UART（printf 保护）

多个任务都调用 `printf`，不加保护会导致输出混乱。

```c
SemaphoreHandle_t xUartMutex;
xUartMutex = xSemaphoreCreateMutex();

/* 封装一个线程安全的打印函数 */
void SafePrintf(const char *fmt, ...) {
    xSemaphoreTake(xUartMutex, portMAX_DELAY);
    
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    
    xSemaphoreGive(xUartMutex);
}

/* 各任务调用 SafePrintf，不会互相打断 */
void vTask1(void *p) {
    for (;;) {
        SafePrintf("[Task1] count=%d\r\n", count++);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void vTask2(void *p) {
    for (;;) {
        SafePrintf("[Task2] value=%.2f\r\n", value);
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}
```

### 场景 2：保护 SPI 总线（多任务共享）

```c
SemaphoreHandle_t xSpiMutex;
xSpiMutex = xSemaphoreCreateMutex();

/* 任务A：写 Flash */
void vFlashTask(void *p) {
    for (;;) {
        if (xSemaphoreTake(xSpiMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            W25Q_WriteData(addr, data, len); /* SPI Flash 操作 */
            xSemaphoreGive(xSpiMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* 任务B：刷新 LCD */
void vLcdTask(void *p) {
    for (;;) {
        if (xSemaphoreTake(xSpiMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            LCD_RefreshScreen(framebuf); /* SPI LCD 操作 */
            xSemaphoreGive(xSpiMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(33)); /* ~30fps */
    }
}
```

### 场景 3：保护共享数据结构

```c
typedef struct {
    float    speed;
    float    position;
    uint32_t error_flags;
} MotorStatus_t;

static MotorStatus_t g_motor_status;
SemaphoreHandle_t xStatusMutex;
xStatusMutex = xSemaphoreCreateMutex();

/* 电机控制任务：写入状态 */
void vMotorTask(void *p) {
    for (;;) {
        float new_speed = CalcSpeed();
        
        xSemaphoreTake(xStatusMutex, portMAX_DELAY);
        g_motor_status.speed    = new_speed;
        g_motor_status.position += new_speed * 0.01f;
        xSemaphoreGive(xStatusMutex);
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* 显示任务：读取状态 */
void vDisplayTask(void *p) {
    MotorStatus_t snapshot;
    for (;;) {
        /* 获取一份快照，减少持锁时间 */
        xSemaphoreTake(xStatusMutex, portMAX_DELAY);
        snapshot = g_motor_status; /* 复制整个结构体 */
        xSemaphoreGive(xStatusMutex);
        
        /* 拿到快照后慢慢显示，不占用锁 */
        Display_ShowSpeed(snapshot.speed);
        Display_ShowPosition(snapshot.position);
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

---

## 死锁与预防

### 什么是死锁

两个任务互相等待对方持有的互斥量，永远无法继续。

```
死锁场景：
  任务 A：持有 Mutex1，等待 Mutex2
  任务 B：持有 Mutex2，等待 Mutex1
  → 双方永久阻塞
```

### 预防方法

**方法 1：固定获取顺序（最有效）**

所有任务按照相同的顺序获取多个互斥量。

```c
/* ✅ 所有任务都先获取 Mutex1，再获取 Mutex2 */
void vTaskA(void *p) {
    xSemaphoreTake(xMutex1, portMAX_DELAY); /* 先 1 */
    xSemaphoreTake(xMutex2, portMAX_DELAY); /* 再 2 */
    DoWork();
    xSemaphoreGive(xMutex2);
    xSemaphoreGive(xMutex1);
}

void vTaskB(void *p) {
    xSemaphoreTake(xMutex1, portMAX_DELAY); /* 先 1（与 A 顺序一致） */
    xSemaphoreTake(xMutex2, portMAX_DELAY); /* 再 2 */
    DoWork();
    xSemaphoreGive(xMutex2);
    xSemaphoreGive(xMutex1);
}
```

**方法 2：使用超时，失败后释放已持有的锁重试**

```c
void vTaskSafe(void *p) {
    for (;;) {
        if (xSemaphoreTake(xMutex1, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (xSemaphoreTake(xMutex2, pdMS_TO_TICKS(100)) == pdTRUE) {
                /* 两个都拿到了 */
                DoWork();
                xSemaphoreGive(xMutex2);
                xSemaphoreGive(xMutex1);
            } else {
                /* Mutex2 超时，释放 Mutex1，避免死锁 */
                xSemaphoreGive(xMutex1);
                vTaskDelay(pdMS_TO_TICKS(10)); /* 稍等再重试 */
            }
        }
    }
}
```

**方法 3：减少互斥量数量，避免持有多个**

如果可能，重新设计以避免同时持有多个互斥量。

---

## 常见错误

### 在中断中使用互斥量

```c
/* ❌ 错误：互斥量不能在中断中使用 */
void SomeIRQHandler(void) {
    xSemaphoreTake(xMutex, 0);     /* 不可以 */
    ModifySharedData();
    xSemaphoreGive(xMutex);        /* 不可以 */
}

/* ✅ 中断中用临界区保护极短操作 */
void SomeIRQHandler(void) {
    UBaseType_t saved = taskENTER_CRITICAL_FROM_ISR();
    g_irq_counter++;  /* 只做极快的操作 */
    taskEXIT_CRITICAL_FROM_ISR(saved);
}
```

### 忘记释放互斥量

```c
/* ❌ 危险：函数提前返回时忘记释放 */
void BadFunction(void) {
    xSemaphoreTake(xMutex, portMAX_DELAY);
    
    if (error_condition) {
        return;  /* 忘记 Give！其他所有等待该互斥量的任务永久阻塞 */
    }
    
    DoWork();
    xSemaphoreGive(xMutex);
}

/* ✅ 每个出口都要释放 */
void GoodFunction(void) {
    xSemaphoreTake(xMutex, portMAX_DELAY);
    
    if (error_condition) {
        xSemaphoreGive(xMutex); /* 提前返回前先释放 */
        return;
    }
    
    DoWork();
    xSemaphoreGive(xMutex);
}
```

### 临界区内执行耗时操作

```c
/* ❌ 危险：临界区时间过长，其他任务长时间无法运行 */
xSemaphoreTake(xMutex, portMAX_DELAY);
HAL_Delay(100);          /* 阻塞 100ms，期间无法切换任务 */
SendDataViaUART(bigBuf); /* 耗时操作 */
xSemaphoreGive(xMutex);

/* ✅ 临界区内只做必要的最小操作 */
xSemaphoreTake(xMutex, portMAX_DELAY);
uint32_t snapshot = g_shared_value; /* 快速复制 */
xSemaphoreGive(xMutex);

/* 临界区外处理 */
SendDataViaUART(&snapshot, sizeof(snapshot));
```

---

## API 速查

| API | 说明 | 备注 |
|-----|------|------|
| `xSemaphoreCreateMutex()` | 创建普通互斥量 | 初始可用 |
| `xSemaphoreCreateRecursiveMutex()` | 创建递归互斥量 | 需开启 `configUSE_RECURSIVE_MUTEXES` |
| `xSemaphoreCreateMutexStatic(&buf)` | 静态创建互斥量 | — |
| `xSemaphoreTake(mutex, timeout)` | 获取互斥量 | 仅任务，不可在中断中用 |
| `xSemaphoreGive(mutex)` | 释放互斥量 | 仅持有者可释放 |
| `xSemaphoreTakeRecursive(mutex, timeout)` | 递归获取 | 仅递归互斥量 |
| `xSemaphoreGiveRecursive(mutex)` | 递归释放 | 仅递归互斥量 |
| `xSemaphoreGetMutexHolder(mutex)` | 查询当前持有者句柄 | 调试用 |
| `vSemaphoreDelete(mutex)` | 删除互斥量 | — |
| `taskENTER_CRITICAL()` | 进入临界区（屏蔽中断） | 任务中 |
| `taskEXIT_CRITICAL()` | 退出临界区 | 任务中 |
| `taskENTER_CRITICAL_FROM_ISR()` | 中断中进入临界区 | 中断中 |
| `taskEXIT_CRITICAL_FROM_ISR(key)` | 中断中退出临界区 | 中断中 |
