# SPI 总线模块 API 文档（ESP-IDF）

## 📋 模块概述

ESP-IDF 的 SPI Master 驱动提供高速同步串行通信，包括：
- 全双工 4 线通信（SCK / MOSI / MISO / CS）
- 支持 DMA 传输，大数据量无需 CPU 介入
- 多设备共享同一 SPI 总线（每个设备独立 CS）
- 支持 SPI Mode 0~3（CPOL/CPHA 组合）
- 支持半双工（3 线）和双线/四线（QSPI）模式
- 事务队列，支持异步提交

---

## 🗺️ 使用场景速查

| 场景 | 推荐配置 |
|------|----------|
| TFT/OLED 刷屏 | DMA 模式，高时钟（40~80MHz），TX only |
| SPI Flash（W25Q） | DMA 模式，Mode 0，全双工 |
| SPI ADC（MCP3208） | 轮询模式，Mode 0，低速 |
| 两块 MCU 互联 | 全双工，DMA，事务队列 |
| 触摸屏（XPT2046） | 与 LCD 共用总线，独立 CS，低速 |

**SPI 总线选择（ESP32-S3）**

| 总线 | 说明 | 推荐用途 |
|------|------|---------|
| `SPI1_HOST` | 内部 Flash 使用，不可用 | — |
| `SPI2_HOST` | 通用 SPI，支持 DMA | 首选 |
| `SPI3_HOST` | 通用 SPI，支持 DMA | 第二路 SPI |

---

## 📚 相关文件

- **头文件**: `driver/spi_master.h`
- **源文件**: `driver/spi_master.c`（IDF 内部）

---

## 🔧 数据类型

### spi_bus_config_t — SPI 总线配置

```c
typedef struct {
    int mosi_io_num;      // MOSI 引脚（-1 = 不使用）
    int miso_io_num;      // MISO 引脚（-1 = 不使用）
    int sclk_io_num;      // SCK 引脚
    int quadwp_io_num;    // QSPI WP 引脚（-1 = 不使用）
    int quadhd_io_num;    // QSPI HD 引脚（-1 = 不使用）
    int data4_io_num;     // 8 线模式（-1 = 不使用）
    int data5_io_num;
    int data6_io_num;
    int data7_io_num;
    int max_transfer_sz;  // 单次最大传输字节数（0 = 默认 4096）
    uint32_t flags;       // 总线标志
    int intr_flags;       // 中断标志
} spi_bus_config_t;
```

---

### spi_device_interface_config_t — SPI 设备配置

```c
typedef struct {
    uint8_t command_bits;     // 命令阶段位数（0 = 不使用）
    uint8_t address_bits;     // 地址阶段位数（0 = 不使用）
    uint8_t dummy_bits;       // 哑元位数（读操作时用）
    uint8_t mode;             // SPI 模式（0~3）
    uint16_t duty_cycle_pos;  // 时钟占空比（0 = 默认 128/256）
    uint16_t cs_ena_pretrans; // CS 拉低到第一个时钟的延迟（半个 SCK 周期数）
    uint8_t cs_ena_posttrans; // 最后一个时钟到 CS 拉高的延迟
    int clock_speed_hz;       // 时钟频率（Hz）
    int input_delay_ns;       // MISO 输入延迟（ns），影响最高速度
    int spics_io_num;         // CS 引脚（-1 = 软件控制）
    uint32_t flags;           // 设备标志
    int queue_size;           // 事务队列深度
    transaction_cb_t pre_cb;  // 事务开始前回调
    transaction_cb_t post_cb; // 事务完成后回调
} spi_device_interface_config_t;
```

---

### spi_transaction_t — SPI 事务描述符

