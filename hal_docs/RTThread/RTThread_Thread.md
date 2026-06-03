# RT-Thread 线程（Thread）

## 目录

- [核心概念](#核心概念)
- [线程状态机](#线程状态机)
- [动态创建线程](#动态创建线程)
- [静态创建线程](#静态创建线程)
- [线程优先级](#线程优先级)
- [延时函数](#延时函数)
- [线程管理 API](#线程管理-api)
- [典型使用场景](#典型使用场景)
- [常见错误](#常见错误)
- [API 速查](#api-速查)

---

## 核心概念

线程是 RT-Thread 中最基本的调度单元，等价于 FreeRTOS 的任务（Task）。每个线程有独立的：

- **栈空间**：保存局部变量、函数调用链、寄存器上下文
- **优先级**：决定调度顺序，0 最高，数字越大越低
- **时间片**：同优先级线程轮转时每次最多运行的 tick 数

RT-Thread 使用**抢占式优先级调度**：高优先级线程一旦就绪，立即抢占当前运行的低优先级线程。同优先级线程之间按时间片轮转。

> **与 FreeRTOS 的关键区别：**
> - RT-Thread 优先级数值**越小越高**（FreeRTOS 是数值越大越高）
> - RT-Thread 栈大小单位是**字节**（FreeRTOS 是 words，1 word = 4 字节）
> - RT-Thread Nano 中 `main()` 本身就是一个线程，无需手动启动调度器

---

## 线程状态机

```
         rt_thread_create + rt_thread_startup
                        ↓
                   就绪（Ready）
                        ↓ 调度器选中
                   运行（Running）
                   ↙           ↘
        主动阻塞/等待IPC       被高优先级抢占
              ↓                      ↓
       挂起（Suspended）         就绪（Ready）
              ↓
    超时/资源可用/rt_thread_resume
              ↓
         就绪（Ready）

  rt_thread_delete / 线程函数返回
              ↓
          关闭（Close）
```

| 状态 | 说明 |
|------|------|
| 就绪（Ready） | 等待被调度器选中运行 |
| 运行（Running） | 当前正在 CPU 上执行 |
| 挂起（Suspended） | 等待延时超时、IPC 对象、手动恢复 |
| 关闭（Close） | 线程已删除，资源待回收 |

---

## 动态创建线程

动态创建使用堆内存，是最常用的方式。

```c
#include <rtthread.h>
#include "main.h"  /* HAL 头文件 */

/* 线程入口函数：void 返回值，void* 参数 */
void led_thread_entry(void *parameter)
{
    /* 初始化代码，只执行一次 */
    
    while (1)
    {
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        rt_thread_mdelay(500);  /* 阻塞延时 500ms，让出 CPU */
    }
    /* RT-Thread Nano 中线程函数 return 是安全的，系统自动回收 */
}

int main(void)
{
    rt_thread_t tid;

    tid = rt_thread_create(
        "led",              /* 线程名，最长 RT_NAME_MAX（默认8）字符 */
        led_thread_entry,   /* 线程入口函数 */
        RT_NULL,            /* 传给入口函数的参数，不需要填 RT_NULL */
        512,                /* 栈大小，单位：字节（不是 words！） */
        10,                 /* 优先级，0 最高，数字越大越低 */
        20                  /* 时间片，单位 tick，同优先级轮转用 */
    );

    if (tid != RT_NULL)
        rt_thread_startup(tid);   /* 将线程加入就绪队列 */

    /* main 线程本身继续执行，或者进入 while(1) */
    while (1)
    {
        rt_thread_mdelay(1000);
    }
}
```

**传递参数给线程：**

```c
/* 通过 parameter 传递数据 */
typedef struct {
    rt_uint8_t  led_pin;
    rt_uint32_t interval_ms;
} LedParam_t;

void led_thread_entry(void *parameter)
{
    LedParam_t *param = (LedParam_t *)parameter;

    while (1)
    {
        HAL_GPIO_TogglePin(GPIOC, param->led_pin);
        rt_thread_mdelay(param->interval_ms);
    }
}

/* 创建多个 LED 线程，各自独立 */
static LedParam_t led1_param = {GPIO_PIN_13, 500};
static LedParam_t led2_param = {GPIO_PIN_14, 200};

rt_thread_t t1 = rt_thread_create("led1", led_thread_entry, &led1_param, 256, 10, 10);
rt_thread_t t2 = rt_thread_create("led2", led_thread_entry, &led2_param, 256, 10, 10);
rt_thread_startup(t1);
rt_thread_startup(t2);
```

---

## 静态创建线程

静态创建不使用堆内存，适合不想动态分配的场景（内存确定性更好）。

```c
/* 提前分配栈空间和线程控制块 */
static rt_uint8_t     sensor_stack[512];
static struct rt_thread sensor_tcb;

void sensor_thread_entry(void *parameter)
{
    while (1)
    {
        ReadSensor();
        rt_thread_mdelay(10);
    }
}

/* 初始化（等价于 create，但不使用堆） */
rt_err_t result = rt_thread_init(
    &sensor_tcb,            /* 线程控制块 */
    "sensor",               /* 线程名 */
    sensor_thread_entry,    /* 入口函数 */
    RT_NULL,                /* 参数 */
    sensor_stack,           /* 栈缓冲区 */
    sizeof(sensor_stack),   /* 栈大小 */
    8,                      /* 优先级 */
    20                      /* 时间片 */
);

if (result == RT_EOK)
    rt_thread_startup(&sensor_tcb);
```

> **动态 vs 静态：** 开发阶段用动态创建方便调整栈大小；产品阶段如果内存紧张，改为静态创建可避免碎片。

---

## 线程优先级

**数值越小，优先级越高。** 优先级 0 最高，`RT_THREAD_PRIORITY_MAX-1` 最低（空闲线程占用）。

默认 `RT_THREAD_PRIORITY_MAX` 为 32，用户线程优先级范围建议 1 ~ 30。

| 优先级范围 | 建议用途 |
|-----------|---------|
| 1 ~ 4    | 硬实时任务：电机控制、紧急中断响应 |
| 5 ~ 10   | 时间敏感：高频传感器采集、通信协议处理 |
| 11 ~ 18  | 普通业务：数据处理、控制算法 |
| 19 ~ 26  | 低优先级：日志记录、状态显示、串口上报 |
| 27 ~ 30  | 后台维护：心跳、内存监控 |
| 31       | 空闲线程（系统占用，不可使用） |

**优先级实战建议：**

```c
/* 采集线程优先级高于处理线程，处理线程高于上报线程 */
rt_thread_t acquire = rt_thread_create("acq",  acq_entry,  RT_NULL, 512, 5,  20);
rt_thread_t process = rt_thread_create("proc", proc_entry, RT_NULL, 512, 10, 20);
rt_thread_t report  = rt_thread_create("rpt",  rpt_entry,  RT_NULL, 512, 15, 20);
```

---

## 延时函数

```c
/* 最常用：毫秒延时，阻塞并让出 CPU */
rt_thread_mdelay(100);   /* 延时 100ms */

/* tick 延时（需手动换算） */
rt_thread_delay(RT_TICK_PER_SECOND / 10);            /* 延时 100ms */
rt_thread_delay(rt_tick_from_millisecond(100));       /* 同上，更清晰 */

/* 主动让出 CPU（不延时，让同优先级其他线程运行） */
rt_thread_yield();
```

**`rt_thread_mdelay` vs 裸机 `HAL_Delay` 对比：**

```c
/* ❌ HAL_Delay：忙等待，期间 CPU 无法做其他事 */
HAL_Delay(500);

/* ✅ rt_thread_mdelay：阻塞挂起，CPU 可运行其他线程 */
rt_thread_mdelay(500);
```

---

## 线程管理 API

```c
/* 挂起：让线程暂停，直到被 resume 或超时 */
rt_thread_suspend(tid);

/* 恢复：唤醒被挂起的线程 */
rt_thread_resume(tid);

/* 删除动态线程（释放堆内存） */
rt_thread_delete(tid);

/* 脱离静态线程（不释放内存，仅标记为关闭） */
rt_thread_detach(&tcb);

/* 获取当前正在运行的线程句柄 */
rt_thread_t self = rt_thread_self();

/* 动态修改优先级 */
rt_thread_control(tid, RT_THREAD_CTRL_CHANGE_PRIORITY, (void *)new_priority);
```

**查看栈使用情况（调试）：**

```c
/* 通过 FinSH Shell 命令（需开启 RT_USING_FINSH）*/
/* 串口输入：list_thread
   输出示例：
   thread   pri  status      sp     stack size max used  left tick  error
   -------- ---  ------- ---------- ----------  ------  ---------- ---
   led       10  suspend 0x00000058 0x00000200    28%   0x00000013 000
   sensor     8  running 0x00000078 0x00000200    35%   0x00000008 000
*/
```

---

## 典型使用场景

### 场景 1：LED 心跳线程

最简单的示例，后台定时闪烁，不干扰其他逻辑。

```c
void heartbeat_entry(void *p)
{
    while (1)
    {
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        rt_thread_mdelay(500);
    }
}

/* 优先级低一点，不影响重要任务 */
rt_thread_t t = rt_thread_create("hb", heartbeat_entry, RT_NULL, 256, 25, 10);
rt_thread_startup(t);
```

### 场景 2：固定周期采样线程

```c
void sample_entry(void *p)
{
    rt_tick_t last_tick = rt_tick_get();

    while (1)
    {
        /* 精确固定周期：rt_thread_delay 基于绝对时间计算 */
        rt_thread_delay_until(&last_tick, rt_tick_from_millisecond(10));

        /* 10ms 采集一次 ADC */
        uint16_t adc = HAL_ADC_GetValue(&hadc1);
        rt_mq_send(sensor_mq, &adc, sizeof(adc));  /* 发给处理任务 */
    }
}

rt_thread_t t = rt_thread_create("sample", sample_entry, RT_NULL, 512, 5, 5);
rt_thread_startup(t);
```

> `rt_thread_delay_until` 类似 FreeRTOS 的 `vTaskDelayUntil`，能消除执行时间抖动，保证严格的周期性。

### 场景 3：线程间流水线处理

```c
/* 三个线程：采集 → 处理 → 上报，通过消息队列串联 */
static rt_mq_t raw_mq;
static rt_mq_t result_mq;

void acquire_entry(void *p)
{
    while (1)
    {
        uint16_t raw = ADC_Read();
        rt_mq_send(raw_mq, &raw, sizeof(raw));
        rt_thread_mdelay(10);
    }
}

void process_entry(void *p)
{
    uint16_t raw;
    while (1)
    {
        rt_mq_recv(raw_mq, &raw, sizeof(raw), RT_WAITING_FOREVER);
        float voltage = raw * 3.3f / 4096.0f;
        rt_mq_send(result_mq, &voltage, sizeof(voltage));
    }
}

void report_entry(void *p)
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
    raw_mq    = rt_mq_create("raw",    sizeof(uint16_t), 20, RT_IPC_FLAG_FIFO);
    result_mq = rt_mq_create("result", sizeof(float),    20, RT_IPC_FLAG_FIFO);

    rt_thread_startup(rt_thread_create("acq",  acquire_entry, RT_NULL, 512, 5,  20));
    rt_thread_startup(rt_thread_create("proc", process_entry, RT_NULL, 512, 8,  20));
    rt_thread_startup(rt_thread_create("rpt",  report_entry,  RT_NULL, 512, 12, 20));

    return 0;
}
```

### 场景 4：一次性初始化线程（执行完自动退出）

```c
void init_entry(void *p)
{
    /* 执行耗时初始化操作 */
    Calibrate_Sensor();
    Load_Config_From_Flash();
    rt_kprintf("Init done\n");

    /* 不需要 while(1)，函数返回后 RT-Thread 自动回收线程资源 */
}

rt_thread_t t = rt_thread_create("init", init_entry, RT_NULL, 1024, 3, 10);
rt_thread_startup(t);
/* 其他线程可继续运行，init 线程完成后自动消失 */
```

---

## 常见错误

### 栈太小导致溢出

```c
/* ❌ 危险：栈只有 64 字节，根本不够用 */
rt_thread_create("t", entry, RT_NULL, 64, 10, 20);

/* ✅ 基本配置建议 */
/* 简单线程（只有 rt_kprintf）：256 字节 */
/* 一般业务线程：512 字节 */
/* 有浮点运算/较深调用栈：1024 字节 */
/* 使用 list_thread 查看 max used，保持 < 70% */
```

**开启栈溢出检测（rtconfig.h）：**

```c
#define RT_USING_OVERFLOW_CHECK   /* 开启后溢出时进入 rt_hw_exception_install 注册的异常处理 */
```

### 忘记调用 rt_thread_startup

```c
/* ❌ 常见遗漏：创建后没有 startup，线程永远不会运行 */
rt_thread_t t = rt_thread_create("t", entry, RT_NULL, 512, 10, 20);
/* 忘记调用 rt_thread_startup(t) */

/* ✅ 创建后必须 startup */
if (t != RT_NULL)
    rt_thread_startup(t);
```

### 在线程外调用 rt_thread_mdelay

```c
/* ❌ 不能在中断中或调度器启动前调用 mdelay */
void SomeIRQHandler(void)
{
    rt_thread_mdelay(10);  /* 中断中不能调用会阻塞线程的函数 */
}

/* ✅ 中断中直接返回，通知线程处理 */
void SomeIRQHandler(void)
{
    rt_sem_release(sem);  /* 通知线程，线程里再做延时 */
}
```

### 动态线程 delete 后仍使用句柄

```c
rt_thread_t t = rt_thread_create(...);
rt_thread_startup(t);

rt_thread_delete(t);  /* 删除线程 */
rt_thread_resume(t);  /* ❌ 危险：t 已经是无效句柄 */

/* ✅ 删除后将句柄置空 */
rt_thread_delete(t);
t = RT_NULL;
```

---

## API 速查

| API | 说明 |
|-----|------|
| `rt_thread_create(name, fn, param, stack, prio, tick)` | 动态创建线程（返回句柄） |
| `rt_thread_init(&tcb, name, fn, param, stack, size, prio, tick)` | 静态创建线程 |
| `rt_thread_startup(tid)` | 启动线程，进入就绪态 |
| `rt_thread_delete(tid)` | 删除动态线程，释放内存 |
| `rt_thread_detach(&tcb)` | 脱离静态线程 |
| `rt_thread_mdelay(ms)` | 毫秒级阻塞延时 |
| `rt_thread_delay(ticks)` | tick 级阻塞延时 |
| `rt_thread_delay_until(&tick, ticks)` | 绝对时间延时（固定周期） |
| `rt_thread_yield()` | 主动让出 CPU |
| `rt_thread_suspend(tid)` | 挂起线程 |
| `rt_thread_resume(tid)` | 恢复线程 |
| `rt_thread_self()` | 获取当前线程句柄 |
| `rt_thread_control(tid, cmd, arg)` | 控制线程（改优先级等） |
| `rt_tick_get()` | 获取当前系统 tick |
| `rt_tick_from_millisecond(ms)` | 毫秒转 tick |
