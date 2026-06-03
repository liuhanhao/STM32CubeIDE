# RT-Thread 互斥量（Mutex）

## 目录

- [核心概念](#核心概念)
- [创建与使用](#创建与使用)
- [优先级继承机制](#优先级继承机制)
- [递归互斥量](#递归互斥量)
- [临界区（中断中保护数据）](#临界区中断中保护数据)
- [典型使用场景](#典型使用场景)
- [常见错误](#常见错误)
- [API 速查](#api-速查)

---

## 核心概念

互斥量（Mutex，Mutual Exclusion）用于**保护共享资源**，确保同一时刻只有一个线程能访问被保护的代码区域。

与信号量的本质区别：

| 特性 | 信号量 | 互斥量 |
|------|--------|--------|
| 用途 | 同步（通知事件） | 互斥（保护资源） |
| release 限制 | 任何线程/中断都可以 | **只有持有者才能 release** |
| 优先级继承 | ❌ 无 | ✅ 有 |
| 中断中使用 | ✅ 可以 release | ❌ 不可以 |
| 初始状态 | 0 | 1（可用） |

**一句话总结：** 信号量是"信号枪"，谁都可以打；互斥量是"钥匙"，谁拿谁还。

---

## 创建与使用

### 创建互斥量

```c
rt_mutex_t uart_mutex;

uart_mutex = rt_mutex_create(
    "uart_mtx",         /* 名称 */
    RT_IPC_FLAG_PRIO    /* 等待规则：PRIO 高优先级先得（互斥量推荐） */
);

if (uart_mutex == RT_NULL) {
    /* 内存不足 */
}
```

### 基本用法

```c
/* 线程 A */
void task_a_entry(void *p)
{
    while (1)
    {
        /* 获取互斥量，等待最多 100ms */
        if (rt_mutex_take(uart_mutex, rt_tick_from_millisecond(100)) == RT_EOK)
        {
            /* 临界区开始：独占 UART */
            rt_kprintf("Task A: counter = %d\n", counter_a++);
            /* 临界区结束 */

            /* 必须由持有者释放 */
            rt_mutex_release(uart_mutex);
        }
        rt_thread_mdelay(200);
    }
}

/* 线程 B：同样使用 uart_mutex，与 A 互斥 */
void task_b_entry(void *p)
{
    while (1)
    {
        rt_mutex_take(uart_mutex, RT_WAITING_FOREVER);
        rt_kprintf("Task B: counter = %d\n", counter_b++);
        rt_mutex_release(uart_mutex);

        rt_thread_mdelay(300);
    }
}
```

### 执行流程示意

```
时间轴：
  线程B take → 获得锁 → [临界区] → release
  线程A take → 阻塞等待... → （B release后）→ 获得锁 → [临界区] → release
```

---

## 优先级继承机制

互斥量内置**优先级继承**，这是它相比信号量最重要的特性，用于防止**优先级反转**问题。

### 什么是优先级反转

```
场景：
  线程 L（低优先级）持有互斥量
  线程 H（高优先级）等待同一互斥量
  线程 M（中优先级）就绪，抢占了线程 L

结果：
  H 等 L 释放锁，但 L 被 M 阻塞了
  → 高优先级 H 被中优先级 M 间接"阻塞"，反转了！
```

### 优先级继承如何解决

```
RT-Thread 互斥量的解决方案：
  H 等待 L 持有的互斥量时，
  临时将 L 的优先级提升到与 H 相同
  → L 能抢占 M，尽快执行并释放锁
  → H 得到锁，L 优先级恢复原值

时序：
  L（优先级15）take 互斥量
  H（优先级5） take 互斥量 → 阻塞，同时将 L 的优先级提升到 5
  M（优先级10）就绪 → 无法抢占 L（L 临时优先级 5 > M 的 10）
  L 执行完释放互斥量 → 优先级恢复 15
  H 获得锁继续执行
```

这就是为什么**保护共享资源一定要用互斥量，而不是用信号量**。

---

## 递归互斥量

普通互斥量不可重入——同一个线程连续两次 `take` 会死锁。递归互斥量允许同一线程多次 `take`，每次 `take` 对应一次 `release`。

```c
/* 创建递归互斥量 */
/* RT-Thread 的 rt_mutex 本身就支持递归（owner 计数） */
/* 同一线程可以多次 take，需要对应次数的 release */

rt_mutex_t recursive_mtx = rt_mutex_create("rec", RT_IPC_FLAG_PRIO);

void recursive_function(int depth)
{
    rt_mutex_take(recursive_mtx, RT_WAITING_FOREVER);
    
    rt_kprintf("depth = %d\n", depth);
    
    if (depth > 0)
        recursive_function(depth - 1);  /* 递归调用，再次 take 同一互斥量 */
    
    rt_mutex_release(recursive_mtx);    /* 对应释放 */
}

/* 调用：take 3次，release 3次，最终释放锁 */
recursive_function(2);
```

> RT-Thread 的 `rt_mutex_t` 内部有 `hold` 计数，同一线程每次 `take` hold+1，每次 `release` hold-1，hold 降为 0 时锁才真正释放。

---

## 临界区（中断中保护数据）

互斥量不能在中断中使用。在中断中修改与线程共享的数据，应使用临界区。

### 方式 1：关闭中断（最轻量，适合极短操作）

```c
/* 关闭中断，保存中断状态 */
rt_base_t level = rt_hw_interrupt_disable();

/* 临界区：不能被中断打断 */
g_shared_counter++;

/* 恢复中断 */
rt_hw_interrupt_enable(level);
```

### 方式 2：挂起调度器（允许中断，禁止线程切换）

```c
/* 挂起调度器：中断仍可响应，但不会切换线程 */
rt_enter_critical();

/* 临界区：不会被其他线程抢占 */
g_buffer[g_write_pos] = new_data;
g_write_pos = (g_write_pos + 1) % BUFFER_SIZE;

rt_exit_critical();
```

**两种方式对比：**

| 方式 | 中断响应 | 适用场景 |
|------|---------|---------|
| `rt_hw_interrupt_disable` | ❌ 屏蔽中断 | 极短操作（几条指令），对中断延迟敏感时慎用 |
| `rt_enter_critical` | ✅ 允许中断 | 稍长操作，只需防止线程竞争 |

---

## 典型使用场景

### 场景 1：多线程共享 UART

多个线程都需要打印日志，不加保护会出现输出混乱（乱码、交叉）。

```c
static rt_mutex_t uart_mutex;
uart_mutex = rt_mutex_create("uart", RT_IPC_FLAG_PRIO);

/* 封装线程安全的打印函数 */
void safe_printf(const char *fmt, ...)
{
    char buf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    rt_mutex_take(uart_mutex, RT_WAITING_FOREVER);
    HAL_UART_Transmit(&huart1, (uint8_t *)buf, strlen(buf), HAL_MAX_DELAY);
    rt_mutex_release(uart_mutex);
}

/* 各线程直接调用 safe_printf，无需自己管锁 */
void sensor_entry(void *p)
{
    while (1)
    {
        safe_printf("[sensor] temp = %.1f\n", ReadTemp());
        rt_thread_mdelay(1000);
    }
}

void motor_entry(void *p)
{
    while (1)
    {
        safe_printf("[motor] rpm = %d\n", GetRPM());
        rt_thread_mdelay(200);
    }
}
```

### 场景 2：保护 SPI Flash 访问

```c
static rt_mutex_t flash_mutex;
flash_mutex = rt_mutex_create("flash", RT_IPC_FLAG_PRIO);

rt_err_t flash_write(rt_uint32_t addr, const rt_uint8_t *data, rt_size_t len)
{
    rt_mutex_take(flash_mutex, RT_WAITING_FOREVER);

    W25Q_Write(addr, data, len);  /* SPI Flash 写操作 */

    rt_mutex_release(flash_mutex);
    return RT_EOK;
}

rt_err_t flash_read(rt_uint32_t addr, rt_uint8_t *buf, rt_size_t len)
{
    rt_mutex_take(flash_mutex, RT_WAITING_FOREVER);

    W25Q_Read(addr, buf, len);

    rt_mutex_release(flash_mutex);
    return RT_EOK;
}
```

### 场景 3：保护全局状态结构体

```c
typedef struct {
    float    temperature;
    float    humidity;
    rt_uint8_t alarm_flag;
} SystemState_t;

static SystemState_t g_state;
static rt_mutex_t    state_mutex;

/* 初始化 */
state_mutex = rt_mutex_create("state", RT_IPC_FLAG_PRIO);

/* 写入状态（传感器线程） */
void update_state(float temp, float humi)
{
    rt_mutex_take(state_mutex, RT_WAITING_FOREVER);
    g_state.temperature = temp;
    g_state.humidity    = humi;
    rt_mutex_release(state_mutex);
}

/* 读取状态（显示线程） */
SystemState_t get_state(void)
{
    SystemState_t snapshot;
    rt_mutex_take(state_mutex, RT_WAITING_FOREVER);
    snapshot = g_state;  /* 拷贝一份，快速释放锁 */
    rt_mutex_release(state_mutex);
    return snapshot;
}
```

---

## 常见错误

### 在中断中使用互斥量

```c
/* ❌ 错误：互斥量不能在中断中使用 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    rt_mutex_take(uart_mutex, RT_WAITING_FOREVER);  /* 会崩溃 */
    rt_kprintf("Button pressed\n");
    rt_mutex_release(uart_mutex);
}

/* ✅ 正确：中断只通知线程，线程里操作互斥量 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    rt_sem_release(btn_sem);  /* 通知线程 */
}

void btn_thread_entry(void *p)
{
    while (1)
    {
        rt_sem_take(btn_sem, RT_WAITING_FOREVER);
        rt_mutex_take(uart_mutex, RT_WAITING_FOREVER);
        rt_kprintf("Button pressed\n");
        rt_mutex_release(uart_mutex);
    }
}
```

### 忘记 release（死锁）

```c
/* ❌ 错误：提前 return 导致锁没释放 */
void process_data(void)
{
    rt_mutex_take(data_mutex, RT_WAITING_FOREVER);

    if (validate_data() != RT_EOK)
        return;  /* 直接返回，锁没有被释放！其他线程永久阻塞 */

    do_process();
    rt_mutex_release(data_mutex);
}

/* ✅ 正确：确保所有路径都会 release */
void process_data(void)
{
    rt_mutex_take(data_mutex, RT_WAITING_FOREVER);

    if (validate_data() != RT_EOK)
    {
        rt_mutex_release(data_mutex);  /* 提前释放 */
        return;
    }

    do_process();
    rt_mutex_release(data_mutex);
}
```

### 死锁（两个线程互相等待）

```c
/* ❌ 危险：线程 A 和 B 以相反顺序获取两个锁 */
void task_a(void *p)
{
    rt_mutex_take(mutex1, RT_WAITING_FOREVER);
    rt_thread_mdelay(1);  /* 此时 B 可能拿到了 mutex2 */
    rt_mutex_take(mutex2, RT_WAITING_FOREVER);  /* A 等 B 释放 mutex2 */
    /* A 等 B，B 等 A → 死锁 */
}

void task_b(void *p)
{
    rt_mutex_take(mutex2, RT_WAITING_FOREVER);
    rt_mutex_take(mutex1, RT_WAITING_FOREVER);  /* B 等 A 释放 mutex1 */
}

/* ✅ 正确：所有线程以相同顺序获取锁 */
void task_a(void *p)
{
    rt_mutex_take(mutex1, RT_WAITING_FOREVER);  /* 先 1 后 2 */
    rt_mutex_take(mutex2, RT_WAITING_FOREVER);
    /* ... */
    rt_mutex_release(mutex2);
    rt_mutex_release(mutex1);
}

void task_b(void *p)
{
    rt_mutex_take(mutex1, RT_WAITING_FOREVER);  /* 同样先 1 后 2 */
    rt_mutex_take(mutex2, RT_WAITING_FOREVER);
    /* ... */
    rt_mutex_release(mutex2);
    rt_mutex_release(mutex1);
}
```

### 非持有者释放互斥量

```c
/* ❌ 错误：线程 B 释放了线程 A 持有的互斥量 */
/* 互斥量是"钥匙"，只有拿钥匙的人才能还钥匙 */
/* RT-Thread 会检测 owner，非持有者 release 会报错 */
```

---

## API 速查

| API | 说明 |
|-----|------|
| `rt_mutex_create(name, flag)` | 动态创建互斥量 |
| `rt_mutex_init(&mtx, name, flag)` | 静态创建互斥量 |
| `rt_mutex_delete(mutex)` | 删除动态互斥量 |
| `rt_mutex_detach(&mtx)` | 脱离静态互斥量 |
| `rt_mutex_take(mutex, timeout)` | 获取互斥量（阻塞） |
| `rt_mutex_trytake(mutex)` | 尝试获取（非阻塞） |
| `rt_mutex_release(mutex)` | 释放互斥量（只有持有者可调用） |
| `rt_hw_interrupt_disable()` | 关闭中断（临界区，返回中断状态值） |
| `rt_hw_interrupt_enable(level)` | 恢复中断 |
| `rt_enter_critical()` | 挂起调度器（临界区） |
| `rt_exit_critical()` | 恢复调度器 |
