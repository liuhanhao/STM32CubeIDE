# ESP32 使用 PlatformIO + ESP-IDF 开发 LVGL GUI 指南

## 概述

本指南使用 **ESP-IDF 框架**（非 Arduino）在 PlatformIO 中集成 LVGL，适合需要完整 IDF 能力的项目。

| 项目 | 说明 |
|------|------|
| 框架 | ESP-IDF（espidf） |
| GUI 库 | LVGL v8.x |
| 显示驱动 | esp_lcd（IDF 官方组件） |
| 触摸驱动 | esp_lcd_touch（IDF 官方组件） |
| 推荐芯片 | ESP32-S3（有 PSRAM，体验最佳） |

**为什么选 IDF 框架而不是 Arduino？**
- 直接使用 `esp_lcd` 硬件加速驱动，SPI/I80/RGB 均有官方支持
- FreeRTOS 任务、内存管理完全可控
- PSRAM 配置更灵活，可用 `heap_caps_malloc` 精确分配
- 无 Arduino 抽象层开销，性能更好

---

## 一、硬件准备

### 1.1 显示接口选择

| 接口 | IDF 驱动组件 | 典型屏幕 | 备注 |
|------|------------|---------|------|
| SPI | `esp_lcd_ili9341` | 240×320 ILI9341 | 最通用 |
| SPI | `esp_lcd_st7789` | 240×240 ST7789 | 小方屏 |
| 8080 并口 | `esp_lcd` 内置 | 320×480 | 速度快 |
| RGB 并口 | `esp_lcd_rgb_panel` | 480×800+ | S3 专属，最快 |

### 1.2 典型接线（SPI ILI9341，240×320）

| 屏幕引脚 | ESP32-S3 GPIO | 说明 |
|---------|--------------|------|
| VCC | 3.3V | 供电 |
| GND | GND | 地 |
| CS | GPIO15 | 片选 |
| RESET | GPIO4 | 复位 |
| DC/RS | GPIO2 | 数据/命令 |
| SDI(MOSI) | GPIO23 | SPI 数据 |
| SCK | GPIO18 | SPI 时钟 |
| LED | GPIO32 | 背光（PWM 或直接 3.3V） |
| SDO(MISO) | GPIO19 | 可选，读屏用 |
| T_CLK | GPIO18 | 触摸 SPI 时钟（共用） |
| T_CS | GPIO21 | 触摸片选 |
| T_DIN | GPIO23 | 触摸数据（共用 MOSI） |
| T_DO | GPIO19 | 触摸数据（共用 MISO） |
| T_IRQ | GPIO34 | 触摸中断（可选） |

---

## 二、PlatformIO 工程配置

### 2.1 platformio.ini

```ini
; ESP32-S3-N16R8 + LVGL + ESP-IDF

[env:esp32s3-lvgl-idf]
platform        = espressif32
board           = esp32-s3-devkitc-1
framework       = espidf

upload_protocol = esptool
monitor_speed   = 115200
monitor_filters = esp32_exception_decoder, colorize

; LVGL 通过 IDF Component Manager 引入（见 idf_component.yml）
; 无需在 lib_deps 中添加
```

> IDF 框架下 LVGL 不通过 `lib_deps` 引入，而是通过 **IDF Component Manager** 管理，配置在 `main/idf_component.yml` 中。

### 2.2 sdkconfig.defaults（Flash + PSRAM 配置）

```ini
# Flash: 16MB QIO
CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y
CONFIG_ESPTOOLPY_FLASHSIZE="16MB"
CONFIG_ESPTOOLPY_FLASHMODE_QIO=y
CONFIG_ESPTOOLPY_FLASHFREQ_80M=y

# PSRAM: 8MB Octal SPI（R8）
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCT=y
CONFIG_SPIRAM_SPEED_80M=y
CONFIG_SPIRAM_FETCH_INSTRUCTIONS=y
CONFIG_SPIRAM_RODATA=y

# 分区表
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"
```


---

## 三、IDF Component Manager 引入 LVGL

### 3.1 工程目录结构

