# UART 串口模块 API 文档（ESP-IDF）

## 📋 模块概述

ESP-IDF 的 UART 驱动提供全双工异步串口通信，包括：
- 可配置波特率、数据位、停止位、校验位
- 硬件 FIFO（128 字节 TX/RX）
- 支持阻塞、中断、DMA 三种传输模式
- 硬件流控（RTS/CTS）
- RS485 半双工模式
- UART 事件队列（接收数据、FIFO 溢出、帧错误等）

---

## 🗺️ 使用场景速查

**三种传输模式怎么选**

| 场景 | 推荐方式 | 说明 |
|------|----------|------|
| 调试打印、偶发少量数据 | `uart_write_bytes` 阻塞发送 | 简单直接 |
| 不定长数据接收（命令行、AT 指令） | 事件队列 + `UART_DATA` 事件 | 最灵活 |
| 大量数据传输（日志、固件升级） | DMA 模式 | CPU 占用低 |
| GPS / 蓝牙模块连续数据流 | 事件队列 + 空闲检测 | 自动帧分割 |

**常见项目场景**

| 项目 | 配置建议 |
|------|----------|
| 调试串口 / ESP_LOG 输出 | UART0，115200，默认配置 |
| 与上位机通信 | UART1/2，事件队列接收，`uart_write_bytes` 发送 |
| GPS 模块 | UART1，9600，事件队列 + `UART_DATA` 事件 |
| RS485 总线 | UART2，半双工模式，配置 DE 引脚 |

---

## 📚 相关文件

- **头文件**: `driver/uart.h`
- **源文件**: `driver/uart.c`（IDF 内部）

---

## 🔧 数据类型

### uart_config_t — UART 配置结构体

```c
typedef struct {
    int baud_rate;                    // 波特率
    uart_word_length_t data_bits;     // 数据位
    uart_parity_t parity;             // 校验位
    uart_stop_bits_t stop_bits;       // 停止位
    uart_hw_flowcontrol_t flow_ctrl;  // 硬件流控
    uint8_t rx_flow_ctrl_thresh;      // RX FIFO 触发 RTS 的阈值（0~127）
    union {
        uart_sclk_t source_clk;       // 时钟源（IDF 5.x）
    };
} uart_config_t;
```

**成员说明：**

| 成员 | 说明 | 常用值 |
|------|------|--------|
| `baud_rate` | 波特率 | 9600, 115200, 921600 |
| `data_bits` | 数据位 | `UART_DATA_8_BITS`（最常用） |
| `parity` | 校验 | `UART_PARITY_DISABLE`（最常用） |
| `stop_bits` | 停止位 | `UART_STOP_BITS_1`（最常用） |
| `flow_ctrl` | 流控 | `UART_HW_FLOWCTRL_DISABLE`（最常用） |
| `source_clk` | 时钟源 | `UART_SCLK_DEFAULT` |

---

### uart_event_t — UART 事件结构体

```c
typedef struct {
    uart_event_type_t type;  // 事件类型
    size_t size;             // 数据大小（UART_DATA 事件时有效）
    bool timeout_flag;       // 是否为超时触发
} uart_event_t;
```

**事件类型：**

| 事件 | 说明 |
|------|------|
| `UART_DATA` | 接收到数据 |
| `UART_BREAK` | 检测到 Break 信号 |
| `UART_BUFFER_FULL` | RX 环形缓冲区满 |
| `UART_FIFO_OVF` | RX FIFO 溢出 |
| `UART_FRAME_ERR` | 帧错误 |
| `UART_PARITY_ERR` | 校验错误 |
| `UART_DATA_BREAK` | 数据后跟 Break |
| `UART_PATTERN_DET` | 检测到特定字符序列 |

---

## 📌 常量定义

### UART 端口号

ESP32-S3 有 3 个 UART 端口：

| 端口 | 默认引脚 | 说明 |
|------|---------|------|
| `UART_NUM_0` | TX:GPIO43, RX:GPIO44 | 默认调试串口（ESP_LOG 输出） |
| `UART_NUM_1` | 可任意配置 | 用户串口 |
| `UART_NUM_2` | 可任意配置 | 用户串口 |

> ESP32-S3 的 UART 引脚可通过 GPIO Matrix 路由到任意 GPIO，不像 STM32 固定引脚。

---

## 🎯 核心函数

### 初始化函数

#### uart_param_config()

配置 UART 通信参数。

```c
esp_err_t uart_param_config(uart_port_t uart_num, const uart_config_t *uart_config);
```

---

#### uart_set_pin()

配置 UART 使用的 GPIO 引脚（可路由到任意 GPIO）。

```c
esp_err_t uart_set_pin(uart_port_t uart_num,
                        int tx_io_num,
                        int rx_io_num,
                        int rts_io_num,
                        int cts_io_num);
```

**参数：**
- `tx_io_num`: TX 引脚，不使用传 `UART_PIN_NO_CHANGE`
- `rx_io_num`: RX 引脚
- `rts_io_num`: RTS 引脚，不使用传 `UART_PIN_NO_CHANGE`
- `cts_io_num`: CTS 引脚，不使用传 `UART_PIN_NO_CHANGE`

