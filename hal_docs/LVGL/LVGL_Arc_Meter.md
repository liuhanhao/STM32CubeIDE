# lv_arc & lv_meter — 圆弧与仪表盘 API 文档

## 📋 模块概述

- **lv_arc**：圆弧形进度条，可作为旋钮使用（可拖动）
- **lv_meter**：模拟仪表盘，支持多个刻度盘、指针和颜色弧段

---

## 📌 启用配置

```c
#define LV_USE_ARC   1
#define LV_USE_METER 1
```

---

## 一、lv_arc — 圆弧

### 核心函数

#### lv_arc_create()

```c
lv_obj_t *lv_arc_create(lv_obj_t *parent);
```

---

#### lv_arc_set_value()

设置当前值。

```c
void lv_arc_set_value(lv_obj_t *obj, int16_t value);
```

---

#### lv_arc_set_range()

设置值范围（默认 0~100）。

```c
void lv_arc_set_range(lv_obj_t *obj, int16_t min, int16_t max);
```

---

#### lv_arc_set_bg_angles()

设置背景弧的起止角度。

```c
void lv_arc_set_bg_angles(lv_obj_t *obj, uint16_t start, uint16_t end);
```

**角度说明：** 0° = 右侧（3点钟方向），顺时针增加。
- 135° ~ 45°（顺时针 270°）= 典型仪表盘范围

---

#### lv_arc_set_angles()

设置指示弧的起止角度（直接控制，不通过 value）。

```c
void lv_arc_set_angles(lv_obj_t *obj, uint16_t start, uint16_t end);
```

---

#### lv_arc_set_rotation()

旋转整个圆弧（改变起始方向）。

```c
void lv_arc_set_rotation(lv_obj_t *obj, uint16_t rotation);
```

---

#### lv_arc_set_mode()

设置圆弧模式。

```c
void lv_arc_set_mode(lv_obj_t *obj, lv_arc_mode_t type);
```

| 模式 | 说明 |
|------|------|
| `LV_ARC_MODE_NORMAL` | 从起点到当前值（默认）|
| `LV_ARC_MODE_REVERSE` | 从当前值到终点（反向）|
| `LV_ARC_MODE_SYMMETRICAL` | 从中点向两侧扩展 |

---

### 圆弧样式部件

| 部件 | 说明 |
|------|------|
| `LV_PART_MAIN` | 背景弧 |
| `LV_PART_INDICATOR` | 指示弧（进度）|
| `LV_PART_KNOB` | 旋钮（可拖动点）|

---

### 完整示例

#### 示例1：仪表盘风格圆弧（只读）

```c
lv_obj_t *arc = lv_arc_create(lv_scr_act());
lv_obj_set_size(arc, 150, 150);
lv_obj_center(arc);

/* 背景弧：135° ~ 45°（270° 范围）*/
lv_arc_set_bg_angles(arc, 135, 45);
lv_arc_set_range(arc, 0, 100);
lv_arc_set_value(arc, 75);

/* 背景弧样式 */
lv_obj_set_style_arc_color(arc, lv_color_hex(0x333333), LV_PART_MAIN);
lv_obj_set_style_arc_width(arc, 12, LV_PART_MAIN);

/* 指示弧样式 */
lv_obj_set_style_arc_color(arc, lv_color_hex(0x2196F3), LV_PART_INDICATOR);
lv_obj_set_style_arc_width(arc, 12, LV_PART_INDICATOR);
lv_obj_set_style_arc_rounded(arc, true, LV_PART_INDICATOR);  // 圆头

/* 隐藏旋钮（纯显示）*/
lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);

/* 中心数值标签 */
lv_obj_t *label = lv_label_create(lv_scr_act());
lv_label_set_text(label, "75%");
lv_obj_set_style_text_font(label, &lv_font_montserrat_16, LV_PART_MAIN);
lv_obj_align_to(label, arc, LV_ALIGN_CENTER, 0, 0);
```

---

#### 示例2：可拖动旋钮（音量控制）

