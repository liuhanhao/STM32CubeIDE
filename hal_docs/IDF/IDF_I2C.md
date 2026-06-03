# I2C 总线模块 API 文档（ESP-IDF）

## 📋 模块概述

ESP-IDF 的 I2C Master 驱动提供双线串行总线通信，包括：
- 标准模式（100kHz）、快速模式（400kHz）、快速增强模式（1MHz）
- 7 位和 10 位设备地址
- 支持多设备挂载同一总线
- 支持 ACK 检测和超时处理
- 支持 DMA 传输（IDF 5.x 新驱动）

> **注意：** ESP-IDF 5.x 引入了新的 I2C 驱动 API（`i2c_master.h`），旧 API（`driver/i2c.h`）仍可用但标记为 Legacy。本文档以**新 API** 为主，兼顾旧 API 说明。

---

## 🗺️ 使用场景速查

| 场景 | 推荐配置 |
|------|----------|
| 传感器读取（MPU6050、BMP280） | 标准/快速模式，`i2c_master_transmit_receive` |
| OLED 显示（SSD1306） | 快速模式（400kHz），只发送 |
| EEPROM 读写（AT24C） | 标准模式，注意写入延迟 |
| 多传感器系统 | 同一总线，不同地址 |
| 触摸控制器（FT6206、GT911） | 快速模式，中断触发读取 |

---

## 📚 相关文件

- **新 API 头文件**: `driver/i2c_master.h`（IDF 5.x）
- **旧 API 头文件**: `driver/i2c.h`（Legacy，IDF 4.x/5.x 均可用）

---

## 🔧 数据类型（新 API）

### i2c_master_bus_config_t — 总线配置

```c
typedef struct {
    i2c_port_num_t i2c_port;       // I2C 端口号（I2C_NUM_0 或 I2C_NUM_1）
    gpio_num_t sda_io_num;         // SDA 引脚
    gpio_num_t scl_io_num;         // SCL 引脚
    i2c_clock_source_t clk_source; // 时钟源（I2C_CLK_SRC_DEFAULT）
    uint32_t glitch_ignore_cnt;    // 毛刺过滤（7 = 推荐值）
    int intr_priority;             // 中断优先级（0 = 默认）
    struct {
        uint32_t enable_internal_pullup : 1;  // 使能内部上拉
    } flags;
} i2c_master_bus_config_t;
```

---

### i2c_device_config_t — 设备配置

```c
typedef struct {
    i2c_addr_bit_len_t dev_addr_length;  // 地址位数（7 位或 10 位）
    uint16_t device_address;             // 设备 7 位地址（不含 R/W 位）
    uint32_t scl_speed_hz;               // SCL 频率（Hz）
    uint32_t scl_wait_us;                // SCL 超时（us，0 = 默认）
    struct {
        uint32_t disable_ack_check : 1;  // 禁用 ACK 检测（调试用）
    } flags;
} i2c_device_config_t;
```

---

## 🎯 核心函数（新 API，IDF 5.x）

### 总线与设备初始化

#### i2c_new_master_bus()

创建 I2C Master 总线。

```c
esp_err_t i2c_new_master_bus(const i2c_master_bus_config_t *bus_config,
                               i2c_master_bus_handle_t *ret_bus_handle);
```

---

#### i2c_master_bus_add_device()

在总线上添加一个 I2C 设备。

```c
esp_err_t i2c_master_bus_add_device(i2c_master_bus_handle_t bus_handle,
                                     const i2c_device_config_t *dev_config,
                                     i2c_master_dev_handle_t *ret_handle);
```

---

#### i2c_master_bus_rm_device()

从总线移除设备。

```c
esp_err_t i2c_master_bus_rm_device(i2c_master_dev_handle_t handle);
```

---

#### i2c_del_master_bus()

删除总线，释放资源。

```c
esp_err_t i2c_del_master_bus(i2c_master_bus_handle_t bus_handle);
```

---

### 数据传输

#### i2c_master_transmit()

向设备发送数据（写操作）。

```c
esp_err_t i2c_master_transmit(i2c_master_dev_handle_t i2c_dev,
                                const uint8_t *write_buffer,
                                size_t write_size,
                                int xfer_timeout_ms);
```

**参数：**
- `write_buffer`: 发送数据缓冲区
- `write_size`: 发送字节数
- `xfer_timeout_ms`: 超时（ms），`-1` = 永久等待

---

#### i2c_master_receive()

从设备接收数据（读操作）。

```c
esp_err_t i2c_master_receive(i2c_master_dev_handle_t i2c_dev,
                               uint8_t *read_buffer,
                               size_t read_size,
                               int xfer_timeout_ms);
```

---

#### i2c_master_transmit_receive()

先写后读（最常用：写寄存器地址，再读数据）。

```c
esp_err_t i2c_master_transmit_receive(i2c_master_dev_handle_t i2c_dev,
                                       const uint8_t *write_buffer,
                                       size_t write_size,
                                       uint8_t *read_buffer,
                                       size_t read_size,
                                       int xfer_timeout_ms);
```