```
esp32_lvgl/
├── main/
│   ├── CMakeLists.txt
│   ├── idf_component.yml      ← 组件依赖声明
│   ├── main.c
│   ├── lv_conf.h              ← LVGL 配置
│   ├── display.c / display.h  ← 显示驱动封装
│   ├── touch.c / touch.h      ← 触摸驱动封装（可选）
│   └── ui/
│       ├── ui.c
│       └── ui.h
├── CMakeLists.txt
├── platformio.ini
├── sdkconfig.defaults
└── partitions.csv
```

### 3.2 main/idf_component.yml

```yaml
## IDF Component Manager 依赖声明
dependencies:
  idf: ">=5.0"

  ## LVGL 核心库
  lvgl/lvgl: "^8.3.11"

  ## esp_lcd 显示驱动（IDF 内置，无需额外声明）
  ## 触摸驱动（XPT2046，按需选择）
  espressif/esp_lcd_touch_xpt2046: "^1.0"

  ## ILI9341 驱动（按实际屏幕选择）
  espressif/esp_lcd_ili9341: "^1.0"
  # espressif/esp_lcd_st7789: "^1.0"   # ST7789 屏幕用这个
```

> 首次编译时 PlatformIO 会自动调用 `idf.py` 下载组件到 `managed_components/` 目录。

### 3.3 main/CMakeLists.txt

```cmake
idf_component_register(
    SRCS
        "main.c"
        "display.c"
        "touch.c"
        "ui/ui.c"
    INCLUDE_DIRS
        "."
        "ui"
    REQUIRES
        esp_lcd
        esp_timer
        freertos
        driver
)
```

---

## 四、lv_conf.h 配置

在 `main/lv_conf.h` 中创建（LVGL 会自动查找同目录的配置文件）：

```c
#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

#define LV_CONF_SKIP 0   /* 必须为 0，启用此配置文件 */

/* ── 颜色深度 ─────────────────────────── */
#define LV_COLOR_DEPTH 16   /* RGB565，SPI 屏标准格式 */

/* ── 内存配置 ─────────────────────────── */
/* 使用 IDF 的 heap_caps，让 LVGL 从 PSRAM 分配内存 */
#define LV_MEM_CUSTOM      1
#define LV_MEM_CUSTOM_INCLUDE  <stdlib.h>
#define LV_MEM_CUSTOM_ALLOC    malloc
#define LV_MEM_CUSTOM_FREE     free
#define LV_MEM_CUSTOM_REALLOC  realloc

/* ── 显示刷新 ─────────────────────────── */
#define LV_DISP_DEF_REFR_PERIOD  10   /* ms */
#define LV_INDEV_DEF_READ_PERIOD 10   /* ms */

/* ── 字体 ─────────────────────────────── */
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/* ── 日志（通过 ESP_LOG 输出）────────── */
#define LV_USE_LOG      1
#define LV_LOG_LEVEL    LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF   0   /* 关闭，改用下方自定义 */
#define LV_LOG_CUSTOM_PRINT_CB lv_log_print_cb  /* 见 main.c */

/* ── 控件 ─────────────────────────────── */
#define LV_USE_BTN      1
#define LV_USE_LABEL    1
#define LV_USE_IMG      1
#define LV_USE_ARC      1
#define LV_USE_BAR      1
#define LV_USE_SLIDER   1
#define LV_USE_SWITCH   1
#define LV_USE_CHART    1
#define LV_USE_METER    1
#define LV_USE_TABVIEW  1
#define LV_USE_TEXTAREA 1
#define LV_USE_MSGBOX   1

/* ── 主题 ─────────────────────────────── */
#define LV_USE_THEME_DEFAULT 1
#define LV_THEME_DEFAULT_DARK 1   /* 深色主题 */

#endif /* LV_CONF_H */
```


---

## 五、显示驱动封装（display.c）

使用 IDF 官方 `esp_lcd` 组件驱动 SPI 屏幕。

### 5.1 display.h

```c
#pragma once
#include "lvgl.h"

/* 屏幕分辨率 */
#define LCD_H_RES  240
#define LCD_V_RES  320

/* SPI 引脚 */
#define LCD_PIN_MOSI  23
#define LCD_PIN_CLK   18
#define LCD_PIN_CS    15
#define LCD_PIN_DC     2
#define LCD_PIN_RST    4
#define LCD_PIN_BL    32

/* SPI 时钟频率 */
#define LCD_SPI_CLOCK_HZ  (40 * 1000 * 1000)

void display_init(void);
```

