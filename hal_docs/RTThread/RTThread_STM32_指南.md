# RT-Thread 工程创建与使用指南

> **平台支持：** RT-Thread 同时支持 **STM32（HAL 库）** 和 **ESP32（ESP-IDF）**。
> - STM32：通过 RT-Thread Studio 或 CubeMX + Nano 包集成，本指南以此为主
> - ESP32（IDF）：使用 RT-Thread 完整版，通过 IDF Component Manager 引入 `rt-thread` 组件，线程/信号量/队列等 API 与 STM32 版本完全一致

## 目录

- [RT-Thread 简介与选型](#rt-thread-简介与选型)
- [方法一：RT-Thread Studio 创建工程](#方法一rt-thread-studio-创建工程)
- [方法二：STM32CubeMX + RT-Thread Nano](#方法二stm32cubemx--rt-thread-nano)
- [核心概念](#核心概念)
- [线程（Thread）](#线程thread) → 详细文档：[RTThread_Thread.md](RTThread_Thread.md)
- [信号量（Semaphore）](#信号量semaphore) → 详细文档：[RTThread_Semaphore.md](RTThread_Semaphore.md)
- [互斥量（Mutex）](#互斥量mutex) → 详细文档：[RTThread_Mutex.md](RTThread_Mutex.md)
- [消息队列（Message Queue）](#消息队列message-queue) → 详细文档：[RTThread_MessageQueue.md](RTThread_MessageQueue.md)
- [邮箱（Mailbox）](#邮箱mailbox) → 详细文档：[RTThread_Mailbox.md](RTThread_Mailbox.md)
- [事件集（Event）](#事件集event) → 详细文档：[RTThread_Event.md](RTThread_Event.md)
- [软件定时器（Timer）](#软件定时器timer) → 详细文档：[RTThread_Timer.md](RTThread_Timer.md)
- [常见问题与注意事项](#常见问题与注意事项)
- [RT-Thread vs FreeRTOS 对比](#rt-thread-vs-freertos-对比)
- [快速 API 速查表](#快速-api-速查表)

---

## RT-Thread 简介与选型

RT-Thread 是国产开源实时操作系统，分为两个版本：

| 版本 | ROM 占用 | RAM 占用 | 适用场景 |
|------|---------|---------|---------|
| **RT-Thread Nano** | ~6KB | ~1.2KB | STM32F103 等资源受限 MCU，替代 FreeRTOS |
| **RT-Thread 完整版** | >64KB | >4KB | 有网络、文件系统、设备驱动需求的项目 |

**STM32F103C8T6（20KB RAM / 64KB Flash）推荐使用 RT-Thread Nano**，资源占用与 FreeRTOS 相当，API 更丰富。

### 两种上手方式

| 方式 | 优点 | 缺点 |
|------|------|------|
| **RT-Thread Studio** | 专用 IDE，一键创建，工具链集成好 | 需要单独安装，与 STM32CubeIDE 并行 |
| **CubeMX + Nano 包** | 沿用现有 CubeMX 流程，HAL 库集成无缝 | 配置步骤稍多 |

---

## 方法一：RT-Thread Studio 创建工程

### 1.1 安装 RT-Thread Studio

前往官网下载：[https://www.rt-thread.org/studio.html](https://www.rt-thread.org/studio.html)

> 安装包内含 GCC 工具链、OpenOCD 调试支持，安装完成即可用，无需另配环境。

### 1.2 新建工程

1. 打开 RT-Thread Studio，菜单 `File → New → RT-Thread Project`
2. 填写工程名，如 `rt_demo`
3. 配置选项：

| 配置项 | 设置 |
|--------|------|
| Board | 选 `STM32F103C8` 或 `Generic STM32F1 Series` |
| RT-Thread 版本 | 选 `Nano`（资源受限选这个） |
| 调试器 | `ST-Link` |
| 下载接口 | `SWD` |

4. 点击 **Finish**，Studio 自动下载 BSP 包并生成工程

### 1.3 工程结构说明

```
rt_demo/
├── applications/
│   └── main.c              ← 用户代码入口，在这里创建线程
├── rt-thread/
│   ├── components/         ← RT-Thread 组件（shell、libc 等）
│   ├── include/            ← rtthread.h 等头文件
│   └── src/                ← 内核源码（不需要改）
├── drivers/
│   └── drv_gpio.c          ← BSP 驱动，已适配 HAL
├── board/
│   └── board.c             ← 时钟初始化、堆配置
└── rtconfig.h              ← RT-Thread 功能开关配置
```

### 1.4 编译与下载

- 点击工具栏锤子图标 **Build** 编译
- 接好 ST-Link（SWD 接线：SWDIO、SWCLK、3.3V、GND）
- 点击绿色下载按钮 **Download** 烧录

### 1.5 配置 rtconfig.h

RT-Thread Studio 提供图形化配置界面：左侧 `RT-Thread Settings` 双击打开，或直接编辑 `rtconfig.h`。

常用配置项：

```c
/* 最大线程优先级数，STM32F1 建议 32，节省内存 */
#define RT_THREAD_PRIORITY_MAX  32

/* 时钟节拍频率（1ms/tick） */
#define RT_TICK_PER_SECOND      1000

/* 内核对象名最大长度 */
#define RT_NAME_MAX             8

/* 堆大小，单位字节，STM32F103 建议 4096~8192 */
#define RT_HEAP_SIZE            4096

/* 开启组件：shell（调试命令行） */
#define RT_USING_FINSH
#define FINSH_USING_MSH

/* 开启软件定时器 */
#define RT_USING_TIMER_SOFT
```

---

## 方法二：STM32CubeMX + RT-Thread Nano

此方法在现有 CubeMX 工程（参考 [STM32CubeMX_F103C8T6_指南.md](../STM32CubeMX_F103C8T6_指南.md)）基础上集成 RT-Thread Nano。

### 2.1 在 CubeMX 中启用 RT-Thread Nano

打开已有的 CubeMX 工程（或新建），左侧 `Middleware and Software Packs` → `RTOS RT-Thread`（如无则点击右上角 `+` 添加软件包）：

1. 点击 `PACK MANAGER`，搜索 `RT-Thread`，安装 `rt-thread.rtthread` 包
2. 回到 `Middleware` 列表，启用 `RT-Thread`
3. Interface 选 `NANO`

> **如果找不到 RT-Thread 选项：** 在 CubeMX 菜单 `Help → Manage embedded software packages`，切换到 `From Local...`，下载 RT-Thread Nano CubeMX 包（.pack 文件）后手动导入。下载地址：https://github.com/RT-Thread/rt-thread/releases（找 rtthread-nano-xxx.pack）

### 2.2 修改 SYS 时基源

与 FreeRTOS 相同，RT-Thread 占用 SysTick，需要将 HAL 时基换掉：

左侧 `System Core → SYS`：
- **Timebase Source** 改为 `TIM4`（或任意空闲定时器）

### 2.3 NVIC 优先级分组

左侧 `System Core → NVIC`：
- **Priority Group** 确认为 `4 bits for pre-emption priority`

RT-Thread 在中断中调用 API 的限制与 FreeRTOS 相同：
- 优先级高于 `RT_CPUS_NR`（默认值区间）的中断不能调用阻塞型 API
- 中断中使用非阻塞版本：如 `rt_sem_release()`（该函数本身可在中断中使用）

### 2.4 生成代码

点击 **GENERATE CODE**，CubeMX 生成文件中会包含：

| 文件 | 内容 |
|------|------|
| `Middlewares/Third_Party/RT-Thread/` | RT-Thread Nano 内核源码 |
| `Core/Src/rtthread_init.c` | 线程初始化桩代码 |
| `rtconfig.h` | Nano 配置文件 |
| `main.c` | `rt_hw_board_init()` 和 `rtthread_init()` 已插入 |

### 2.5 手动集成（不用 CubeMX 包的方式）

如果 CubeMX 包安装麻烦，也可以手动把 RT-Thread Nano 源码拷进工程：

1. 从 GitHub 克隆：`git clone https://github.com/RT-Thread/rtthread-nano`
2. 将以下文件夹复制到工程目录：

```
工程目录/
└── RT-Thread/
    ├── include/        ← rtthread.h, rtdef.h 等
    ├── src/            ← thread.c, ipc.c, scheduler.c 等内核源文件
    ├── libcpu/
    │   └── arm/
    │       └── cortex-m3/  ← context_gcc.S（汇编上下文切换）
    └── bsp/            ← board.c（时钟和堆初始化，需根据芯片修改）
```

3. 在 STM32CubeIDE 中将以上路径加入 `Include Paths` 和 `Source Locations`
4. 修改 `board.c` 中的堆配置：

```c
/* board.c */
#define RT_HW_HEAP_BEGIN    (&__heap_start)   /* 链接脚本中的符号 */
#define RT_HW_HEAP_END      (&__heap_end)

void rt_hw_board_init(void)
{
    /* 初始化 HAL */
    HAL_Init();
    SystemClock_Config();
    
    /* RT-Thread 堆初始化 */
    rt_system_heap_init(RT_HW_HEAP_BEGIN, RT_HW_HEAP_END);
    
    /* 初始化系统时钟节拍（在 SysTick_Handler 中调用 rt_tick_increase()） */
    rt_hw_systick_init();
}
```

5. 在 `stm32f1xx_it.c` 的 `SysTick_Handler` 中添加：

```c
void SysTick_Handler(void)
{
    rt_tick_increase();   /* RT-Thread 节拍驱动 */
    HAL_IncTick();        /* HAL 时基（已换成 TIM4，这行可删） */
}
```

6. `main()` 入口修改：

```c
int main(void)
{
    /* HAL 和时钟初始化在 rt_hw_board_init 中已做，这里不再重复 */
    /* RT-Thread 通过 rtthread_startup() 启动，在启动文件汇编中调用 */
    /* 用户代码写在 main_thread_entry 或自己创建的线程里 */
    
    /* 如果使用 Nano，main() 本身就是一个 RT-Thread 线程 */
    while (1)
    {
        rt_thread_mdelay(500);
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    }
}
```

> **Nano 的 main() 魔法：** RT-Thread Nano 中，`main()` 函数会被封装成一个优先级为 10、栈大小 256 words 的线程自动运行，用户不需要手动调用 `rt_thread_startup()`。

---

## 核心概念

### 调度机制

RT-Thread 使用**抢占式优先级调度**，优先级数值**越小越高**（与 NVIC 方向相同，与 FreeRTOS 相反）。

| 调度条件 | 触发时机 |
|---------|---------|
| 高优先级线程就绪 | 立即抢占 |
| 同优先级竞争 | 时间片轮转（可配置） |
| 线程主动让出 | 调用 `rt_thread_mdelay`、等待 IPC 对象等 |

> **优先级 0 最高，RT_THREAD_PRIORITY_MAX-1 最低。** 空闲线程占用最低优先级，用户线程从 1 开始。

### 线程状态机

```
         创建并启动
              ↓
           就绪（Ready）
              ↓ 调度器选中
           运行（Running）
              ↓ 主动阻塞或被抢占
    ┌──挂起（Suspended）←── 等待信号量/队列/延时
    └──就绪（Ready）   ←── 超时/资源可用/恢复
         ↓ 线程函数返回或 rt_thread_delete
       关闭（Close）
```

### 关键数据类型

| 类型 | 说明 |
|------|------|
| `rt_thread_t` | 线程句柄（指针） |
| `rt_sem_t` | 信号量句柄 |
| `rt_mutex_t` | 互斥量句柄 |
| `rt_mq_t` | 消息队列句柄 |
| `rt_mailbox_t` | 邮箱句柄 |
| `rt_event_t` | 事件集句柄 |
| `rt_timer_t` | 定时器句柄 |
| `rt_tick_t` | 节拍计数（毫秒） |
| `rt_err_t` | 错误码（`RT_EOK` 表示成功） |
| `rt_base_t` | 基础整型 |

---

## 线程（Thread）

### 动态创建线程（推荐）

```c
rt_thread_t tid;

/* 线程入口函数，必须是 void 返回值，void* 参数 */
void led_thread_entry(void *parameter)
{
    /* 初始化代码，只执行一次 */
    
    while (1)
    {
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        rt_thread_mdelay(500);   /* 阻塞延时 500ms，让出 CPU */
    }
    /* 正常不会到这里；如果要退出，直接 return 即可，RT-Thread 会自动回收 */
}

/* 在 main() 或初始化函数中创建并启动线程 */
tid = rt_thread_create(
    "led",              /* 线程名称（最长 RT_NAME_MAX 字符） */
    led_thread_entry,   /* 线程入口函数 */
    RT_NULL,            /* 传给入口函数的参数 */
    512,                /* 栈大小，单位字节（不是 words！） */
    10,                 /* 优先级，数值越小越高 */
    20                  /* 时间片，单位 tick，同优先级轮转时使用 */
);

if (tid != RT_NULL)
    rt_thread_startup(tid);   /* 启动线程，进入就绪态 */
```

### 静态创建线程（不使用动态内存）

```c
/* 静态线程需要提前分配栈和控制块 */
static rt_uint8_t led_stack[512];
static struct rt_thread led_tcb;

void led_thread_entry(void *parameter)
{
    while (1)
    {
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        rt_thread_mdelay(500);
    }
}

/* 初始化（对应动态创建的 create） */
rt_thread_init(
    &led_tcb,           /* 线程控制块指针 */
    "led",              /* 线程名 */
    led_thread_entry,   /* 入口函数 */
    RT_NULL,            /* 参数 */
    led_stack,          /* 栈缓冲区 */
    sizeof(led_stack),  /* 栈大小 */
    10,                 /* 优先级 */
    20                  /* 时间片 */
);
rt_thread_startup(&led_tcb);
```

### 优先级规划建议

| 优先级 | 用途 |
|--------|------|
| 1 ~ 3  | 时间敏感任务（电机控制、快速采样） |
| 4 ~ 8  | 通信任务（UART、CAN、I2C 处理） |
| 9 ~ 15 | 普通业务逻辑（数据处理、控制算法） |
| 16 ~ 25 | 低优先级后台任务（日志、状态上报） |
| 最大值-1 | 空闲任务（系统占用，不可使用） |

### 线程管理 API

```c
rt_thread_mdelay(500);                    /* 延时 500ms，阻塞让出 CPU */
rt_thread_delay(rt_tick_from_millisecond(100)); /* 延时 100ms（tick 换算版） */

rt_thread_suspend(tid);                   /* 挂起线程 */
rt_thread_resume(tid);                    /* 恢复线程 */
rt_thread_delete(tid);                    /* 删除动态线程（释放内存） */
rt_thread_detach(&led_tcb);               /* 删除静态线程 */

rt_thread_self();                         /* 获取当前线程句柄 */
rt_thread_yield();                        /* 主动让出 CPU（同优先级轮转） */

/* 获取栈使用情况（调试用） */
rt_uint8_t *sp = tid->stack_addr;
/* 或者通过 finsh shell 命令 list_thread 查看 */
```

### `rt_thread_mdelay` vs `rt_thread_delay`

```c
/* rt_thread_mdelay：毫秒参数，最常用 */
rt_thread_mdelay(100);  /* 延时 100ms */

/* rt_thread_delay：tick 参数，需要手动换算 */
rt_thread_delay(RT_TICK_PER_SECOND / 10);    /* 延时 100ms（当 tick=1000 时） */
rt_thread_delay(rt_tick_from_millisecond(100)); /* 同上，换算宏更清晰 */
```

---

## 信号量（Semaphore）

信号量用于**任务间同步**和**中断通知任务**。

### 创建信号量

```c
rt_sem_t sem;

/* 创建二值信号量，初始值 0（等待模式） */
sem = rt_sem_create("sem1", 0, RT_IPC_FLAG_FIFO);
/* 第三参数：RT_IPC_FLAG_FIFO（先等先得）或 RT_IPC_FLAG_PRIO（高优先级先得） */

if (sem == RT_NULL) {
    /* 创建失败，内存不足 */
}
```

### 在中断中释放信号量（通知任务）

```c
/* 按键 GPIO 中断回调 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == KEY_Pin)
    {
        /* 释放信号量，唤醒等待的任务 */
        /* rt_sem_release 可在中断中安全调用 */
        rt_sem_release(sem);
    }
}

/* 任务中等待信号量 */
void key_thread_entry(void *parameter)
{
    while (1)
    {
        /* 永久等待，直到信号量被释放 */
        rt_sem_take(sem, RT_WAITING_FOREVER);
        
        /* 处理按键事件 */
        ProcessKey();
    }
}
```

### 计数信号量

```c
/* 创建计数信号量，最大值5，初始值5（5个资源可用） */
rt_sem_t res_sem = rt_sem_create("res", 5, RT_IPC_FLAG_FIFO);

/* 申请资源（计数-1） */
rt_sem_take(res_sem, RT_WAITING_FOREVER);
UseResource();

/* 释放资源（计数+1） */
rt_sem_release(res_sem);
```

### 信号量 API

```c
rt_sem_take(sem, RT_WAITING_FOREVER);      /* 获取（阻塞等待） */
rt_sem_take(sem, rt_tick_from_millisecond(100)); /* 等待最多 100ms */
rt_sem_trytake(sem);                       /* 尝试获取，立即返回（非阻塞） */
rt_sem_release(sem);                       /* 释放（可在中断中调用） */
rt_sem_delete(sem);                        /* 删除动态信号量 */
rt_sem_detach(&sem_obj);                   /* 删除静态信号量 */
```

---

## 互斥量（Mutex）

互斥量用于**保护共享资源**，防止多线程同时访问。内置**优先级继承**，防止优先级反转。

> **注意：互斥量不能在中断中使用**，中断中保护数据请用临界区（`rt_enter_critical` / `rt_exit_critical`）。

```c
rt_mutex_t uart_mutex;

/* 创建互斥量 */
uart_mutex = rt_mutex_create("uart_mtx", RT_IPC_FLAG_PRIO);

/* 线程 A：带互斥保护的串口输出 */
void taskA_entry(void *p)
{
    while (1)
    {
        /* 获取互斥量，最多等 100ms */
        if (rt_mutex_take(uart_mutex, rt_tick_from_millisecond(100)) == RT_EOK)
        {
            /* 临界区：独占 UART */
            rt_kprintf("Task A running\n");
            
            /* 释放互斥量，必须由持有者释放 */
            rt_mutex_release(uart_mutex);
        }
        rt_thread_mdelay(200);
    }
}

/* 线程 B 同样用 uart_mutex，与 A 互斥 */
```

### 互斥量 API

```c
rt_mutex_take(mutex, RT_WAITING_FOREVER);  /* 获取（阻塞） */
rt_mutex_release(mutex);                   /* 释放 */
rt_mutex_delete(mutex);                    /* 删除 */
```

### 临界区（中断中保护数据）

```c
/* 方式1：关闭中断（最轻量，适合极短操作） */
rt_base_t level = rt_hw_interrupt_disable();
g_shared_var = new_value;                  /* 临界区 */
rt_hw_interrupt_enable(level);

/* 方式2：挂起调度器（允许中断，禁止任务切换） */
rt_enter_critical();
g_shared_var = new_value;
rt_exit_critical();
```

---

## 消息队列（Message Queue）

消息队列传递固定大小的数据块，每次发送/接收一条消息，先进先出。

```c
rt_mq_t mq;

/* 创建消息队列：最多 10 条消息，每条 4 字节 */
mq = rt_mq_create("mq1", 4, 10, RT_IPC_FLAG_FIFO);

/* 发送消息（任务中） */
rt_uint32_t data = 1234;
rt_mq_send(mq, &data, sizeof(data));      /* 非阻塞，队列满时返回错误 */
rt_mq_send_wait(mq, &data, sizeof(data), RT_WAITING_FOREVER); /* 阻塞版本 */

/* 发送紧急消息（插入队头） */
rt_mq_urgent(mq, &data, sizeof(data));

/* 接收消息（阻塞等待） */
rt_uint32_t received;
rt_mq_recv(mq, &received, sizeof(received), RT_WAITING_FOREVER);

/* 传递结构体 */
typedef struct {
    rt_uint16_t id;
    float       value;
} SensorMsg_t;

rt_mq_t sensor_mq = rt_mq_create("sensor", sizeof(SensorMsg_t), 5, RT_IPC_FLAG_FIFO);

SensorMsg_t msg = {.id = 1, .value = 25.6f};
rt_mq_send(sensor_mq, &msg, sizeof(msg));

SensorMsg_t recv_msg;
rt_mq_recv(sensor_mq, &recv_msg, sizeof(recv_msg), RT_WAITING_FOREVER);
```

### 在中断中发送消息

```c
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    rt_uint8_t byte = rx_buf[0];
    /* rt_mq_send 在中断中可以调用（非阻塞版本） */
    rt_mq_send(uart_mq, &byte, sizeof(byte));
    
    /* 重新开启接收 */
    HAL_UART_Receive_IT(huart, rx_buf, 1);
}
```

---

## 邮箱（Mailbox）

邮箱传递 4 字节（一个指针大小）的值，适合传递指针（指向堆上分配的数据块），比消息队列效率更高。

```c
rt_mailbox_t mb;

/* 创建邮箱，容量 10 封 */
mb = rt_mb_create("mb1", 10, RT_IPC_FLAG_FIFO);

/* 发送（发送一个 rt_ubase_t 大小的值，通常是指针） */
rt_uint32_t value = 0xABCD;
rt_mb_send(mb, value);                           /* 非阻塞 */
rt_mb_send_wait(mb, value, RT_WAITING_FOREVER);  /* 阻塞版本 */

/* 接收 */
rt_ubase_t recv_val;
rt_mb_recv(mb, &recv_val, RT_WAITING_FOREVER);

/* 典型用法：发送指针，实现零拷贝 */
typedef struct { float x; float y; } Point_t;

void sender_entry(void *p)
{
    while (1)
    {
        /* 从内存池申请空间 */
        Point_t *pt = rt_malloc(sizeof(Point_t));
        pt->x = 1.0f; pt->y = 2.0f;
        
        /* 发送指针（只传 4 字节地址，不复制数据） */
        rt_mb_send(mb, (rt_ubase_t)pt);
    }
}

void receiver_entry(void *p)
{
    while (1)
    {
        Point_t *pt;
        rt_mb_recv(mb, (rt_ubase_t *)&pt, RT_WAITING_FOREVER);
        
        /* 处理数据 */
        process(pt);
        
        /* 处理完记得释放内存 */
        rt_free(pt);
    }
}
```

---

## 事件集（Event）

事件集有 32 个独立事件位，一个线程可以等待多个事件，支持 AND（全部满足）和 OR（任意满足）。

```c
rt_event_t event;

/* 定义事件位 */
#define EVT_SENSOR_DONE   (1 << 0)
#define EVT_UART_RECV     (1 << 1)
#define EVT_BUTTON_PRESS  (1 << 2)

/* 创建事件集 */
event = rt_event_create("evt1", RT_IPC_FLAG_PRIO);

/* 发送事件（设置位），可在中断中调用 */
rt_event_send(event, EVT_SENSOR_DONE);
rt_event_send(event, EVT_UART_RECV | EVT_BUTTON_PRESS);   /* 同时设多位 */

/* 等待任意一个事件（OR） */
rt_uint32_t recv_evt;
rt_event_recv(event,
    EVT_SENSOR_DONE | EVT_UART_RECV,    /* 等待的位 */
    RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, /* OR 逻辑，收到后清除这些位 */
    RT_WAITING_FOREVER,
    &recv_evt                           /* 实际触发的事件位 */
);

if (recv_evt & EVT_SENSOR_DONE)   { /* 传感器完成 */ }
if (recv_evt & EVT_UART_RECV)     { /* 串口收到数据 */ }

/* 等待全部事件同时满足（AND） */
rt_event_recv(event,
    EVT_SENSOR_DONE | EVT_UART_RECV | EVT_BUTTON_PRESS,
    RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR,
    RT_WAITING_FOREVER,
    &recv_evt
);
```

---

## 软件定时器（Timer）

RT-Thread 软件定时器在定时器线程中执行回调，回调中**不能调用会引起线程挂起的 API**（如 `rt_sem_take` 带超时）。

```c
rt_timer_t timer;

/* 定时器回调函数 */
void timer_callback(void *parameter)
{
    /* 不能阻塞，快速执行 */
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    
    /* 可以发送信号量/邮箱/消息（非阻塞版本）通知任务处理 */
    rt_sem_release(some_sem);
}

/* 创建周期定时器，1000ms 周期 */
timer = rt_timer_create(
    "timer1",
    timer_callback,
    RT_NULL,                            /* 传给回调的参数 */
    rt_tick_from_millisecond(1000),     /* 超时时间 1000ms */
    RT_TIMER_FLAG_PERIODIC              /* RT_TIMER_FLAG_ONE_SHOT 单次 */
);

rt_timer_start(timer);    /* 启动 */
rt_timer_stop(timer);     /* 停止 */
rt_timer_delete(timer);   /* 删除 */

/* 修改定时周期（需先停止） */
rt_timer_stop(timer);
rt_timer_control(timer, RT_TIMER_CTRL_SET_TIME,
    (void *)rt_tick_from_millisecond(500));
rt_timer_start(timer);
```

---

## 常见问题与注意事项

### 1. 线程栈大小单位是字节，不是 words

FreeRTOS 栈大小单位是 words（1 word = 4 字节），RT-Thread 是**字节**。相同栈空间：

```c
/* FreeRTOS */
xTaskCreate(fn, "t", 128, NULL, 2, NULL);   /* 128 words = 512 字节 */

/* RT-Thread */
rt_thread_create("t", fn, RT_NULL, 512, 10, 20);  /* 512 字节，相同大小 */
```

### 2. 优先级方向相反

```c
/* FreeRTOS：数值越大越高 */
xTaskCreate(fn, "t", 128, NULL, 5, NULL);    /* 优先级 5 */

/* RT-Thread：数值越小越高 */
rt_thread_create("t", fn, RT_NULL, 512, 5, 20); /* 优先级 5，注意含义相反 */
```

### 3. `rt_kprintf` 替代 `printf`

RT-Thread Nano 中，`rt_kprintf` 是线程安全的串口输出，不需要额外互斥量：

```c
rt_kprintf("value = %d\n", val);  /* 推荐，线程安全 */
```

使用 `rt_kprintf` 需要在 `board.c` 中实现 `rt_hw_console_output`：

```c
void rt_hw_console_output(const char *str)
{
    /* 将 str 通过 UART1 输出 */
    HAL_UART_Transmit(&huart1, (uint8_t *)str, strlen(str), HAL_MAX_DELAY);
}
```

### 4. 堆空间不足的排查

```c
/* 查询当前可用堆 */
rt_size_t free = rt_memory_info(RT_NULL, RT_NULL, RT_NULL); /* 返回空闲字节 */

/* 或者启用 finsh shell，运行命令 free 查看内存统计 */
```

症状：`rt_thread_create`、`rt_sem_create` 等返回 `RT_NULL`，系统启动时崩溃。
解决：增大 `rtconfig.h` 中的 `RT_HEAP_SIZE`，或将大数组改为静态分配。

### 5. 中断中只能用非阻塞 API

```c
/* ❌ 错误：在中断中调用阻塞版本 */
void SomeIRQHandler(void)
{
    rt_sem_take(sem, RT_WAITING_FOREVER);  /* 不能在中断里阻塞等待 */
}

/* ✅ 正确：在中断中只用释放/发送（不阻塞的那侧） */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    rt_sem_release(sem);         /* 释放：可以 */
    rt_mq_send(mq, &data, 4);   /* 发送（非阻塞版）：可以 */
    rt_event_send(event, EVT);  /* 发送事件：可以 */
}
```

### 6. 线程函数不能直接 return（Nano 除外）

RT-Thread 完整版中，线程函数返回会导致错误。RT-Thread Nano 中线程函数 `return` 后系统会自动调用 `rt_thread_exit()` 清理资源，是安全的。

```c
/* 完整版：线程不能退出 */
void thread_entry(void *p)
{
    /* 初始化 */
    for (;;)
    {
        /* 循环体 */
    }
    /* 如果确实要退出，主动删除自身 */
    rt_thread_delete(rt_thread_self());
}
```

### 7. 调试：使用 FinSH Shell

RT-Thread 提供命令行调试工具 FinSH，通过串口输入命令查看系统状态：

在 `rtconfig.h` 开启：
```c
#define RT_USING_FINSH
#define FINSH_USING_MSH
```

常用命令：

| 命令 | 说明 |
|------|------|
| `list_thread` | 查看所有线程状态、优先级、栈使用 |
| `list_sem` | 查看信号量状态 |
| `list_mutex` | 查看互斥量状态 |
| `list_timer` | 查看定时器状态 |
| `free` | 查看内存使用情况 |
| `ps` | 类 Linux 进程列表 |

---

## RT-Thread vs FreeRTOS 对比

| 特性 | RT-Thread Nano | FreeRTOS |
|------|---------------|----------|
| 优先级方向 | 数值小 = 优先级高 | 数值大 = 优先级高 |
| 栈大小单位 | 字节 | Words（×4 = 字节） |
| 延时函数 | `rt_thread_mdelay(ms)` | `vTaskDelay(pdMS_TO_TICKS(ms))` |
| 信号量释放 | `rt_sem_release(sem)` | `xSemaphoreGive(sem)` |
| 队列发送 | `rt_mq_send(mq, &data, size)` | `xQueueSend(q, &data, timeout)` |
| 调试 Shell | 内置 FinSH（可选） | 无（需第三方） |
| 线程通知 | 通过邮箱/消息队列 | 原生 TaskNotify，开销更小 |
| 内存管理 | `rt_malloc` / `rt_free` | `pvPortMalloc` / `vPortFree` |
| ISR 中发送 | 直接调用，无需 FromISR 后缀 | 必须用 `xxxFromISR` 系列 API |
| 国产化支持 | 是，有完善中文文档 | 否 |

> **最大区别：** RT-Thread 在中断中直接调用 `rt_sem_release()`、`rt_mq_send()` 即可（内部自动处理），无需像 FreeRTOS 那样使用 `FromISR` 后缀函数，使用更简单。

---

## 典型工程结构

三任务架构示例（传感器采集 → 数据处理 → 串口上报）：

```c
/* applications/main.c */
#include <rtthread.h>
#include "main.h"

static rt_mq_t sensor_mq;   /* 传感器数据队列 */
static rt_mq_t result_mq;   /* 处理结果队列 */

typedef struct { rt_uint16_t adc; rt_uint32_t ts; } SensorData_t;
typedef struct { float voltage;   rt_uint32_t ts; } ResultData_t;

/* 任务1：传感器采集（高优先级，固定周期） */
void sensor_task_entry(void *p)
{
    while (1)
    {
        SensorData_t d;
        d.adc = HAL_ADC_GetValue(&hadc1);
        d.ts  = rt_tick_get();
        rt_mq_send(sensor_mq, &d, sizeof(d));
        rt_thread_mdelay(10);   /* 10ms 采集一次 */
    }
}

/* 任务2：数据处理（中等优先级） */
void process_task_entry(void *p)
{
    SensorData_t raw;
    while (1)
    {
        rt_mq_recv(sensor_mq, &raw, sizeof(raw), RT_WAITING_FOREVER);
        ResultData_t res;
        res.voltage = raw.adc * 3.3f / 4096.0f;
        res.ts      = raw.ts;
        rt_mq_send(result_mq, &res, sizeof(res));
    }
}

/* 任务3：串口上报（低优先级） */
void report_task_entry(void *p)
{
    ResultData_t res;
    while (1)
    {
        rt_mq_recv(result_mq, &res, sizeof(res), RT_WAITING_FOREVER);
        rt_kprintf("[%lu] voltage = %.3f V\n", res.ts, res.voltage);
    }
}

int main(void)
{
    /* 创建消息队列 */
    sensor_mq = rt_mq_create("s_mq", sizeof(SensorData_t), 10, RT_IPC_FLAG_FIFO);
    result_mq = rt_mq_create("r_mq", sizeof(ResultData_t), 10, RT_IPC_FLAG_FIFO);

    /* 创建并启动线程 */
    rt_thread_t t;

    t = rt_thread_create("sensor",  sensor_task_entry,  RT_NULL, 512, 5,  20);
    rt_thread_startup(t);

    t = rt_thread_create("process", process_task_entry, RT_NULL, 512, 8,  20);
    rt_thread_startup(t);

    t = rt_thread_create("report",  report_task_entry,  RT_NULL, 512, 12, 20);
    rt_thread_startup(t);

    /* main 线程本身可以结束，其他线程独立运行 */
    return 0;
}
```

---

## 快速 API 速查表

### 线程

| API | 说明 |
|-----|------|
| `rt_thread_create(name, fn, param, stack, prio, tick)` | 动态创建线程（需 startup） |
| `rt_thread_startup(tid)` | 启动线程 |
| `rt_thread_delete(tid)` | 删除动态线程 |
| `rt_thread_mdelay(ms)` | 毫秒延时（阻塞） |
| `rt_thread_delay(ticks)` | tick 延时（阻塞） |
| `rt_thread_suspend(tid)` | 挂起线程 |
| `rt_thread_resume(tid)` | 恢复线程 |
| `rt_thread_self()` | 获取当前线程句柄 |
| `rt_thread_yield()` | 主动让出 CPU |

### 信号量

| API | 说明 |
|-----|------|
| `rt_sem_create(name, val, flag)` | 创建信号量 |
| `rt_sem_take(sem, timeout)` | 获取（阻塞） |
| `rt_sem_trytake(sem)` | 尝试获取（非阻塞） |
| `rt_sem_release(sem)` | 释放（可在中断中调用） |
| `rt_sem_delete(sem)` | 删除 |

### 互斥量

| API | 说明 |
|-----|------|
| `rt_mutex_create(name, flag)` | 创建互斥量 |
| `rt_mutex_take(mutex, timeout)` | 获取 |
| `rt_mutex_release(mutex)` | 释放 |
| `rt_mutex_delete(mutex)` | 删除 |

### 消息队列

| API | 说明 |
|-----|------|
| `rt_mq_create(name, msg_size, max_msgs, flag)` | 创建消息队列 |
| `rt_mq_send(mq, buf, size)` | 发送（非阻塞，可在中断中调用） |
| `rt_mq_send_wait(mq, buf, size, timeout)` | 发送（阻塞版本） |
| `rt_mq_urgent(mq, buf, size)` | 发送到队头 |
| `rt_mq_recv(mq, buf, size, timeout)` | 接收 |
| `rt_mq_delete(mq)` | 删除 |

### 事件集

| API | 说明 |
|-----|------|
| `rt_event_create(name, flag)` | 创建事件集 |
| `rt_event_send(event, set)` | 发送事件位（可在中断中调用） |
| `rt_event_recv(event, set, opt, timeout, &recved)` | 接收事件 |
| `rt_event_delete(event)` | 删除 |

### 时间换算

```c
rt_tick_from_millisecond(100)  /* 100ms → ticks */
RT_WAITING_FOREVER             /* 永久等待 */
RT_WAITING_NO                  /* 不等待，立即返回（相当于 0） */
```