```c
static lv_obj_t *vol_label;

static void arc_cb(lv_event_t *e)
{
    lv_obj_t *arc = lv_event_get_target(e);
    int16_t val = lv_arc_get_value(arc);
    lv_label_set_text_fmt(vol_label, "%d", val);
}

void volume_arc_init(lv_obj_t *parent)
{
    lv_obj_t *arc = lv_arc_create(parent);
    lv_obj_set_size(arc, 120, 120);
    lv_obj_center(arc);

    lv_arc_set_bg_angles(arc, 135, 45);
    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, 60);

    lv_obj_set_style_arc_color(arc, lv_color_hex(0x4CAF50), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc, 10, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 10, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(arc, lv_color_hex(0x2196F3), LV_PART_KNOB);
    lv_obj_set_style_pad_all(arc, 4, LV_PART_KNOB);

    vol_label = lv_label_create(parent);
    lv_label_set_text(vol_label, "60");
    lv_obj_set_style_text_font(vol_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align_to(vol_label, arc, LV_ALIGN_CENTER, 0, 0);

    lv_obj_add_event_cb(arc, arc_cb, LV_EVENT_VALUE_CHANGED, NULL);
}
```

---

#### 示例3：加载动画（旋转圆弧）

```c
/* 使用 lv_spinner 更简单，但自定义圆弧也可以实现 */
lv_obj_t *arc = lv_arc_create(lv_scr_act());
lv_obj_set_size(arc, 60, 60);
lv_obj_center(arc);

lv_arc_set_bg_angles(arc, 0, 360);
lv_arc_set_angles(arc, 0, 60);  // 60° 的弧段

lv_obj_set_style_arc_color(arc, lv_color_hex(0x2196F3), LV_PART_INDICATOR);
lv_obj_set_style_arc_width(arc, 4, LV_PART_INDICATOR);
lv_obj_remove_style(arc, NULL, LV_PART_MAIN);
lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);

/* 旋转动画 */
lv_anim_t a;
lv_anim_init(&a);
lv_anim_set_var(&a, arc);
lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_arc_set_rotation);
lv_anim_set_values(&a, 0, 360);
lv_anim_set_time(&a, 1000);
lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
lv_anim_set_path_cb(&a, lv_anim_path_linear);
lv_anim_start(&a);
```

---

## 二、lv_meter — 仪表盘

### 核心函数

#### lv_meter_create()

```c
lv_obj_t *lv_meter_create(lv_obj_t *parent);
```

---

#### lv_meter_add_scale()

添加刻度盘。

```c
lv_meter_scale_t *lv_meter_add_scale(lv_obj_t *obj);
```

---

#### lv_meter_set_scale_ticks()

设置刻度线。

```c
void lv_meter_set_scale_ticks(lv_obj_t *obj, lv_meter_scale_t *scale,
                               uint16_t cnt, uint16_t width, uint16_t len,
                               lv_color_t color);
```

---

#### lv_meter_set_scale_major_ticks()

设置主刻度线（带数字标签）。

```c
void lv_meter_set_scale_major_ticks(lv_obj_t *obj, lv_meter_scale_t *scale,
                                     uint16_t nth, uint16_t width, uint16_t len,
                                     lv_color_t color, int16_t label_gap);
```

---

#### lv_meter_set_scale_range()

设置刻度范围和角度。

```c
void lv_meter_set_scale_range(lv_obj_t *obj, lv_meter_scale_t *scale,
                               int32_t min, int32_t max,
                               uint32_t angle_range, uint32_t rotation);
```

---

#### lv_meter_add_arc()

添加颜色弧段（危险区域标注）。

```c
lv_meter_indicator_t *lv_meter_add_arc(lv_obj_t *obj, lv_meter_scale_t *scale,
                                        uint16_t width, lv_color_t color,
                                        int16_t r_mod);
```

---

#### lv_meter_add_needle_line()

添加指针。

