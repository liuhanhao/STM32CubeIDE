# Mac 环境下使用 PlatformIO 开发 ESP32-S3-N16R8 模板工程指南

## 芯片规格说明

| 参数 | 规格 |
|------|------|
| 芯片 | ESP32-S3 (Xtensa LX7 双核, 240MHz) |
| Flash | 16MB (N16) |
| PSRAM | 8MB Octal SPI (R8) |
| USB | 内置 USB-OTG / USB-Serial-JTAG |
| WiFi | 2.4GHz 802.11 b/g/n |
| 蓝牙 | BLE 5.0 |

---

## 一、环境准备

### 1.1 安装 VS Code

从 [https://code.visualstudio.com](https://code.visualstudio.com) 下载并安装 VS Code。

### 1.2 安装 PlatformIO IDE 插件

1. 打开 VS Code，点击左侧扩展图标（`⌘ + Shift + X`）
2. 搜索 `PlatformIO IDE`，点击 **Install**
3. 安装完成后**重启 VS Code**

首次启动 PlatformIO 会自动安装 Python 虚拟环境，需要等几分钟。

### 1.3 解决国内安装失败问题（重要）

PlatformIO 默认使用 PyPI 官方源，在国内可能失败。需要将镜像源改为阿里云：

1. 按 `⌘ + Shift + P` → 输入 `Open User Settings JSON`
2. 添加或修改以下配置：

```json
"platformio-ide.customPyPiIndexUrl": "https://mirrors.aliyun.com/pypi/simple/"
```

3. 保存后，删除损坏的虚拟环境并重启 VS Code：

```bash
rm -rf ~/.platformio/penv
```

> 注意：不能使用 `~` 波浪号，必须写绝对路径，否则该设置无效。

### 1.4 ESP32 平台包

**无需手动安装**。PlatformIO 在首次编译时会自动检测 `platformio.ini` 里的 `platform = espressif32` 并自动下载，直接创建工程编译即可。

### 1.5 检查驱动（USB 转串口）

ESP32-S3 有两种连接方式：

**方式 A：USB-Serial-JTAG（芯片内置，无需驱动，推荐）**
- 使用标注 `USB` 的 Type-C 口连接
- macOS 自动识别为 `/dev/cu.usbmodem*`

**方式 B：外置 USB-UART 芯片（CH340/CP2102）**
- CH340：[https://www.wch.cn/downloads/CH341SER_MAC_ZIP.html](https://www.wch.cn/downloads/CH341SER_MAC_ZIP.html)
- CP2102：[https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers)
- macOS 识别为 `/dev/cu.usbserial-*`

验证设备是否识别：
```bash
ls /dev/cu.*
```

---

## 二、创建工程

### 2.1 新建 PlatformIO 工程

1. 点击 VS Code 左侧 PlatformIO 图标，点击 **New Project**
2. 填写配置：
   - **Name**：`esp32s3_template`
   - **Board**：搜索选择 `Espressif ESP32-S3-DevKitC-1-N8 (8 MB QD, No PSRAM)`
   - **Framework**：`Espidf`
   - **Location**：取消勾选 "Use default location"，手动选择目标路径
3. 点击 **Finish**，等待工程创建完成

> Board 选 DevKitC-1-N8 而不是 DevKitM-1（引脚少）或 S3-Box（有特定外设）。Flash 和 PSRAM 参数后续通过配置文件覆盖，与 board 选项无关。

### 2.2 修改默认工程路径

`platformio-ide.projectsDir` 设置中必须使用绝对路径，`~` 不会被展开：

```json
"platformio-ide.projectsDir": "/Users/你的用户名/Desktop/PlatformIO"
```

或通过命令行永久修改 New Project 向导的默认路径：
```bash
~/.platformio/penv/bin/pio settings set projects_dir /Users/你的用户名/Desktop/PlatformIO/Projects
```

### 2.3 工程目录结构（ESP-IDF 框架）

```
esp32s3_template/
├── .pio/                        # PlatformIO 缓存（无需关注）
├── .vscode/                     # VS Code 配置
├── src/
│   ├── CMakeLists.txt
│   └── main.c                   # 主程序入口
├── .gitignore
├── CMakeLists.txt
├── platformio.ini               # PlatformIO 配置（核心）
├── sdkconfig.defaults           # ESP-IDF 自定义配置种子（手动维护）
└── sdkconfig.esp32-s3-n16r8-idf # 编译自动生成的完整配置（勿手动编辑）
```

---

## 三、配置工程

### 3.1 ESP-IDF 框架 platformio.ini（推荐）

```ini
; ESP32-S3-N16R8 | Flash: 16MB | PSRAM: 8MB Octal SPI

[env:esp32-s3-n16r8-idf]
platform        = espressif32
board           = esp32-s3-devkitc-1
framework       = espidf

upload_protocol = esptool
monitor_speed   = 115200
monitor_filters = esp32_exception_decoder, colorize
```

> ESP-IDF 框架不能用 `board_build.*` 和 `build_flags = -DCONFIG_*` 来配置 Flash/PSRAM，
> 这些配置会被忽略。必须通过 `sdkconfig.defaults` 文件来配置（见 3.2）。

### 3.2 sdkconfig.defaults（ESP-IDF 框架核心配置）

在工程根目录创建 `sdkconfig.defaults`，这是配置 Flash 和 PSRAM 的正确位置：

```ini
# ESP32-S3-N16R8 sdkconfig.defaults
# Flash: 16MB QIO | PSRAM: 8MB Octal SPI

# ── Flash 配置 ─────────────────────────────
CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y
CONFIG_ESPTOOLPY_FLASHSIZE="16MB"
CONFIG_ESPTOOLPY_FLASHMODE_QIO=y
CONFIG_ESPTOOLPY_FLASHMODE="qio"
CONFIG_ESPTOOLPY_FLASHFREQ_80M=y
CONFIG_ESPTOOLPY_FLASHFREQ="80m"

# ── PSRAM 配置（R8 = 8MB Octal SPI PSRAM）─
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCT=y
CONFIG_SPIRAM_SPEED_80M=y
CONFIG_SPIRAM_FETCH_INSTRUCTIONS=y
CONFIG_SPIRAM_RODATA=y
CONFIG_SPIRAM_TRY_ALLOCATE_WIFI_LWIP=y

# ── 分区表（16MB Flash）────────────────────
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="default_16MB.csv"
CONFIG_PARTITION_TABLE_OFFSET=0x8000
```

### 3.3 关于 sdkconfig 自动生成文件

每次编译后会自动生成 `sdkconfig.esp32-s3-n16r8-idf`（3000+ 行），这是正常的：

```
sdkconfig.defaults（你写的几行）
        ↓  编译时合并
sdkconfig.esp32-s3-n16r8-idf（自动生成的完整配置）
        ↓
最终固件
```

- `sdkconfig.defaults` — 手动维护，提交到 git
- `sdkconfig.*` — 自动生成，建议加入 `.gitignore`

验证配置是否生效，在生成的 sdkconfig 文件中搜索：
```
CONFIG_ESPTOOLPY_FLASHSIZE="16MB"   ← Flash 正确
CONFIG_SPIRAM_MODE_OCT=y            ← PSRAM 正确
```


---

## 四、模板主程序

### 4.1 ESP-IDF 框架 src/main.c

```c
#include <esp_log.h>
#include <esp_psram.h>
#include <esp_chip_info.h>
#include <esp_flash.h>

static const char *TAG = "MAIN";

void app_main(void)
{
    /* ── 芯片信息 ── */
    esp_chip_info_t chip;
    esp_chip_info(&chip);
    ESP_LOGI(TAG, "芯片: ESP32-S3, 核心数: %d, 版本: %d",
             chip.cores, chip.revision);

    /* ── Flash 大小 ── */
    uint32_t flash_size = 0;
    esp_flash_get_size(NULL, &flash_size);
    ESP_LOGI(TAG, "Flash 大小: %lu MB", flash_size / (1024 * 1024));

    /* ── PSRAM ── */
    if (esp_psram_is_initialized()) {
        size_t psram_size = esp_psram_get_size();
        ESP_LOGI(TAG, "PSRAM 大小: %u MB", (unsigned)(psram_size / (1024 * 1024)));
    } else {
        ESP_LOGE(TAG, "PSRAM 未初始化，检查 sdkconfig.defaults 配置");
    }

    /* ── 堆内存 ── */
    ESP_LOGI(TAG, "内部堆可用: %lu bytes", esp_get_free_heap_size());
}
```

预期串口输出：
```
I (xxx) MAIN: 芯片: ESP32-S3, 核心数: 2, 版本: 0
I (xxx) MAIN: Flash 大小: 16 MB
I (xxx) MAIN: PSRAM 大小: 8 MB
I (xxx) MAIN: 内部堆可用: xxxxxx bytes
```

---

## 五、编译与烧录

### 5.1 编译

```bash
pio run
```

或点击 VS Code 底部状态栏 **✓ Build** 按钮。

首次编译会自动下载 `espressif32` 平台包和 ESP-IDF 工具链，需要等待 5~15 分钟。

### 5.2 进入下载模式

**自动（推荐）**：直接点击 **→ Upload**，esptool 自动控制 DTR/RTS 进入下载模式。

**手动（自动失败时）**：
1. 按住 **BOOT**
2. 按一下 **RESET**，松开
3. 松开 **BOOT**

### 5.3 上传

```bash
# 自动检测端口
pio run --target upload

# 指定端口
pio run --target upload --upload-port /dev/cu.usbmodem1101
```

### 5.4 串口监视器

```bash
pio device monitor --port /dev/cu.usbmodem1101 --baud 115200
```

或点击底部状态栏 **🔌 Monitor** 按钮。

> 使用内置 USB 做串口时，每次复位后 Mac 需要约 1-2 秒重新枚举，串口监视器会短暂断开，属正常现象。

---

## 六、常见问题

### Q1：PlatformIO 安装失败，报 HTTP 403

**原因：** PyPI 镜像（通常是清华源）对新版 pip/platformio 的 whl 文件返回 403。

**解决：**
1. 在 VS Code `settings.json` 中改为阿里云镜像：
   ```json
   "platformio-ide.customPyPiIndexUrl": "https://mirrors.aliyun.com/pypi/simple/"
   ```
2. 删除损坏的虚拟环境：`rm -rf ~/.platformio/penv`
3. 重启 VS Code

### Q2：PSRAM 大小为 0 或未检测到

检查 `sdkconfig.defaults` 是否包含：
```ini
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCT=y
```

### Q3：Flash 大小显示为 2MB 而不是 16MB

**原因：** ESP-IDF 框架中 `board_build.flash_size = 16MB` 对 sdkconfig 无效。

**解决：** 在 `sdkconfig.defaults` 中添加：
```ini
CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y
CONFIG_ESPTOOLPY_FLASHSIZE="16MB"
```
然后删除旧的 `sdkconfig.*` 文件，重新编译。

### Q4：上传失败 "Failed to connect to ESP32-S3"

- 手动进入 Boot 模式（参考五-5.2）
- 端口被占用 → 关闭其他串口监视器
- 确认端口：`ls /dev/cu.*`

### Q5：Mac 上 USB 设备频繁断开

```bash
sudo pmset -a disablesleep 1
```

或换用有源 USB Hub。

---

## 七、推荐分区表（16MB Flash）

如需自定义分区，在工程根目录创建 `partitions.csv`：

```csv
# Name,    Type, SubType,  Offset,   Size
nvs,       data, nvs,      0x9000,   0x5000
otadata,   data, ota,      0xe000,   0x2000
app0,      app,  ota_0,    0x10000,  0x640000
app1,      app,  ota_1,    0x650000, 0x640000
spiffs,    data, spiffs,   0xC90000, 0x360000
coredump,  data, coredump, 0xFF0000, 0x10000
```

ESP-IDF 框架在 `sdkconfig.defaults` 中引用：
```ini
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"
```

---

## 八、验证清单

- [ ] 编译通过，无错误
- [ ] 串口输出芯片型号 `ESP32-S3`
- [ ] Flash 大小显示 `16 MB`
- [ ] PSRAM 大小显示 `8 MB`
- [ ] `sdkconfig.*` 中 `CONFIG_SPIRAM_MODE_OCT=y`

---

## 参考资料

- [PlatformIO ESP32-S3 文档](https://docs.platformio.org/en/latest/boards/espressif32/esp32-s3-devkitc-1.html)
- [ESP32-S3 技术参考手册](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf)
- [ESP-IDF PSRAM 文档](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/flash_psram_config.html)
- [Espressif ESP32-S3 数据手册](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)
