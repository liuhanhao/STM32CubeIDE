# lv_bar & lv_slider — 进度条与滑块 API 文档

## 📋 模块概述

- **lv_bar**：只读进度条，用于显示进度、电量、信号强度等
- **lv_slider**：可拖动滑块，用于调节数值（音量、亮度等）

两者共享相似的 API，区别在于 slider 可以被用户拖动。

---

## 📌 启用配置

```c
#define LV_USE_BAR    1
#define LV_USE_SLIDER 1
```

---

## 一、lv_bar — 进度条

### 核心函数

#### lv_bar_create()

```c
lv_obj_t *lv_bar_create(lv_obj_t *parent);
```

---

#### lv_bar_set_value()

设置当前值（可选动画）。

```c
void lv_bar_set_value(lv_obj_t *obj, int32_t value, lv_anim_enable_t anim);
```

---

#### lv_bar_set_range()

设置值范围（默认 0~100）。

```c
void lv_bar_set_range(lv_obj_t *obj, int32_t min, int32_t max);
```

---

#### lv_bar_get_value()

获取当前值。

```c
int32_t lv_bar_get_value(const lv_obj_t *obj);
```

---

#### lv_bar_set_start_value()

设置起始值（用于显示范围段，如电池充电区间）。

```c
void lv_bar_set_start_value(lv_obj_t *obj, int32_t start_value, lv_anim_enable_t anim);
```

---

### 进度条样式部件

| 部件 | 说明 |
|------|------|
| `LV_PART_MAIN` | 背景轨道 |
| `LV_PART_INDICATOR` | 填充部分（进度）|

---

### 完整示例

#### 示例1：基础进度条

```c
lv_obj_t *bar = lv_bar_create(lv_scr_act());
lv_obj_set_size(bar, 200, 15);
lv_obj_center(bar);

/* 自定义颜色 */
lv_obj_set_style_bg_color(bar, lv_color_hex(0x333333), LV_PART_MAIN);
lv_obj_set_style_bg_color(bar, lv_color_hex(0x4CAF50), LV_PART_INDICATOR);
lv_obj_set_style_radius(bar, 7, LV_PART_MAIN);
lv_obj_set_style_radius(bar, 7, LV_PART_INDICATOR);

lv_bar_set_value(bar, 75, LV_ANIM_ON);
```

---

#### 示例2：带标签的进度条

```c
lv_obj_t *cont = lv_obj_create(lv_scr_act());
lv_obj_set_size(cont, 220, 50);
lv_obj_center(cont);
lv_obj_set_style_border_width(cont, 0, LV_PART_MAIN);
lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, LV_PART_MAIN);

lv_obj_t *bar = lv_bar_create(cont);
lv_obj_set_size(bar, 180, 12);
lv_obj_align(bar, LV_ALIGN_LEFT_MID, 0, 0);
lv_obj_set_style_bg_color(bar, lv_color_hex(0x2196F3), LV_PART_INDICATOR);

lv_obj_t *label = lv_label_create(cont);
lv_label_set_text(label, "75%");
lv_obj_align(label, LV_ALIGN_RIGHT_MID, 0, 0);

lv_bar_set_value(bar, 75, LV_ANIM_OFF);
```

---

#### 示例3：电池电量指示

```c
lv_obj_t *create_battery_bar(lv_obj_t *parent, int percent)
{
    lv_obj_t *bar = lv_bar_create(parent);
    lv_obj_set_size(bar, 40, 18);
    lv_bar_set_range(bar, 0, 100);
    lv_bar_set_value(bar, percent, LV_ANIM_OFF);

    /* 根据电量选择颜色 */
    lv_color_t color;
    if (percent > 50)      color = lv_color_hex(0x4CAF50);  // 绿色
    else if (percent > 20) color = lv_color_hex(0xFFC107);  // 黄色
    else                   color = lv_color_hex(0xF44336);  // 红色

    lv_obj_set_style_bg_color(bar, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, color, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar, 3, LV_PART_MAIN);
    lv_obj_set_style_radius(bar, 3, LV_PART_INDICATOR);
    lv_obj_set_style_border_width(bar, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(bar, lv_color_hex(0x888888), LV_PART_MAIN);

    return bar;
}
```

---

#### 示例4：动态更新进度（下载进度）

```c
static lv_obj_t *download_bar;
static lv_obj_t *download_label;

void download_ui_init(void)
{
    download_bar = lv_bar_create(lv_scr_act());
    lv_obj_set_size(download_bar, 240, 15);
    lv_obj_align(download_bar, LV_ALIGN_CENTER, 0, -15);
    lv_obj_set_style_bg_color(download_bar, lv_color_hex(0x2196F3), LV_PART_INDICATOR);

    download_label = lv_label_create(lv_scr_act());
    lv_label_set_text(download_label, "下载中: 0%");
    lv_obj_align(download_label, LV_ALIGN_CENTER, 0, 15);
}

void download_update(int percent)
{
    lv_bar_set_value(download_bar, percent, LV_ANIM_ON);
    lv_label_set_text_fmt(download_label, "下载中: %d%%", percent);

    if (percent >= 100) {
        lv_label_set_text(download_label, "下载完成！");
    }
}
```