### 5.2 display.c

```c
#include "display.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_ili9341.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "lvgl.h"

static const char *TAG = "DISPLAY";

/* LVGL 绘制缓冲区（分配在 PSRAM）*/
#define DRAW_BUF_SIZE (LCD_H_RES * 20)   /* 20 行高度 */
static lv_color_t *buf1 = NULL;
static lv_color_t *buf2 = NULL;

static esp_lcd_panel_handle_t panel_handle = NULL;

/* ── LVGL 刷新回调 ──────────────────────────────────────── */
static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io,
                                     esp_lcd_panel_io_event_data_t *edata,
                                     void *user_ctx)
{
    lv_disp_drv_t *disp_drv = (lv_disp_drv_t *)user_ctx;
    lv_disp_flush_ready(disp_drv);
    return false;
}

static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    esp_lcd_panel_draw_bitmap(panel_handle,
                              area->x1, area->y1,
                              area->x2 + 1, area->y2 + 1,
                              color_map);
    /* 若未使用 DMA 中断通知，在此直接调用 flush_ready */
    /* lv_disp_flush_ready(drv); */
}

/* ── 显示初始化 ─────────────────────────────────────────── */
void display_init(void)
{
    /* 1. 配置背光 GPIO */
    gpio_config_t bl_cfg = {
        .pin_bit_mask = (1ULL << LCD_PIN_BL),
        .mode         = GPIO_MODE_OUTPUT,
    };
    gpio_config(&bl_cfg);
    gpio_set_level(LCD_PIN_BL, 1);   /* 背光开 */

    /* 2. 初始化 SPI 总线 */
    spi_bus_config_t buscfg = {
        .mosi_io_num   = LCD_PIN_MOSI,
        .miso_io_num   = -1,          /* 不需要 MISO */
        .sclk_io_num   = LCD_PIN_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * 20 * sizeof(lv_color_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

    /* 3. 创建 LCD Panel IO（SPI 接口）*/
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_cfg = {
        .dc_gpio_num       = LCD_PIN_DC,
        .cs_gpio_num       = LCD_PIN_CS,
        .pclk_hz           = LCD_SPI_CLOCK_HZ,
        .lcd_cmd_bits      = 8,
        .lcd_param_bits    = 8,
        .spi_mode          = 0,
        .trans_queue_depth = 10,
        .on_color_trans_done = notify_lvgl_flush_ready,
        /* .user_ctx 在注册 LVGL 驱动后设置 */
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST,
                                              &io_cfg, &io_handle));

    /* 4. 创建 ILI9341 Panel */
    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = LCD_PIN_RST,
        .color_space    = ESP_LCD_COLOR_SPACE_BGR,   /* ILI9341 是 BGR */
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_cfg, &panel_handle));

    /* 5. 复位并初始化屏幕 */
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    ESP_LOGI(TAG, "LCD 初始化完成，分辨率 %d×%d", LCD_H_RES, LCD_V_RES);

    /* 6. 初始化 LVGL */
    lv_init();

    /* 从 PSRAM 分配缓冲区 */
    buf1 = heap_caps_malloc(DRAW_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    buf2 = heap_caps_malloc(DRAW_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    assert(buf1 && buf2);

    static lv_disp_draw_buf_t draw_buf;
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, DRAW_BUF_SIZE);

    /* 7. 注册 LVGL 显示驱动 */
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res  = LCD_H_RES;
    disp_drv.ver_res  = LCD_V_RES;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    /* 将 disp_drv 指针传给 DMA 回调 */
    /* io_cfg.user_ctx = &disp_drv; （需在 io 创建前设置，或用全局变量）*/
}
```


---

## 六、触摸驱动封装（touch.c，可选）

### 6.1 touch.h

```c
#pragma once
#include "lvgl.h"

/* 触摸 SPI 引脚（与 LCD 共用 SPI 总线）*/
#define TOUCH_PIN_CS   21
#define TOUCH_PIN_IRQ  34   /* -1 表示不使用中断 */

void touch_init(void);
```

### 6.2 touch.c

