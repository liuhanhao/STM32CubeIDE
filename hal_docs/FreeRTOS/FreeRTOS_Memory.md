# FreeRTOS 内存管理

## 目录

- [核心概念](#核心概念)
- [五种堆管理方案（heap_1 ~ heap_5）](#五种堆管理方案heap_1--heap_5)
- [heap_4 详解（推荐）](#heap_4-详解推荐)
- [内存分配与释放](#内存分配与释放)
- [堆监控与调试](#堆监控与调试)
- [静态分配（零动态内存）](#静态分配零动态内存)
- [内存碎片问题](#内存碎片问题)
- [STM32F103C8T6 内存规划](#stm32f103c8t6-内存规划)
- [内存不足的常见症状与排查](#内存不足的常见症状与排查)
- [典型使用场景](#典型使用场景)

---

## 核心概念

FreeRTOS 有自己独立的堆管理系统，不使用 C 标准库的 `malloc/free`（默认情况下）。

**FreeRTOS 的内存分配用途：**
- 任务控制块（TCB）+ 任务栈
- 队列缓冲区
- 信号量、互斥量控制块
- 软件定时器控制块
- 事件组控制块

**堆的范围：** 由 `configTOTAL_HEAP_SIZE` 定义，位于 BSS 段的一块静态数组 `ucHeap[]` 中。

```c
/* FreeRTOS 源码内部（portable/MemMang/heap_4.c） */
static uint8_t ucHeap[configTOTAL_HEAP_SIZE];
```

---

## 五种堆管理方案（heap_1 ~ heap_5）

STM32CubeMX 默认使用 **heap_4**，绝大多数项目用它即可。

| 方案 | 支持 pvPortFree | 内存碎片整理 | 线程安全 | 适用场景 |
|------|----------------|------------|---------|---------|
| **heap_1** | ❌ | ❌ | ✅ | 只分配不释放；系统运行后内存布局固定 |
| **heap_2** | ✅ | ❌（可能碎片） | ✅ | 反复分配相同大小的块 |
| **heap_3** | ✅ | — | ❌ 需自行加锁 | 直接用标准库 malloc/free |
| **heap_4** | ✅ | ✅（合并相邻空闲块） | ✅ | **通用首选，大多数项目** |
| **heap_5** | ✅ | ✅ | ✅ | 内存跨多个非连续物理区域 |

### heap_1：只分配，不释放

```
内存布局：
[已分配A][已分配B][已分配C][────────── 空闲 ──────────]
                            ↑ 分配指针单向移动
```

适合任务数量固定、启动时一次性分配完的系统（最简单、最可预测）。

### heap_2：可释放，但有碎片

```
内存布局（释放 B 后）：
[已分配A][──空闲──][已分配C][────────── 空闲 ──────────]
          ↑ 空闲块，但不与右侧空闲块合并
```

适合反复分配释放**相同大小**内存的场景（如消息池）。

### heap_4：可释放，相邻块合并（推荐）

```
内存布局（释放 A 和 B 后）：
[────────── 合并后的大空闲块 ──────────][已分配C][空闲]
```

### heap_5：跨非连续内存区域

适合有外部 SRAM（FSMC 连接）或多段内存的设备。需要手动调用 `vPortDefineHeapRegions()` 初始化。

```c
/* heap_5 初始化示例（内部 SRAM + 外部 SRAM） */
HeapRegion_t xHeapRegions[] = {
    { (uint8_t *)0x20000000, 0x5000 }, /* 内部 SRAM 20KB */
    { (uint8_t *)0x60000000, 0x8000 }, /* 外部 SRAM 32KB */
    { NULL, 0 }                         /* 结束标记 */
};
vPortDefineHeapRegions(xHeapRegions);
```

---

## heap_4 详解（推荐）

heap_4 使用**空闲链表**管理内存，分配时搜索足够大的空闲块，释放时自动与相邻空闲块合并。

### 内存分配过程

```
初始状态：
[────────────────────── 整块空闲（configTOTAL_HEAP_SIZE）──────────────────────]

分配任务A（512字节）：
[任务A TCB+Stack 512B][控制头8B][────────────── 空闲 ──────────────────────────]

分配队列B（80字节）：
[任务A][队列B 80B][控制头][──────────────────── 空闲 ───────────────────────────]

释放队列B后（如果任务被删除）：
[任务A][────── 空闲 80+控制头 ────][────────────────── 空闲 ────────────────────]
两段相邻空闲块合并 ↓
[任务A][──────────────────────────────── 合并后空闲 ─────────────────────────────]
```

### heap_4 的最小分配单位

```c
/* heap_4 内部对齐要求，通常为 8 字节 */
/* 申请 1 字节，实际分配 8 字节 + 控制头 8 字节 = 最少 16 字节 */
/* 申请 100 字节，实际分配 104 字节（对齐到 8 的倍数）+ 8 字节控制头 = 112 字节 */
```

---

## 内存分配与释放

### FreeRTOS 对象的分配

每次创建 FreeRTOS 对象时，内部自动调用 `pvPortMalloc`：

```c
/* 以下操作都会消耗堆空间 */
xTaskCreate(fn, "T", 128, NULL, 2, NULL);       /* TCB(~100B) + 栈(128×4=512B) = ~612B */
xQueueCreate(10, sizeof(uint32_t));              /* 控制块(~80B) + 缓冲(10×4=40B) = ~120B */
xSemaphoreCreateBinary();                        /* ~80B */
xSemaphoreCreateMutex();                         /* ~88B */
xEventGroupCreate();                             /* ~72B */
xTimerCreate(...);                               /* ~48B */
```

### 直接使用 pvPortMalloc/vPortFree

FreeRTOS 也暴露了直接的内存分配接口，可以在任务中使用：

```c
/* 动态分配缓冲区 */
uint8_t *pBuf = (uint8_t *)pvPortMalloc(256);
if (pBuf == NULL) {
    /* 分配失败：堆空间不足 */
    return;
}

/* 使用缓冲区 */
memset(pBuf, 0, 256);
DoSomethingWithBuffer(pBuf);

/* 释放（heap_4 支持，heap_1 不支持） */
vPortFree(pBuf);
```

> **不要混用** `pvPortMalloc/vPortFree` 和标准库的 `malloc/free`，两套内存池完全独立。

---

## 堆监控与调试

```c
/* 当前剩余堆大小（字节）*/
size_t freeNow = xPortGetFreeHeapSize();
printf("Free heap: %u bytes\r\n", freeNow);

/* 历史最小剩余堆大小（字节）
 * 反映堆使用的峰值，堆越小越危险
 * 仅 heap_4 和 heap_5 支持 */
size_t freeMin = xPortGetMinimumEverFreeHeapSize();
printf("Min free heap ever: %u bytes\r\n", freeMin);

/* 建议：调试期间在 vApplicationIdleHook 或定时打印 */
void vApplicationIdleHook(void) {
    static uint32_t lastPrint = 0;
    if (xTaskGetTickCount() - lastPrint > 5000) { /* 每 5 秒打印一次 */
        lastPrint = xTaskGetTickCount();
        printf("[Heap] free=%u min=%u\r\n",
               xPortGetFreeHeapSize(),
               xPortGetMinimumEverFreeHeapSize());
    }
}
```

### 任务栈使用监控

```c
/* 任务栈最小剩余（words），越小说明越接近栈溢出 */
UBaseType_t watermark = uxTaskGetStackHighWaterMark(NULL); /* NULL=查自身 */
printf("[Stack] min remaining: %u words\r\n", watermark);

/* 遍历所有任务打印栈水位 */
void PrintAllTasksStack(void) {
    TaskStatus_t tasks[10];
    UBaseType_t count = uxTaskGetSystemState(tasks, 10, NULL);
    for (UBaseType_t i = 0; i < count; i++) {
        printf("%-16s stack min: %u words\r\n",
               tasks[i].pcTaskName,
               tasks[i].usStackHighWaterMark);
    }
}
```

---

## 静态分配（零动态内存）

对于安全关键系统，可以完全避免动态内存分配，所有内存在编译时确定。

**开启静态分配：**

```c
/* FreeRTOSConfig.h */
#define configSUPPORT_STATIC_ALLOCATION  1
#define configSUPPORT_DYNAMIC_ALLOCATION 0  /* 可选：禁用动态分配 */
```

**静态创建所有对象：**

```c
/* 静态任务 */
static StackType_t  xTask1Stack[256];
static StaticTask_t xTask1TCB;
xTaskCreateStatic(vTask1, "T1", 256, NULL, 2, xTask1Stack, &xTask1TCB);

/* 静态队列 */
static uint8_t       xQueueStorage[10 * sizeof(uint32_t)];
static StaticQueue_t xQueueBuffer;
xQueueCreateStatic(10, sizeof(uint32_t), xQueueStorage, &xQueueBuffer);

/* 静态信号量 */
static StaticSemaphore_t xSemBuffer;
xSemaphoreCreateBinaryStatic(&xSemBuffer);

/* 静态互斥量 */
static StaticSemaphore_t xMutexBuffer;
xSemaphoreCreateMutexStatic(&xMutexBuffer);

/* 静态定时器 */
static StaticTimer_t xTimerBuffer;
xTimerCreateStatic("T", pdMS_TO_TICKS(1000), pdTRUE, 0, vCallback, &xTimerBuffer);

/* 静态事件组 */
static StaticEventGroup_t xEGBuffer;
xEventGroupCreateStatic(&xEGBuffer);
```

**必须实现的回调（静态空闲任务和定时器任务内存）：**

```c
/* 为空闲任务提供静态内存 */
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t  **ppxIdleTaskStackBuffer,
                                    uint32_t      *pulIdleTaskStackSize) {
    static StaticTask_t IdleTCB;
    static StackType_t  IdleStack[configMINIMAL_STACK_SIZE];
    *ppxIdleTaskTCBBuffer   = &IdleTCB;
    *ppxIdleTaskStackBuffer = IdleStack;
    *pulIdleTaskStackSize   = configMINIMAL_STACK_SIZE;
}

/* 为定时器任务提供静态内存（如果使用软件定时器） */
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                     StackType_t  **ppxTimerTaskStackBuffer,
                                     uint32_t      *pulTimerTaskStackSize) {
    static StaticTask_t TimerTCB;
    static StackType_t  TimerStack[configTIMER_TASK_STACK_DEPTH];
    *ppxTimerTaskTCBBuffer   = &TimerTCB;
    *ppxTimerTaskStackBuffer = TimerStack;
    *pulTimerTaskStackSize   = configTIMER_TASK_STACK_DEPTH;
}
```

---

## 内存碎片问题

内存碎片指：虽然总空闲内存足够，但没有一块连续空间能满足分配请求。

```
碎片示例：
总空闲 = 300 字节，但分散在 10 个 30 字节的小块中
此时申请 200 字节连续空间 → 失败！

[30B空闲][20B已用][30B空闲][15B已用][30B空闲]...
```

**heap_4 的碎片抑制：**

heap_4 在释放内存时会检查并合并相邻空闲块，显著减少碎片。但无法消除非相邻碎片。

**实践建议：**

```
1. 系统初始化时一次性创建所有任务/队列/信号量，不在运行时反复创建/删除
2. 避免频繁分配释放不同大小的内存块
3. 如果必须动态分配，使用固定大小的内存池（池式分配）
4. 使用静态分配彻底消除碎片问题
```

---

## STM32F103C8T6 内存规划

STM32F103C8T6 总 SRAM 20KB（0x20000000 ~ 0x20004FFF）。

**内存布局参考：**

```
0x20000000 ┌──────────────────────────────┐
           │ .data（已初始化全局变量）      │ ~0.5KB
           ├──────────────────────────────┤
           │ .bss（未初始化全局变量）       │ ~1KB
           │ 包含 FreeRTOS 堆（ucHeap[]） │
           ├──────────────────────────────┤
           │ FreeRTOS 堆（ucHeap[]）       │ 建议 8~12KB
           │ - 任务 TCB + 栈              │
           │ - 队列缓冲区                 │
           │ - 信号量、互斥量              │
           ├──────────────────────────────┤
           │ 主栈（MSP Stack）            │ 建议 1~2KB
           │ 用于：硬件中断处理           │
0x20004FFF └──────────────────────────────┘
```

**典型配置建议（20KB SRAM）：**

```c
/* FreeRTOSConfig.h */
#define configTOTAL_HEAP_SIZE  ((size_t)(10 * 1024))  /* 10KB */

/* 启动文件（.s）中 */
Stack_Size EQU 0x00000400  /* 主栈 1KB */
Heap_Size  EQU 0x00000000  /* C 堆设为 0，不使用标准库 malloc */
```

**内存预算示例（3 任务系统）：**

```
FreeRTOS 堆分配预算（共 10KB）：

任务 A：TCB~100B + 栈128W×4=512B = ~620B
任务 B：TCB~100B + 栈256W×4=1024B = ~1130B
任务 C：TCB~100B + 栈128W×4=512B = ~620B
空闲任务：TCB~100B + 栈64W×4=256B = ~370B
定时器任务：TCB~100B + 栈128W×4=512B = ~620B

队列1（深度10，uint32_t）：~120B
队列2（深度5，结构体20B）：~184B
互斥量×2：~180B
信号量×2：~160B
定时器×3：~150B

合计约：620+1130+620+370+620+120+184+180+160+150 = ~4350B

建议预留 50% 余量，即堆设为 8~10KB
```

---

## 内存不足的常见症状与排查

### 症状

| 症状 | 可能原因 |
|------|---------|
| 系统启动后立即崩溃/卡死 | 堆太小，首次分配失败 |
| `xTaskCreate` 返回 `errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY` | 堆不足 |
| 随机崩溃，难以复现 | 栈溢出 |
| HardFault 异常 | 访问了溢出栈覆盖的内存 |
| 任务无响应但其他任务正常 | 该任务栈溢出，PC 跑飞 |

### 排查步骤

```c
/* 步骤1：在 MX_FREERTOS_Init 结束后立即打印堆使用情况 */
void MX_FREERTOS_Init(void) {
    /* ... 创建所有任务和对象 ... */
    
    printf("After init, free heap: %u\r\n", xPortGetFreeHeapSize());
    /* 如果这里剩余很少（<2KB），需要增大 TOTAL_HEAP_SIZE */
}

/* 步骤2：开启栈溢出检测（configCHECK_FOR_STACK_OVERFLOW = 2） */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    printf("Stack overflow: %s\r\n", pcTaskName);
    for(;;); /* 卡在这里便于调试器发现 */
}

/* 步骤3：开启 malloc 失败钩子 */
void vApplicationMallocFailedHook(void) {
    printf("malloc failed! Free heap: %u\r\n", xPortGetFreeHeapSize());
    for(;;);
}
/* FreeRTOSConfig.h 中开启：configUSE_MALLOC_FAILED_HOOK = 1 */

/* 步骤4：定期打印各任务栈水位 */
void vMonitorTask(void *p) {
    for (;;) {
        vTaskList(pcBuffer);      /* 列出所有任务状态 */
        printf("%s", pcBuffer);
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
```

### 调整建议

```
问题：堆不足           → 增大 configTOTAL_HEAP_SIZE（从 SRAM 预算中挤）
问题：某任务栈溢出      → 增大该任务的 usStackDepth（创建参数第3个）
问题：SRAM 整体不足    → 使用静态分配、减少任务数量、合并相似任务
问题：碎片导致分配失败  → 初始化时一次性创建所有对象，不在运行时创建/删除
```

---

## 典型使用场景

### 场景：使用 pvPortMalloc 实现简单内存池

避免频繁小块分配导致碎片，预先分配一批固定大小的缓冲区。

```c
#define POOL_ITEM_SIZE  64
#define POOL_ITEM_COUNT 8

typedef struct PoolItem {
    struct PoolItem *next;
    uint8_t data[POOL_ITEM_SIZE];
} PoolItem_t;

static PoolItem_t *pool_head = NULL;
static SemaphoreHandle_t xPoolMutex;

/* 初始化内存池 */
void Pool_Init(void) {
    xPoolMutex = xSemaphoreCreateMutex();
    for (int i = 0; i < POOL_ITEM_COUNT; i++) {
        PoolItem_t *item = (PoolItem_t *)pvPortMalloc(sizeof(PoolItem_t));
        item->next = pool_head;
        pool_head  = item;
    }
}

/* 从池中申请 */
uint8_t *Pool_Alloc(void) {
    PoolItem_t *item = NULL;
    xSemaphoreTake(xPoolMutex, portMAX_DELAY);
    if (pool_head) {
        item       = pool_head;
        pool_head  = pool_head->next;
    }
    xSemaphoreGive(xPoolMutex);
    return item ? item->data : NULL;
}

/* 归还到池中 */
void Pool_Free(uint8_t *data) {
    PoolItem_t *item = (PoolItem_t *)(data - offsetof(PoolItem_t, data));
    xSemaphoreTake(xPoolMutex, portMAX_DELAY);
    item->next = pool_head;
    pool_head  = item;
    xSemaphoreGive(xPoolMutex);
}
```