---

## 二、lv_slider — 滑块

### 核心函数

#### lv_slider_create()

```c
lv_obj_t *lv_slider_create(lv_obj_t *parent);
```

---

#### lv_slider_set_value()

设置值。

```c
void lv_slider_set_value(lv_obj_t *obj, int32_t value, lv_anim_enable_t anim);
```

---

#### lv_slider_set_range()

设置范围。

```c
void lv_slider_set_range(lv_obj_t *obj, int32_t min, int32_t max);
```

---

#### lv_slider_get_value()

获取当前值。

```c
int32_t lv_slider_get_value(const lv_obj_t *obj);
```

---

#### lv_slider_is_dragged()

检查是否正在被拖动。

```c
bool lv_slider_is_dragged(const lv_obj_t *obj);
```

---

### 滑块样式部件

| 部件 | 说明 |
|------|------|
| `LV_PART_MAIN` | 背景轨道 |
| `LV_PART_INDICATOR` | 已填充部分 |
| `LV_PART_KNOB` | 拖动旋钮 |

---

### 完整示例

#### 示例5：音量滑块

```c
static void volume_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    int32_t vol = lv_slider_get_value(slider);
    ESP_LOGI("SLIDER", "音量: %ld", vol);
    /* set_volume(vol); */
}

lv_obj_t *slider = lv_slider_create(lv_scr_act());
lv_obj_set_size(slider, 200, 10);
lv_obj_center(slider);
lv_slider_set_range(slider, 0, 100);
lv_slider_set_value(slider, 60, LV_ANIM_OFF);

/* 自定义样式 */
lv_obj_set_style_bg_color(slider, lv_color_hex(0x333333), LV_PART_MAIN);
lv_obj_set_style_bg_color(slider, lv_color_hex(0x2196F3), LV_PART_INDICATOR);
lv_obj_set_style_bg_color(slider, lv_color_hex(0xffffff), LV_PART_KNOB);
lv_obj_set_style_pad_all(slider, 6, LV_PART_KNOB);  // 旋钮大小

lv_obj_add_event_cb(slider, volume_cb, LV_EVENT_VALUE_CHANGED, NULL);
```

---

#### 示例6：带实时数值显示的滑块

```c
static lv_obj_t *brightness_label;

static void brightness_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    int32_t val = lv_slider_get_value(slider);
    lv_label_set_text_fmt(brightness_label, "亮度: %ld%%", val);
}

void brightness_control_init(lv_obj_t *parent)
{
    brightness_label = lv_label_create(parent);
    lv_label_set_text(brightness_label, "亮度: 80%");
    lv_obj_align(brightness_label, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *slider = lv_slider_create(parent);
    lv_obj_set_size(slider, 200, 10);
    lv_obj_align(slider, LV_ALIGN_TOP_MID, 0, 40);
    lv_slider_set_value(slider, 80, LV_ANIM_OFF);

    lv_obj_add_event_cb(slider, brightness_cb, LV_EVENT_VALUE_CHANGED, NULL);
}
```

---

#### 示例7：垂直滑块

```c
lv_obj_t *slider = lv_slider_create(lv_scr_act());
lv_obj_set_size(slider, 10, 150);  // 宽 < 高 → 自动变为垂直
lv_obj_center(slider);
lv_slider_set_value(slider, 50, LV_ANIM_OFF);
```

---

## ⚠️ 注意事项

### 1. 进度条动画

`lv_bar_set_value(bar, value, LV_ANIM_ON)` 会触发平滑动画，
动画时长由 `lv_obj_set_style_anim_time` 控制（默认 200ms）。

### 2. 滑块事件触发时机

`LV_EVENT_VALUE_CHANGED` 在拖动过程中持续触发。
如果只需要拖动结束后的值，检查 `lv_slider_is_dragged()` 为 false 时处理。

### 3. 范围设置顺序

先设置范围，再设置值，否则值可能被裁剪：
```c
lv_bar_set_range(bar, 0, 1000);  // 先设范围
lv_bar_set_value(bar, 750, LV_ANIM_OFF);  // 再设值
```

---

## 📖 相关文档

- [LVGL_Core.md](LVGL_Core.md) — 事件系统
- [LVGL_Arc_Meter.md](LVGL_Arc_Meter.md) — 圆弧与仪表盘
- [LVGL_Style.md](LVGL_Style.md) — 样式
- [README.md](README.md) — 模块导航
