# LVGL 文件系统资源加载指南

## 📋 概述

LVGL 支持从外部存储加载资源（图片、字体、音频等），核心是注册一个**文件系统驱动**，
让 LVGL 通过统一的 `"驱动器字母:路径"` 格式访问不同存储介质。

```
lv_img_set_src(img, "S:icons/wifi.bin");
                     ↑
                     S: = 你注册的驱动器字母（对应 SD 卡 / SPIFFS / LittleFS）
```

---

## 一、方案对比

| 方案 | 存储位置 | 容量 | 是否需要额外硬件 | 适合场景 |
|------|---------|------|----------------|---------|
| C 数组编译进固件 | Flash（程序区） | 受 Flash 限制 | 否 | 图片少、固定不变 |
| SPIFFS | Flash（数据分区） | 通常 1~4MB | 否（ESP32 内置） | 中等数量、偶尔更新 |
| LittleFS | Flash（数据分区） | 通常 1~4MB | 否（ESP32 内置） | 比 SPIFFS 更可靠 |
| SD 卡（FATFS） | SD 卡 | GB 级别 | 是（SPI SD 模块） | 图片多、需要频繁更新 |

---

## 二、图片格式说明

LVGL 文件系统加载的图片**不是普通 PNG/JPG**，需要转换为 LVGL 专用的 `.bin` 格式。

### 转换工具

**在线转换：** https://lvgl.io/tools/imageconverter

| 选项 | 推荐值 |
|------|--------|
| Name | 变量名（C 数组用），bin 文件名随意 |
| Color format | `True color with alpha`（有透明通道）|
| Output format | `Binary` ← 文件系统方案选这个 |

> C 数组方案选 `C array`，文件系统方案选 `Binary`，两者不同。

### 颜色格式与 lv_conf.h 的关系

| `LV_COLOR_DEPTH` | 转换时选 Color format |
|------------------|--------------------|
| 16（RGB565） | `True color` 或 `True color with alpha` |
| 32（ARGB8888） | `True color with alpha` |

---

## 三、方案 A：C 数组（编译进固件，无文件系统）

最简单，适合图片数量少的场景。

### 转换

工具选 `C array`，下载 `.c` 文件放入项目：

```
main/
└── images/
    ├── images.h
    ├── icon_wifi.c
    └── bg_home.c
```

### CMakeLists.txt

```cmake
idf_component_register(
    SRCS
        "main.c"
        "images/icon_wifi.c"
        "images/bg_home.c"
    INCLUDE_DIRS "." "images"
    ...
)
```

### images.h（统一声明）

```c
#pragma once
#include "lvgl.h"

LV_IMG_DECLARE(icon_wifi);
LV_IMG_DECLARE(bg_home);
```

### 使用

```c
#include "images.h"

lv_obj_t *img = lv_img_create(lv_scr_act());
lv_img_set_src(img, &icon_wifi);   // 注意：传指针，不是字符串路径
lv_obj_center(img);
```

---

## 四、方案 B：SPIFFS / LittleFS（ESP32 内部 Flash）

不需要额外硬件，图片存在 ESP32 的 Flash 数据分区里。

### 4.1 分区表配置

在 `partitions.csv` 中添加数据分区：

```csv
# Name,   Type, SubType, Offset,   Size,  Flags
nvs,      data, nvs,     0x9000,   0x6000,
phy_init, data, phy,     0xf000,   0x1000,
factory,  app,  factory, 0x10000,  1500K,
storage,  data, spiffs,  ,         1M,       ← SPIFFS 分区
```

### 4.2 准备图片文件

将 `.bin` 图片放入一个本地文件夹（如 `data/`），用 PlatformIO 工具上传：

```ini
; platformio.ini
[env:esp32s3]
board_build.filesystem = spiffs       ; 或 littlefs
board_build.partitions = partitions.csv
```

```
项目根目录/
└── data/                   ← PlatformIO 自动打包上传此目录
    ├── bg_home.bin
    ├── icon_wifi.bin
    └── icon_battery.bin
```

上传文件系统：PlatformIO → `Upload Filesystem Image`（或命令行 `pio run -t uploadfs`）

### 4.3 挂载 SPIFFS

```c
#include "esp_spiffs.h"
#include "esp_log.h"

static const char *TAG = "SPIFFS";

void spiffs_init(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path       = "/spiffs",    // 挂载路径
        .partition_label = NULL,         // 使用默认 spiffs 分区
        .max_files       = 10,
        .format_if_mount_failed = false,
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS 挂载失败: %s", esp_err_to_name(ret));
        return;
    }

    size_t total = 0, used = 0;
    esp_spiffs_info(NULL, &total, &used);
    ESP_LOGI(TAG, "SPIFFS: 总 %d KB，已用 %d KB", total / 1024, used / 1024);
}
```

