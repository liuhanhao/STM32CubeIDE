# RT-Thread 软件定时器（Timer）

## 目录

- [核心概念](#核心概念)
- [工作原理](#工作原理)
- [创建与使用](#创建与使用)
- [单次定时器 vs 周期定时器](#单次定时器-vs-周期定时器)
- [控制定时器](#控制定时器)
- [定时器回调的限制](#定时器回调的限制)
- [软件定时器 vs 硬件定时器](#软件定时器-vs-硬件定时器)
- [典型使用场景](#典型使用场景)
- [常见错误](#常见错误)
- [API 速查](#api-速查)

---

## 核心概念

软件定时器是 RT-Thread 提供的基于系统时钟节拍（tick）的定时机制。定时器超时后，系统自动调用用户注册的**回调函数**。

| 特性 | 说明 |
|------|------|
| 精度 | ±1 tick（默认 1ms） |
| 数量 | 几乎无限制，受堆内存约束 |
| 回调线程 | 在定时器线程（timer thread）中执行 |
| 类型 | 单次（One Shot）和周期（Periodic） |

**与硬件定时器对比：**

| 对比项 | 软件定时器 | 硬件定时器（TIM） |
|--------|-----------|-----------------|
| 精度 | ±1ms | 微秒甚至纳秒级 |
| 数量 | 几乎不限 | F103 最多 7 个 |
| 适合场景 | 超时检测、周期 UI 刷新、低频控制 | 精确 PWM、输入捕获、高频中断 |
| 回调延迟 | 受定时器线程优先级影响（可能 > 1ms）| 立即（硬件中断，< 1μs） |

---

## 工作原理

RT-Thread 的软件定时器有两种运行模式，由 `rtconfig.h` 中的 `RT_USING_TIMER_SOFT` 控制：

### 硬定时器（默认，SOFT 未开启）

回调在 SysTick 中断上下文中直接执行：

```
SysTick 中断（每 1ms）
    └── rt_tick_increase()
            └── 检查所有定时器是否超时
                    └── 直接调用超时的定时器回调函数（中断上下文）
```

> 此模式回调在中断中执行，**不能调用任何可能引起线程挂起的 API**（如 `rt_thread_mdelay`、带超时的 `rt_sem_take`）。

### 软定时器（开启 RT_USING_TIMER_SOFT）

```c
/* rtconfig.h 中开启 */
#define RT_USING_TIMER_SOFT
#define RT_TIMER_THREAD_PRIO     4    /* 定时器线程优先级 */
#define RT_TIMER_THREAD_STACK_SIZE 512
```

回调在专用定时器线程中执行：

```
SysTick 中断（每 1ms）
    └── rt_tick_increase()
            └── 检查定时器超时，发送消息到定时器线程

定时器线程（单独线程）
    └── 接收消息
            └── 调用超时的定时器回调函数（线程上下文）
```

> 软定时器模式下回调在线程上下文中执行，但依然**不建议在回调中长时间阻塞**，会阻塞其他定时器的回调。

---

## 创建与使用

```c
rt_timer_t timer;

/* 定时器回调函数 */
void timer_callback(void *parameter)
{
    /* 参数是创建时传入的 parameter */
    rt_uint32_t id = (rt_uint32_t)parameter;
    rt_kprintf("Timer %d expired\n", id);

    /* 快速处理，不要长时间阻塞 */
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
}

/* 创建周期定时器，1000ms 触发一次 */
timer = rt_timer_create(
    "timer1",               /* 名称 */
    timer_callback,         /* 回调函数 */
    (void *)1,              /* 传给回调的参数（parameter） */
    rt_tick_from_millisecond(1000),  /* 超时时间 1000ms */
    RT_TIMER_FLAG_PERIODIC  /* 周期定时器 */
);

if (timer != RT_NULL)
    rt_timer_start(timer);  /* 启动定时器 */
```

---

## 单次定时器 vs 周期定时器

### 单次定时器

触发一次后自动停止，不再重复。

```c
/* 创建单次定时器：3 秒后执行一次 */
rt_timer_t one_shot = rt_timer_create(
    "once",
    one_shot_callback,
    RT_NULL,
    rt_tick_from_millisecond(3000),
    RT_TIMER_FLAG_ONE_SHOT   /* 单次 */
);
rt_timer_start(one_shot);

/* 典型用法：延时执行某个操作 */
void one_shot_callback(void *p)
{
    /* 3 秒后关闭背光 */
    LCD_BacklightOff();
    /* 单次定时器超时后自动停止，不需要手动 stop */
}
```

### 周期定时器

每隔固定时间自动触发。

```c
/* 创建周期定时器：每 500ms 闪烁一次 LED */
rt_timer_t led_timer = rt_timer_create(
    "led_tmr",
    led_blink_callback,
    RT_NULL,
    rt_tick_from_millisecond(500),
    RT_TIMER_FLAG_PERIODIC   /* 周期 */
);
rt_timer_start(led_timer);

void led_blink_callback(void *p)
{
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
}
```

---

## 控制定时器

```c
/* 停止定时器（可以之后再 start） */
rt_timer_stop(timer);

/* 重新启动（reset：从头开始计时） */
rt_timer_start(timer);   /* stop 后再 start，重置超时时间 */

/* 修改定时周期（需先 stop，修改后再 start） */
rt_timer_stop(timer);
rt_timer_control(timer, RT_TIMER_CTRL_SET_TIME,
                 (void *)rt_tick_from_millisecond(2000));  /* 改成 2 秒 */
rt_timer_start(timer);

/* 获取定时器剩余时间 */
rt_tick_t remain_tick;
rt_timer_control(timer, RT_TIMER_CTRL_GET_TIME, &remain_tick);
rt_kprintf("remain: %d ms\n", remain_tick * 1000 / RT_TICK_PER_SECOND);

/* 删除定时器（释放内存） */
rt_timer_delete(timer);
timer = RT_NULL;
```

---

## 定时器回调的限制

回调函数执行时处于**定时器线程**（或中断）上下文，有严格限制：

```c
/* ❌ 不要在回调中做以下事情 */
void bad_callback(void *p)
{
    rt_thread_mdelay(100);                           /* 不能延时 */
    rt_sem_take(sem, RT_WAITING_FOREVER);            /* 不能永久等待 */
    rt_mutex_take(mutex, RT_WAITING_FOREVER);        /* 不能永久等待互斥量 */
    HAL_UART_Transmit(&h, buf, len, HAL_MAX_DELAY);  /* 不能无限等待 HAL */
}

/* ✅ 回调应该快速完成，复杂逻辑通过 IPC 交给线程处理 */
void good_callback(void *p)
{
    /* 方式1：直接操作硬件（快速） */
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);

    /* 方式2：设置标志位，线程轮询处理 */
    g_timer_flag = 1;

    /* 方式3：通过非阻塞 IPC 通知线程 */
    rt_sem_release(process_sem);           /* 信号量：可以 */
    rt_mq_send(mq, &tick, sizeof(tick));   /* 消息队列（非阻塞版）：可以 */
    rt_event_send(evt, EVT_TIMER_TICK);    /* 事件集：可以 */
    rt_mb_send(mb, tick);                  /* 邮箱（非阻塞版）：可以 */
}
```

---

## 软件定时器 vs 硬件定时器

### 什么时候用软件定时器

- 周期 > 10ms 的低频定时（LED 闪烁、超时检测、UI 刷新）
- 需要多个独立定时器（硬件定时器不够用）
- 不需要精确到微秒

### 什么时候必须用硬件定时器

- PWM 输出（频率精度要求高）
- 输入捕获（测量脉冲宽度）
- 周期 < 1ms 的高频任务
- 需要在硬件层面响应，不能容忍线程调度延迟

### 替代方案：线程 + `rt_thread_delay_until`

对于周期性任务，直接用线程的固定周期延时也是很好的选择，回调中不能阻塞的限制就不存在了：

```c
/* 软件定时器方案：受回调限制 */
void periodic_callback(void *p)
{
    DoQuickWork();  /* 只能做快速操作 */
}

/* 线程方案：更灵活，可以阻塞等待 */
void periodic_entry(void *p)
{
    rt_tick_t last_tick = rt_tick_get();
    while (1)
    {
        rt_thread_delay_until(&last_tick, rt_tick_from_millisecond(100));
        DoAnything();  /* 可以做任意操作，包括阻塞等待 */
    }
}
```

---

## 典型使用场景

### 场景 1：通信超时检测

```c
static rt_timer_t ack_timer;
static rt_uint8_t ack_timeout_flag = 0;

void ack_timeout_callback(void *p)
{
    ack_timeout_flag = 1;
    rt_kprintf("ACK timeout!\n");
}

/* 发送数据时启动超时定时器（单次） */
void send_with_timeout(const rt_uint8_t *data, rt_size_t len)
{
    ack_timeout_flag = 0;
    HAL_UART_Transmit(&huart1, data, len, HAL_MAX_DELAY);

    /* 启动 500ms 超时单次定时器 */
    rt_timer_start(ack_timer);

    /* 等待 ACK 或超时 */
    while (!ack_received && !ack_timeout_flag)
        rt_thread_mdelay(1);

    rt_timer_stop(ack_timer);  /* 收到 ACK 后停止定时器 */

    if (ack_timeout_flag)
        Retransmit();
}

/* 初始化 */
ack_timer = rt_timer_create("ack", ack_timeout_callback, RT_NULL,
                             rt_tick_from_millisecond(500), RT_TIMER_FLAG_ONE_SHOT);
```

### 场景 2：LCD 背光自动关闭

```c
static rt_timer_t backlight_timer;

void backlight_off_callback(void *p)
{
    LCD_BacklightOff();
    rt_kprintf("Backlight off\n");
}

/* 任何用户操作时，重置 30 秒倒计时 */
void reset_backlight_timer(void)
{
    LCD_BacklightOn();

    /* 如果定时器还在运行，先 stop 再 start（重置超时时间） */
    rt_timer_stop(backlight_timer);
    rt_timer_start(backlight_timer);
}

/* 初始化：30 秒无操作后关背光 */
backlight_timer = rt_timer_create("bl", backlight_off_callback, RT_NULL,
                                  rt_tick_from_millisecond(30000),
                                  RT_TIMER_FLAG_ONE_SHOT);
rt_timer_start(backlight_timer);
```

### 场景 3：多路独立周期任务（替代多个线程）

```c
/* 用多个软件定时器替代多个低优先级后台线程，节省栈内存 */

static rt_uint8_t heartbeat_flag = 0;
static rt_uint8_t status_flag    = 0;
static rt_uint8_t watchdog_flag  = 0;

void heartbeat_cb(void *p) { heartbeat_flag = 1; }
void status_cb(void *p)    { status_flag    = 1; }
void watchdog_cb(void *p)  { watchdog_flag  = 1; }

/* 一个线程轮询所有标志位处理，比多个线程省内存 */
void background_entry(void *p)
{
    rt_timer_start(rt_timer_create("hb",  heartbeat_cb, RT_NULL, 500,  RT_TIMER_FLAG_PERIODIC));
    rt_timer_start(rt_timer_create("st",  status_cb,    RT_NULL, 1000, RT_TIMER_FLAG_PERIODIC));
    rt_timer_start(rt_timer_create("wdg", watchdog_cb,  RT_NULL, 2000, RT_TIMER_FLAG_PERIODIC));

    while (1)
    {
        if (heartbeat_flag) { heartbeat_flag = 0; ToggleHeartbeatLED(); }
        if (status_flag)    { status_flag    = 0; UploadStatus();        }
        if (watchdog_flag)  { watchdog_flag  = 0; HAL_IWDG_Refresh(&hiwdg); }
        rt_thread_mdelay(10);
    }
}
```

### 场景 4：按键长短按判断

```c
static rt_timer_t key_timer;
static rt_tick_t  key_press_tick;

/* GPIO 中断：按键按下，记录时间并启动定时器 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == KEY_Pin)
    {
        if (HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin) == GPIO_PIN_RESET)
        {
            key_press_tick = rt_tick_get();  /* 记录按下时刻 */
            rt_timer_start(key_timer);        /* 启动 1 秒长按定时器 */
        }
        else  /* 按键释放 */
        {
            rt_timer_stop(key_timer);  /* 停止长按计时 */
            rt_tick_t duration = rt_tick_get() - key_press_tick;
            if (duration < rt_tick_from_millisecond(1000))
                rt_event_send(key_event, EVT_BTN_SHORT);  /* 短按 */
        }
    }
}

/* 定时器回调：1 秒内没有松开，判断为长按 */
void key_long_press_callback(void *p)
{
    rt_event_send(key_event, EVT_BTN_LONG);
}

/* 初始化 */
key_timer = rt_timer_create("key", key_long_press_callback, RT_NULL,
                             rt_tick_from_millisecond(1000), RT_TIMER_FLAG_ONE_SHOT);
```

---

## 常见错误

### 在回调中调用阻塞 API

```c
/* ❌ 错误：在回调中等待信号量（可能阻塞定时器线程） */
void bad_callback(void *p)
{
    rt_sem_take(sem, RT_WAITING_FOREVER);   /* 死锁风险 */
    DoSomething();
    rt_sem_release(sem);
}

/* ✅ 只在回调中 release，take 放在处理线程里 */
void good_callback(void *p)
{
    rt_sem_release(process_sem);  /* 通知处理线程 */
}

void process_entry(void *p)
{
    while (1)
    {
        rt_sem_take(process_sem, RT_WAITING_FOREVER);  /* 在线程里等待 */
        DoSomething();
    }
}
```

### 忘记 stop 后再修改周期

```c
/* ❌ 直接调用 control 修改时间，定时器可能处于运行状态 */
rt_timer_control(timer, RT_TIMER_CTRL_SET_TIME, &new_time);  /* 可能不生效 */

/* ✅ 先 stop，修改后再 start */
rt_timer_stop(timer);
rt_timer_control(timer, RT_TIMER_CTRL_SET_TIME,
                 (void *)rt_tick_from_millisecond(2000));
rt_timer_start(timer);
```

### 单次定时器超时后再次 start

```c
/* 单次定时器超时后自动停止，想再次使用需要重新 start */
rt_timer_t one_shot = rt_timer_create(..., RT_TIMER_FLAG_ONE_SHOT);
rt_timer_start(one_shot);

/* ... 定时器超时，自动停止 ... */

rt_timer_start(one_shot);  /* ✅ 再次启动是安全的，会重新计时 */
```

---

## API 速查

| API | 说明 |
|-----|------|
| `rt_timer_create(name, fn, param, time, flag)` | 动态创建定时器 |
| `rt_timer_init(&tmr, name, fn, param, time, flag)` | 静态创建定时器 |
| `rt_timer_delete(timer)` | 删除动态定时器 |
| `rt_timer_detach(&tmr)` | 脱离静态定时器 |
| `rt_timer_start(timer)` | 启动定时器（开始计时） |
| `rt_timer_stop(timer)` | 停止定时器 |
| `rt_timer_control(timer, cmd, arg)` | 控制定时器 |

**`rt_timer_control` 的 cmd 参数：**

| cmd | arg 类型 | 说明 |
|-----|---------|------|
| `RT_TIMER_CTRL_SET_TIME` | `rt_tick_t *` | 设置超时时间（先 stop 再用）|
| `RT_TIMER_CTRL_GET_TIME` | `rt_tick_t *` | 获取超时时间 |
| `RT_TIMER_CTRL_SET_ONESHOT` | `RT_NULL` | 改为单次模式 |
| `RT_TIMER_CTRL_SET_PERIODIC` | `RT_NULL` | 改为周期模式 |

**`flag` 参数：**

| 值 | 含义 |
|----|------|
| `RT_TIMER_FLAG_ONE_SHOT` | 单次触发 |
| `RT_TIMER_FLAG_PERIODIC` | 周期触发 |
| `RT_TIMER_FLAG_HARD_TIMER` | 在中断中执行回调（默认）|
| `RT_TIMER_FLAG_SOFT_TIMER` | 在定时器线程中执行回调 |