---

#### uart_driver_install()

安装 UART 驱动，分配内部缓冲区和事件队列。

```c
esp_err_t uart_driver_install(uart_port_t uart_num,
                               int rx_buffer_size,
                               int tx_buffer_size,
                               int queue_size,
                               QueueHandle_t *uart_queue,
                               int intr_alloc_flags);
```

**参数：**
- `rx_buffer_size`: RX 环形缓冲区大小（建议 ≥ 256，必须 > FIFO 大小 128）
- `tx_buffer_size`: TX 环形缓冲区大小（0 = 不使用，发送阻塞直到完成）
- `queue_size`: 事件队列深度（0 = 不使用事件队列）
- `uart_queue`: 事件队列句柄输出（不需要时传 NULL）
- `intr_alloc_flags`: 通常传 `0`

---

#### uart_driver_delete()

卸载 UART 驱动，释放资源。

```c
esp_err_t uart_driver_delete(uart_port_t uart_num);
```

---

### 数据发送函数

#### uart_write_bytes()

发送数据（阻塞直到数据写入 TX 缓冲区）。

```c
int uart_write_bytes(uart_port_t uart_num, const void *src, size_t size);
```

**返回值：** 实际写入的字节数，失败返回 `-1`

**使用示例：**
```c
const char *msg = "Hello ESP32!\r\n";
uart_write_bytes(UART_NUM_1, msg, strlen(msg));
```

---

#### uart_write_bytes_with_break()

发送数据后附加 Break 信号（用于 LIN 总线等协议）。

```c
int uart_write_bytes_with_break(uart_port_t uart_num,
                                 const void *src,
                                 size_t size,
                                 int brk_len);
```

---

#### uart_wait_tx_done()

等待 TX FIFO 和移位寄存器全部发送完毕。

```c
esp_err_t uart_wait_tx_done(uart_port_t uart_num, TickType_t ticks_to_wait);
```

**使用示例：**
```c
uart_write_bytes(UART_NUM_1, data, len);
uart_wait_tx_done(UART_NUM_1, pdMS_TO_TICKS(100));  // 等待最多 100ms
```

---

### 数据接收函数

#### uart_read_bytes()

从 RX 缓冲区读取数据。

```c
int uart_read_bytes(uart_port_t uart_num,
                    void *buf,
                    uint32_t length,
                    TickType_t ticks_to_wait);
```

**参数：**
- `length`: 期望读取的字节数
- `ticks_to_wait`: 等待超时（`portMAX_DELAY` = 永久等待）

**返回值：** 实际读取的字节数

**使用示例：**
```c
uint8_t data[128];
int len = uart_read_bytes(UART_NUM_1, data, sizeof(data), pdMS_TO_TICKS(100));
if (len > 0) {
    ESP_LOGI("UART", "收到 %d 字节", len);
}
```

---

#### uart_get_buffered_data_len()

查询 RX 缓冲区中已有多少字节可读。

```c
esp_err_t uart_get_buffered_data_len(uart_port_t uart_num, size_t *size);
```

**使用示例：**
```c
size_t available;
uart_get_buffered_data_len(UART_NUM_1, &available);
if (available > 0) {
    // 有数据可读
}
```

---

#### uart_flush_input()

清空 RX 缓冲区（丢弃未读数据）。

```c
esp_err_t uart_flush_input(uart_port_t uart_num);
```

---

## 💡 完整应用示例

### 示例1：基础串口初始化与收发

```c
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "string.h"

#define UART_PORT   UART_NUM_1
#define UART_TX_PIN GPIO_NUM_17
#define UART_RX_PIN GPIO_NUM_18
#define BUF_SIZE    1024

static const char *TAG = "UART";

void uart_init(void)
{
    /* 1. 配置通信参数 */
    uart_config_t uart_cfg = {
        .baud_rate  = 115200,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_cfg));

    /* 2. 配置引脚 */
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT,
                                  UART_TX_PIN, UART_RX_PIN,
                                  UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    /* 3. 安装驱动（RX 缓冲区 1024，TX 缓冲区 0，不使用事件队列）*/
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, BUF_SIZE, 0, 0, NULL, 0));
}

void app_main(void)
{
    uart_init();

    uint8_t rx_buf[128];
    while (1) {
        /* 发送 */
        const char *msg = "Hello from ESP32!\r\n";
        uart_write_bytes(UART_PORT, msg, strlen(msg));

        /* 接收（等待 100ms）*/
        int len = uart_read_bytes(UART_PORT, rx_buf, sizeof(rx_buf) - 1,
                                   pdMS_TO_TICKS(100));
        if (len > 0) {
            rx_buf[len] = '\0';
            ESP_LOGI(TAG, "收到: %s", rx_buf);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

---

### 示例2：事件队列接收（推荐方式）

```c
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#define UART_PORT   UART_NUM_1
#define UART_TX_PIN GPIO_NUM_17
#define UART_RX_PIN GPIO_NUM_18
#define BUF_SIZE    1024
#define EVT_QUEUE_SIZE 20

static const char *TAG = "UART";
static QueueHandle_t uart_queue;