### 4.4 注册 LVGL 文件系统驱动（SPIFFS）

```c
#include "lvgl.h"
#include <stdio.h>

static void *lvgl_spiffs_open(lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode)
{
    char full_path[128];
    snprintf(full_path, sizeof(full_path), "/spiffs/%s", path);
    return fopen(full_path, (mode == LV_FS_MODE_WR) ? "wb" : "rb");
}

static lv_fs_res_t lvgl_spiffs_close(lv_fs_drv_t *drv, void *file_p)
{
    fclose((FILE *)file_p);
    return LV_FS_RES_OK;
}

static lv_fs_res_t lvgl_spiffs_read(lv_fs_drv_t *drv, void *file_p,
                                     void *buf, uint32_t btr, uint32_t *br)
{
    *br = fread(buf, 1, btr, (FILE *)file_p);
    return LV_FS_RES_OK;
}

static lv_fs_res_t lvgl_spiffs_seek(lv_fs_drv_t *drv, void *file_p,
                                     uint32_t pos, lv_fs_whence_t whence)
{
    int w = (whence == LV_FS_SEEK_SET) ? SEEK_SET :
            (whence == LV_FS_SEEK_CUR) ? SEEK_CUR : SEEK_END;
    fseek((FILE *)file_p, pos, w);
    return LV_FS_RES_OK;
}

static lv_fs_res_t lvgl_spiffs_tell(lv_fs_drv_t *drv, void *file_p, uint32_t *pos_p)
{
    *pos_p = (uint32_t)ftell((FILE *)file_p);
    return LV_FS_RES_OK;
}

void lvgl_spiffs_fs_init(void)
{
    static lv_fs_drv_t drv;
    lv_fs_drv_init(&drv);

    drv.letter   = 'F';              // 驱动器字母：路径前缀 "F:"
    drv.open_cb  = lvgl_spiffs_open;
    drv.close_cb = lvgl_spiffs_close;
    drv.read_cb  = lvgl_spiffs_read;
    drv.seek_cb  = lvgl_spiffs_seek;
    drv.tell_cb  = lvgl_spiffs_tell;

    lv_fs_drv_register(&drv);
}
```

### 4.5 使用

```c
spiffs_init();           // 挂载文件系统
lvgl_spiffs_fs_init();   // 注册 LVGL 驱动

// 加载图片（路径不含 /spiffs 前缀，驱动内部自动拼接）
lv_obj_t *img = lv_img_create(lv_scr_act());
lv_img_set_src(img, "F:bg_home.bin");
lv_obj_center(img);
```

---

## 五、方案 C：SD 卡（FATFS）

图片数量多、需要随时更换，推荐此方案。

### 5.1 硬件接线（SPI 模式）

| SD 模块引脚 | ESP32-S3 GPIO | 说明 |
|-----------|-------------|------|
| VCC | 3.3V | 供电 |
| GND | GND | 地 |
| MOSI | GPIO11 | SPI 数据输出 |
| MISO | GPIO13 | SPI 数据输入 |
| SCK | GPIO12 | SPI 时钟 |
| CS | GPIO10 | 片选 |

> SD 卡建议使用独立的 SPI Host（如 SPI3_HOST），避免与 LCD 屏的 SPI 冲突。

### 5.2 SD 卡初始化挂载

```c
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "esp_log.h"

static const char *TAG = "SD";

#define SD_PIN_MOSI  11
#define SD_PIN_MISO  13
#define SD_PIN_CLK   12
#define SD_PIN_CS    10

void sd_card_init(void)
{
    /* 初始化 SPI 总线（SD 卡专用，用 SPI3_HOST）*/
    spi_bus_config_t bus_cfg = {
        .mosi_io_num   = SD_PIN_MOSI,
        .miso_io_num   = SD_PIN_MISO,
        .sclk_io_num   = SD_PIN_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    /* SDSPI 设备配置 */
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI3_HOST;

    sdspi_device_config_t slot_cfg = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_cfg.gpio_cs = SD_PIN_CS;
    slot_cfg.host_id = SPI3_HOST;

    /* 挂载 FAT 文件系统 */
    esp_vfs_fat_sdmmc_mount_config_t mount_cfg = {
        .format_if_mount_failed = false,
        .max_files              = 10,
        .allocation_unit_size   = 16 * 1024,
    };

    sdmmc_card_t *card;
    esp_err_t ret = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_cfg,
                                             &mount_cfg, &card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SD 卡挂载失败: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "SD 卡挂载成功，容量: %llu MB",
             ((uint64_t)card->csd.capacity) * card->csd.sector_size / (1024 * 1024));
}
```

### 5.3 注册 LVGL 文件系统驱动（SD 卡）

