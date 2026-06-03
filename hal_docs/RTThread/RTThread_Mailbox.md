# RT-Thread 邮箱（Mailbox）

## 目录

- [核心概念](#核心概念)
- [工作原理](#工作原理)
- [创建与使用](#创建与使用)
- [零拷贝模式（传递指针）](#零拷贝模式传递指针)
- [配合内存池使用](#配合内存池使用)
- [在中断中使用邮箱](#在中断中使用邮箱)
- [典型使用场景](#典型使用场景)
- [邮箱 vs 消息队列](#邮箱-vs-消息队列)
- [常见错误](#常见错误)
- [API 速查](#api-速查)

---

## 核心概念

邮箱是 RT-Thread 特有的 IPC 机制，每封"邮件"是一个固定 4 字节（`rt_ubase_t`）的值。

邮箱有两种用途：

| 用途 | 说明 |
|------|------|
| **传递整数值** | 直接把 32 位整数作为邮件内容发送 |
| **零拷贝传指针** | 把指向数据的指针作为邮件发送，接收方通过指针访问数据，避免数据复制 |

**邮箱 vs 消息队列的核心区别：**

```
消息队列：每条消息的内容被完整复制到队列缓冲区
          发 100 字节 → 队列里存 100 字节副本

邮箱：每封邮件只存 4 字节（指针）
      发指向 100 字节数据的指针 → 队列里只存 4 字节地址
      接收方通过地址读取原始数据，零拷贝
```

邮箱在系统资源和速度上比消息队列更高效，是 RT-Thread 内核内部也大量使用的机制。

---

## 工作原理

```
发送方                     邮箱缓冲区              接收方
  │                      ┌──────────┐               │
  │──rt_mb_send(ptr)──→  │ 0x20000A │  ──rt_mb_recv(ptr)──→
  │                      │ 0x20000B │               │
  │                      │   ...    │               │
  │                      └──────────┘               │
  │                                                 │
  │        实际数据（在堆/静态区）                      │
  │  data[0]=0x12  data[1]=0x34 ...  ←─────访问─────┘
```

每格只存一个 4 字节地址，接收方通过地址直接访问数据，没有额外的内存复制。

---

## 创建与使用

### 创建邮箱

```c
rt_mailbox_t mb;

/* 创建邮箱，容量 10 封（每封 4 字节） */
mb = rt_mb_create(
    "mb1",              /* 名称 */
    10,                 /* 邮箱容量（能存几封邮件） */
    RT_IPC_FLAG_FIFO    /* 等待规则 */
);

if (mb == RT_NULL) {
    /* 内存不足 */
}
```

### 发送邮件

```c
/* 发送一个 32 位值（非阻塞，邮箱满立即返回 -RT_EFULL） */
rt_err_t ret = rt_mb_send(mb, 0xABCD1234);

/* 发送，邮箱满时阻塞等待 */
rt_mb_send_wait(mb, 0xABCD1234, RT_WAITING_FOREVER);

/* 发送，最多等 100ms */
rt_mb_send_wait(mb, value, rt_tick_from_millisecond(100));
```

### 接收邮件

```c
rt_ubase_t recv_val;

/* 阻塞接收，永久等待 */
if (rt_mb_recv(mb, &recv_val, RT_WAITING_FOREVER) == RT_EOK)
{
    rt_kprintf("received: 0x%08X\n", recv_val);
}

/* 超时接收 */
rt_err_t ret = rt_mb_recv(mb, &recv_val, rt_tick_from_millisecond(500));
if (ret == RT_EOK) {
    /* 有邮件 */
} else {
    /* 超时 */
}
```

---

## 零拷贝模式（传递指针）

这是邮箱最重要的用法：发送方将**指向数据的指针**作为邮件发送，接收方通过指针直接访问数据，完全避免数据复制。

### 基本示例

```c
/* 发送字符串（传指针，不复制字符串内容） */
const char *msg = "hello from task A";
rt_mb_send(mb, (rt_ubase_t)msg);  /* 只发送指针（4字节地址） */

/* 接收并使用 */
rt_ubase_t recv;
rt_mb_recv(mb, &recv, RT_WAITING_FOREVER);
const char *str = (const char *)recv;
rt_kprintf("got: %s\n", str);
```

### 传递结构体指针

```c
typedef struct {
    float    x;
    float    y;
    float    z;
    rt_uint32_t timestamp;
} ImuData_t;

rt_mailbox_t imu_mb = rt_mb_create("imu", 8, RT_IPC_FLAG_FIFO);

/* 发送方：动态分配数据，只传指针 */
void imu_acquire_entry(void *p)
{
    while (1)
    {
        /* 分配内存存放数据 */
        ImuData_t *data = (ImuData_t *)rt_malloc(sizeof(ImuData_t));
        if (data == RT_NULL) { rt_thread_mdelay(1); continue; }

        data->x         = ReadGyroX();
        data->y         = ReadGyroY();
        data->z         = ReadGyroZ();
        data->timestamp = rt_tick_get();

        /* 发送指针（4字节），不复制 ImuData_t 的 16 字节内容 */
        if (rt_mb_send(imu_mb, (rt_ubase_t)data) != RT_EOK)
        {
            rt_free(data);  /* 邮箱满，释放内存避免泄漏 */
        }

        rt_thread_mdelay(10);
    }
}

/* 接收方：通过指针访问数据，处理完后释放内存 */
void imu_process_entry(void *p)
{
    rt_ubase_t recv;
    while (1)
    {
        rt_mb_recv(imu_mb, &recv, RT_WAITING_FOREVER);
        ImuData_t *data = (ImuData_t *)recv;

        /* 处理数据 */
        ProcessIMU(data->x, data->y, data->z);

        /* ⚠️ 必须释放内存，否则内存泄漏 */
        rt_free(data);
    }
}
```

---

## 配合内存池使用

动态 `rt_malloc`/`rt_free` 有内存碎片问题。固定大小的数据推荐配合**内存池（Memory Pool）**使用，效率更高且无碎片。

```c
/* 内存池：提前分配 10 块 ImuData_t 大小的内存 */
static rt_mp_t imu_pool;
imu_pool = rt_mp_create("imu_pool", 10, sizeof(ImuData_t));

/* 发送方：从内存池申请 */
void imu_acquire_entry(void *p)
{
    while (1)
    {
        /* 从内存池申请（分配速度固定，无碎片） */
        ImuData_t *data = (ImuData_t *)rt_mp_alloc(imu_pool, RT_WAITING_FOREVER);

        FillImuData(data);

        if (rt_mb_send(imu_mb, (rt_ubase_t)data) != RT_EOK)
        {
            rt_mp_free(data);  /* 邮箱满，还回内存池 */
        }

        rt_thread_mdelay(10);
    }
}

/* 接收方：处理完后归还内存池 */
void imu_process_entry(void *p)
{
    rt_ubase_t recv;
    while (1)
    {
        rt_mb_recv(imu_mb, &recv, RT_WAITING_FOREVER);
        ImuData_t *data = (ImuData_t *)recv;

        ProcessIMU(data);

        rt_mp_free(data);  /* 归还内存池，不是 rt_free */
    }
}
```

---

## 在中断中使用邮箱

```c
/* ✅ rt_mb_send 可以在中断中直接调用（非阻塞版本） */
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
    /* 将接收完成的缓冲区指针通过邮箱通知处理线程 */
    rt_mb_send(spi_rx_mb, (rt_ubase_t)rx_buf_ptr);
}

/* ❌ 中断中不能用阻塞版本 */
void SomeIRQHandler(void)
{
    rt_mb_send_wait(mb, value, RT_WAITING_FOREVER);  /* 不可以 */
}
```

---

## 典型使用场景

### 场景 1：摄像头帧缓冲区传递（大数据零拷贝）

```c
/* 两块帧缓冲区：一块采集时处理方用另一块 */
static rt_uint8_t frame_buf_A[320 * 240 * 2];
static rt_uint8_t frame_buf_B[320 * 240 * 2];

rt_mailbox_t frame_mb = rt_mb_create("frame", 2, RT_IPC_FLAG_FIFO);

void camera_capture_entry(void *p)
{
    rt_uint8_t *bufs[2] = {frame_buf_A, frame_buf_B};
    rt_uint8_t idx = 0;

    while (1)
    {
        /* 采集到当前缓冲区 */
        Camera_Capture(bufs[idx]);

        /* 发送缓冲区指针（只传 4 字节，不复制 150KB 帧数据）*/
        rt_mb_send(frame_mb, (rt_ubase_t)bufs[idx]);

        /* 切换到另一块缓冲区继续采集 */
        idx ^= 1;
    }
}

void image_process_entry(void *p)
{
    rt_ubase_t recv;
    while (1)
    {
        rt_mb_recv(frame_mb, &recv, RT_WAITING_FOREVER);
        rt_uint8_t *frame = (rt_uint8_t *)recv;

        /* 直接在原始数据上处理，无复制开销 */
        Detect_Objects(frame, 320, 240);
    }
}
```

### 场景 2：音频流处理

```c
/* 音频缓冲区管理 */
#define AUDIO_BLOCK_SIZE 512
#define BLOCK_NUM        4

static rt_uint16_t audio_blocks[BLOCK_NUM][AUDIO_BLOCK_SIZE];
static rt_mp_t audio_pool;

rt_mailbox_t fill_mb;    /* 空块 → 采集线程 */
rt_mailbox_t proc_mb;    /* 满块 → 处理线程 */

void audio_init(void)
{
    /* 内存池管理音频块 */
    audio_pool = rt_mp_create("audio", BLOCK_NUM, sizeof(rt_uint16_t) * AUDIO_BLOCK_SIZE);
    fill_mb    = rt_mb_create("fill", BLOCK_NUM, RT_IPC_FLAG_FIFO);
    proc_mb    = rt_mb_create("proc", BLOCK_NUM, RT_IPC_FLAG_FIFO);

    /* 把所有空块发给采集线程 */
    for (int i = 0; i < BLOCK_NUM; i++)
        rt_mb_send(fill_mb, (rt_ubase_t)audio_blocks[i]);
}

/* 采集线程：拿空块 → 填数据 → 发给处理线程 */
void audio_capture_entry(void *p)
{
    rt_ubase_t recv;
    while (1)
    {
        rt_mb_recv(fill_mb, &recv, RT_WAITING_FOREVER);
        rt_uint16_t *block = (rt_uint16_t *)recv;

        HAL_SAI_Receive(&hsai, (rt_uint8_t *)block, AUDIO_BLOCK_SIZE, HAL_MAX_DELAY);

        rt_mb_send(proc_mb, (rt_ubase_t)block);
    }
}

/* 处理线程：拿满块 → 处理 → 还回空块池 */
void audio_process_entry(void *p)
{
    rt_ubase_t recv;
    while (1)
    {
        rt_mb_recv(proc_mb, &recv, RT_WAITING_FOREVER);
        rt_uint16_t *block = (rt_uint16_t *)recv;

        AudioFFT(block, AUDIO_BLOCK_SIZE);

        rt_mb_send(fill_mb, (rt_ubase_t)block);  /* 还给采集线程 */
    }
}
```

### 场景 3：简单整数通知（附带数值）

```c
/* 不需要传结构体，只需要传一个状态码或错误码 */
rt_mailbox_t status_mb = rt_mb_create("status", 5, RT_IPC_FLAG_FIFO);

/* 发送状态码 */
rt_mb_send(status_mb, 0x01);   /* 状态 OK */
rt_mb_send(status_mb, 0xFF);   /* 状态 ERROR */

/* 接收处理 */
rt_ubase_t status;
rt_mb_recv(status_mb, &status, RT_WAITING_FOREVER);
if (status == 0xFF)
    HandleError();
```

---

## 邮箱 vs 消息队列

| 对比项 | 邮箱 | 消息队列 |
|--------|------|---------|
| 每封消息大小 | 固定 4 字节 | 创建时指定，任意大小 |
| 数据传递方式 | 传指针（零拷贝） | 值拷贝（完整复制） |
| 适合数据大小 | 大数据（通过指针）| 小数据（< 64 字节直接拷贝）|
| 内存效率 | 高（只占指针大小）| 中（每条消息完整存储）|
| 使用复杂度 | 中（需管理指针生命周期）| 低（发完即可，无需考虑生命周期）|
| 中断中发送 | ✅ `rt_mb_send` | ✅ `rt_mq_send` |

**选择原则：**
- 传递结构体 < 64 字节 → **消息队列**（简单，无需管理内存）
- 传递大数据（图像帧、音频块）→ **邮箱 + 内存池**（零拷贝，高效）
- 传递整数状态值 → **邮箱**或**消息队列**都可以

---

## 常见错误

### 接收方忘记释放内存

```c
/* ❌ 危险：接收后没有 rt_free，内存泄漏 */
void process_entry(void *p)
{
    rt_ubase_t recv;
    while (1)
    {
        rt_mb_recv(mb, &recv, RT_WAITING_FOREVER);
        ImuData_t *data = (ImuData_t *)recv;
        ProcessData(data);
        /* 忘记 rt_free(data) → 每次循环泄漏 sizeof(ImuData_t) 字节 */
    }
}

/* ✅ 用完必须释放 */
rt_free(data);           /* 对应 rt_malloc */
rt_mp_free(data);        /* 对应 rt_mp_alloc */
```

### 发送失败时没有释放内存

```c
/* ❌ 邮箱满时 rt_mb_send 返回失败，但内存没有释放 */
ImuData_t *data = (ImuData_t *)rt_malloc(sizeof(ImuData_t));
FillData(data);
rt_mb_send(mb, (rt_ubase_t)data);  /* 如果失败，data 泄漏 */

/* ✅ 发送失败时释放 */
if (rt_mb_send(mb, (rt_ubase_t)data) != RT_EOK)
{
    rt_free(data);  /* 发送失败，释放内存 */
}
```

### 接收方使用后发送方又修改了数据

```c
/* ❌ 发送指针后，发送方继续修改原始数据，接收方读到的是修改后的值 */
static ImuData_t shared_data;  /* 静态变量，被所有调用共享 */

void bad_sender(void *p)
{
    while (1)
    {
        FillData(&shared_data);
        rt_mb_send(mb, (rt_ubase_t)&shared_data);  /* 发送静态变量地址 */
        /* 下一循环会修改 shared_data，接收方可能读到错误数据 */
    }
}

/* ✅ 每次发送使用独立的内存（动态分配或内存池） */
void good_sender(void *p)
{
    while (1)
    {
        ImuData_t *data = (ImuData_t *)rt_malloc(sizeof(ImuData_t));
        FillData(data);
        rt_mb_send(mb, (rt_ubase_t)data);  /* 每次都是独立内存 */
    }
}
```

---

## API 速查

| API | 说明 | 可用场景 |
|-----|------|---------|
| `rt_mb_create(name, size, flag)` | 动态创建邮箱 | 任何时候 |
| `rt_mb_init(&mb, name, buf, size, flag)` | 静态创建邮箱 | 任何时候 |
| `rt_mb_delete(mb)` | 删除动态邮箱 | 线程 |
| `rt_mb_detach(&mb)` | 脱离静态邮箱 | 线程 |
| `rt_mb_send(mb, value)` | 发送邮件（非阻塞，**可在中断中调用**）| 线程/中断 |
| `rt_mb_send_wait(mb, value, timeout)` | 发送邮件（阻塞版本）| 线程 |
| `rt_mb_recv(mb, &value, timeout)` | 接收邮件（可阻塞）| 线程 |

**内存池相关（配合邮箱使用）：**

| API | 说明 |
|-----|------|
| `rt_mp_create(name, block_count, block_size)` | 创建内存池 |
| `rt_mp_alloc(mp, timeout)` | 从内存池申请内存块 |
| `rt_mp_free(block)` | 归还内存块到内存池 |
| `rt_malloc(size)` | 动态分配内存（有碎片，备用） |
| `rt_free(ptr)` | 释放动态内存 |