```c
#include "touch.h"
#include "esp_lcd_touch_xpt2046.h"
#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "TOUCH";
static esp_lcd_touch_handle_t tp_handle = NULL;

/* ── LVGL 触摸读取回调 ──────────────────────────────────── */
static void lvgl_touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    uint16_t touch_x[1], touch_y[1], touch_strength[1];
    uint8_t touch_cnt = 0;

    esp_lcd_touch_read_data(tp_handle);
    bool touched = esp_lcd_touch_get_coordinates(tp_handle,
                                                  touch_x, touch_y,
                                                  touch_strength, &touch_cnt, 1);
    if (touched && touch_cnt > 0) {
        data->state   = LV_INDEV_STATE_PR;
        data->point.x = touch_x[0];
        data->point.y = touch_y[0];
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

/* ── 触摸初始化 ─────────────────────────────────────────── */
void touch_init(void)
{
    /* XPT2046 挂在同一 SPI 总线上，创建新的 Panel IO */
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_spi_config_t tp_io_cfg = {
        .dc_gpio_num       = -1,   /* XPT2046 无 DC 引脚 */
        .cs_gpio_num       = TOUCH_PIN_CS,
        .pclk_hz           = 2 * 1000 * 1000,   /* 触摸 SPI 频率低一些 */
        .lcd_cmd_bits      = 8,
        .lcd_param_bits    = 8,
        .spi_mode          = 0,
        .trans_queue_depth = 3,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST,
                                              &tp_io_cfg, &tp_io_handle));

    esp_lcd_touch_config_t tp_cfg = {
        .x_max        = 240,
        .y_max        = 320,
        .rst_gpio_num = -1,
        .int_gpio_num = TOUCH_PIN_IRQ,
        .flags = {
            .swap_xy  = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };
    ESP_ERROR_CHECK(esp_lcd_touch_new_spi_xpt2046(tp_io_handle, &tp_cfg, &tp_handle));

    /* 注册 LVGL 输入设备 */
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = lvgl_touch_cb;
    lv_indev_drv_register(&indev_drv);

    ESP_LOGI(TAG, "触摸初始化完成");
}
```

---

## 七、LVGL Tick 定时器

LVGL 需要一个毫秒级 tick 源，IDF 框架用 `esp_timer` 实现：

```c
/* 在 main.c 中添加 */
#include "esp_timer.h"
#include "lvgl.h"

static void lvgl_tick_cb(void *arg)
{
    lv_tick_inc(2);   /* 每 2ms 调用一次，参数与定时器周期一致 */
}

void lvgl_tick_init(void)
{
    const esp_timer_create_args_t timer_args = {
        .callback = lvgl_tick_cb,
        .name     = "lvgl_tick",
    };
    esp_timer_handle_t timer;
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(timer, 2 * 1000));   /* 2ms，单位 us */
}
```

---

## 八、FreeRTOS GUI 任务

LVGL 的 `lv_timer_handler()` 必须在单一任务中调用，用互斥锁保护跨任务的 UI 更新。

```c
/* main.c */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "lvgl.h"
#include "display.h"
#include "touch.h"
#include "ui/ui.h"

static const char *TAG = "MAIN";

/* LVGL 互斥锁，跨任务更新 UI 时使用 */
SemaphoreHandle_t lvgl_mutex;

/* ── LVGL 日志桥接到 ESP_LOG ────────────────────────────── */
static void lv_log_print_cb(const char *buf)
{
    ESP_LOGW("LVGL", "%s", buf);
}

/* ── GUI 任务（运行在核心 1）────────────────────────────── */
static void gui_task(void *arg)
{
    display_init();
    touch_init();
    lvgl_tick_init();
    ui_init();   /* 创建界面 */

    ESP_LOGI(TAG, "GUI 任务启动");

    while (1) {
        if (xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(10))) {
            lv_timer_handler();
            xSemaphoreGive(lvgl_mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

/* ── 业务任务示例（运行在核心 0）────────────────────────── */
static lv_obj_t *g_label = NULL;   /* 全局控件指针，供其他任务访问 */

static void sensor_task(void *arg)
{
    int count = 0;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        count++;

        /* 更新 UI 时加锁 */
        if (xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(100))) {
            if (g_label) {
                lv_label_set_text_fmt(g_label, "计数: %d", count);
            }
            xSemaphoreGive(lvgl_mutex);
        }
    }
}

/* ── app_main ───────────────────────────────────────────── */
void app_main(void)
{
    lv_log_register_print_cb(lv_log_print_cb);

    lvgl_mutex = xSemaphoreCreateMutex();
    assert(lvgl_mutex);

    /* GUI 任务固定在核心 1，栈 8KB */
    xTaskCreatePinnedToCore(gui_task,    "gui",    8192, NULL, 5, NULL, 1);
    /* 业务任务在核心 0 */
    xTaskCreatePinnedToCore(sensor_task, "sensor", 4096, NULL, 3, NULL, 0);
}
```