---

#### i2c_master_bus_reset()

复位 I2C 总线（总线卡死时使用）。

```c
esp_err_t i2c_master_bus_reset(i2c_master_bus_handle_t bus_handle);
```

---

#### i2c_master_probe()

探测指定地址的设备是否存在（发送地址，检查 ACK）。

```c
esp_err_t i2c_master_probe(i2c_master_bus_handle_t bus_handle,
                             uint16_t address,
                             int xfer_timeout_ms);
```

**返回值：**
- `ESP_OK`: 设备存在（收到 ACK）
- `ESP_ERR_NOT_FOUND`: 设备不存在（NACK）

---

## 🔧 旧 API（Legacy，`driver/i2c.h`）

旧 API 在 IDF 4.x 项目中广泛使用，了解有助于阅读现有代码。

### 旧 API 初始化

```c
#include "driver/i2c.h"

#define I2C_PORT    I2C_NUM_0
#define I2C_SDA     GPIO_NUM_21
#define I2C_SCL     GPIO_NUM_22
#define I2C_FREQ    400000   // 400kHz

void i2c_legacy_init(void)
{
    i2c_config_t conf = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = I2C_SDA,
        .scl_io_num       = I2C_SCL,
        .sda_pullup_en    = GPIO_PULLUP_ENABLE,
        .scl_pullup_en    = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ,
    };
    i2c_param_config(I2C_PORT, &conf);
    i2c_driver_install(I2C_PORT, I2C_MODE_MASTER, 0, 0, 0);
}
```

### 旧 API 读写（命令链方式）

```c
/* 写寄存器 */
esp_err_t i2c_write_reg(uint8_t dev_addr, uint8_t reg, uint8_t data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}

/* 读寄存器 */
esp_err_t i2c_read_reg(uint8_t dev_addr, uint8_t reg, uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    /* 写阶段：发送寄存器地址 */
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    /* 读阶段：重复起始条件 + 读数据 */
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}
```

---

## 💡 完整应用示例

### 示例1：MPU6050 初始化与读取（新 API）

```c
#include "driver/i2c_master.h"
#include "esp_log.h"

#define MPU6050_ADDR    0x68   // AD0 接 GND 时为 0x68，接 VCC 为 0x69
#define REG_PWR_MGMT_1  0x6B
#define REG_ACCEL_XOUT  0x3B

static const char *TAG = "MPU6050";
static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t mpu_handle;

void mpu6050_init(void)
{
    /* 1. 创建 I2C 总线 */
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port          = I2C_NUM_0,
        .sda_io_num        = GPIO_NUM_21,
        .scl_io_num        = GPIO_NUM_22,
        .clk_source        = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));

    /* 2. 添加 MPU6050 设备 */
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = MPU6050_ADDR,
        .scl_speed_hz    = 400000,  // 400kHz
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &mpu_handle));

    /* 3. 探测设备是否存在 */
    esp_err_t ret = i2c_master_probe(bus_handle, MPU6050_ADDR, 100);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "MPU6050 未找到，检查接线");
        return;
    }

    /* 4. 唤醒 MPU6050（写 PWR_MGMT_1 = 0x00）*/
    uint8_t wake_cmd[] = {REG_PWR_MGMT_1, 0x00};
    ESP_ERROR_CHECK(i2c_master_transmit(mpu_handle, wake_cmd, sizeof(wake_cmd), 100));

    ESP_LOGI(TAG, "MPU6050 初始化完成");
}

void mpu6050_read_accel(int16_t *ax, int16_t *ay, int16_t *az)
{
    uint8_t reg = REG_ACCEL_XOUT;
    uint8_t data[6];

    /* 先写寄存器地址，再读 6 字节数据 */
    ESP_ERROR_CHECK(i2c_master_transmit_receive(mpu_handle,
                                                 &reg, 1,
                                                 data, 6,
                                                 100));
    *ax = (int16_t)((data[0] << 8) | data[1]);
    *ay = (int16_t)((data[2] << 8) | data[3]);
    *az = (int16_t)((data[4] << 8) | data[5]);
}

void app_main(void)
{
    mpu6050_init();

    int16_t ax, ay, az;
    while (1) {
        mpu6050_read_accel(&ax, &ay, &az);
        ESP_LOGI(TAG, "加速度: X=%d, Y=%d, Z=%d", ax, ay, az);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

---

### 示例2：SSD1306 OLED 显示（新 API）

```c
#include "driver/i2c_master.h"

#define OLED_ADDR   0x3C
#define OLED_CMD    0x00   // 命令前缀字节
#define OLED_DATA   0x40   // 数据前缀字节

static i2c_master_dev_handle_t oled_handle;

/* 发送单条命令 */
void oled_cmd(uint8_t cmd)
{
    uint8_t buf[2] = {OLED_CMD, cmd};
    i2c_master_transmit(oled_handle, buf, 2, 100);
}

