# RT-Thread 事件集（Event）

## 目录

- [核心概念](#核心概念)
- [工作原理](#工作原理)
- [创建与基本使用](#创建与基本使用)
- [等待模式：AND 与 OR](#等待模式and-与-or)
- [在中断中使用事件集](#在中断中使用事件集)
- [典型使用场景](#典型使用场景)
- [事件集 vs 信号量 vs 消息队列](#事件集-vs-信号量-vs-消息队列)
- [常见错误](#常见错误)
- [API 速查](#api-速查)

---

## 核心概念

事件集（Event Set）提供 **32 个独立的事件位（bit）**，每一位代表一个独立的事件。线程可以等待一个或多个事件位，支持两种等待逻辑：

- **OR（任意满足）**：等待的事件位中，有任意一个被置位就唤醒
- **AND（全部满足）**：等待的所有事件位都被置位才唤醒

事件集的核心优势：**一个线程可以同时等待多个独立事件的组合**，这是信号量和消息队列无法直接做到的。

| 特性 | 说明 |
|------|------|
| 事件位数量 | 32 位 |
| 支持等待 OR | ✅ 任意一个事件满足即唤醒 |
| 支持等待 AND | ✅ 全部事件同时满足才唤醒 |
| 携带数据 | ❌ 不携带数据，只有位状态 |
| 接收后清除 | 可选：自动清除或手动清除 |

---

## 工作原理

事件集内部维护一个 32 位整数，每一位对应一个事件：

```
事件集内部位图（32 bit）：
  bit31 ... bit3  bit2  bit1  bit0
    0   ...   0     1     0     1
                    ↑           ↑
              EVT_UART_RX  EVT_BUTTON

发送（置位）：rt_event_send(event, EVT_UART_RX)
              bit1 → 1

等待（OR）：rt_event_recv(event, EVT_BUTTON | EVT_UART_RX, OR ...)
            bit0=1 OR bit1=1 → 满足，唤醒

等待（AND）：rt_event_recv(event, EVT_BUTTON | EVT_UART_RX, AND ...)
             bit0=1 AND bit1=1 → 同时满足才唤醒
```

### 自动清除 vs 手动清除

接收时通过 `RT_EVENT_FLAG_CLEAR` 控制是否自动清除已接收的事件位：

```c
/* 自动清除（推荐）：接收后对应 bit 自动归 0 */
RT_EVENT_FLAG_OR  | RT_EVENT_FLAG_CLEAR
RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR

/* 不清除：事件位保持置位，其他线程也能看到 */
RT_EVENT_FLAG_OR
```

---

## 创建与基本使用

```c
#include <rtthread.h>

/* 定义事件位，用宏便于维护 */
#define EVT_SENSOR_DONE   (1 << 0)   /* bit0：传感器采集完成 */
#define EVT_UART_RECEIVED (1 << 1)   /* bit1：串口收到数据 */
#define EVT_BUTTON_PRESS  (1 << 2)   /* bit2：按键按下 */
#define EVT_TIMER_TICK    (1 << 3)   /* bit3：定时器到期 */

static rt_event_t my_event;

/* 创建事件集 */
my_event = rt_event_create(
    "evt1",             /* 名称 */
    RT_IPC_FLAG_PRIO    /* 等待规则：PRIO 高优先级先得 */
);

if (my_event == RT_NULL) {
    /* 内存不足 */
}

/* 发送事件（设置 bit，可在中断中调用） */
rt_event_send(my_event, EVT_SENSOR_DONE);
rt_event_send(my_event, EVT_UART_RECEIVED | EVT_BUTTON_PRESS);  /* 同时设多位 */

/* 接收事件（带清除标志） */
rt_uint32_t recv_set;
rt_event_recv(
    my_event,
    EVT_SENSOR_DONE | EVT_UART_RECEIVED,   /* 等待这些位 */
    RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, /* OR 逻辑，收到后清除 */
    RT_WAITING_FOREVER,                     /* 永久等待 */
    &recv_set                               /* 实际触发的位 */
);

/* 判断是哪个事件触发的 */
if (recv_set & EVT_SENSOR_DONE)    { HandleSensorData(); }
if (recv_set & EVT_UART_RECEIVED)  { HandleUartData();   }
```

---

## 等待模式：AND 与 OR

### OR 模式（任意一个满足）

```c
/* 等待"传感器完成"或"串口收到数据"，其中一个满足就唤醒 */
rt_uint32_t recv_set;
rt_event_recv(
    my_event,
    EVT_SENSOR_DONE | EVT_UART_RECEIVED,
    RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
    RT_WAITING_FOREVER,
    &recv_set
);

/* recv_set 中只有实际触发的位为 1 */
if (recv_set & EVT_SENSOR_DONE)   { /* 传感器数据来了 */ }
if (recv_set & EVT_UART_RECEIVED) { /* 串口数据来了 */ }
```

### AND 模式（全部同时满足）

```c
/* 等待"传感器"、"GPS"、"时钟同步"三个事件同时完成 */
#define EVT_SENSOR_READY (1 << 0)
#define EVT_GPS_READY    (1 << 1)
#define EVT_NTP_DONE     (1 << 2)
#define EVT_ALL_READY    (EVT_SENSOR_READY | EVT_GPS_READY | EVT_NTP_DONE)

rt_uint32_t recv_set;
rt_err_t ret = rt_event_recv(
    my_event,
    EVT_ALL_READY,
    RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR,  /* AND：三个全部置位才唤醒 */
    rt_tick_from_millisecond(5000),            /* 最多等 5 秒 */
    &recv_set
);

if (ret == RT_EOK)
    StartMainTask();    /* 三个条件都满足，开始主逻辑 */
else
    HandleInitTimeout();
```

### 实际应用对比

```c
/* 场景：处理线程等待"任意一个数据源有数据"就处理 */
/* 三个数据源：ADC、UART、CAN */

#define EVT_ADC_DATA  (1 << 0)
#define EVT_UART_DATA (1 << 1)
#define EVT_CAN_DATA  (1 << 2)

void process_entry(void *p)
{
    rt_uint32_t recv_set;
    while (1)
    {
        /* 等待任意一个数据源就绪 */
        rt_event_recv(evt, EVT_ADC_DATA | EVT_UART_DATA | EVT_CAN_DATA,
                      RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                      RT_WAITING_FOREVER, &recv_set);

        if (recv_set & EVT_ADC_DATA)  { ProcessADC();  }
        if (recv_set & EVT_UART_DATA) { ProcessUART(); }
        if (recv_set & EVT_CAN_DATA)  { ProcessCAN();  }
    }
}
```

---

## 在中断中使用事件集

`rt_event_send()` 可以直接在中断中调用，与信号量类似，无需特殊的 FromISR 版本。

```c
/* ✅ 在各种中断回调中直接发送事件 */

/* GPIO 外部中断：按键 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == KEY_Pin)
        rt_event_send(my_event, EVT_BUTTON_PRESS);
}

/* UART 接收完成 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    rt_event_send(my_event, EVT_UART_RECEIVED);
}

/* ADC 转换完成 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    rt_event_send(my_event, EVT_SENSOR_DONE);
}
```

**中断中只能 send，不能 recv：**

```c
/* ❌ 中断中不能等待事件（会阻塞中断，系统崩溃） */
void SomeIRQHandler(void)
{
    rt_event_recv(evt, EVT_X, ..., RT_WAITING_FOREVER, &set);  /* 不可以 */
}
```

---

## 典型使用场景

### 场景 1：多外设初始化完成同步（AND 模式）

系统启动时，多个外设并行初始化，主线程等待全部完成再继续。

```c
#define EVT_FLASH_INIT  (1 << 0)
#define EVT_LCD_INIT    (1 << 1)
#define EVT_SENSOR_INIT (1 << 2)
#define EVT_COMM_INIT   (1 << 3)

static rt_event_t init_event;

void flash_init_entry(void *p)
{
    Flash_Init();
    rt_event_send(init_event, EVT_FLASH_INIT);
}

void lcd_init_entry(void *p)
{
    LCD_Init();
    rt_event_send(init_event, EVT_LCD_INIT);
}

void sensor_init_entry(void *p)
{
    Sensor_Calibrate();
    rt_event_send(init_event, EVT_SENSOR_INIT);
}

void main_entry(void *p)
{
    init_event = rt_event_create("init", RT_IPC_FLAG_PRIO);

    /* 并行启动所有初始化线程 */
    rt_thread_startup(rt_thread_create("flash",  flash_init_entry,  RT_NULL, 512, 10, 20));
    rt_thread_startup(rt_thread_create("lcd",    lcd_init_entry,    RT_NULL, 512, 10, 20));
    rt_thread_startup(rt_thread_create("sensor", sensor_init_entry, RT_NULL, 512, 10, 20));

    /* 等待所有初始化完成（AND 模式） */
    rt_uint32_t recv_set;
    rt_event_recv(init_event,
        EVT_FLASH_INIT | EVT_LCD_INIT | EVT_SENSOR_INIT,
        RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR,
        rt_tick_from_millisecond(10000),  /* 最多等 10 秒 */
        &recv_set);

    rt_kprintf("All subsystems ready, starting main loop\n");
    StartMainApplication();
}
```

### 场景 2：状态机驱动（OR 模式 + 多事件源）

```c
#define EVT_BTN_SHORT (1 << 0)   /* 短按 */
#define EVT_BTN_LONG  (1 << 1)   /* 长按 */
#define EVT_TIMEOUT   (1 << 2)   /* 超时 */
#define EVT_ERROR     (1 << 3)   /* 错误 */

typedef enum { STATE_IDLE, STATE_WORKING, STATE_ERROR } AppState_t;

void state_machine_entry(void *p)
{
    AppState_t state = STATE_IDLE;
    rt_uint32_t recv_set;

    while (1)
    {
        rt_event_recv(ui_event,
            EVT_BTN_SHORT | EVT_BTN_LONG | EVT_TIMEOUT | EVT_ERROR,
            RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
            RT_WAITING_FOREVER, &recv_set);

        switch (state)
        {
            case STATE_IDLE:
                if (recv_set & EVT_BTN_SHORT) { StartWork(); state = STATE_WORKING; }
                break;

            case STATE_WORKING:
                if (recv_set & EVT_BTN_LONG)  { StopWork();   state = STATE_IDLE;    }
                if (recv_set & EVT_TIMEOUT)   { SaveResult(); state = STATE_IDLE;    }
                if (recv_set & EVT_ERROR)     { HandleError(); state = STATE_ERROR;  }
                break;

            case STATE_ERROR:
                if (recv_set & EVT_BTN_SHORT) { Reset(); state = STATE_IDLE; }
                break;
        }
    }
}
```

### 场景 3：生产者-消费者多路合并

多个数据生产线程共用一个事件集，消费线程任意一路有数据就处理。

```c
#define EVT_TEMP_DATA  (1 << 0)
#define EVT_PRESS_DATA (1 << 1)
#define EVT_HUMI_DATA  (1 << 2)

static rt_event_t data_event;
static float g_temp, g_pressure, g_humidity;  /* 各传感器最新值 */
static rt_mutex_t data_mutex;

/* 温度传感器线程 */
void temp_entry(void *p)
{
    while (1)
    {
        float t = ReadTemp();
        rt_mutex_take(data_mutex, RT_WAITING_FOREVER);
        g_temp = t;
        rt_mutex_release(data_mutex);
        rt_event_send(data_event, EVT_TEMP_DATA);   /* 通知有新数据 */
        rt_thread_mdelay(1000);
    }
}

/* 数据处理线程 */
void process_entry(void *p)
{
    rt_uint32_t recv_set;
    while (1)
    {
        rt_event_recv(data_event,
            EVT_TEMP_DATA | EVT_PRESS_DATA | EVT_HUMI_DATA,
            RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
            RT_WAITING_FOREVER, &recv_set);

        rt_mutex_take(data_mutex, RT_WAITING_FOREVER);
        if (recv_set & EVT_TEMP_DATA)  rt_kprintf("T=%.1f ", g_temp);
        if (recv_set & EVT_PRESS_DATA) rt_kprintf("P=%.1f ", g_pressure);
        if (recv_set & EVT_HUMI_DATA)  rt_kprintf("H=%.1f ", g_humidity);
        rt_mutex_release(data_mutex);
        rt_kprintf("\n");
    }
}
```

### 场景 4：通信协议超时检测

```c
#define EVT_ACK_RECEIVED (1 << 0)
#define EVT_NAK_RECEIVED (1 << 1)

static rt_event_t ack_event;

/* 发送数据帧并等待应答 */
rt_err_t send_frame_and_wait_ack(const rt_uint8_t *frame, rt_size_t len)
{
    /* 清除旧事件位 */
    rt_event_recv(ack_event, EVT_ACK_RECEIVED | EVT_NAK_RECEIVED,
                  RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, RT_WAITING_NO, RT_NULL);

    /* 发送数据帧 */
    HAL_UART_Transmit(&huart1, frame, len, HAL_MAX_DELAY);

    /* 等待 ACK 或 NAK，超时 500ms */
    rt_uint32_t recv_set;
    rt_err_t ret = rt_event_recv(ack_event,
        EVT_ACK_RECEIVED | EVT_NAK_RECEIVED,
        RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
        rt_tick_from_millisecond(500),
        &recv_set);

    if (ret != RT_EOK)    return -RT_ETIMEOUT;
    if (recv_set & EVT_NAK_RECEIVED) return -RT_ERROR;
    return RT_EOK;
}

/* UART 接收线程：解析应答帧，发送对应事件 */
void uart_rx_entry(void *p)
{
    /* ... 解析 UART 数据帧 ... */
    if (frame_type == FRAME_ACK)
        rt_event_send(ack_event, EVT_ACK_RECEIVED);
    else if (frame_type == FRAME_NAK)
        rt_event_send(ack_event, EVT_NAK_RECEIVED);
}
```

---

## 事件集 vs 信号量 vs 消息队列

| 对比项 | 事件集 | 信号量 | 消息队列 |
|--------|--------|--------|---------|
| 携带数据 | ❌ 只有位状态 | ❌ 只有计数值 | ✅ 可携带任意数据 |
| 等待多个条件 | ✅ AND/OR 等待多位 | ❌ 一次只等一个 | ❌ 需要多个队列+轮询 |
| 事件不累积 | 位状态（发多次等同发一次）| 计数累积 | 消息累积（不丢失）|
| 中断中发送 | ✅ | ✅ | ✅ |
| 适合场景 | 多事件组合同步 | 简单同步/计数资源 | 传递数据+同步 |

**选择建议：**
- 等待多个条件同时发生（AND）→ **事件集**
- 等待多个事件中的任意一个（OR）→ **事件集**
- 简单的一对一同步通知 → **信号量**（更轻量）
- 同步的同时传递数据 → **消息队列**

---

## 常见错误

### 事件位重复使用导致逻辑混乱

```c
/* ❌ 两个不相关的子系统用同一个事件集的相同 bit */
#define EVT_MOTOR_DONE (1 << 0)  /* 电机控制模块用 */
#define EVT_BTN_PRESS  (1 << 0)  /* UI 模块也用 bit0，冲突！ */

/* ✅ 每个事件占用唯一的 bit */
#define EVT_MOTOR_DONE (1 << 0)
#define EVT_BTN_PRESS  (1 << 1)
/* 一个事件集最多 32 个独立事件，不够时创建多个事件集 */
```

### 不加 CLEAR 导致事件反复触发

```c
/* ❌ 问题：不清除 bit，下次 recv 仍然立即返回 */
rt_event_recv(evt, EVT_BUTTON, RT_EVENT_FLAG_OR, RT_WAITING_FOREVER, &set);
/* bit 没有清除，下次调用立刻返回，相当于死循环 */

/* ✅ 加上 RT_EVENT_FLAG_CLEAR */
rt_event_recv(evt, EVT_BUTTON, RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
              RT_WAITING_FOREVER, &set);
```

### 事件不累积（与信号量不同）

```c
/* ⚠️ 注意：事件集中，连续 send 同一个 bit，只有一次效果 */
rt_event_send(evt, EVT_DATA);  /* bit0 → 1 */
rt_event_send(evt, EVT_DATA);  /* bit0 已经是 1，没有变化 */
rt_event_recv(evt, EVT_DATA, FLAG_OR | FLAG_CLEAR, ...);  /* 收到一次 */
/* 只会唤醒一次，第二次 send 被"淹没" */

/* 如果需要累积计数（来了 3 次要处理 3 次），用信号量而不是事件集 */
```

---

## API 速查

| API | 说明 | 可用场景 |
|-----|------|---------|
| `rt_event_create(name, flag)` | 动态创建事件集 | 任何时候 |
| `rt_event_init(&evt, name, flag)` | 静态创建事件集 | 任何时候 |
| `rt_event_delete(event)` | 删除动态事件集 | 线程 |
| `rt_event_detach(&evt)` | 脱离静态事件集 | 线程 |
| `rt_event_send(event, set)` | 发送事件（置位，**可在中断中调用**）| 线程/中断 |
| `rt_event_recv(event, set, opt, timeout, &recved)` | 接收事件（可阻塞）| 线程 |

**`opt` 参数组合：**

| 组合 | 含义 |
|------|------|
| `RT_EVENT_FLAG_OR` | 任意位满足即唤醒，不清除 |
| `RT_EVENT_FLAG_OR \| RT_EVENT_FLAG_CLEAR` | 任意位满足即唤醒，**自动清除已接收的位** |
| `RT_EVENT_FLAG_AND` | 所有位同时满足才唤醒，不清除 |
| `RT_EVENT_FLAG_AND \| RT_EVENT_FLAG_CLEAR` | 所有位同时满足才唤醒，**自动清除** |