```c
#include "lvgl.h"
#include <stdio.h>

static void *lvgl_sd_open(lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode)
{
    char full_path[128];
    snprintf(full_path, sizeof(full_path), "/sdcard/%s", path);
    return fopen(full_path, (mode == LV_FS_MODE_WR) ? "wb" : "rb");
}

static lv_fs_res_t lvgl_sd_close(lv_fs_drv_t *drv, void *file_p)
{
    fclose((FILE *)file_p);
    return LV_FS_RES_OK;
}

static lv_fs_res_t lvgl_sd_read(lv_fs_drv_t *drv, void *file_p,
                                  void *buf, uint32_t btr, uint32_t *br)
{
    *br = fread(buf, 1, btr, (FILE *)file_p);
    return LV_FS_RES_OK;
}

static lv_fs_res_t lvgl_sd_seek(lv_fs_drv_t *drv, void *file_p,
                                  uint32_t pos, lv_fs_whence_t whence)
{
    int w = (whence == LV_FS_SEEK_SET) ? SEEK_SET :
            (whence == LV_FS_SEEK_CUR) ? SEEK_CUR : SEEK_END;
    fseek((FILE *)file_p, pos, w);
    return LV_FS_RES_OK;
}

static lv_fs_res_t lvgl_sd_tell(lv_fs_drv_t *drv, void *file_p, uint32_t *pos_p)
{
    *pos_p = (uint32_t)ftell((FILE *)file_p);
    return LV_FS_RES_OK;
}

void lvgl_sd_fs_init(void)
{
    static lv_fs_drv_t drv;
    lv_fs_drv_init(&drv);

    drv.letter   = 'S';            // 驱动器字母：路径前缀 "S:"
    drv.open_cb  = lvgl_sd_open;
    drv.close_cb = lvgl_sd_close;
    drv.read_cb  = lvgl_sd_read;
    drv.seek_cb  = lvgl_sd_seek;
    drv.tell_cb  = lvgl_sd_tell;

    lv_fs_drv_register(&drv);
}
```

### 5.4 SD 卡目录结构建议

```
SD卡根目录/
├── images/
│   ├── bg_home.bin
│   ├── bg_setting.bin
│   ├── icon_wifi.bin
│   └── icon_battery.bin
└── audio/
    ├── click.wav
    └── alarm.wav
```

### 5.5 使用

```c
sd_card_init();        // 挂载 SD 卡
lvgl_sd_fs_init();     // 注册 LVGL 驱动

// 加载图片
lv_obj_t *img = lv_img_create(lv_scr_act());
lv_img_set_src(img, "S:images/bg_home.bin");
lv_obj_center(img);
```

---

## 六、音频文件读取

LVGL 本身不负责播放音频，但可以用相同的文件系统驱动读取音频数据，
再交给 I2S 或 DAC 播放。

### 6.1 SD 卡读取 WAV 文件（ESP32 I2S 播放）

```c
#include <stdio.h>
#include "driver/i2s_std.h"
#include "esp_log.h"

static const char *TAG = "AUDIO";

/* WAV 文件头结构 */
typedef struct {
    char     riff[4];        // "RIFF"
    uint32_t file_size;
    char     wave[4];        // "WAVE"
    char     fmt[4];         // "fmt "
    uint32_t fmt_size;       // 16
    uint16_t audio_format;   // 1 = PCM
    uint16_t channels;       // 1 = 单声道, 2 = 立体声
    uint32_t sample_rate;    // 采样率（如 44100）
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample; // 16
    char     data[4];         // "data"
    uint32_t data_size;
} wav_header_t;

void play_wav_from_sd(const char *filename)
{
    char path[64];
    snprintf(path, sizeof(path), "/sdcard/audio/%s", filename);

    FILE *f = fopen(path, "rb");
    if (!f) {
        ESP_LOGE(TAG, "无法打开文件: %s", path);
        return;
    }

    /* 读取并验证 WAV 头 */
    wav_header_t header;
    fread(&header, sizeof(wav_header_t), 1, f);

    if (header.audio_format != 1) {
        ESP_LOGE(TAG, "仅支持 PCM 格式");
        fclose(f);
        return;
    }

    ESP_LOGI(TAG, "播放 %s: %d Hz, %d ch, %d bit",
             filename, header.sample_rate,
             header.channels, header.bits_per_sample);

    /* 循环读取 PCM 数据写入 I2S */
    uint8_t buf[512];
    size_t bytes_read;
    size_t bytes_written;

    while ((bytes_read = fread(buf, 1, sizeof(buf), f)) > 0) {
        // i2s_channel_write 为 ESP-IDF 5.x API
        // i2s_channel_handle_t tx_chan 需提前初始化
        // i2s_channel_write(tx_chan, buf, bytes_read, &bytes_written, portMAX_DELAY);
    }

    fclose(f);
}
```