```c
typedef struct {
    uint32_t flags;       // 事务标志（SPI_TRANS_*）
    uint16_t cmd;         // 命令数据（command_bits > 0 时有效）
    uint64_t addr;        // 地址数据（address_bits > 0 时有效）
    size_t length;        // 数据长度（位数，不是字节数）
    size_t rxlength;      // 接收长度（位数，0 = 与 length 相同）
    void *user;           // 用户自定义数据（传给回调）
    union {
        const void *tx_buffer;  // 发送缓冲区
        uint8_t tx_data[4];     // 短数据（≤4 字节时直接内嵌）
    };
    union {
        void *rx_buffer;        // 接收缓冲区
        uint8_t rx_data[4];     // 短数据接收
    };
} spi_transaction_t;
```

**常用事务标志：**

| 标志 | 说明 |
|------|------|
| `SPI_TRANS_USE_TXDATA` | 使用内嵌 `tx_data`（≤4 字节） |
| `SPI_TRANS_USE_RXDATA` | 使用内嵌 `rx_data`（≤4 字节） |
| `SPI_TRANS_MODE_DIO` | 双线模式 |
| `SPI_TRANS_MODE_QIO` | 四线模式 |
| `SPI_TRANS_CS_KEEP_ACTIVE` | 事务结束后保持 CS 低电平 |

---

## 🎯 核心函数

### 总线初始化

#### spi_bus_initialize()

初始化 SPI 总线（每条总线只需调用一次）。

```c
esp_err_t spi_bus_initialize(spi_host_device_t host_id,
                              const spi_bus_config_t *bus_config,
                              spi_dma_chan_t dma_chan);
```

**参数：**
- `host_id`: `SPI2_HOST` 或 `SPI3_HOST`
- `dma_chan`: `SPI_DMA_CH_AUTO`（自动分配，推荐）或 `SPI_DMA_DISABLED`

---

#### spi_bus_free()

释放 SPI 总线。

```c
esp_err_t spi_bus_free(spi_host_device_t host_id);
```

---

### 设备管理

#### spi_bus_add_device()

在总线上添加一个 SPI 设备，获取设备句柄。

```c
esp_err_t spi_bus_add_device(spi_host_device_t host_id,
                              const spi_device_interface_config_t *dev_config,
                              spi_device_handle_t *handle);
```

---

#### spi_bus_remove_device()

从总线移除设备。

```c
esp_err_t spi_bus_remove_device(spi_device_handle_t handle);
```

---

### 数据传输

#### spi_device_polling_transmit()

轮询模式传输（阻塞，适合短数据）。

```c
esp_err_t spi_device_polling_transmit(spi_device_handle_t handle,
                                       spi_transaction_t *trans_desc);
```

**特点：** CPU 全程参与，延迟最低，适合 ≤ 32 字节的短事务。

---

#### spi_device_transmit()

中断/DMA 模式传输（阻塞等待完成）。

```c
esp_err_t spi_device_transmit(spi_device_handle_t handle,
                               spi_transaction_t *trans_desc);
```

**特点：** 使用 DMA，CPU 在传输期间可处理其他事，适合大数据量。

---

#### spi_device_queue_trans()

异步提交事务（非阻塞）。

```c
esp_err_t spi_device_queue_trans(spi_device_handle_t handle,
                                  spi_transaction_t *trans_desc,
                                  TickType_t ticks_to_wait);
```

---

#### spi_device_get_trans_result()

获取异步事务的结果（阻塞等待）。

```c
esp_err_t spi_device_get_trans_result(spi_device_handle_t handle,
                                       spi_transaction_t **trans_desc,
                                       TickType_t ticks_to_wait);
```

---

#### spi_device_acquire_bus() / spi_device_release_bus()

独占总线（多个事务连续发送时避免 CS 切换开销）。

```c
esp_err_t spi_device_acquire_bus(spi_device_handle_t device, TickType_t wait);
void spi_device_release_bus(spi_device_handle_t dev);
```

---

## 💡 完整应用示例

### 示例1：SPI 初始化 + 发送命令（LCD 控制器）

