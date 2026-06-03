# LVGL 动画系统 API 文档

## 📋 模块概述

LVGL 内置动画系统，可以对任意数值属性做平滑过渡：
- 位置、尺寸、透明度、颜色等任意属性
- 多种缓动函数（线性、弹性、回弹等）
- 支持延迟、重复、往返播放
- 路径动画（自定义插值函数）

---

## 一、基础动画

### 核心函数

#### lv_anim_init()

初始化动画描述符（必须先调用）。

```c
void lv_anim_init(lv_anim_t *a);
```

---

#### lv_anim_set_var()

设置动画目标对象。

```c
void lv_anim_set_var(lv_anim_t *a, void *var);
```

---

#### lv_anim_set_exec_cb()

设置执行回调（每帧调用，用于更新属性值）。

```c
void lv_anim_set_exec_cb(lv_anim_t *a, lv_anim_exec_xcb_t exec_cb);
```

**回调函数签名：**
```c
void exec_cb(void *var, int32_t value);
// var: 目标对象（lv_anim_set_var 设置的）
// value: 当前插值（在 start_value 和 end_value 之间）
```

---

#### lv_anim_set_values()

设置起始值和结束值。

```c
void lv_anim_set_values(lv_anim_t *a, int32_t start, int32_t end);
```

---

#### lv_anim_set_time()

设置动画时长（毫秒）。

```c
void lv_anim_set_time(lv_anim_t *a, uint32_t duration);
```

---

#### lv_anim_set_delay()

设置延迟时间（毫秒，动画开始前等待）。

```c
void lv_anim_set_delay(lv_anim_t *a, uint32_t delay);
```

---

#### lv_anim_set_repeat_count()

设置重复次数。

```c
void lv_anim_set_repeat_count(lv_anim_t *a, uint16_t cnt);
```

**特殊值：** `LV_ANIM_REPEAT_INFINITE` = 无限重复

---

#### lv_anim_set_repeat_delay()

设置每次重复之间的延迟。

```c
void lv_anim_set_repeat_delay(lv_anim_t *a, uint32_t delay);
```

---

#### lv_anim_set_playback_time()

设置回放时长（动画播放完后反向播放）。

```c
void lv_anim_set_playback_time(lv_anim_t *a, uint32_t time);
```

---

#### lv_anim_set_playback_delay()

设置回放前的延迟。

```c
void lv_anim_set_playback_delay(lv_anim_t *a, uint32_t delay);
```

---

#### lv_anim_set_path_cb()

设置缓动函数（插值路径）。

```c
void lv_anim_set_path_cb(lv_anim_t *a, lv_anim_path_cb_t path_cb);
```

**内置缓动函数：**

| 函数 | 效果 |
|------|------|
| `lv_anim_path_linear` | 线性（匀速，默认）|
| `lv_anim_path_ease_in` | 缓入（慢→快）|
| `lv_anim_path_ease_out` | 缓出（快→慢）|
| `lv_anim_path_ease_in_out` | 缓入缓出（慢→快→慢）|
| `lv_anim_path_overshoot` | 超出后回弹 |
| `lv_anim_path_bounce` | 弹跳效果 |
| `lv_anim_path_step` | 步进（直接跳到终点）|

---

#### lv_anim_set_ready_cb()

设置动画完成回调。

```c
void lv_anim_set_ready_cb(lv_anim_t *a, lv_anim_ready_cb_t ready_cb);
```

---

#### lv_anim_start()

启动动画。

```c
lv_anim_t *lv_anim_start(const lv_anim_t *a);
```

**返回值：** 动画句柄（可用于后续操作）

---

#### lv_anim_del()

删除动画。

```c
bool lv_anim_del(void *var, lv_anim_exec_xcb_t exec_cb);
```

**参数：** `exec_cb = NULL` 表示删除该对象的所有动画

---

## 二、完整示例

### 示例1：位置动画（从左滑入）

```c
lv_obj_t *box = lv_obj_create(lv_scr_act());
lv_obj_set_size(box, 100, 50);
lv_obj_set_pos(box, -100, 100);  // 初始在屏幕左侧外

lv_anim_t a;
lv_anim_init(&a);
lv_anim_set_var(&a, box);
lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_x);  // 修改 X 坐标
lv_anim_set_values(&a, -100, 110);  // 从 -100 到 110
lv_anim_set_time(&a, 500);          // 500ms
lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
lv_anim_start(&a);
```

---

### 示例2：透明度淡入淡出（无限循环）

```c
lv_obj_t *label = lv_label_create(lv_scr_act());
lv_label_set_text(label, "闪烁提示");
lv_obj_center(label);

lv_anim_t a;
lv_anim_init(&a);
lv_anim_set_var(&a, label);
lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);  // 透明 → 不透明
lv_anim_set_time(&a, 800);
lv_anim_set_playback_time(&a, 800);   // 反向播放（不透明 → 透明）
lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
lv_anim_start(&a);
```

---

### 示例3：尺寸弹性动画（点击反馈）