/* 发送数据块 */
void oled_data(const uint8_t *data, size_t len)
{
    /* 需要在数据前加 0x40 前缀 */
    uint8_t *buf = malloc(len + 1);
    buf[0] = OLED_DATA;
    memcpy(buf + 1, data, len);
    i2c_master_transmit(oled_handle, buf, len + 1, 100);
    free(buf);
}

void oled_init(i2c_master_bus_handle_t bus)
{
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = OLED_ADDR,
        .scl_speed_hz    = 400000,
    };
    i2c_master_bus_add_device(bus, &cfg, &oled_handle);

    /* SSD1306 初始化序列 */
    oled_cmd(0xAE);  // 关闭显示
    oled_cmd(0xD5);  // 设置时钟分频
    oled_cmd(0x80);
    oled_cmd(0xA8);  // 设置多路复用比
    oled_cmd(0x3F);  // 64 行
    oled_cmd(0xD3);  // 设置显示偏移
    oled_cmd(0x00);
    oled_cmd(0x40);  // 设置起始行
    oled_cmd(0x8D);  // 电荷泵
    oled_cmd(0x14);  // 使能
    oled_cmd(0x20);  // 内存寻址模式
    oled_cmd(0x00);  // 水平寻址
    oled_cmd(0xA1);  // 列地址映射
    oled_cmd(0xC8);  // 行扫描方向
    oled_cmd(0xDA);  // COM 引脚配置
    oled_cmd(0x12);
    oled_cmd(0x81);  // 对比度
    oled_cmd(0xCF);
    oled_cmd(0xD9);  // 预充电周期
    oled_cmd(0xF1);
    oled_cmd(0xDB);  // VCOMH 电压
    oled_cmd(0x40);
    oled_cmd(0xA4);  // 全局显示开
    oled_cmd(0xA6);  // 正常显示（非反色）
    oled_cmd(0xAF);  // 开启显示
}
```

---

### 示例3：I2C 总线扫描（调试用）

```c
void i2c_scan(i2c_master_bus_handle_t bus)
{
    ESP_LOGI("I2C_SCAN", "开始扫描 I2C 总线...");
    int found = 0;
    for (uint8_t addr = 0x08; addr < 0x78; addr++) {
        esp_err_t ret = i2c_master_probe(bus, addr, 50);
        if (ret == ESP_OK) {
            ESP_LOGI("I2C_SCAN", "发现设备: 0x%02X", addr);
            found++;
        }
    }
    if (found == 0) {
        ESP_LOGW("I2C_SCAN", "未发现任何设备，检查接线和上拉电阻");
    } else {
        ESP_LOGI("I2C_SCAN", "共发现 %d 个设备", found);
    }
}
```

---

## ⚠️ 注意事项

### 1. 上拉电阻

I2C 总线必须有上拉电阻（SDA 和 SCL 各一个）：
- 内部上拉（约 45kΩ）：适合短距离、低速（≤ 100kHz）、调试阶段
- 外部上拉（4.7kΩ）：正式产品推荐，支持 400kHz 及以上

```c
/* 使用内部上拉（调试方便）*/
.flags.enable_internal_pullup = true,

/* 正式产品：禁用内部上拉，外接 4.7kΩ 到 3.3V */
.flags.enable_internal_pullup = false,
```

### 2. 设备地址

I2C 地址是 7 位（0x00~0x7F），传输时左移 1 位加 R/W 位。
新 API 中直接填 7 位地址，驱动自动处理 R/W 位。

常见设备地址：

| 设备 | 地址 | 说明 |
|------|------|------|
| MPU6050 | 0x68 / 0x69 | AD0 引脚决定 |
| SSD1306 | 0x3C / 0x3D | SA0 引脚决定 |
| BMP280 | 0x76 / 0x77 | SDO 引脚决定 |
| AT24C02 | 0x50~0x57 | A0~A2 引脚决定 |
| FT6206 | 0x38 | 固定 |
| GT911 | 0x5D / 0x14 | INT 引脚上电时序决定 |

### 3. 总线卡死处理

I2C 总线卡死（SDA 被拉低）时，调用 `i2c_master_bus_reset()` 发送 9 个时钟脉冲强制释放总线。

### 4. EEPROM 写入延迟

AT24C 系列 EEPROM 写入后需要等待约 5ms（页写入时间），期间不响应 I2C 请求：

```c
/* 写入后等待 */
i2c_master_transmit(eeprom_handle, data, len, 100);
vTaskDelay(pdMS_TO_TICKS(5));  // 等待写入完成
```

### 5. 新旧 API 混用

同一个 I2C 端口不能同时使用新旧 API。选定一种后全项目统一使用。

---

## 📖 相关文档

- [IDF_GPIO.md](IDF_GPIO.md) — GPIO 配置
- [IDF_SPI.md](IDF_SPI.md) — SPI 总线
- [IDF_UART.md](IDF_UART.md) — 串口通信
- [README.md](README.md) — 模块导航