---

## 九、UI 实现示例（ui/ui.c）

```c
#include "ui.h"
#include "lvgl.h"

/* 外部声明，供其他任务访问 */
extern lv_obj_t *g_label;

void ui_init(void)
{
    /* ── 屏幕背景 ───────────────────── */
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x1a1a2e), LV_PART_MAIN);

    /* ── 标题标签 ───────────────────── */
    lv_obj_t *title = lv_label_create(lv_scr_act());
    lv_label_set_text(title, "ESP32-S3 GUI");
    lv_obj_set_style_text_color(title, lv_color_hex(0xe0e0e0), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    /* ── 动态计数标签（供 sensor_task 更新）── */
    g_label = lv_label_create(lv_scr_act());
    lv_label_set_text(g_label, "计数: 0");
    lv_obj_set_style_text_color(g_label, lv_color_hex(0x00d4ff), LV_PART_MAIN);
    lv_obj_align(g_label, LV_ALIGN_CENTER, 0, -30);

    /* ── 进度条 ─────────────────────── */
    lv_obj_t *bar = lv_bar_create(lv_scr_act());
    lv_obj_set_size(bar, 180, 20);
    lv_obj_align(bar, LV_ALIGN_CENTER, 0, 20);
    lv_bar_set_value(bar, 60, LV_ANIM_ON);

    /* ── 按钮 ───────────────────────── */
    lv_obj_t *btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn, 120, 45);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_event_cb(btn, [](lv_event_t *e) {
        lv_obj_t *lbl = lv_obj_get_child(lv_event_get_target(e), 0);
        lv_label_set_text(lbl, "已点击");
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, "点击测试");
    lv_obj_center(btn_lbl);
}
```

---

## 十、SquareLine Studio 集成（可视化设计）

SquareLine Studio 导出的代码可以直接在 IDF 框架下使用。

### 10.1 导出设置

| 选项 | 值 |
|------|---|
| Platform | ESP-IDF |
| LVGL Version | 与 `idf_component.yml` 一致 |
| Resolution | 实际屏幕分辨率 |
| Color Depth | 16 bit |

### 10.2 集成步骤

1. 导出后将 `ui/` 文件夹复制到 `main/ui/`
2. 在 `main/CMakeLists.txt` 中添加源文件：

```cmake
idf_component_register(
    SRCS
        "main.c"
        "display.c"
        "touch.c"
        "ui/ui.c"
        "ui/screens/ui_Screen1.c"   # SquareLine 导出的屏幕文件
    INCLUDE_DIRS "." "ui"
    ...
)
```

3. `app_main` 中调用 `ui_init()` 即可，其余不变。

---

## 十一、性能优化

### 11.1 PSRAM 缓冲区策略

```c
/* 有 PSRAM 时，用全屏双缓冲获得最流畅效果 */
#define DRAW_BUF_SIZE (LCD_H_RES * LCD_V_RES)

buf1 = heap_caps_malloc(DRAW_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
buf2 = heap_caps_malloc(DRAW_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
```

### 11.2 开启 DMA 传输

`spi_bus_initialize` 中已设置 `SPI_DMA_CH_AUTO`，DMA 默认启用。
配合 `on_color_trans_done` 回调通知 LVGL，CPU 在 DMA 传输期间可处理其他任务。

### 11.3 提高 SPI 时钟

```c
/* ILI9341 最高支持 80MHz（部分屏幕稳定性不同，从 40MHz 开始测试）*/
.pclk_hz = 80 * 1000 * 1000,
```

### 11.4 GUI 任务优先级