```c
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define LCD_HOST    SPI2_HOST
#define PIN_MOSI    23
#define PIN_CLK     18
#define PIN_CS      15
#define PIN_DC       2   // 数据/命令选择
#define PIN_RST      4

static const char *TAG = "SPI_LCD";
static spi_device_handle_t spi_handle;

/* DC 引脚在事务开始前通过回调设置 */
static void lcd_spi_pre_transfer_cb(spi_transaction_t *t)
{
    int dc = (int)t->user;
    gpio_set_level(PIN_DC, dc);
}

void spi_lcd_init(void)
{
    /* 1. 配置 DC 和 RST 引脚 */
    gpio_config_t io_cfg = {
        .pin_bit_mask = (1ULL << PIN_DC) | (1ULL << PIN_RST),
        .mode         = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_cfg);
    gpio_set_level(PIN_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(10));

    /* 2. 初始化 SPI 总线 */
    spi_bus_config_t buscfg = {
        .mosi_io_num     = PIN_MOSI,
        .miso_io_num     = -1,       // LCD 只需发送
        .sclk_io_num     = PIN_CLK,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = 320 * 240 * 2,  // 全屏 RGB565
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    /* 3. 添加 LCD 设备 */
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 40 * 1000 * 1000,  // 40MHz
        .mode           = 0,                   // SPI Mode 0
        .spics_io_num   = PIN_CS,
        .queue_size     = 7,
        .pre_cb         = lcd_spi_pre_transfer_cb,  // DC 引脚控制
    };
    ESP_ERROR_CHECK(spi_bus_add_device(LCD_HOST, &devcfg, &spi_handle));

    ESP_LOGI(TAG, "SPI LCD 初始化完成");
}

/* 发送命令（DC=0）*/
void lcd_cmd(uint8_t cmd)
{
    spi_transaction_t t = {
        .length    = 8,
        .tx_data   = {cmd},
        .flags     = SPI_TRANS_USE_TXDATA,
        .user      = (void *)0,  // DC = 0（命令）
    };
    spi_device_polling_transmit(spi_handle, &t);
}

/* 发送数据（DC=1）*/
void lcd_data(const uint8_t *data, int len)
{
    if (len == 0) return;
    spi_transaction_t t = {
        .length    = len * 8,
        .tx_buffer = data,
        .user      = (void *)1,  // DC = 1（数据）
    };
    spi_device_transmit(spi_handle, &t);
}
```

---

### 示例2：读写 SPI Flash（W25Q 系列）

```c
#include "driver/spi_master.h"

#define FLASH_HOST  SPI2_HOST
#define PIN_MOSI    23
#define PIN_MISO    19
#define PIN_CLK     18
#define PIN_CS      5

static spi_device_handle_t flash_handle;

void spi_flash_init(void)
{
    spi_bus_config_t buscfg = {
        .mosi_io_num   = PIN_MOSI,
        .miso_io_num   = PIN_MISO,
        .sclk_io_num   = PIN_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };
    spi_bus_initialize(FLASH_HOST, &buscfg, SPI_DMA_CH_AUTO);

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 10 * 1000 * 1000,  // 10MHz（保守值）
        .mode           = 0,
        .spics_io_num   = PIN_CS,
        .queue_size     = 1,
    };
    spi_bus_add_device(FLASH_HOST, &devcfg, &flash_handle);
}

/* 读取 JEDEC ID（0x9F 命令）*/
uint32_t flash_read_id(void)
{
    uint8_t tx[4] = {0x9F, 0, 0, 0};
    uint8_t rx[4] = {0};

    spi_transaction_t t = {
        .length    = 32,
        .tx_buffer = tx,
        .rx_buffer = rx,
    };
    spi_device_polling_transmit(flash_handle, &t);

    return (rx[1] << 16) | (rx[2] << 8) | rx[3];
}

/* 读取数据 */
void flash_read(uint32_t addr, uint8_t *buf, size_t len)
{
    uint8_t cmd[4] = {
        0x03,                    // Read 命令
        (addr >> 16) & 0xFF,
        (addr >> 8) & 0xFF,
        addr & 0xFF,
    };

    /* 发送命令（保持 CS 低）*/
    spi_transaction_t t_cmd = {
        .length    = 32,
        .tx_buffer = cmd,
        .flags     = SPI_TRANS_CS_KEEP_ACTIVE,
    };
    spi_device_polling_transmit(flash_handle, &t_cmd);

    /* 接收数据 */
    spi_transaction_t t_data = {
        .length    = len * 8,
        .rx_buffer = buf,
    };
    spi_device_polling_transmit(flash_handle, &t_data);
}
```

