# 使用 PlatformIO 在 STM32F103C8T6 上集成 RT-Thread Nano

> 本指南介绍如何在 **PlatformIO + STM32Cube 框架**下为 STM32F103C8T6 集成 RT-Thread Nano，无需 RT-Thread Studio 或 STM32CubeMX，直接手动集成内核源码。

---

## 目录

1. [工程创建与配置](#1-工程创建与配置)
2. [RT-Thread Nano 源码集成](#2-rt-thread-nano-源码集成)
3. [rtconfig.h 配置](#3-rtconfigh-配置)
4. [board.c 与启动适配](#4-boardc-与启动适配)
5. [第一个 RT-Thread 工程：多线程 Blink](#5-第一个-rt-thread-工程多线程-blink)
6. [线程间通信示例](#6-线程间通信示例)
7. [中断与 RT-Thread 协作](#7-中断与-rt-thread-协作)
8. [FinSH Shell 调试配置](#8-finsh-shell-调试配置)
9. [常见问题](#9-常见问题)
10. [platformio.ini 完整模板](#10-platformioini-完整模板)

---

## 1. 工程创建与配置

### 1.1 创建 STM32Cube 框架工程

```ini
; platformio.ini
[env:bluepill_f103c8]
platform  = ststm32
board     = bluepill_f103c8
framework = stm32cube

upload_protocol = stlink
debug_tool      = stlink
monitor_speed   = 115200
```

### 1.2 RT-Thread Nano 资源占用

STM32F103C8T6 有 20KB SRAM / 64KB Flash，RT-Thread Nano 资源占用如下：

| 资源 | 占用 | 剩余（参考） |
|------|------|------------|
| Flash（内核） | ~6 KB | ~58 KB |
| RAM（内核固定） | ~1.2 KB | ~18.8 KB |
| RAM（堆，可配置） | 4~8 KB | 视配置而定 |

> RT-Thread Nano 与 FreeRTOS 资源占用相当，适合 STM32F103C8T6 这类资源受限 MCU。


---

## 2. RT-Thread Nano 源码集成

PIO Registry 目前没有官方的 RT-Thread Nano 库包，需要手动将源码放入工程的 `lib/` 目录。

### 2.1 获取 RT-Thread Nano 源码

```bash
# 方式 1：克隆官方 Nano 仓库
git clone https://github.com/RT-Thread/rtthread-nano.git

# 方式 2：从 RT-Thread 主仓库提取 Nano 分支
git clone -b nano https://github.com/RT-Thread/rt-thread.git
```

### 2.2 复制必要文件到工程

将以下文件复制到工程 `lib/rtthread/` 目录：

```
lib/rtthread/
├── include/
│   ├── rtthread.h          ← 主头文件，用户只需包含这一个
│   ├── rtdef.h             ← 基础类型定义
│   ├── rtservice.h         ← 内核服务接口
│   ├── rttypes.h           ← 类型别名
│   └── rtconfig_project.h ← 可选，工程级配置覆盖
├── src/
│   ├── clock.c             ← 时钟节拍
│   ├── components.c        ← 组件初始化
│   ├── device.c            ← 设备框架（可选）
│   ├── idle.c              ← 空闲线程
│   ├── ipc.c               ← 信号量、互斥量、队列、邮箱、事件
│   ├── irq.c               ← 中断管理
│   ├── kservice.c          ← 内核服务（rt_kprintf 等）
│   ├── mem.c               ← 内存管理（小内存算法）
│   ├── mempool.c           ← 内存池（可选）
│   ├── object.c            ← 内核对象管理
│   ├── scheduler.c         ← 调度器
│   ├── signal.c            ← 信号（可选）
│   ├── thread.c            ← 线程管理
│   └── timer.c             ← 定时器
└── libcpu/
    └── arm/
        └── cortex-m3/
            ├── context_gcc.S   ← 上下文切换汇编（GCC 版本）
            ├── cpuport.c       ← CPU 移植层
            └── cpuport.h
```

> **最小集合：** 如果只需要线程、信号量、队列，可以只编译 `clock.c`、`idle.c`、`ipc.c`、`kservice.c`、`mem.c`、`object.c`、`scheduler.c`、`thread.c`、`timer.c` 以及 `libcpu/arm/cortex-m3/` 下的文件。

### 2.3 配置 platformio.ini 头文件路径

```ini
build_flags =
    -DUSE_HAL_DRIVER
    -DSTM32F103xB
    -Ilib/rtthread/include
    -Ilib/rtthread/libcpu/arm/cortex-m3
    -Iinclude                   ; 存放 rtconfig.h 的目录
```


---

## 3. rtconfig.h 配置

在 `include/` 目录下创建 `rtconfig.h`，这是 RT-Thread 的核心配置文件：

```c
/* include/rtconfig.h */
#ifndef RT_CONFIG_H__
#define RT_CONFIG_H__

/* ── 基础内核配置 ─────────────────────────────────────── */
/* 内核对象名最大长度（字节），影响 RAM 占用 */
#define RT_NAME_MAX         8

/* 字节对齐大小，Cortex-M3 使用 4 字节对齐 */
#define RT_ALIGN_SIZE       4

/* 最大线程优先级数，STM32F103 建议 32，节省内存 */
/* 优先级范围：0（最高）~ RT_THREAD_PRIORITY_MAX-1（最低） */
#define RT_THREAD_PRIORITY_MAX  32

/* 时钟节拍频率，1000 = 1ms/tick */
#define RT_TICK_PER_SECOND  1000

/* ── 调试配置 ─────────────────────────────────────────── */
/* 开启断言（开发阶段建议开启，发布时可关闭节省 Flash） */
#define RT_DEBUG

/* 开启线程栈溢出检测 */
#define RT_USING_OVERFLOW_CHECK

/* ── 内存管理 ─────────────────────────────────────────── */
/* 使用小内存管理算法（适合 STM32F103 这类小内存 MCU） */
#define RT_USING_SMALL_MEM

/* 堆大小（字节），STM32F103C8T6 建议 4096~8192 */
#define RT_HEAP_SIZE        (6 * 1024)

/* ── IPC 对象开关 ─────────────────────────────────────── */
#define RT_USING_SEMAPHORE      /* 信号量 */
#define RT_USING_MUTEX          /* 互斥量（含优先级继承） */
#define RT_USING_EVENT          /* 事件集 */
#define RT_USING_MAILBOX        /* 邮箱 */
#define RT_USING_MESSAGEQUEUE   /* 消息队列 */

/* ── 软件定时器 ───────────────────────────────────────── */
#define RT_USING_TIMER_SOFT         /* 开启软件定时器线程 */
#define RT_TIMER_THREAD_PRIO        4       /* 定时器线程优先级 */
#define RT_TIMER_THREAD_STACK_SIZE  512     /* 定时器线程栈大小（字节） */

/* ── 控制台输出 ───────────────────────────────────────── */
/* 开启 rt_kprintf，需要在 board.c 中实现 rt_hw_console_output */
#define RT_USING_CONSOLE
#define RT_CONSOLEBUF_SIZE      128     /* 控制台缓冲区大小 */
#define RT_CONSOLE_DEVICE_NAME  "uart1" /* 控制台设备名 */

/* ── FinSH Shell（可选，调试用）─────────────────────────── */
/* 取消注释以启用 FinSH 命令行调试工具 */
/* #define RT_USING_FINSH          */
/* #define FINSH_USING_MSH         */
/* #define FINSH_THREAD_STACK_SIZE 1024 */

/* ── 组件初始化 ───────────────────────────────────────── */
/* 使用自动初始化机制（INIT_BOARD_EXPORT 等宏） */
#define RT_USING_COMPONENTS_INIT

/* ── 钩子函数（可选）─────────────────────────────────── */
/* #define RT_USING_IDLE_HOOK */      /* 空闲线程钩子 */
/* #define RT_USING_TICK_HOOK */      /* 节拍钩子 */

#endif /* RT_CONFIG_H__ */
```


---

## 4. board.c 与启动适配

### 4.1 创建 board.c

在 `src/` 目录下创建 `board.c`，负责硬件初始化和 RT-Thread 与 HAL 的桥接：

```c
/* src/board.c */
#include "stm32f1xx_hal.h"
#include <rtthread.h>

/* ── 堆内存区域定义 ───────────────────────────────────── */
/* 方式 1：静态数组作为堆（简单可靠，推荐） */
static uint8_t rt_heap[RT_HEAP_SIZE];

/* 方式 2：使用链接脚本中的 _heap_start/_heap_end 符号（更灵活） */
/* extern int _heap_start, _heap_end; */

/* ── 硬件初始化 ───────────────────────────────────────── */
void rt_hw_board_init(void)
{
    /* 初始化 HAL 库 */
    HAL_Init();

    /* 配置系统时钟：HSE 8MHz → PLL ×9 = 72MHz */
    SystemClock_Config();

    /* 初始化 RT-Thread 堆 */
    rt_system_heap_init(rt_heap, rt_heap + RT_HEAP_SIZE);

    /* 初始化串口（用于 rt_kprintf 输出） */
    MX_USART1_Init();

    /* 如果使用了 RT_USING_COMPONENTS_INIT，在这里调用板级组件初始化 */
#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif
}

/* ── SysTick 中断处理 ─────────────────────────────────── */
/* RT-Thread 使用 SysTick 作为节拍源，在 SysTick_Handler 中驱动 */
void SysTick_Handler(void)
{
    rt_tick_increase();     /* RT-Thread 节拍递增 */
    /* 注意：不要在这里调用 HAL_IncTick()，HAL 时基已切换到 TIM4 */
}

/* ── HAL 时基切换到 TIM4 ──────────────────────────────── */
/* RT-Thread 占用 SysTick，HAL_Delay() 需要切换到其他定时器 */
TIM_HandleTypeDef htim4;

HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    RCC_ClkInitTypeDef clkconfig;
    uint32_t uwTimclock, uwAPB1Prescaler, uwPrescalerValue, pFLatency;

    __HAL_RCC_TIM4_CLK_ENABLE();

    HAL_RCC_GetClockConfig(&clkconfig, &pFLatency);
    uwAPB1Prescaler = clkconfig.APB1CLKDivider;
    uwTimclock = (uwAPB1Prescaler == RCC_HCLK_DIV1)
                 ? HAL_RCC_GetPCLK1Freq()
                 : 2 * HAL_RCC_GetPCLK1Freq();

    uwPrescalerValue = (uint32_t)((uwTimclock / 1000000U) - 1U);

    htim4.Instance               = TIM4;
    htim4.Init.Period            = (1000000U / 1000U) - 1U;
    htim4.Init.Prescaler         = uwPrescalerValue;
    htim4.Init.ClockDivision     = 0;
    htim4.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_Base_Init(&htim4);

    HAL_NVIC_SetPriority(TIM4_IRQn, TickPriority, 0U);
    HAL_NVIC_EnableIRQ(TIM4_IRQn);
    HAL_TIM_Base_Start_IT(&htim4);

    return HAL_OK;
}

void TIM4_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim4);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM4)
        HAL_IncTick();
}

/* ── 控制台输出实现 ───────────────────────────────────── */
/* rt_kprintf 最终调用这个函数输出字符串 */
void rt_hw_console_output(const char *str)
{
    extern UART_HandleTypeDef huart1;
    HAL_UART_Transmit(&huart1, (uint8_t *)str, strlen(str), HAL_MAX_DELAY);
}
```

### 4.2 修改启动文件（startup_stm32f103xb.s）

RT-Thread Nano 的启动方式与裸机不同，需要将 `Reset_Handler` 中的跳转目标从 `main` 改为 `rtthread_startup`。

PIO 使用的 STM32Cube 框架启动文件位于 `.pio/packages/framework-stm32cubef1/`，**不要直接修改**，而是在工程 `src/` 目录下创建自定义启动文件覆盖它：

```bash
# 复制启动文件到工程 src/ 目录
cp .pio/packages/framework-stm32cubef1/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/gcc/startup_stm32f103xb.s src/
```

然后修改 `src/startup_stm32f103xb.s` 中的跳转目标：

```asm
; 找到这一行（原始内容）：
    bl  main

; 改为：
    bl  rtthread_startup
```

> **替代方案（更简单）：** 不修改启动文件，在 `main()` 函数中直接调用 `rtthread_startup()`，RT-Thread Nano 支持这种方式。见第 5 节。


---

## 5. 第一个 RT-Thread 工程：多线程 Blink

RT-Thread Nano 中，`main()` 函数会被自动封装为一个线程运行，这是最简单的上手方式。

### 5.1 main.c（推荐方式：main 即线程）

```c
/* src/main.c */
#include "stm32f1xx_hal.h"
#include <rtthread.h>

/* ── 外设句柄（在 board.c 中定义，这里声明） ─────────── */
extern UART_HandleTypeDef huart1;

/* ── 外设初始化函数声明 ───────────────────────────────── */
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
void MX_USART1_Init(void);

/* ── 线程函数声明 ─────────────────────────────────────── */
static void led_thread_entry(void *parameter);
static void log_thread_entry(void *parameter);

/* ── main 函数（RT-Thread Nano 中 main 本身是一个线程）── */
int main(void)
{
    /* board.c 中的 rt_hw_board_init() 已完成 HAL 和时钟初始化 */
    /* 这里直接初始化 GPIO 等外设 */
    MX_GPIO_Init();

    rt_kprintf("RT-Thread Nano on STM32F103C8T6\n");
    rt_kprintf("System clock: %lu Hz\n", SystemCoreClock);

    /* 创建 LED 闪烁线程 */
    rt_thread_t led_tid = rt_thread_create(
        "led",              /* 线程名 */
        led_thread_entry,   /* 入口函数 */
        RT_NULL,            /* 参数 */
        512,                /* 栈大小（字节） */
        10,                 /* 优先级（数值越小越高） */
        20                  /* 时间片（tick） */
    );
    if (led_tid != RT_NULL)
        rt_thread_startup(led_tid);

    /* 创建日志输出线程 */
    rt_thread_t log_tid = rt_thread_create(
        "log",
        log_thread_entry,
        RT_NULL,
        512,
        15,     /* 低于 LED 线程优先级 */
        20
    );
    if (log_tid != RT_NULL)
        rt_thread_startup(log_tid);

    /* main 线程可以继续做其他事，或者直接返回 */
    /* Nano 中 main 返回后系统继续运行其他线程 */
    return 0;
}

/* ── LED 闪烁线程：每 200ms 翻转一次 ─────────────────── */
static void led_thread_entry(void *parameter)
{
    while (1)
    {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        rt_thread_mdelay(200);
    }
}

/* ── 日志线程：每 1000ms 输出系统信息 ────────────────── */
static void log_thread_entry(void *parameter)
{
    rt_uint32_t count = 0;

    while (1)
    {
        rt_kprintf("[%lu] tick=%lu, free_heap=%d bytes\n",
                   count++,
                   rt_tick_get(),
                   rt_memory_info(RT_NULL, RT_NULL, RT_NULL));
        rt_thread_mdelay(1000);
    }
}

/* ── 外设初始化 ───────────────────────────────────────── */
static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = GPIO_PIN_13;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
}

void MX_USART1_Init(void)
{
    extern UART_HandleTypeDef huart1;
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = GPIO_PIN_9;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin  = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = 115200;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

/* SystemClock_Config 与 PIO 基础教程相同，此处省略 */
```


---

## 6. 线程间通信示例

### 6.1 消息队列：传感器数据流

```c
#include <rtthread.h>

typedef struct {
    rt_uint16_t adc_value;
    rt_uint32_t timestamp;
} SensorData_t;

static rt_mq_t sensor_mq;

/* 采集线程（高优先级） */
static void sensor_thread_entry(void *parameter)
{
    SensorData_t data;

    while (1)
    {
        data.adc_value = HAL_ADC_GetValue(&hadc1);
        data.timestamp = rt_tick_get();

        /* 发送消息（非阻塞，队列满时返回错误） */
        if (rt_mq_send(sensor_mq, &data, sizeof(data)) != RT_EOK)
        {
            rt_kprintf("sensor_mq full!\n");
        }

        rt_thread_mdelay(50);   /* 20Hz 采集 */
    }
}

/* 处理线程（低优先级） */
static void process_thread_entry(void *parameter)
{
    SensorData_t received;

    while (1)
    {
        /* 阻塞等待消息 */
        rt_err_t ret = rt_mq_recv(sensor_mq, &received,
                                   sizeof(received), RT_WAITING_FOREVER);
        if (ret == RT_EOK)
        {
            float voltage = received.adc_value * 3.3f / 4095.0f;
            rt_kprintf("[%lu] ADC=%u, V=%.3f\n",
                       received.timestamp, received.adc_value, voltage);
        }
    }
}

/* 在 main() 中初始化 */
void app_mq_init(void)
{
    /* 创建消息队列：最多 10 条，每条 sizeof(SensorData_t) 字节 */
    sensor_mq = rt_mq_create("s_mq", sizeof(SensorData_t), 10, RT_IPC_FLAG_FIFO);
    RT_ASSERT(sensor_mq != RT_NULL);

    rt_thread_t t;
    t = rt_thread_create("sensor", sensor_thread_entry, RT_NULL, 512, 5, 20);
    rt_thread_startup(t);

    t = rt_thread_create("process", process_thread_entry, RT_NULL, 512, 10, 20);
    rt_thread_startup(t);
}
```

### 6.2 信号量：按键中断通知线程

```c
static rt_sem_t button_sem;

/* 在初始化时创建 */
button_sem = rt_sem_create("btn_sem", 0, RT_IPC_FLAG_FIFO);

/* GPIO 中断回调 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0)
    {
        /* rt_sem_release 可在中断中直接调用，无需 FromISR 后缀 */
        rt_sem_release(button_sem);
    }
}

/* 按键处理线程 */
static void button_thread_entry(void *parameter)
{
    while (1)
    {
        /* 永久等待信号量 */
        rt_sem_take(button_sem, RT_WAITING_FOREVER);

        /* 消抖 */
        rt_thread_mdelay(20);

        /* 处理按键 */
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        rt_kprintf("Button pressed!\n");
    }
}
```

### 6.3 互斥量：保护串口共享资源

```c
static rt_mutex_t uart_mutex;

/* 初始化 */
uart_mutex = rt_mutex_create("uart_mtx", RT_IPC_FLAG_PRIO);

/* 线程安全的串口打印 */
void uart_print_safe(const char *str)
{
    if (rt_mutex_take(uart_mutex, rt_tick_from_millisecond(100)) == RT_EOK)
    {
        HAL_UART_Transmit(&huart1, (uint8_t *)str, strlen(str), 200);
        rt_mutex_release(uart_mutex);
    }
}
```

### 6.4 事件集：多条件同步

```c
static rt_event_t sys_event;

#define EVT_ADC_DONE    (1 << 0)    /* ADC 采集完成 */
#define EVT_UART_RECV   (1 << 1)    /* 串口收到数据 */
#define EVT_BUTTON      (1 << 2)    /* 按键按下 */

/* 初始化 */
sys_event = rt_event_create("sys_evt", RT_IPC_FLAG_PRIO);

/* 在各自的中断/线程中发送事件 */
rt_event_send(sys_event, EVT_ADC_DONE);

/* 等待任意一个事件（OR 逻辑） */
static void monitor_thread_entry(void *parameter)
{
    rt_uint32_t recv_evt;

    while (1)
    {
        rt_event_recv(sys_event,
                      EVT_ADC_DONE | EVT_UART_RECV | EVT_BUTTON,
                      RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                      RT_WAITING_FOREVER,
                      &recv_evt);

        if (recv_evt & EVT_ADC_DONE)  rt_kprintf("ADC done\n");
        if (recv_evt & EVT_UART_RECV) rt_kprintf("UART received\n");
        if (recv_evt & EVT_BUTTON)    rt_kprintf("Button pressed\n");
    }
}
```


---

## 7. 中断与 RT-Thread 协作

### 7.1 RT-Thread 中断使用规则

RT-Thread 在中断中调用 API 比 FreeRTOS 更简单：**不需要 `FromISR` 后缀**，直接调用释放/发送类函数即可。

```c
/* ✅ RT-Thread：中断中直接调用，内部自动处理 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    rt_sem_release(uart_rx_sem);        /* 直接调用，无需 FromISR */
    rt_mq_send(uart_mq, &rx_byte, 1);  /* 直接调用，无需 FromISR */
    rt_event_send(sys_event, EVT_UART_RECV);
}

/* ❌ 中断中不能调用阻塞等待类函数 */
void SomeIRQHandler(void)
{
    rt_sem_take(sem, RT_WAITING_FOREVER);  /* 错误！中断中不能阻塞 */
    rt_mq_recv(mq, &buf, 4, 100);         /* 错误！中断中不能阻塞 */
}
```

### 7.2 UART 接收中断 → 消息队列

```c
static rt_mq_t uart_rx_mq;
static uint8_t rx_byte;

/* 初始化 */
uart_rx_mq = rt_mq_create("uart_rx", 1, 64, RT_IPC_FLAG_FIFO);
HAL_UART_Receive_IT(&huart1, &rx_byte, 1);

/* 接收完成回调 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        rt_mq_send(uart_rx_mq, &rx_byte, 1);
        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);  /* 重新开启接收 */
    }
}

/* 串口处理线程 */
static void uart_thread_entry(void *parameter)
{
    uint8_t byte;
    while (1)
    {
        rt_mq_recv(uart_rx_mq, &byte, 1, RT_WAITING_FOREVER);
        /* 处理接收到的字节 */
        rt_kprintf("RX: 0x%02X\n", byte);
    }
}
```

### 7.3 中断优先级配置

RT-Thread 对中断优先级的要求与 FreeRTOS 类似，但配置更简单：

```c
/* RT-Thread 内核中断（SysTick、PendSV、SVC）使用最低优先级 */
/* 用户中断优先级可以高于内核中断，但中断中只能调用非阻塞 API */

/* 推荐配置：用户中断优先级 1~14，内核相关中断优先级 15 */
HAL_NVIC_SetPriority(USART1_IRQn, 6, 0);   /* 串口中断 */
HAL_NVIC_SetPriority(EXTI0_IRQn,  5, 0);   /* 外部中断 */
HAL_NVIC_SetPriority(TIM2_IRQn,   7, 0);   /* 定时器中断 */
```

---

## 8. FinSH Shell 调试配置

FinSH 是 RT-Thread 内置的命令行调试工具，通过串口输入命令查看系统状态，在开发阶段非常有用。

### 8.1 开启 FinSH

在 `rtconfig.h` 中取消注释：

```c
#define RT_USING_FINSH
#define FINSH_USING_MSH
#define FINSH_THREAD_STACK_SIZE  1024   /* FinSH 线程栈，至少 1KB */
```

### 8.2 实现串口输入（FinSH 需要）

FinSH 需要能读取串口输入，在 `board.c` 中实现：

```c
/* FinSH 从串口读取字符 */
char rt_hw_console_getchar(void)
{
    uint8_t ch = 0;
    /* 非阻塞读取，无数据时返回 -1 */
    if (HAL_UART_Receive(&huart1, &ch, 1, 0) == HAL_OK)
        return ch;
    return -1;
}
```

### 8.3 常用 FinSH 命令

通过串口监视器（115200 波特率）连接后，输入以下命令：

| 命令 | 说明 |
|------|------|
| `list_thread` | 查看所有线程状态、优先级、栈使用情况 |
| `list_sem` | 查看所有信号量 |
| `list_mutex` | 查看所有互斥量 |
| `list_mq` | 查看所有消息队列 |
| `list_timer` | 查看所有定时器 |
| `free` | 查看内存使用情况 |
| `ps` | 类 Linux 进程列表 |
| `version` | 查看 RT-Thread 版本 |

`list_thread` 输出示例：

```
thread   pri  status      sp     stack size max used left tick  error
-------- ---  ------- ---------- ----------  ------  ---------- ---
led       10  suspend 0x000000a0 0x00000200    26%   0x00000014 000
log       15  suspend 0x000000b0 0x00000200    28%   0x000003e8 000
tidle      31  ready   0x00000060 0x00000100    38%   0x00000020 000
```


---

## 9. 常见问题

### Q1：编译报错 `undefined reference to rt_hw_board_init`

**原因：** RT-Thread 内核在启动时会调用 `rt_hw_board_init()`，但工程中没有实现这个函数。

**解决：** 确认 `src/board.c` 已创建并包含 `rt_hw_board_init()` 实现，且该文件已被 PIO 编译（在 `src/` 目录下会自动编译）。

### Q2：系统启动后卡死，串口无输出

**原因 1：** `rtconfig.h` 中 `RT_HEAP_SIZE` 超过可用 RAM。STM32F103C8T6 只有 20KB SRAM，堆建议不超过 8KB。

**解决：** 将 `RT_HEAP_SIZE` 改为 `(4 * 1024)` 或 `(6 * 1024)`，用 `rt_memory_info()` 监控剩余。

**原因 2：** `rt_hw_console_output()` 未实现或串口未初始化。

**解决：** 确认 `board.c` 中 `rt_hw_console_output()` 已实现，且 `MX_USART1_Init()` 在 `rt_hw_board_init()` 中被调用。

### Q3：`rt_thread_create` 返回 `RT_NULL`

**原因：** 堆空间不足，无法为线程分配栈内存。

**解决：** 减小线程栈大小（从 1024 改为 512 字节），或增大 `RT_HEAP_SIZE`，或减少线程数量。

### Q4：PIO 编译时找不到 `rtconfig.h`

**原因：** `rtconfig.h` 放置位置不在编译搜索路径中。

**解决：** 确认 `platformio.ini` 的 `build_flags` 中包含 `-Iinclude`，且 `rtconfig.h` 在工程 `include/` 目录下。

### Q5：`SysTick_Handler` 重复定义链接错误

**原因：** STM32Cube 框架的 `stm32f1xx_it.c` 中已有 `SysTick_Handler`，与 `board.c` 中的定义冲突。

**解决：** 删除或注释掉 `stm32f1xx_it.c` 中的 `SysTick_Handler` 函数，只保留 `board.c` 中的版本。

### Q6：线程优先级与 FreeRTOS 方向相反导致混淆

RT-Thread 优先级数值越小越高（0 最高），与 FreeRTOS 相反。

```c
/* RT-Thread：数值小 = 优先级高 */
rt_thread_create("high", fn, RT_NULL, 512, 3, 20);   /* 高优先级 */
rt_thread_create("low",  fn, RT_NULL, 512, 20, 20);  /* 低优先级 */

/* FreeRTOS：数值大 = 优先级高（对比参考） */
xTaskCreate(fn, "high", 128, NULL, 5, NULL);   /* 高优先级 */
xTaskCreate(fn, "low",  128, NULL, 1, NULL);   /* 低优先级 */
```

### Q7：栈大小单位与 FreeRTOS 不同

```c
/* RT-Thread：栈大小单位是字节 */
rt_thread_create("t", fn, RT_NULL, 512, 10, 20);   /* 512 字节 */

/* FreeRTOS：栈大小单位是 words（1 word = 4 字节） */
xTaskCreate(fn, "t", 128, NULL, 2, NULL);           /* 128 words = 512 字节 */
```


---

## 10. platformio.ini 完整模板

```ini
; ============================================================
; STM32F103C8T6 + RT-Thread Nano - PlatformIO 工程配置模板
; ============================================================
[env:bluepill_f103c8]
platform  = ststm32
board     = bluepill_f103c8
framework = stm32cube

; 烧录 & 调试
upload_protocol = stlink
debug_tool      = stlink
debug_init_break = tbreak main

; 串口监视器
monitor_speed   = 115200
monitor_filters = direct

; 编译标志
build_flags =
    -DUSE_HAL_DRIVER
    -DSTM32F103xB
    ; RT-Thread Nano 头文件路径（手动集成方式）
    -Ilib/rtthread/include
    -Ilib/rtthread/libcpu/arm/cortex-m3
    ; 用户配置文件目录（rtconfig.h 放在 include/ 下）
    -Iinclude
```

### 工程目录结构

```
my_rtthread_project/
├── include/
│   └── rtconfig.h              ← 必须手动创建，见第 3 节
├── src/
│   ├── main.c                  ← 主程序，见第 5 节
│   └── board.c                 ← 硬件适配层，见第 4 节
├── lib/
│   └── rtthread/               ← RT-Thread Nano 源码
│       ├── include/
│       ├── src/
│       └── libcpu/arm/cortex-m3/
└── platformio.ini
```

### RT-Thread vs FreeRTOS 在 PIO 下的对比

| 对比项 | RT-Thread Nano | FreeRTOS |
|--------|---------------|----------|
| PIO 库支持 | 无官方包，需手动集成 | `feilipu/FreeRTOS-Kernel` 可用 |
| 集成复杂度 | 较高（需 board.c 适配） | 较低（lib_deps 自动下载） |
| 中断中调用 API | 直接调用，无需 FromISR | 必须用 `xxxFromISR` 系列 |
| 优先级方向 | 数值小 = 优先级高 | 数值大 = 优先级高 |
| 栈大小单位 | 字节 | Words（×4 = 字节） |
| 调试工具 | 内置 FinSH Shell | 无内置，需第三方 |
| 中文文档 | 丰富 | 较少 |
| 适合场景 | 需要 Shell 调试、国产化要求 | 快速上手、生态丰富 |

---

*文档版本：v1.0 | 适用 PlatformIO Core 6.x + RT-Thread Nano 4.x | 最后更新：2026-05*

> 相关文档：
> - [RTThread_STM32_指南.md](RTThread_STM32_指南.md) — CubeMX 方式集成 RT-Thread
> - [RTThread_Thread.md](RTThread_Thread.md) — 线程详细 API
> - [RTThread_MessageQueue.md](RTThread_MessageQueue.md) — 消息队列详细 API
> - [STM32F103C8T6_PIO_Tutorial.md](../HAL/STM32F103C8T6_PIO_Tutorial.md) — PIO 基础教程
> - [FreeRTOS_PIO_STM32F103_指南.md](../FreeRTOS/FreeRTOS_PIO_STM32F103_指南.md) — PIO + FreeRTOS 对比参考