```c
static void btn_press_anim(lv_obj_t *btn, bool pressed)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, btn);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_width);
    lv_anim_set_time(&a, 150);

    if (pressed) {
        lv_anim_set_values(&a, lv_obj_get_width(btn),
                           lv_obj_get_width(btn) - 10);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
    } else {
        lv_anim_set_values(&a, lv_obj_get_width(btn),
                           lv_obj_get_width(btn) + 10);
        lv_anim_set_path_cb(&a, lv_anim_path_overshoot);
    }
    lv_anim_start(&a);
}

static void btn_event_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSED) {
        btn_press_anim(btn, true);
    } else if (code == LV_EVENT_RELEASED) {
        btn_press_anim(btn, false);
    }
}
```

---

### 示例4：进度条动画

```c
lv_obj_t *bar = lv_bar_create(lv_scr_act());
lv_obj_set_size(bar, 200, 15);
lv_obj_center(bar);

/* 方法1：使用 lv_bar_set_value 内置动画 */
lv_bar_set_value(bar, 80, LV_ANIM_ON);

/* 方法2：自定义动画（更多控制）*/
lv_anim_t a;
lv_anim_init(&a);
lv_anim_set_var(&a, bar);
lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_bar_set_value);
lv_anim_set_values(&a, 0, 100);
lv_anim_set_time(&a, 2000);
lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
lv_anim_start(&a);
```

---

### 示例5：屏幕切换动画

```c
/* 创建新屏幕 */
lv_obj_t *new_screen = lv_obj_create(NULL);
lv_obj_set_style_bg_color(new_screen, lv_color_hex(0x1a1a2e), LV_PART_MAIN);

/* 添加内容到新屏幕 */
lv_obj_t *label = lv_label_create(new_screen);
lv_label_set_text(label, "新页面");
lv_obj_center(label);

/* 带动画切换（淡入，300ms，无延迟，切换后删除旧屏幕）*/
lv_scr_load_anim(new_screen, LV_SCR_LOAD_ANIM_FADE_ON, 300, 0, true);
```

---

### 示例6：自定义属性动画（颜色渐变）

LVGL 动画只支持 `int32_t` 类型，颜色需要分通道动画或使用自定义回调：

```c
typedef struct {
    lv_obj_t *obj;
    lv_color_t start_color;
    lv_color_t end_color;
} color_anim_ctx_t;

static void color_anim_cb(color_anim_ctx_t *ctx, int32_t v)
{
    /* v 从 0 到 255，表示插值比例 */
    lv_color_t color = lv_color_mix(ctx->end_color, ctx->start_color, v);
    lv_obj_set_style_bg_color(ctx->obj, color, LV_PART_MAIN);
}

void start_color_anim(lv_obj_t *obj,
                       lv_color_t from_color,
                       lv_color_t to_color,
                       uint32_t duration)
{
    static color_anim_ctx_t ctx;  // 必须是静态的
    ctx.obj         = obj;
    ctx.start_color = from_color;
    ctx.end_color   = to_color;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, &ctx);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)color_anim_cb);
    lv_anim_set_values(&a, 0, 255);
    lv_anim_set_time(&a, duration);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);
}

/* 使用 */
start_color_anim(box, lv_color_hex(0x2196F3), lv_color_hex(0xF44336), 1000);
```

---

## 三、LEDC 渐变（配合 ESP-IDF）

LVGL 动画也可以驱动硬件 PWM，实现 LED 渐变：

```c
static void led_brightness_cb(void *var, int32_t value)
{
    /* value: 0~100 */
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0,
                  (value * 8191) / 100);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

void led_fade_in(void)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, NULL);  // 不需要对象
    lv_anim_set_exec_cb(&a, led_brightness_cb);
    lv_anim_set_values(&a, 0, 100);
    lv_anim_set_time(&a, 1000);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}
```

---

## ⚠️ 注意事项

### 1. 动画在 lv_timer_handler 中更新

动画帧更新依赖 `lv_timer_handler` 的调用，确保它被周期性调用（每 5~10ms）。

### 2. 删除对象前停止动画

删除对象前，先停止该对象上的所有动画，否则动画回调会访问已释放的内存：

```c
lv_anim_del(obj, NULL);  // 删除该对象的所有动画
lv_obj_del(obj);
```

### 3. exec_cb 的类型转换

`lv_anim_set_exec_cb` 接受 `lv_anim_exec_xcb_t`（`void (*)(void*, int32_t)`），
LVGL 内置的 setter 函数（如 `lv_obj_set_x`）签名兼容，可以直接强制转换：

```c
lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_x);
```

### 4. 动画值范围

动画值是 `int32_t`，对于透明度（0~255）、角度（0~3600）等都可以直接使用。
颜色需要自定义回调（见示例6）。

---

## 📖 相关文档

- [LVGL_Core.md](LVGL_Core.md) — 核心概念
- [LVGL_Widgets.md](LVGL_Widgets.md) — 常用控件
- [LVGL_Style.md](LVGL_Style.md) — 样式与主题
- [LVGL_Layout.md](LVGL_Layout.md) — 布局系统
- [README.md](README.md) — 模块导航