```c
lv_meter_indicator_t *lv_meter_add_needle_line(lv_obj_t *obj,
                                                lv_meter_scale_t *scale,
                                                uint16_t width,
                                                lv_color_t color,
                                                int16_t r_mod);
```

---

#### lv_meter_set_indicator_value()

设置指针/弧段的值。

```c
void lv_meter_set_indicator_value(lv_obj_t *obj,
                                   lv_meter_indicator_t *indic,
                                   int32_t value);
```

---

#### lv_meter_set_indicator_start_value() / lv_meter_set_indicator_end_value()

设置弧段的起止值。

```c
void lv_meter_set_indicator_start_value(lv_obj_t *obj,
                                         lv_meter_indicator_t *indic,
                                         int32_t value);
void lv_meter_set_indicator_end_value(lv_obj_t *obj,
                                       lv_meter_indicator_t *indic,
                                       int32_t value);
```

---

### 完整示例

#### 示例4：速度仪表盘

```c
static lv_meter_indicator_t *speed_needle;

lv_obj_t *meter = lv_meter_create(lv_scr_act());
lv_obj_set_size(meter, 200, 200);
lv_obj_center(meter);

/* 添加刻度盘 */
lv_meter_scale_t *scale = lv_meter_add_scale(meter);

/* 小刻度线（41 条，每 2° 一条）*/
lv_meter_set_scale_ticks(meter, scale, 41, 2, 10,
                          lv_color_hex(0x888888));

/* 主刻度线（每 8 条小刻度一条主刻度，带数字）*/
lv_meter_set_scale_major_ticks(meter, scale, 8, 4, 15,
                                lv_color_hex(0xffffff), 10);

/* 范围：0~200，角度 270°，起始 135° */
lv_meter_set_scale_range(meter, scale, 0, 200, 270, 135);

/* 绿色区域（0~120）*/
lv_meter_indicator_t *green = lv_meter_add_arc(meter, scale, 3,
                                                lv_color_hex(0x4CAF50), 0);
lv_meter_set_indicator_start_value(meter, green, 0);
lv_meter_set_indicator_end_value(meter, green, 120);

/* 黄色区域（120~160）*/
lv_meter_indicator_t *yellow = lv_meter_add_arc(meter, scale, 3,
                                                  lv_color_hex(0xFFC107), 0);
lv_meter_set_indicator_start_value(meter, yellow, 120);
lv_meter_set_indicator_end_value(meter, yellow, 160);

/* 红色区域（160~200）*/
lv_meter_indicator_t *red = lv_meter_add_arc(meter, scale, 3,
                                               lv_color_hex(0xF44336), 0);
lv_meter_set_indicator_start_value(meter, red, 160);
lv_meter_set_indicator_end_value(meter, red, 200);

/* 指针 */
speed_needle = lv_meter_add_needle_line(meter, scale, 4,
                                         lv_color_hex(0xffffff), -10);
lv_meter_set_indicator_value(meter, speed_needle, 0);

/* 设置速度 */
void set_speed(int speed) {
    lv_meter_set_indicator_value(meter, speed_needle, speed);
}
```

---

## ⚠️ 注意事项

### 1. 圆弧角度方向

LVGL 圆弧角度：0° = 右侧（3点钟），顺时针增加。
典型仪表盘：`lv_arc_set_bg_angles(arc, 135, 45)` 表示从左下到右下的 270° 范围。

### 2. 隐藏旋钮

纯显示用的圆弧需要隐藏旋钮并禁用点击：
```c
lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
```

### 3. 仪表盘性能

`lv_meter` 包含复杂绘制逻辑，在低性能设备上可能影响刷新率。
如果只需要简单圆弧进度，用 `lv_arc` 更轻量。

---

## 📖 相关文档

- [LVGL_Bar_Slider.md](LVGL_Bar_Slider.md) — 进度条与滑块
- [LVGL_Animation.md](LVGL_Animation.md) — 动画（指针动画）
- [LVGL_Style.md](LVGL_Style.md) — 样式
- [README.md](README.md) — 模块导航