---

### 示例3：多设备共享 SPI 总线

```c
/* LCD 和触摸屏共用 SPI2，各自独立 CS */

static spi_device_handle_t lcd_handle;
static spi_device_handle_t touch_handle;

void multi_device_init(void)
{
    /* 总线只初始化一次 */
    spi_bus_config_t buscfg = {
        .mosi_io_num   = 23,
        .miso_io_num   = 19,
        .sclk_io_num   = 18,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 320 * 240 * 2,
    };
    spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);

    /* LCD 设备：高速，无 MISO */
    spi_device_interface_config_t lcd_cfg = {
        .clock_speed_hz = 40 * 1000 * 1000,
        .mode           = 0,
        .spics_io_num   = 15,   // LCD CS
        .queue_size     = 7,
    };
    spi_bus_add_device(SPI2_HOST, &lcd_cfg, &lcd_handle);

    /* 触摸屏设备：低速，需要 MISO */
    spi_device_interface_config_t touch_cfg = {
        .clock_speed_hz = 2 * 1000 * 1000,  // 2MHz
        .mode           = 0,
        .spics_io_num   = 21,   // 触摸 CS
        .queue_size     = 1,
    };
    spi_bus_add_device(SPI2_HOST, &touch_cfg, &touch_handle);
}
```

---

## ⚠️ 注意事项

### 1. DMA 缓冲区对齐

使用 DMA 传输时，`tx_buffer` 和 `rx_buffer` 必须是 DMA 可访问的内存：
- 内部 SRAM：直接可用
- PSRAM：需要在 `sdkconfig` 中启用 `CONFIG_SPIRAM_USE_MALLOC`，或用 `heap_caps_malloc(size, MALLOC_CAP_DMA)`

```c
/* 正确：从 DMA 可访问内存分配 */
uint8_t *buf = heap_caps_malloc(size, MALLOC_CAP_DMA);

/* 错误：栈上的局部数组可能不对齐 */
uint8_t buf[256];  // 可能导致 DMA 传输失败
```

### 2. 事务长度单位

`spi_transaction_t.length` 的单位是**位（bit）**，不是字节：
```c
/* 发送 4 字节 */
t.length = 4 * 8;  // = 32 位
```

### 3. 轮询 vs 中断模式

| 函数 | 模式 | 适用场景 |
|------|------|---------|
| `spi_device_polling_transmit` | 轮询，CPU 忙等 | ≤ 32 字节，延迟敏感 |
| `spi_device_transmit` | 中断/DMA，CPU 可让出 | 大数据量，与 FreeRTOS 配合 |

### 4. 最高时钟频率

| 条件 | 最高频率 |
|------|---------|
| 无 `input_delay_ns` 限制 | 80MHz |
| 有 MISO（全双工） | 通常 40~60MHz（取决于走线） |
| 长走线或外部设备限制 | 10~40MHz |

### 5. CS 引脚控制

`spics_io_num` 设为 `-1` 时，CS 由用户手动控制（用 `gpio_set_level`）。
设为有效引脚时，驱动自动在事务开始/结束时控制 CS。

---

## 📖 相关文档

- [IDF_GPIO.md](IDF_GPIO.md) — GPIO 配置
- [IDF_I2C.md](IDF_I2C.md) — I2C 总线
- [IDF_UART.md](IDF_UART.md) — 串口通信
- [README.md](README.md) — 模块导航