### 6.2 I2S 初始化（ESP32 + MAX98357 功放模块）

```c
#include "driver/i2s_std.h"

#define I2S_PIN_BCLK   6   // 位时钟
#define I2S_PIN_LRCLK  7   // 左右声道时钟
#define I2S_PIN_DOUT   8   // 数据输出

i2s_chan_handle_t tx_chan;

void i2s_init(uint32_t sample_rate)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(
        I2S_NUM_0, I2S_ROLE_MASTER);
    i2s_new_channel(&chan_cfg, &tx_chan, NULL);

    i2s_std_config_t std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(
                        I2S_DATA_BIT_WIDTH_16BIT,
                        I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_PIN_BCLK,
            .ws   = I2S_PIN_LRCLK,
            .dout = I2S_PIN_DOUT,
            .din  = I2S_GPIO_UNUSED,
        },
    };

    i2s_channel_init_std_mode(tx_chan, &std_cfg);
    i2s_channel_enable(tx_chan);
}
```

---

## 七、lv_conf.h 文件系统相关配置

使用文件系统前，确保 `lv_conf.h` 中开启了文件系统支持：

```c
/* 启用文件系统（必须）*/
#define LV_USE_FS_STDIO  1         // 使用标准 C stdio（fopen/fread）
#define LV_FS_STDIO_LETTER  'S'    // 对应驱动器字母（与注册时一致）
#define LV_FS_STDIO_PATH    ""     // 路径前缀（驱动内部自己拼，留空）
#define LV_FS_STDIO_CACHE_SIZE  0  // 读缓存，0 = 禁用

/* 或者完全手动注册驱动（更灵活，推荐）*/
/* 上面的配置和手动注册二选一 */
```

> 手动注册驱动（第四、五章的方式）比 `LV_USE_FS_STDIO` 更灵活，
> 可以同时注册多个驱动器字母（如 `S:` = SD 卡，`F:` = SPIFFS）。

---

## 八、同时使用多个存储介质

可以注册不同驱动器字母，同时访问 SD 卡和 SPIFFS：

```c
void fs_all_init(void)
{
    spiffs_init();
    sd_card_init();

    lvgl_spiffs_fs_init();  // 注册 'F:' → /spiffs
    lvgl_sd_fs_init();      // 注册 'S:' → /sdcard
}

// 使用时按驱动器字母区分
lv_img_set_src(img1, "F:icon_wifi.bin");      // 从内部 Flash
lv_img_set_src(img2, "S:images/bg_home.bin"); // 从 SD 卡
```

---

## 九、初始化顺序

```
1. sd_card_init() / spiffs_init()   ← 先挂载文件系统
2. lv_init()                        ← 初始化 LVGL
3. display_init()                   ← 注册显示驱动
4. lvgl_sd_fs_init()                ← 注册 LVGL 文件系统驱动
5. ui_init()                        ← 创建 UI（此时可以使用文件路径加载图片）
```

---

## 十、常见问题

### Q1：图片显示乱码/花屏

颜色格式不匹配。重新转换时确认：
- `LV_COLOR_DEPTH 16` → 工具选 `True color`
- `LV_COLOR_DEPTH 32` → 工具选 `True color with alpha`

### Q2：`lv_img_set_src` 无反应，图片不显示

- 检查驱动器字母是否与注册时一致
- 检查文件路径是否正确（注意大小写，FATFS 不区分，SPIFFS 区分）
- 确认文件系统已挂载成功（看串口日志）

### Q3：SD 卡与 LCD 屏 SPI 冲突

两者使用不同的 SPI Host：
```c
SPI2_HOST → LCD 屏
SPI3_HOST → SD 卡
```

### Q4：SPIFFS 上传后文件找不到

SPIFFS 路径没有子目录支持（文件名包含 `/` 会出问题）。
所有文件放根目录，用文件名区分：
```
/spiffs/bg_home.bin      ✅
/spiffs/images/bg.bin    ❌ SPIFFS 不支持子目录
```
LittleFS 支持子目录，建议改用 LittleFS。

### Q5：音频播放卡顿

SD 卡 SPI 读取速度有限，建议：
- 先读入 RAM 缓冲区再播放（内存够用时）
- 或增大读取 buf 大小（减少文件操作次数）
- SPI 时钟频率调高（SD 卡通常支持到 20~40MHz）

---

## 📖 相关文档

- [LVGL_Image.md](LVGL_Image.md) — 图像控件 API
- [LVGL_Core.md](LVGL_Core.md) — 核心初始化
- [IDF_SPIFFS.md](../IDF/IDF_SPIFFS.md) — SPIFFS 文件系统
- [PIO_ESP32_GUI_LVGL_指南.md](../ESP32/PIO_ESP32_GUI_LVGL_指南.md) — ESP32 LVGL 完整工程