```c
/* GUI 任务优先级设高一些，保证刷新及时 */
xTaskCreatePinnedToCore(gui_task, "gui", 8192, NULL, 5, NULL, 1);
/* 业务任务优先级低于 GUI */
xTaskCreatePinnedToCore(sensor_task, "sensor", 4096, NULL, 3, NULL, 0);
```

---

## 十二、中文字体支持

### 12.1 生成字体文件

1. 访问 [https://lvgl.io/tools/fontconverter](https://lvgl.io/tools/fontconverter)
2. 上传 TTF 字体（推荐思源黑体 SourceHanSansCN-Regular.ttf）
3. 配置：
   - Name：`lv_font_cn_16`
   - Size：16
   - Bpp：4（抗锯齿）
   - Range：`0x4E00-0x9FFF,0x20-0x7E`（常用汉字 + ASCII）
4. 下载生成的 `lv_font_cn_16.c`，放入 `main/fonts/`

### 12.2 在 CMakeLists.txt 中添加

```cmake
idf_component_register(
    SRCS
        "main.c"
        "display.c"
        "fonts/lv_font_cn_16.c"   # 添加字体文件
    ...
)
```

### 12.3 使用

```c
/* lv_conf.h 中声明 */
LV_FONT_DECLARE(lv_font_cn_16);

/* 使用时 */
lv_obj_set_style_text_font(label, &lv_font_cn_16, LV_PART_MAIN);
lv_label_set_text(label, "你好，世界");
```

---

## 十三、常见问题

### Q1：编译报错 `lvgl/lvgl.h: No such file`

组件未下载。执行一次完整编译，PlatformIO 会自动触发 `idf.py` 下载 `managed_components/`。
或手动执行：
```bash
cd 工程目录
~/.platformio/packages/framework-espidf/tools/idf.py reconfigure
```

### Q2：屏幕全白或全黑，无显示

- 检查 `LCD_PIN_DC`、`LCD_PIN_CS`、`LCD_PIN_RST` 引脚是否正确
- 确认 `esp_lcd_panel_disp_on_off(panel_handle, true)` 已调用
- 检查背光引脚是否拉高

### Q3：颜色显示异常（红蓝互换）

ILI9341 是 BGR 格式，ST7789 是 RGB 格式：
```c
/* ILI9341 */
.color_space = ESP_LCD_COLOR_SPACE_BGR,

/* ST7789 */
.color_space = ESP_LCD_COLOR_SPACE_RGB,
```

### Q4：触摸坐标偏移或方向错误

```c
esp_lcd_touch_config_t tp_cfg = {
    .flags = {
        .swap_xy  = 1,   /* 交换 X/Y */
        .mirror_x = 1,   /* X 轴镜像 */
        .mirror_y = 0,
    },
};
```

### Q5：`lv_timer_handler` 调用后 Watchdog 触发

GUI 任务中 `vTaskDelay` 不能省略，否则低优先级任务无法运行，触发 IDLE 看门狗：
```c
lv_timer_handler();
vTaskDelay(pdMS_TO_TICKS(5));   /* 必须有，让出 CPU */
```

### Q6：跨任务更新 UI 导致崩溃

所有 LVGL API 调用必须在持有 `lvgl_mutex` 时进行：
```c
if (xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(100))) {
    lv_label_set_text(label, "新内容");
    xSemaphoreGive(lvgl_mutex);
}
```

---

## 十四、验证清单

- [ ] 编译通过，`managed_components/lvgl__lvgl` 目录存在
- [ ] 屏幕点亮，背景色正确
- [ ] LVGL 标签和按钮正常显示
- [ ] 触摸点击坐标准确（有触摸屏时）
- [ ] 串口无 `assert` 或 `heap alloc failed` 报错
- [ ] 跨任务更新 UI 无崩溃

---

## 参考资料

- [LVGL 官方文档](https://docs.lvgl.io/8.3)
- [esp_lcd 组件文档](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/lcd.html)
- [LVGL + ESP-IDF 官方示例](https://github.com/lvgl/lv_port_esp32)
- [IDF Component Manager](https://docs.espressif.com/projects/idf-component-manager/en/latest)
- [SquareLine Studio](https://squareline.io)
- [LVGL 字体转换工具](https://lvgl.io/tools/fontconverter)
