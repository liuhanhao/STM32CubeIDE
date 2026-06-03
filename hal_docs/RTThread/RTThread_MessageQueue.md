# RT-Thread 消息队列（Message Queue）

## 目录

- [核心概念](#核心概念)
- [工作原理](#工作原理)
- [创建消息队列](#创建消息队列)
- [发送消息](#发送消息)
- [接收消息](#接收消息)
- [在中断中使用消息队列](#在中断中使用消息队列)
- [传递复杂数据](#传递复杂数据)
- [典型使用场景](#典型使用场景)
- [消息队列 vs 邮箱 vs 信号量](#消息队列-vs-邮箱-vs-信号量)
- [常见错误](#常见错误)
- [API 速查](#api-速查)

---

## 核心概念

消息队列是 RT-Thread 中最常用的**线程间通信**机制，在传递同步信号的同时也可以携带数据。

- **FIFO 顺序**：先发送的消息先被接收
- **固定大小**：每条消息大小在创建时固定，收发双方必须一致
- **深度缓冲**：队列可以缓存多条消息，生产者和消费者速率不必完全一致
- **阻塞机制**：队列满时发送可阻塞等待，队列空时接收可阻塞等待

**消息队列 vs 全局变量共享数据：**

```
全局变量：需要手动加锁，代码复杂，容易出错
消息队列：内部已处理线程安全，发送接收即可，架构更清晰
```

---

## 工作原理

### 数据存储方式

RT-Thread 消息队列采用**按值复制**的方式存储消息，每次 `rt_mq_send` 都会将数据完整复制进队列缓冲区。

```
发送方变量  ──复制──→  [消息队列缓冲区]  ──复制──→  接收方变量
   msg                   内部存储              recv_msg
```

- 发送后修改原始变量不影响队列中的数据
- 如果消息体较大（如图像帧），应改用**邮箱**传递指针，避免大量内存复制

### 阻塞机制

```
队列已满时发送（timeout > 0）：
  发送线程 → 进入挂起 → 等待队列出现空位 → 超时 or 有位置后唤醒

队列为空时接收（timeout > 0）：
  接收线程 → 进入挂起 → 等待有消息 → 超时 or 有消息后唤醒
```

被阻塞的线程不占 CPU，调度器条件满足时自动唤醒。

---

## 创建消息队列

```c
rt_mq_t mq;

/* 创建消息队列
 * 参数1：名称
 * 参数2：每条消息的大小（字节），必须与实际数据类型一致
 * 参数3：队列深度（最多缓存几条消息）
 * 参数4：等待规则
 */
mq = rt_mq_create("mq1", sizeof(rt_uint32_t), 10, RT_IPC_FLAG_FIFO);

if (mq == RT_NULL) {
    /* 内存不足，创建失败，需增大 rtconfig.h 中的 RT_HEAP_SIZE */
}
```

**内存占用估算：**

```
内存 ≈ 控制块（约 80 字节）+ 深度 × （消息大小 + 4字节链表开销）
示例：深度 10，uint32_t（4字节） → 80 + 10 × 8 = 160 字节
示例：深度 5，结构体 20 字节   → 80 + 5 × 24  = 200 字节
```

**深度如何选：**

```
深度 ≥ 生产者在消费者最慢时的最大突发量 × 1.5（留余量）
示例：中断每 1ms 来一个数据，处理线程最慢每 5ms 处理一次
      最大积累 = 5 条，建议深度设 10
```

---

## 发送消息

```c
rt_uint32_t data = 1234;

/* 发送到队尾（非阻塞，队列满立即返回 -RT_EFULL） */
rt_err_t ret = rt_mq_send(mq, &data, sizeof(data));
if (ret != RT_EOK) {
    /* 队列满，消息被丢弃 */
}

/* 发送，队列满时阻塞等待（阻塞版本） */
rt_mq_send_wait(mq, &data, sizeof(data), RT_WAITING_FOREVER);

/* 发送，最多等待 100ms */
rt_mq_send_wait(mq, &data, sizeof(data), rt_tick_from_millisecond(100));

/* 紧急消息，发送到队头（优先被接收） */
rt_mq_urgent(mq, &data, sizeof(data));
```

---

## 接收消息

```c
rt_uint32_t received;

/* 阻塞接收，永久等待直到有消息 */
if (rt_mq_recv(mq, &received, sizeof(received), RT_WAITING_FOREVER) == RT_EOK)
{
    /* 成功收到消息 */
    ProcessData(received);
}

/* 等待最多 200ms */
rt_err_t ret = rt_mq_recv(mq, &received, sizeof(received),
                           rt_tick_from_millisecond(200));
if (ret == RT_EOK) {
    /* 有消息 */
} else {
    /* 超时（-RT_ETIMEOUT）或其他错误 */
}

/* 非阻塞接收（立即返回） */
rt_mq_recv(mq, &received, sizeof(received), RT_WAITING_NO);
```

---

## 在中断中使用消息队列

RT-Thread 的 `rt_mq_send` 是非阻塞版本，可以在中断中直接调用。

```c
/* ✅ 中断中直接使用 rt_mq_send（非阻塞，队列满时返回错误） */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    rt_uint8_t byte = rx_byte;

    /* rt_mq_send 在中断中可以直接调用 */
    rt_mq_send(uart_rx_mq, &byte, sizeof(byte));

    /* 重新开启接收 */
    HAL_UART_Receive_IT(huart, &rx_byte, 1);
}

/* 对应的处理线程 */
void uart_proc_entry(void *p)
{
    rt_uint8_t byte;
    while (1)
    {
        rt_mq_recv(uart_rx_mq, &byte, sizeof(byte), RT_WAITING_FOREVER);
        ProcessByte(byte);
    }
}
```

> **与 FreeRTOS 对比：** FreeRTOS 在中断中发送队列必须用 `xQueueSendFromISR` 并处理 `xHigherPriorityTaskWoken`，RT-Thread 直接调用 `rt_mq_send` 即可，更简洁。

**中断中不能用阻塞版本：**

```c
/* ❌ 错误：中断中不能使用阻塞版本 */
void SomeIRQHandler(void)
{
    rt_mq_send_wait(mq, &data, size, RT_WAITING_FOREVER);  /* 会死机 */
}

/* ✅ 中断中只用非阻塞的 rt_mq_send */
void SomeIRQHandler(void)
{
    rt_mq_send(mq, &data, size);  /* 队列满就丢，不阻塞 */
}
```

---

## 传递复杂数据

### 传递结构体（完整数据拷贝）

适合消息体较小（通常 < 64 字节）的场景，最安全，无需担心内存生命周期。

```c
typedef struct {
    rt_uint8_t  sensor_id;
    float       temperature;
    float       humidity;
    rt_uint32_t timestamp;
} SensorData_t;

rt_mq_t sensor_mq = rt_mq_create("sensor", sizeof(SensorData_t), 8, RT_IPC_FLAG_FIFO);

/* 发送 */
SensorData_t tx = {
    .sensor_id   = 1,
    .temperature = 25.3f,
    .humidity    = 60.0f,
    .timestamp   = rt_tick_get()
};
rt_mq_send(sensor_mq, &tx, sizeof(tx));

/* 接收 */
SensorData_t rx;
rt_mq_recv(sensor_mq, &rx, sizeof(rx), RT_WAITING_FOREVER);
rt_kprintf("T=%.1f H=%.1f\n", rx.temperature, rx.humidity);
```

### 传递命令（枚举 + 联合体）

一个队列传递多种不同类型的命令，适合命令路由场景。

```c
typedef enum {
    CMD_LED_ON,
    CMD_LED_OFF,
    CMD_MOTOR_SPEED,
    CMD_BUZZER_BEEP,
} CmdType_t;

typedef struct {
    CmdType_t type;
    union {
        rt_uint8_t  led_id;
        rt_uint16_t motor_rpm;
        rt_uint32_t beep_ms;
    } param;
} Command_t;

rt_mq_t cmd_mq = rt_mq_create("cmd", sizeof(Command_t), 10, RT_IPC_FLAG_FIFO);

/* 发送命令 */
Command_t cmd = {.type = CMD_MOTOR_SPEED, .param.motor_rpm = 1500};
rt_mq_send(cmd_mq, &cmd, sizeof(cmd));

/* 接收并分派 */
void control_entry(void *p)
{
    Command_t recv;
    while (1)
    {
        rt_mq_recv(cmd_mq, &recv, sizeof(recv), RT_WAITING_FOREVER);
        switch (recv.type)
        {
            case CMD_LED_ON:      LED_On(recv.param.led_id);          break;
            case CMD_MOTOR_SPEED: Motor_SetRPM(recv.param.motor_rpm); break;
            case CMD_BUZZER_BEEP: Buzzer_Beep(recv.param.beep_ms);    break;
            default: break;
        }
    }
}
```

---

## 典型使用场景

### 场景 1：ADC 采集 → 数据处理流水线

```c
static rt_mq_t raw_mq;
static rt_mq_t result_mq;

void acquire_entry(void *p)
{
    while (1)
    {
        rt_uint16_t raw = HAL_ADC_GetValue(&hadc1);
        rt_mq_send(raw_mq, &raw, sizeof(raw));
        rt_thread_mdelay(10);   /* 100Hz 采样 */
    }
}

void filter_entry(void *p)
{
    rt_uint16_t raw;
    while (1)
    {
        rt_mq_recv(raw_mq, &raw, sizeof(raw), RT_WAITING_FOREVER);

        /* 低通滤波 */
        static float filtered = 0;
        filtered = filtered * 0.9f + raw * 0.1f;

        float voltage = filtered * 3.3f / 4096.0f;
        rt_mq_send(result_mq, &voltage, sizeof(voltage));
    }
}

void display_entry(void *p)
{
    float voltage;
    while (1)
    {
        rt_mq_recv(result_mq, &voltage, sizeof(voltage), RT_WAITING_FOREVER);
        rt_kprintf("voltage = %.3f V\n", voltage);
    }
}

int main(void)
{
    raw_mq    = rt_mq_create("raw",    sizeof(rt_uint16_t), 20, RT_IPC_FLAG_FIFO);
    result_mq = rt_mq_create("result", sizeof(float),       20, RT_IPC_FLAG_FIFO);

    rt_thread_startup(rt_thread_create("acq",  acquire_entry, RT_NULL, 512, 5,  20));
    rt_thread_startup(rt_thread_create("flt",  filter_entry,  RT_NULL, 512, 8,  20));
    rt_thread_startup(rt_thread_create("disp", display_entry, RT_NULL, 512, 12, 20));
    return 0;
}
```

### 场景 2：UART 接收数据帧解析

```c
static rt_mq_t uart_byte_mq;
uart_byte_mq = rt_mq_create("ubyte", sizeof(rt_uint8_t), 64, RT_IPC_FLAG_FIFO);

/* 中断中接收字节 */
static rt_uint8_t rx_byte;
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    rt_mq_send(uart_byte_mq, &rx_byte, 1);
    HAL_UART_Receive_IT(huart, &rx_byte, 1);
}

/* 解析线程：组装完整数据帧再处理 */
void uart_parse_entry(void *p)
{
    rt_uint8_t byte;
    rt_uint8_t frame[64];
    rt_uint8_t idx = 0;

    while (1)
    {
        rt_mq_recv(uart_byte_mq, &byte, 1, RT_WAITING_FOREVER);

        if (byte == 0xAA && idx == 0)      /* 帧头 */
            frame[idx++] = byte;
        else if (idx > 0)
        {
            frame[idx++] = byte;
            if (byte == 0x55 || idx >= sizeof(frame))  /* 帧尾 */
            {
                ProcessFrame(frame, idx);
                idx = 0;
            }
        }
    }
}
```

### 场景 3：多传感器数据汇总到一个处理线程

```c
/* 三个传感器线程共用一个消息队列，处理线程统一处理 */
typedef struct {
    rt_uint8_t source;   /* 0=温度 1=湿度 2=压力 */
    float      value;
} SensorMsg_t;

rt_mq_t all_sensor_mq = rt_mq_create("all", sizeof(SensorMsg_t), 15, RT_IPC_FLAG_FIFO);

void temp_entry(void *p)
{
    while (1)
    {
        SensorMsg_t m = {.source = 0, .value = ReadTemp()};
        rt_mq_send(all_sensor_mq, &m, sizeof(m));
        rt_thread_mdelay(1000);
    }
}

void humi_entry(void *p)
{
    while (1)
    {
        SensorMsg_t m = {.source = 1, .value = ReadHumi()};
        rt_mq_send(all_sensor_mq, &m, sizeof(m));
        rt_thread_mdelay(2000);
    }
}

void process_entry(void *p)
{
    SensorMsg_t m;
    while (1)
    {
        rt_mq_recv(all_sensor_mq, &m, sizeof(m), RT_WAITING_FOREVER);
        rt_kprintf("source=%d value=%.2f\n", m.source, m.value);
    }
}
```

---

## 消息队列 vs 邮箱 vs 信号量

| 对比项 | 消息队列 | 邮箱 | 信号量 |
|--------|---------|------|--------|
| 携带数据 | ✅ 任意大小 | ✅ 仅 4 字节（指针）| ❌ 不携带 |
| 队列深度 | 多条 | 多个（每封 4 字节）| 计数值 |
| 适合场景 | 传递结构体、命令 | 传递大数据指针（零拷贝）| 纯同步通知 |
| 中断中发送 | ✅ `rt_mq_send` | ✅ `rt_mb_send` | ✅ `rt_sem_release` |
| 内存开销 | 中（消息体完整拷贝）| 低（只拷贝 4 字节）| 低 |

**选择建议：**
- 消息 < 64 字节 → **消息队列**（结构体完整传递，最安全）
- 消息较大（图像、音频帧）→ **邮箱**（传递指针，零拷贝）
- 只需通知，不需要数据 → **信号量**

---

## 常见错误

### 消息大小不匹配

```c
/* ❌ 危险：创建时 4 字节，发送时用 8 字节 */
mq = rt_mq_create("mq", 4, 10, RT_IPC_FLAG_FIFO);

rt_uint64_t big_data = 0x1234;
rt_mq_send(mq, &big_data, sizeof(big_data));  /* 发 8 字节，超出消息大小，内存越界 */

/* ✅ 发送大小必须 <= 创建时指定的消息大小 */
mq = rt_mq_create("mq", sizeof(rt_uint64_t), 10, RT_IPC_FLAG_FIFO);
```

### 队列深度不足导致消息丢失

```c
/* ❌ 问题：中断每 1ms 来一条，处理线程每 10ms 处理一次，深度1不够 */
mq = rt_mq_create("mq", sizeof(rt_uint16_t), 1, RT_IPC_FLAG_FIFO);

/* ✅ 深度应覆盖最大突发量 */
mq = rt_mq_create("mq", sizeof(rt_uint16_t), 20, RT_IPC_FLAG_FIFO);
```

### 传递指向栈上数据的指针

```c
/* ❌ 危险：传递指针时，指针指向的内存必须在接收方使用时仍然有效 */
void bad_sender(void)
{
    rt_uint8_t local_buf[64];  /* 局部变量，在函数栈上 */
    fill_data(local_buf);
    rt_mq_send(mq, &local_buf, sizeof(local_buf));  
    /* 如果队列里存的是 local_buf 的指针，函数返回后 local_buf 失效 */
}

/* ✅ 如果消息队列按值复制，没有问题（RT-Thread 的消息队列就是按值复制） */
/* 只有邮箱才传递指针，需要注意生命周期 */
```

---

## API 速查

| API | 说明 | 可用场景 |
|-----|------|---------|
| `rt_mq_create(name, msg_size, max_msgs, flag)` | 动态创建消息队列 | 任何时候 |
| `rt_mq_init(&mq, name, buf, msg_size, buf_size, flag)` | 静态创建 | 任何时候 |
| `rt_mq_delete(mq)` | 删除动态消息队列 | 线程 |
| `rt_mq_detach(&mq)` | 脱离静态消息队列 | 线程 |
| `rt_mq_send(mq, buf, size)` | 发送（非阻塞，**可在中断中调用**） | 线程/中断 |
| `rt_mq_send_wait(mq, buf, size, timeout)` | 发送（阻塞版本） | 线程 |
| `rt_mq_urgent(mq, buf, size)` | 发送到队头（紧急消息） | 线程/中断 |
| `rt_mq_recv(mq, buf, size, timeout)` | 接收消息（可阻塞） | 线程 |
