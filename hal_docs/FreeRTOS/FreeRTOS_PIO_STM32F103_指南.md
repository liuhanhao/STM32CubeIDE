# 使用 PlatformIO 在 STM32F103C8T6 上集成 FreeRTOS

> 本指南介绍如何在 **PlatformIO + STM32Cube 框架**下为 STM32F103C8T6 集成 FreeRTOS，无需 STM32CubeMX，直接在 `platformio.ini` 中配置并手写初始化代码。

---

## 目录

1. [工程创建与配置](#1-工程创建与配置)
2. [FreeRTOS 源码集成方式](#2-freertos-源码集成方式)
3. [FreeRTOSConfig.h 配置](#3-freertosconfigh-配置)
4. [启动文件与时基配置](#4-启动文件与时基配置)
5. [第一个 FreeRTOS 工程：多任务 Blink](#5-第一个-freertos-工程多任务-blink)
6. [任务间通信示例](#6-任务间通信示例)
7. [中断与 FreeRTOS 协作](#7-中断与-freertos-协作)
8. [常见问题](#8-常见问题)
9. [platformio.ini 完整模板](#9-platformioini-完整模板)

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

> **为什么用 stm32cube 框架而不是 arduino？**
> FreeRTOS 需要直接操作 SysTick、PendSV、SVC 中断向量，arduino 框架对这些有封装，容易冲突。stm32cube 框架直接暴露 HAL 层，与 FreeRTOS 集成更干净。

### 1.2 两种集成方式对比

| 方式 | 说明 | 推荐场景 |
|------|------|---------|
| **方式 A：lib_deps 引入**（推荐） | PIO 自动下载 FreeRTOS 库 | 快速上手，版本管理方便 |
| **方式 B：手动拷贝源码** | 将 FreeRTOS 源码放入 `lib/` | 需要深度定制内核配置 |


---

## 2. FreeRTOS 源码集成方式

### 方式 A：lib_deps 自动引入（推荐）

在 `platformio.ini` 中添加依赖：

```ini
[env:bluepill_f103c8]
platform  = ststm32
board     = bluepill_f103c8
framework = stm32cube

upload_protocol = stlink
debug_tool      = stlink
monitor_speed   = 115200

lib_deps =
    feilipu/FreeRTOS-Kernel @ ^11.1.0

build_flags =
    -IFreeRTOS/Source/include
    -IFreeRTOS/Source/portable/GCC/ARM_CM3
```

> `feilipu/FreeRTOS-Kernel` 是 PIO Registry 上维护最活跃的 FreeRTOS 移植包，已包含 Cortex-M3 的 `port.c` 和 `portmacro.h`。

执行 `pio run` 后 PIO 会自动下载到 `.pio/libdeps/` 目录。

### 方式 B：手动拷贝源码

从官方仓库下载 FreeRTOS，将以下文件复制到工程 `lib/FreeRTOS/` 目录：

```
lib/FreeRTOS/
├── src/
│   ├── tasks.c
│   ├── queue.c
│   ├── list.c
│   ├── timers.c
│   ├── event_groups.c
│   ├── stream_buffer.c
│   └── portable/
│       ├── GCC/ARM_CM3/
│       │   └── port.c          ← Cortex-M3 移植层
│       └── MemMang/
│           └── heap_4.c        ← 推荐使用 heap_4
└── include/
    ├── FreeRTOS.h
    ├── task.h
    ├── queue.h
    ├── semphr.h
    ├── timers.h
    ├── event_groups.h
    ├── portable.h
    └── portmacro.h             ← 从 portable/GCC/ARM_CM3/ 复制过来
```

在 `platformio.ini` 中添加头文件路径：

```ini
build_flags =
    -Ilib/FreeRTOS/include
    -Ilib/FreeRTOS/src/portable/GCC/ARM_CM3
    -DUSE_HAL_DRIVER
    -DSTM32F103xB
```


---

## 3. FreeRTOSConfig.h 配置

在 `include/` 目录下创建 `FreeRTOSConfig.h`，这是 FreeRTOS 的核心配置文件：

```c
/* include/FreeRTOSConfig.h */
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "stm32f1xx_hal.h"   /* 引入 SystemCoreClock */

/* ── 基础配置 ─────────────────────────────────────────── */
#define configUSE_PREEMPTION                    1   /* 抢占式调度 */
#define configUSE_IDLE_HOOK                     0   /* 空闲钩子 */
#define configUSE_TICK_HOOK                     0   /* 节拍钩子 */
#define configCPU_CLOCK_HZ                      ( SystemCoreClock )
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 )  /* 1ms/tick */
#define configMAX_PRIORITIES                    ( 7 )   /* 用户优先级 1~6 */
#define configMINIMAL_STACK_SIZE                ( ( uint16_t ) 128 )     /* 最小栈 128 words */
#define configTOTAL_HEAP_SIZE                   ( ( size_t ) ( 10 * 1024 ) ) /* 10KB 堆 */
#define configMAX_TASK_NAME_LEN                 ( 16 )
#define configUSE_TRACE_FACILITY                0
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1

/* ── 互斥量与信号量 ───────────────────────────────────── */
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1

/* ── 软件定时器 ───────────────────────────────────────── */
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               ( 2 )
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            ( configMINIMAL_STACK_SIZE * 2 )

/* ── 任务通知 ─────────────────────────────────────────── */
#define configUSE_TASK_NOTIFICATIONS            1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES   1

/* ── 调试与检测 ───────────────────────────────────────── */
#define configCHECK_FOR_STACK_OVERFLOW          2   /* 栈溢出检测，开发阶段开启 */
#define configUSE_MALLOC_FAILED_HOOK            1   /* 内存分配失败钩子 */
#define configUSE_STATS_FORMATTING_FUNCTIONS    0

/* ── 中断优先级配置（Cortex-M3 关键配置）─────────────── */
/* STM32F103 使用 4 位优先级，configMAX_SYSCALL_INTERRUPT_PRIORITY
   设为 5（十进制），即优先级 5~15 的中断可调用 FreeRTOS FromISR API
   优先级 0~4 的中断不能调用任何 FreeRTOS API                        */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5
#define configKERNEL_INTERRUPT_PRIORITY         ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - 4) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - 4) )

/* ── API 函数开关 ─────────────────────────────────────── */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_xResumeFromISR                  1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_xTimerPendFunctionCall          1

/* ── 将 FreeRTOS 中断处理函数映射到 CMSIS 名称 ─────────── */
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

#endif /* FREERTOS_CONFIG_H */
```

> **关键点：** 最后三行宏定义将 FreeRTOS 的中断处理函数名映射到 STM32 HAL 的 CMSIS 标准名称，避免重复定义链接错误。


---

## 4. 启动文件与时基配置

### 4.1 解决 SysTick 冲突

FreeRTOS 使用 SysTick 作为节拍源，而 HAL 库默认也用 SysTick 驱动 `HAL_Delay()`。两者同时使用会冲突，必须将 HAL 时基切换到其他定时器。

**在 `src/main.c` 中重写 HAL 时基初始化：**

```c
/* src/main.c */
#include "stm32f1xx_hal.h"

/* 将 HAL 时基切换到 TIM4，让 SysTick 完全归 FreeRTOS 使用 */
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    RCC_ClkInitTypeDef clkconfig;
    uint32_t uwTimclock, uwAPB1Prescaler;
    uint32_t uwPrescalerValue;
    uint32_t pFLatency;

    /* 使能 TIM4 时钟 */
    __HAL_RCC_TIM4_CLK_ENABLE();

    /* 获取 APB1 时钟频率 */
    HAL_RCC_GetClockConfig(&clkconfig, &pFLatency);
    uwAPB1Prescaler = clkconfig.APB1CLKDivider;

    if (uwAPB1Prescaler == RCC_HCLK_DIV1)
        uwTimclock = HAL_RCC_GetPCLK1Freq();
    else
        uwTimclock = 2 * HAL_RCC_GetPCLK1Freq();  /* APB1 有分频时 TIM 时钟 ×2 */

    /* 配置 TIM4 产生 1ms 中断 */
    uwPrescalerValue = (uint32_t)((uwTimclock / 1000000U) - 1U);

    extern TIM_HandleTypeDef htim4;
    htim4.Instance               = TIM4;
    htim4.Init.Period            = (1000000U / 1000U) - 1U;  /* 1ms */
    htim4.Init.Prescaler         = uwPrescalerValue;
    htim4.Init.ClockDivision     = 0;
    htim4.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    HAL_TIM_Base_Init(&htim4);

    /* 配置 TIM4 中断优先级（必须低于 configMAX_SYSCALL_INTERRUPT_PRIORITY） */
    HAL_NVIC_SetPriority(TIM4_IRQn, TickPriority, 0U);
    HAL_NVIC_EnableIRQ(TIM4_IRQn);
    HAL_TIM_Base_Start_IT(&htim4);

    return HAL_OK;
}

/* TIM4 中断处理函数 */
void TIM4_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim4);
}

/* HAL 时基回调 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM4)
        HAL_IncTick();
}
```

> **简化方案：** 如果不需要在 FreeRTOS 运行期间使用 `HAL_Delay()`，可以跳过上述配置，直接让 SysTick 归 FreeRTOS 独占。FreeRTOS 启动后 `HAL_Delay()` 会失效，改用 `vTaskDelay()` 即可。

### 4.2 NVIC 优先级分组

FreeRTOS 要求使用 **4 位全抢占优先级**（无子优先级），在 `main()` 初始化时设置：

```c
/* 必须在 HAL_Init() 之后、FreeRTOS 启动之前调用 */
HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
```


---

## 5. 第一个 FreeRTOS 工程：多任务 Blink

完整的 `src/main.c` 示例，两个任务分别以不同频率闪烁 LED 并输出串口日志：

```c
/* src/main.c */
#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* ── 外设句柄 ─────────────────────────────────────────── */
UART_HandleTypeDef huart1;
TIM_HandleTypeDef  htim4;   /* HAL 时基用 */

/* ── 函数声明 ─────────────────────────────────────────── */
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_Init(void);

/* ── 任务函数声明 ─────────────────────────────────────── */
void vLedFastTask(void *pvParameters);
void vLedSlowTask(void *pvParameters);

/* ── 调试钩子函数 ─────────────────────────────────────── */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    /* 栈溢出时在这里打断点或输出错误信息 */
    (void)xTask;
    (void)pcTaskName;
    taskDISABLE_INTERRUPTS();
    for (;;);
}

void vApplicationMallocFailedHook(void)
{
    /* 堆内存不足时触发 */
    taskDISABLE_INTERRUPTS();
    for (;;);
}

/* ── 主函数 ───────────────────────────────────────────── */
int main(void)
{
    HAL_Init();
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);  /* FreeRTOS 要求 */
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_Init();

    /* 创建任务 */
    xTaskCreate(vLedFastTask, "LedFast", 128, NULL, 2, NULL);
    xTaskCreate(vLedSlowTask, "LedSlow", 128, NULL, 1, NULL);

    /* 启动调度器，此后不再返回 */
    vTaskStartScheduler();

    /* 正常情况下不会到达这里（堆不足时会到这里） */
    for (;;);
}

/* ── 任务实现 ─────────────────────────────────────────── */

/* 快速任务：每 200ms 翻转 LED，同时输出串口日志 */
void vLedFastTask(void *pvParameters)
{
    (void)pvParameters;
    char msg[] = "Fast task tick\r\n";

    for (;;)
    {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        HAL_UART_Transmit(&huart1, (uint8_t *)msg, sizeof(msg) - 1, 100);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

/* 慢速任务：每 1000ms 输出任务统计信息 */
void vLedSlowTask(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        /* 查询剩余堆大小 */
        size_t freeHeap = xPortGetFreeHeapSize();
        char buf[64];
        int len = snprintf(buf, sizeof(buf), "Free heap: %u bytes\r\n", (unsigned)freeHeap);
        HAL_UART_Transmit(&huart1, (uint8_t *)buf, len, 100);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* ── 外设初始化 ───────────────────────────────────────── */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL     = RCC_PLL_MUL9;   /* 8MHz × 9 = 72MHz */
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = GPIO_PIN_13;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);  /* 初始熄灭 */
}

static void MX_USART1_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    /* PA9 → TX */
    GPIO_InitStruct.Pin   = GPIO_PIN_9;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    /* PA10 → RX */
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
```


---

## 6. 任务间通信示例

### 6.1 队列：传感器数据流

```c
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

typedef struct {
    uint16_t adc_value;
    uint32_t timestamp;
} SensorData_t;

static QueueHandle_t xSensorQueue;

/* 采集任务（高优先级） */
void vSensorTask(void *pvParameters)
{
    (void)pvParameters;
    SensorData_t data;

    for (;;)
    {
        /* 模拟 ADC 采集 */
        data.adc_value = HAL_ADC_GetValue(&hadc1);
        data.timestamp = xTaskGetTickCount();

        /* 发送到队列，等待最多 10ms */
        xQueueSend(xSensorQueue, &data, pdMS_TO_TICKS(10));

        vTaskDelay(pdMS_TO_TICKS(50));  /* 20Hz 采集 */
    }
}

/* 处理任务（低优先级） */
void vProcessTask(void *pvParameters)
{
    (void)pvParameters;
    SensorData_t received;

    for (;;)
    {
        /* 阻塞等待数据 */
        if (xQueueReceive(xSensorQueue, &received, portMAX_DELAY) == pdTRUE)
        {
            float voltage = received.adc_value * 3.3f / 4095.0f;
            /* 处理 voltage... */
        }
    }
}

/* 在 main() 中创建 */
void app_init(void)
{
    xSensorQueue = xQueueCreate(10, sizeof(SensorData_t));
    configASSERT(xSensorQueue != NULL);

    xTaskCreate(vSensorTask,  "Sensor",  256, NULL, 3, NULL);
    xTaskCreate(vProcessTask, "Process", 256, NULL, 2, NULL);
}
```

### 6.2 二值信号量：按键中断通知任务

```c
#include "semphr.h"

static SemaphoreHandle_t xButtonSem;

/* 在 main() 初始化时创建 */
xButtonSem = xSemaphoreCreateBinary();

/* GPIO 中断回调（在 stm32f1xx_it.c 中实现） */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0)  /* PA0 按键 */
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(xButtonSem, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

/* 按键处理任务 */
void vButtonTask(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        /* 永久等待按键中断 */
        xSemaphoreTake(xButtonSem, portMAX_DELAY);

        /* 消抖延时 */
        vTaskDelay(pdMS_TO_TICKS(20));

        /* 处理按键逻辑 */
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    }
}
```

### 6.3 互斥量：保护串口共享资源

```c
#include "semphr.h"

static SemaphoreHandle_t xUartMutex;

/* 初始化 */
xUartMutex = xSemaphoreCreateMutex();

/* 线程安全的串口打印函数 */
void uart_print_safe(const char *str)
{
    if (xSemaphoreTake(xUartMutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        HAL_UART_Transmit(&huart1, (uint8_t *)str, strlen(str), 200);
        xSemaphoreGive(xUartMutex);
    }
}
```


---

## 7. 中断与 FreeRTOS 协作

### 7.1 UART 接收中断 → 队列

```c
/* 在 stm32f1xx_it.c 或 main.c 中 */
static uint8_t rx_byte;
extern QueueHandle_t xUartRxQueue;

/* 启动接收（在初始化时调用） */
HAL_UART_Receive_IT(&huart1, &rx_byte, 1);

/* 接收完成回调 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xQueueSendFromISR(xUartRxQueue, &rx_byte, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

        /* 重新开启接收 */
        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    }
}
```

### 7.2 中断优先级设置规则

```c
/* 可以调用 FreeRTOS FromISR API 的中断：优先级必须 >= 5 */
HAL_NVIC_SetPriority(USART1_IRQn, 6, 0);   /* ✅ 优先级 6，可用 FromISR */
HAL_NVIC_SetPriority(TIM2_IRQn,   5, 0);   /* ✅ 优先级 5，边界值，可用 */

/* 不能调用任何 FreeRTOS API 的中断：优先级 0~4 */
HAL_NVIC_SetPriority(EXTI0_IRQn,  2, 0);   /* ❌ 优先级 2，不能用 FromISR */
```

> **记忆方法：** `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY = 5`，优先级数值 **≥ 5** 的中断才能调用 FreeRTOS API。数值越小优先级越高，所以高优先级中断（0~4）反而不能用 FreeRTOS API。

---

## 8. 常见问题

### Q1：编译报错 `multiple definition of SysTick_Handler`

**原因：** FreeRTOS 的 `port.c` 定义了 `xPortSysTickHandler`，而 `FreeRTOSConfig.h` 中的宏 `#define xPortSysTickHandler SysTick_Handler` 与 HAL 库的 `SysTick_Handler` 冲突。

**解决：** 确认 `stm32f1xx_it.c` 中没有单独定义 `SysTick_Handler`，或者删除 `stm32f1xx_it.c` 中的该函数（让 FreeRTOS 的版本接管）。

### Q2：系统启动后立即进入 HardFault

**原因 1：** `FreeRTOSConfig.h` 中 `configTOTAL_HEAP_SIZE` 超过了芯片实际 RAM。STM32F103C8T6 只有 20KB SRAM，堆不能超过 ~14KB（需留给栈和全局变量）。

**解决：** 将 `configTOTAL_HEAP_SIZE` 改为 `(8 * 1024)` 或更小，用 `xPortGetFreeHeapSize()` 监控剩余。

**原因 2：** NVIC 优先级分组未设置为 `NVIC_PRIORITYGROUP_4`。

**解决：** 在 `HAL_Init()` 之后立即调用 `HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4)`。

### Q3：`vTaskStartScheduler()` 之后程序不运行

**原因：** 堆空间不足，调度器无法创建空闲任务。

**解决：** 减小 `configTOTAL_HEAP_SIZE` 或减少已创建任务的栈大小，确保有足够空间给空闲任务（至少 `configMINIMAL_STACK_SIZE` words）。

### Q4：`HAL_Delay()` 在 FreeRTOS 任务中不工作

**原因：** FreeRTOS 接管了 SysTick，`HAL_Delay()` 依赖 SysTick 计数，调度器运行后 SysTick 的行为改变。

**解决：** 任务中一律使用 `vTaskDelay(pdMS_TO_TICKS(ms))`。如果必须用 `HAL_Delay()`（如在初始化阶段），按第 4 节将 HAL 时基切换到 TIM4。

### Q5：PIO 编译时找不到 `portmacro.h`

**解决：** 在 `platformio.ini` 的 `build_flags` 中添加 Cortex-M3 移植层路径：

```ini
build_flags =
    -I.pio/libdeps/bluepill_f103c8/FreeRTOS-Kernel/include
    -I.pio/libdeps/bluepill_f103c8/FreeRTOS-Kernel/portable/GCC/ARM_CM3
```

或者使用方式 B 手动管理源码，路径更可控。


---

## 9. platformio.ini 完整模板

```ini
; ============================================================
; STM32F103C8T6 + FreeRTOS - PlatformIO 工程配置模板
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

; FreeRTOS 库（方式 A：自动下载）
lib_deps =
    feilipu/FreeRTOS-Kernel @ ^11.1.0

; 编译标志
build_flags =
    -DUSE_HAL_DRIVER
    -DSTM32F103xB
    ; FreeRTOS 头文件路径（根据实际 libdeps 路径调整）
    -I.pio/libdeps/bluepill_f103c8/FreeRTOS-Kernel/include
    -I.pio/libdeps/bluepill_f103c8/FreeRTOS-Kernel/portable/GCC/ARM_CM3
    ; 用户头文件目录（FreeRTOSConfig.h 放在 include/ 下）
    -Iinclude
```

### 工程目录结构

```
my_freertos_project/
├── include/
│   └── FreeRTOSConfig.h        ← 必须手动创建，见第 3 节
├── src/
│   └── main.c                  ← 主程序，见第 5 节
├── lib/                        ← 方式 B 时放 FreeRTOS 源码
└── platformio.ini
```

### 与 STM32CubeMX 方式的对比

| 对比项 | PIO 方式 | CubeMX 方式 |
|--------|---------|------------|
| 工程创建 | 手写 `platformio.ini` | 图形化配置 |
| FreeRTOS 集成 | `lib_deps` 或手动拷贝 | CubeMX 自动生成 |
| `FreeRTOSConfig.h` | 手动编写 | CubeMX 自动生成 |
| 时基冲突处理 | 手动重写 `HAL_InitTick` | CubeMX 自动切换 TIM |
| 代码生成 | 无，完全手写 | 自动生成初始化桩代码 |
| 灵活性 | 高，完全可控 | 中，受 CubeMX 约束 |
| 上手难度 | 较高 | 较低 |

---

*文档版本：v1.0 | 适用 PlatformIO Core 6.x + FreeRTOS Kernel 11.x | 最后更新：2026-05*

> 相关文档：
> - [FreeRTOS_STM32_指南.md](FreeRTOS_STM32_指南.md) — CubeMX 方式集成 FreeRTOS
> - [FreeRTOS_Task.md](FreeRTOS_Task.md) — 任务详细 API
> - [FreeRTOS_Queue.md](FreeRTOS_Queue.md) — 队列详细 API
> - [STM32F103C8T6_PIO_Tutorial.md](../HAL/STM32F103C8T6_PIO_Tutorial.md) — PIO 基础教程
