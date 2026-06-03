# lv_img — 图像控件 API 文档

## 📋 模块概述

`lv_img` 用于显示图像，支持：
- C 数组格式图像（编译进固件）
- 文件系统图像（SPIFFS/LittleFS）
- 内置符号图标（Symbol）
- 缩放、旋转、翻转
- 图像解码器（PNG、JPEG 需要额外解码器）

---

## 📌 启用配置

```c
#define LV_USE_IMG 1
```

---

## 一、图像格式

### C 数组格式（最常用）

将图像转换为 C 数组，编译进固件，无需文件系统。

**转换工具：** [https://lvgl.io/tools/imageconverter](https://lvgl.io/tools/imageconverter)

**转换设置：**
- Color format：`True color with alpha`（ARGB8888）或 `True color`（RGB565）
- Output format：`C array`

**生成的文件结构：**
```c
/* my_image.c */
#include "lvgl.h"

const lv_img_dsc_t my_image = {
    .header = {
        .cf     = LV_IMG_CF_TRUE_COLOR_ALPHA,
        .always_zero = 0,
        .reserved    = 0,
        .w      = 100,
        .h      = 100,
    },
    .data_size = 100 * 100 * 4,
    .data      = (const uint8_t[]){ /* 像素数据 */ },
};
```

---

### 符号图标（内置）

LVGL 内置基于字体的符号图标，无需图像文件：

```c
lv_img_set_src(img, LV_SYMBOL_OK);
```

**常用符号：**

| 符号 | 说明 | 符号 | 说明 |
|------|------|------|------|
| `LV_SYMBOL_OK` | ✓ | `LV_SYMBOL_CLOSE` | ✗ |
| `LV_SYMBOL_LEFT` | ← | `LV_SYMBOL_RIGHT` | → |
| `LV_SYMBOL_UP` | ↑ | `LV_SYMBOL_DOWN` | ↓ |
| `LV_SYMBOL_WIFI` | WiFi | `LV_SYMBOL_BLUETOOTH` | 蓝牙 |
| `LV_SYMBOL_BATTERY_FULL` | 电池满 | `LV_SYMBOL_BATTERY_EMPTY` | 电池空 |
| `LV_SYMBOL_HOME` | 主页 | `LV_SYMBOL_SETTINGS` | 设置 |
| `LV_SYMBOL_BELL` | 铃铛 | `LV_SYMBOL_TRASH` | 垃圾桶 |
| `LV_SYMBOL_EDIT` | 编辑 | `LV_SYMBOL_SAVE` | 保存 |
| `LV_SYMBOL_PLAY` | 播放 | `LV_SYMBOL_PAUSE` | 暂停 |
| `LV_SYMBOL_STOP` | 停止 | `LV_SYMBOL_VOLUME_MAX` | 音量 |
| `LV_SYMBOL_REFRESH` | 刷新 | `LV_SYMBOL_POWER` | 电源 |
| `LV_SYMBOL_PLUS` | + | `LV_SYMBOL_MINUS` | - |
| `LV_SYMBOL_WARNING` | ⚠ | `LV_SYMBOL_CHARGE` | 充电 |

---

## 🎯 核心函数

### lv_img_create()

```c
lv_obj_t *lv_img_create(lv_obj_t *parent);
```

---

### lv_img_set_src()

设置图像源。

```c
void lv_img_set_src(lv_obj_t *obj, const void *src);
```

**src 类型：**
```c
/* C 数组 */
LV_IMG_DECLARE(my_image);
lv_img_set_src(img, &my_image);

/* 符号 */
lv_img_set_src(img, LV_SYMBOL_WIFI);

/* 文件系统路径 */
lv_img_set_src(img, "S:/spiffs/logo.bin");  // S: = SPIFFS 驱动器
```

---

### lv_img_set_zoom()

设置缩放（256 = 原始大小，512 = 2倍，128 = 0.5倍）。

```c
void lv_img_set_zoom(lv_obj_t *obj, uint16_t zoom);
```

---

### lv_img_set_angle()

设置旋转角度（0.1° 为单位，900 = 90°）。

```c
void lv_img_set_angle(lv_obj_t *obj, int16_t angle);
```

---

### lv_img_set_pivot()

设置旋转/缩放的中心点（相对于图像左上角）。

```c
void lv_img_set_pivot(lv_obj_t *obj, lv_coord_t pivot_x, lv_coord_t pivot_y);
```

---

### lv_img_set_offset_x() / lv_img_set_offset_y()

设置图像偏移（用于平铺或滚动效果）。

```c
void lv_img_set_offset_x(lv_obj_t *obj, lv_coord_t x);
void lv_img_set_offset_y(lv_obj_t *obj, lv_coord_t y);
```

---

### lv_img_set_antialias()

启用/禁用旋转缩放时的抗锯齿。

```c
void lv_img_set_antialias(lv_obj_t *obj, bool antialias);
```

---

## 💡 完整应用示例

### 示例1：显示 C 数组图像

```c
/* 在 CMakeLists.txt 中添加图像文件 */
/* idf_component_register(SRCS "main.c" "images/logo.c" ...) */

#include "lvgl.h"

/* 声明图像（在 logo.c 中定义）*/
LV_IMG_DECLARE(logo_img);

void show_logo(void)
{
    lv_obj_t *img = lv_img_create(lv_scr_act());
    lv_img_set_src(img, &logo_img);
    lv_obj_align(img, LV_ALIGN_TOP_MID, 0, 10);
}
```

---

### 示例2：符号图标按钮

```c
/* 创建带符号图标的按钮 */
lv_obj_t *create_icon_btn(lv_obj_t *parent,
                            const char *symbol,
                            lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 50, 50);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, LV_PART_MAIN);

    lv_obj_t *icon = lv_label_create(btn);
    lv_label_set_text(icon, symbol);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_center(icon);

    if (cb) lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    return btn;
}

/* 使用 */
lv_obj_t *play_btn  = create_icon_btn(parent, LV_SYMBOL_PLAY, play_cb);
lv_obj_t *pause_btn = create_icon_btn(parent, LV_SYMBOL_PAUSE, pause_cb);
lv_obj_t *stop_btn  = create_icon_btn(parent, LV_SYMBOL_STOP, stop_cb);
```

---

### 示例3：图像旋转动画（加载指示器）

```c
LV_IMG_DECLARE(loading_icon);

void create_loading_indicator(lv_obj_t *parent)
{
    lv_obj_t *img = lv_img_create(parent);
    lv_img_set_src(img, &loading_icon);
    lv_obj_center(img);

    /* 设置旋转中心为图像中心 */
    lv_img_set_pivot(img,
                     loading_icon.header.w / 2,
                     loading_icon.header.h / 2);

    /* 旋转动画 */
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, img);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_img_set_angle);
    lv_anim_set_values(&a, 0, 3600);  // 0° ~ 360°（单位 0.1°）
    lv_anim_set_time(&a, 1000);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&a, lv_anim_path_linear);
    lv_anim_start(&a);
}
```

---

### 示例4：图像缩放动画（呼吸效果）

```c
LV_IMG_DECLARE(heart_icon);

void create_heartbeat_icon(lv_obj_t *parent)
{
    lv_obj_t *img = lv_img_create(parent);
    lv_img_set_src(img, &heart_icon);
    lv_obj_center(img);

    /* 设置旋转中心为图像中心 */
    lv_img_set_pivot(img,
                     heart_icon.header.w / 2,
                     heart_icon.header.h / 2);

    /* 缩放动画（256=原始，300=放大）*/
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, img);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_img_set_zoom);
    lv_anim_set_values(&a, 256, 300);
    lv_anim_set_time(&a, 600);
    lv_anim_set_playback_time(&a, 600);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);
}
```

---

### 示例5：状态栏图标组

```c
void create_status_icons(lv_obj_t *parent)
{
    /* WiFi 信号强度图标 */
    lv_obj_t *wifi_icon = lv_label_create(parent);
    lv_label_set_text(wifi_icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(wifi_icon, lv_color_hex(0x4CAF50), LV_PART_MAIN);
    lv_obj_align(wifi_icon, LV_ALIGN_TOP_RIGHT, -60, 5);

    /* 蓝牙图标 */
    lv_obj_t *bt_icon = lv_label_create(parent);
    lv_label_set_text(bt_icon, LV_SYMBOL_BLUETOOTH);
    lv_obj_set_style_text_color(bt_icon, lv_color_hex(0x2196F3), LV_PART_MAIN);
    lv_obj_align(bt_icon, LV_ALIGN_TOP_RIGHT, -35, 5);

    /* 电池图标 */
    lv_obj_t *bat_icon = lv_label_create(parent);
    lv_label_set_text(bat_icon, LV_SYMBOL_BATTERY_FULL);
    lv_obj_set_style_text_color(bat_icon, lv_color_hex(0x4CAF50), LV_PART_MAIN);
    lv_obj_align(bat_icon, LV_ALIGN_TOP_RIGHT, -10, 5);
}
```

---

## 二、图像转换工具使用

### 在线转换（推荐）

1. 访问 [https://lvgl.io/tools/imageconverter](https://lvgl.io/tools/imageconverter)
2. 上传图片（PNG/JPG/BMP）
3. 设置：
   - **Name**：变量名（如 `my_logo`）
   - **Color format**：`True color with alpha`（推荐）
   - **Output format**：`C array`
4. 下载生成的 `.c` 文件，放入项目

### 在 CMakeLists.txt 中添加

```cmake
idf_component_register(
    SRCS
        "main.c"
        "images/my_logo.c"   # 添加图像文件
    INCLUDE_DIRS "." "images"
    ...
)
```

---

## ⚠️ 注意事项

### 1. 图像内存占用

C 数组图像存储在 Flash 中，不占用 RAM。
但 LVGL 渲染时会将图像数据读入缓冲区，大图像会增加内存压力。

### 2. 旋转/缩放需要设置 pivot

不设置 pivot 时，默认以图像左上角为旋转中心：
```c
/* 以图像中心旋转 */
lv_img_set_pivot(img, img_width / 2, img_height / 2);
```

### 3. 颜色格式匹配

图像的颜色格式必须与 `lv_conf.h` 中的 `LV_COLOR_DEPTH` 匹配：
- `LV_COLOR_DEPTH 16`：使用 `True color`（RGB565）
- `LV_COLOR_DEPTH 32`：使用 `True color with alpha`（ARGB8888）

### 4. 文件系统图像

从文件系统加载图像需要注册 LVGL 文件系统驱动，
并将图像转换为 LVGL 的 `.bin` 格式（不是普通 PNG/JPG）。

---

## 📖 相关文档

- [LVGL_Core.md](LVGL_Core.md) — 核心概念
- [LVGL_Animation.md](LVGL_Animation.md) — 动画（图像旋转/缩放）
- [LVGL_Font.md](LVGL_Font.md) — 字体（符号图标）
- [README.md](README.md) — 模块导航