static void uart_event_task(void *arg)
{
    uart_event_t event;
    uint8_t *buf = malloc(BUF_SIZE);

    while (1) {
        /* 等待 UART 事件 */
        if (xQueueReceive(uart_queue, &event, portMAX_DELAY)) {
            switch (event.type) {
                case UART_DATA:
                    /* 有数据到来 */
                    int len = uart_read_bytes(UART_PORT, buf, event.size,
                                              pdMS_TO_TICKS(20));
                    buf[len] = '\0';
                    ESP_LOGI(TAG, "收到 %d 字节: %s", len, buf);
                    /* 回显 */
                    uart_write_bytes(UART_PORT, buf, len);
                    break;

                case UART_FIFO_OVF:
                    ESP_LOGW(TAG, "FIFO 溢出，清空缓冲区");
                    uart_flush_input(UART_PORT);
                    xQueueReset(uart_queue);
                    break;

                case UART_BUFFER_FULL:
                    ESP_LOGW(TAG, "RX 缓冲区满，清空");
                    uart_flush_input(UART_PORT);
                    xQueueReset(uart_queue);
                    break;

                case UART_FRAME_ERR:
                    ESP_LOGE(TAG, "帧错误");
                    break;

                default:
                    ESP_LOGD(TAG, "其他事件: %d", event.type);
                    break;
            }
        }
    }
    free(buf);
    vTaskDelete(NULL);
}

void app_main(void)
{
    uart_config_t cfg = {
        .baud_rate  = 115200,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_param_config(UART_PORT, &cfg);
    uart_set_pin(UART_PORT, UART_TX_PIN, UART_RX_PIN,
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    /* 安装驱动，创建事件队列 */
    uart_driver_install(UART_PORT, BUF_SIZE * 2, BUF_SIZE * 2,
                        EVT_QUEUE_SIZE, &uart_queue, 0);

    xTaskCreate(uart_event_task, "uart_event", 4096, NULL, 12, NULL);
}
```

---

### 示例3：printf 重定向到 UART1

```c
#include "driver/uart.h"
#include "esp_vfs_dev.h"
#include "esp_vfs.h"

void uart_printf_redirect_init(void)
{
    uart_config_t cfg = {
        .baud_rate  = 115200,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_param_config(UART_NUM_1, &cfg);
    uart_set_pin(UART_NUM_1, GPIO_NUM_17, GPIO_NUM_18,
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_1, 256, 0, 0, NULL, 0);

    /* 将 stdout/stderr 重定向到 UART1 */
    esp_vfs_dev_uart_use_driver(UART_NUM_1);
    esp_vfs_dev_uart_register();
    freopen("/dev/uart/1", "w", stdout);
    freopen("/dev/uart/1", "w", stderr);
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("printf 已重定向到 UART1\n");
}
```

---

### 示例4：RS485 半双工模式

```c
#include "driver/uart.h"

#define RS485_UART  UART_NUM_2
#define RS485_TX    GPIO_NUM_17
#define RS485_RX    GPIO_NUM_18
#define RS485_DE    GPIO_NUM_5   // 驱动使能引脚（高电平发送，低电平接收）

void rs485_init(void)
{
    uart_config_t cfg = {
        .baud_rate  = 9600,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_param_config(RS485_UART, &cfg);
    uart_set_pin(RS485_UART, RS485_TX, RS485_RX,
                 RS485_DE, UART_PIN_NO_CHANGE);  // RTS 引脚用作 DE
    uart_driver_install(RS485_UART, 256, 0, 0, NULL, 0);

    /* 配置为 RS485 半双工模式，RTS 自动控制 DE 引脚 */
    uart_set_mode(RS485_UART, UART_MODE_RS485_HALF_DUPLEX);
}
```

---

## ⚠️ 注意事项

### 1. RX 缓冲区大小

`rx_buffer_size` 必须大于硬件 FIFO 大小（128 字节），建议至少 256 字节。
高波特率或大数据量时适当增大（1024 或更大）。

### 2. TX 缓冲区

`tx_buffer_size = 0` 时，`uart_write_bytes` 会阻塞直到数据全部写入 FIFO。
设置非零值后，数据先写入 TX 缓冲区，后台异步发送，函数立即返回。

### 3. 事件队列溢出

如果事件处理不及时，事件队列会满，新事件被丢弃。
出现 `UART_BUFFER_FULL` 或 `UART_FIFO_OVF` 时，必须调用 `uart_flush_input` 清空缓冲区，
否则驱动进入异常状态。

### 4. 波特率精度

ESP32 的 UART 时钟源为 APB（80MHz），波特率误差通常 < 0.5%，无需担心。

### 5. UART0 默认用途

UART0 默认用于 ESP_LOG 输出和 IDF 监视器，不建议在应用中重新配置。
用户串口建议使用 UART1 或 UART2。

---

## 📖 相关文档

- [IDF_GPIO.md](IDF_GPIO.md) — GPIO 配置
- [IDF_SPI.md](IDF_SPI.md) — SPI 总线
- [IDF_I2C.md](IDF_I2C.md) — I2C 总线
- [README.md](README.md) — 模块导航
