# RT-Thread 信号量（Semaphore）

## 目录

- [核心概念](#核心概念)
- [二值信号量](#二值信号量)
- [计数信号量](#计数信号量)
- [信号量 vs 互斥量](#信号量-vs-互斥量)
- [在中断中使用信号量](#在中断中使用信号量)
- [典型使用场景](#典型使用场景)
- [常见错误](#常见错误)
- [API 速查](#api-速查)

---

## 核心概念

信号量是一个**整数计数器**，用于实现线程间或中断与线程间的**同步**。

- **`rt_sem_take`（获取/等待）**：计数 -1；若计数为 0 则阻塞等待
- **`rt_sem_release`（释放/通知）**：计数 +1；若有线程在等待则唤醒它

信号量**不携带数据**，只表达"某件事发生了"或"某个资源可用"。需要同时传递数据时，应改用消息队列。

| 类型 | 计数范围 | 主要用途 |
|------|---------|---------|
| 二值信号量 | 0 或 1 | 线程/中断同步，通知"事件发生了" |
| 计数信号量 | 0 ~ N | 管理 N 个资源，或记录事件发生次数 |

> **RT-Thread 与 FreeRTOS 的重要区别：**
> RT-Thread 中 `rt_sem_release()` 可以直接在中断中调用，无需像 FreeRTOS 那样切换到 `xSemaphoreGiveFromISR()` 版本，使用更简单。

---

## 二值信号量

计数只有 0 和 1，适合**一次性事件通知**：一方释放，另一方等待。

### 创建

```c
rt_sem_t sem;

/* 创建二值信号量，初始值 0（等待触发状态） */
sem = rt_sem_create(
    "sem1",             /* 名称，最长 RT_NAME_MAX 字符 */
    0,                  /* 初始值：0 表示创建后立即等待 */
    RT_IPC_FLAG_FIFO    /* 等待规则：FIFO 先等先得；PRIO 高优先级先得 */
);

if (sem == RT_NULL) {
    /* 内存不足，创建失败 */
}
```

### 基本用法：中断唤醒线程

这是信号量最典型的场景：**中断发生 → 唤醒线程处理**。

```
GPIO 中断 ──rt_sem_release──→ [信号量 0→1] ──rt_sem_take──→ 线程被唤醒
```

```c
static rt_sem_t btn_sem = RT_NULL;

/* 在初始化时创建 */
btn_sem = rt_sem_create("btn", 0, RT_IPC_FLAG_FIFO);

/* GPIO 外部中断回调 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == KEY_Pin)
    {
        /* rt_sem_release 可以直接在中断中调用 */
        rt_sem_release(btn_sem);
    }
}

/* 按键处理线程 */
void key_thread_entry(void *parameter)
{
    while (1)
    {
        /* 永久阻塞等待，此时线程不占 CPU */
        rt_sem_take(btn_sem, RT_WAITING_FOREVER);

        /* 被唤醒后处理按键事件 */
        rt_thread_mdelay(20);  /* 消抖 20ms */
        if (HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin) == GPIO_PIN_RESET)
        {
            ToggleLED();
        }
    }
}
```

### 超时等待

```c
/* 等待最多 1000ms */
rt_err_t result = rt_sem_take(sem, rt_tick_from_millisecond(1000));
if (result == RT_EOK)
{
    /* 成功获取 */
    HandleEvent();
}
else  /* result == -RT_ETIMEOUT */
{
    /* 超时，没有事件发生 */
    HandleTimeout();
}

/* 非阻塞尝试获取（立即返回） */
if (rt_sem_trytake(sem) == RT_EOK)
{
    /* 成功 */
}
```

### 二值信号量的局限性

```
问题：事件连续触发两次，但线程来不及处理第一次

时序：
  中断1 → release（0→1）
  中断2 → release（已经是1，计数不超过1，第二次被丢弃）
  线程  → take（1→0，只处理了一次）
  ⚠️ 第二次事件丢失！
```

事件可能快速连发时（如 UART 连续收到多个字节），改用**计数信号量**或**消息队列**。

---

## 计数信号量

计数范围 0 ~ N，能记录多次 `release` 操作，不丢失事件。

### 创建

```c
/* 用途1：记录事件次数，初始 0 */
rt_sem_t event_sem = rt_sem_create("evt", 0, RT_IPC_FLAG_FIFO);

/* 用途2：资源池，5 个资源全部可用，初始值 = 最大值 */
/* RT-Thread 信号量没有最大值限制，靠业务逻辑控制 */
rt_sem_t res_sem = rt_sem_create("res", 5, RT_IPC_FLAG_FIFO);
```

### 用途 1：不丢失事件的连续通知

```c
static rt_sem_t pkt_sem;
pkt_sem = rt_sem_create("pkt", 0, RT_IPC_FLAG_FIFO);

/* UART 每收到一个数据包，release 一次（计数 +1） */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    SavePacket(rx_buf);
    rt_sem_release(pkt_sem);  /* 中断中直接调用，无需 FromISR */
    HAL_UART_Receive_IT(huart, rx_buf, PACKET_SIZE);
}

/* 处理线程：每次 take 处理一个包（计数 -1） */
void packet_thread_entry(void *p)
{
    while (1)
    {
        rt_sem_take(pkt_sem, RT_WAITING_FOREVER);
        ProcessPacket();  /* 连续收了 3 包，take 3 次，一次不丢 */
    }
}
```

### 用途 2：管理资源池

```c
#define BUFFER_NUM 4
static rt_uint8_t buffers[BUFFER_NUM][256];
static rt_uint8_t buf_used[BUFFER_NUM] = {0};

/* 初始值 4 = 4 个缓冲区全部可用 */
rt_sem_t buf_sem = rt_sem_create("buf", BUFFER_NUM, RT_IPC_FLAG_FIFO);

/* 申请缓冲区 */
rt_uint8_t *alloc_buffer(void)
{
    /* 有可用缓冲区时立即返回，否则阻塞等待 */
    rt_sem_take(buf_sem, RT_WAITING_FOREVER);  /* 计数 -1 */
    for (int i = 0; i < BUFFER_NUM; i++)
    {
        if (!buf_used[i]) { buf_used[i] = 1; return buffers[i]; }
    }
    return RT_NULL;
}

/* 释放缓冲区 */
void free_buffer(rt_uint8_t *buf)
{
    for (int i = 0; i < BUFFER_NUM; i++)
    {
        if (buffers[i] == buf)
        {
            buf_used[i] = 0;
            rt_sem_release(buf_sem);  /* 计数 +1，通知等待者 */
            return;
        }
    }
}
```

---

## 信号量 vs 互斥量

| 特性 | 信号量 | 互斥量 |
|------|--------|--------|
| 用途 | **同步**（通知事件发生） | **互斥**（保护共享资源） |
| release 限制 | 任何线程/中断都可以 | 只有持有者才能 release |
| 优先级继承 | ❌ 没有 | ✅ 有，防止优先级反转 |
| 中断中使用 | ✅ 可以 release | ❌ 不可以 |
| 初始状态 | 0（等待触发）| 1（可用） |
| 典型用法 | 中断通知线程、线程间同步 | 保护 UART/SPI/全局变量 |

**选择原则：**
- 通知某件事发生了 → **信号量**
- 保护共享资源 → **互斥量**
- 计数有限资源 → **计数信号量**

---

## 在中断中使用信号量

RT-Thread 的 `rt_sem_release()` 可以直接在中断中调用，这是与 FreeRTOS 最显著的差异之一。

```c
/* ✅ RT-Thread：直接调用，无需 FromISR 后缀 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    rt_sem_release(btn_sem);   /* 直接调用即可 */
}

/* FreeRTOS 对比（参考）：
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(sem, &woken);       // 必须用 FromISR 版本
    portYIELD_FROM_ISR(woken);                // 还需要这一行
}
*/
```

**注意：中断中只能 `release`，不能 `take`（会阻塞中断）：**

```c
/* ❌ 错误：中断中不能等待信号量 */
void SomeIRQHandler(void)
{
    rt_sem_take(sem, RT_WAITING_FOREVER);  /* 会挂起中断，系统崩溃 */
}

/* ✅ 正确：中断中只释放，线程中等待 */
void SomeIRQHandler(void)
{
    rt_sem_release(sem);   /* 通知线程处理 */
}
```

---

## 典型使用场景

### 场景 1：DMA 传输完成通知

```c
static rt_sem_t dma_sem;
dma_sem = rt_sem_create("dma", 0, RT_IPC_FLAG_FIFO);

/* DMA 完成回调（中断上下文） */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    rt_sem_release(dma_sem);  /* 通知等待的线程 */
}

/* SPI 通信线程 */
void spi_thread_entry(void *p)
{
    while (1)
    {
        PrepareData(tx_buf, sizeof(tx_buf));

        /* 启动 DMA 发送 */
        HAL_SPI_Transmit_DMA(&hspi1, tx_buf, sizeof(tx_buf));

        /* 等待 DMA 完成，阻塞期间不占 CPU */
        rt_sem_take(dma_sem, RT_WAITING_FOREVER);

        /* DMA 完成，处理下一帧 */
        ProcessResponse();
    }
}
```

### 场景 2：线程间同步（A 完成后 B 才开始）

```c
static rt_sem_t sync_sem;
sync_sem = rt_sem_create("sync", 0, RT_IPC_FLAG_FIFO);

/* 线程 A：完成初始化后通知线程 B */
void thread_a_entry(void *p)
{
    InitHardware();
    CalibrateSensor();
    rt_kprintf("Init done, notify B\n");

    rt_sem_release(sync_sem);  /* 通知 B */

    while (1)
    {
        DoMainWork();
        rt_thread_mdelay(100);
    }
}

/* 线程 B：等待 A 完成后再开始工作 */
void thread_b_entry(void *p)
{
    rt_sem_take(sync_sem, RT_WAITING_FOREVER);  /* 等 A 通知 */
    rt_kprintf("B starts working\n");

    while (1)
    {
        ReadSensorData();
        rt_thread_mdelay(50);
    }
}
```

### 场景 3：ADC 采集完成通知

```c
static rt_sem_t adc_sem;
adc_sem = rt_sem_create("adc", 0, RT_IPC_FLAG_FIFO);

/* ADC 转换完成中断 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    rt_sem_release(adc_sem);
}

void adc_thread_entry(void *p)
{
    while (1)
    {
        HAL_ADC_Start_IT(&hadc1);  /* 启动 ADC 转换 */

        /* 等待转换完成 */
        rt_sem_take(adc_sem, rt_tick_from_millisecond(100));

        uint16_t val = HAL_ADC_GetValue(&hadc1);
        float voltage = val * 3.3f / 4096.0f;
        rt_kprintf("voltage = %.3f V\n", voltage);

        rt_thread_mdelay(10);
    }
}
```

---

## 常见错误

### 二值信号量被连续 release 导致事件丢失

```c
/* ⚠️ 问题：中断频繁触发，take 跟不上 release */
/* 二值信号量最多积累 1 次，多余的 release 被丢弃 */

/* 解决：改用计数信号量（但 RT-Thread 的信号量本身就是计数信号量） */
/* 只要初始值设 0，每次 release +1，take -1，不会丢失 */
rt_sem_t sem = rt_sem_create("sem", 0, RT_IPC_FLAG_FIFO);
/* 这就已经是计数信号量了，不需要特别声明 */
```

### 忘记检查 rt_sem_create 返回值

```c
/* ❌ 不检查，后续 take/release 会崩溃 */
sem = rt_sem_create("s", 0, RT_IPC_FLAG_FIFO);
rt_sem_take(sem, RT_WAITING_FOREVER);  /* 如果 sem 是 RT_NULL，崩溃 */

/* ✅ 检查创建结果 */
sem = rt_sem_create("s", 0, RT_IPC_FLAG_FIFO);
RT_ASSERT(sem != RT_NULL);  /* 调试断言 */
```

### 用信号量保护资源（应该用互斥量）

```c
/* ❌ 用信号量当锁：没有优先级继承，可能导致优先级反转 */
rt_sem_t lock = rt_sem_create("lock", 1, RT_IPC_FLAG_FIFO);
rt_sem_take(lock, RT_WAITING_FOREVER);
AccessSharedResource();
rt_sem_release(lock);

/* ✅ 保护共享资源应该用互斥量 */
rt_mutex_t mutex = rt_mutex_create("mtx", RT_IPC_FLAG_PRIO);
rt_mutex_take(mutex, RT_WAITING_FOREVER);
AccessSharedResource();
rt_mutex_release(mutex);
```

---

## API 速查

| API | 说明 | 可用场景 |
|-----|------|---------|
| `rt_sem_create(name, val, flag)` | 动态创建信号量 | 任何时候 |
| `rt_sem_init(&sem, name, val, flag)` | 静态创建信号量 | 任何时候 |
| `rt_sem_delete(sem)` | 删除动态信号量 | 线程 |
| `rt_sem_detach(&sem)` | 脱离静态信号量 | 线程 |
| `rt_sem_take(sem, timeout)` | 获取信号量（阻塞） | 线程 |
| `rt_sem_trytake(sem)` | 尝试获取（非阻塞） | 线程/中断 |
| `rt_sem_release(sem)` | 释放信号量 | **线程/中断均可** |

**timeout 常用值：**

| 值 | 含义 |
|----|------|
| `RT_WAITING_FOREVER` | 永久等待 |
| `RT_WAITING_NO` | 不等待，立即返回（等价于 0） |
| `rt_tick_from_millisecond(ms)` | 等待指定毫秒数 |
