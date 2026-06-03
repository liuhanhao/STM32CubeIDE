# LVGL 核心概念 API 文档

## 📋 模块概述

LVGL（Light and Versatile Graphics Library）是一个开源嵌入式 GUI 框架，核心概念包括：
- **对象（Object）**：所有 UI 元素的基类，构成树形结构
- **样式（Style）**：控制外观（颜色、字体、边框、圆角等）
- **事件（Event）**：用户交互和系统通知的回调机制
- **动画（Animation）**：属性值的平滑过渡
- **定时器（Timer）**：周期性任务调度
- **显示驱动（Display Driver）**：连接 LVGL 与实际屏幕

---

## 📚 相关文件

- **头文件**: `lvgl.h`（包含所有 LVGL API）
- **配置文件**: `lv_conf.h`（功能开关和参数配置）

---

## 一、初始化

### lv_init()

初始化 LVGL 库（必须最先调用）。

```c
void lv_init(void);
```

---

### lv_tick_inc()

向 LVGL 提供时间基准（必须每隔固定时间调用）。

```c
void lv_tick_inc(uint32_t tick_period);
```

**参数：** `tick_period` — 距上次调用的毫秒数

**典型用法（esp_timer，2ms 周期）：**
```c
static void lvgl_tick_cb(void *arg) {
    lv_tick_inc(2);
}
```

---

### lv_timer_handler()

处理 LVGL 内部任务（重绘、动画、定时器等），必须在主循环中周期调用。

```c
uint32_t lv_timer_handler(void);
```

**返回值：** 下次调用前建议等待的毫秒数

**典型用法（FreeRTOS 任务）：**
```c
while (1) {
    lv_timer_handler();
    vTaskDelay(pdMS_TO_TICKS(5));
}
```

---

## 二、对象系统

### 对象树结构

LVGL 的所有 UI 元素都是 `lv_obj_t` 对象，构成父子树：

```
lv_scr_act()（当前屏幕，根节点）
├── 容器（lv_obj）
│   ├── 标签（lv_label）
│   └── 按钮（lv_btn）
│       └── 标签（lv_label）
└── 图像（lv_img）
```

- 子对象的坐标相对于父对象
- 父对象删除时，所有子对象自动删除
- 子对象超出父对象范围时默认被裁剪

---

### 核心对象函数

#### lv_obj_create()

创建基础对象（容器）。

```c
lv_obj_t *lv_obj_create(lv_obj_t *parent);
```

**参数：** `parent` — 父对象，传 `lv_scr_act()` 表示放在当前屏幕上

**使用示例：**
```c
/* 创建一个容器 */
lv_obj_t *cont = lv_obj_create(lv_scr_act());
lv_obj_set_size(cont, 200, 100);
lv_obj_center(cont);
```

---

#### lv_obj_del()

删除对象（同时删除所有子对象）。

```c
void lv_obj_del(lv_obj_t *obj);
```

---

#### lv_obj_clean()

删除对象的所有子对象（保留对象本身）。

```c
void lv_obj_clean(lv_obj_t *obj);
```

---

### 位置与尺寸

#### lv_obj_set_pos()

设置对象位置（相对于父对象左上角）。

```c
void lv_obj_set_pos(lv_obj_t *obj, lv_coord_t x, lv_coord_t y);
void lv_obj_set_x(lv_obj_t *obj, lv_coord_t x);
void lv_obj_set_y(lv_obj_t *obj, lv_coord_t y);
```

---

#### lv_obj_set_size()

设置对象尺寸。

```c
void lv_obj_set_size(lv_obj_t *obj, lv_coord_t w, lv_coord_t h);
void lv_obj_set_width(lv_obj_t *obj, lv_coord_t w);
void lv_obj_set_height(lv_obj_t *obj, lv_coord_t h);
```

**特殊尺寸值：**

| 值 | 说明 |
|----|------|
| `LV_SIZE_CONTENT` | 根据内容自动调整大小 |
| `lv_pct(50)` | 父对象的 50% |

---

#### lv_obj_align()

相对于参考点对齐。

```c
void lv_obj_align(lv_obj_t *obj, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs);
```

**常用对齐方式：**

| 常量 | 说明 |
|------|------|
| `LV_ALIGN_CENTER` | 居中 |
| `LV_ALIGN_TOP_MID` | 顶部居中 |
| `LV_ALIGN_BOTTOM_MID` | 底部居中 |
| `LV_ALIGN_LEFT_MID` | 左侧居中 |
| `LV_ALIGN_RIGHT_MID` | 右侧居中 |
| `LV_ALIGN_TOP_LEFT` | 左上角 |
| `LV_ALIGN_TOP_RIGHT` | 右上角 |
| `LV_ALIGN_BOTTOM_LEFT` | 左下角 |
| `LV_ALIGN_BOTTOM_RIGHT` | 右下角 |

---

#### lv_obj_align_to()

相对于另一个对象对齐。

```c
void lv_obj_align_to(lv_obj_t *obj, const lv_obj_t *base,
                      lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs);
```

**使用示例：**
```c
/* 将 label2 放在 label1 的右侧，间距 10px */
lv_obj_align_to(label2, label1, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
```

---

#### lv_obj_center()

在父对象中居中。

```c
void lv_obj_center(lv_obj_t *obj);
```

---

### 显示与隐藏

```c
/* 隐藏对象（不占用布局空间）*/
lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);

/* 显示对象 */
lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);

/* 检查是否隐藏 */
bool hidden = lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN);
```

---

### 常用标志（Flags）

```c
lv_obj_add_flag(obj, flag);    // 添加标志
lv_obj_clear_flag(obj, flag);  // 清除标志
lv_obj_has_flag(obj, flag);    // 检查标志
```

| 标志 | 说明 |
|------|------|
| `LV_OBJ_FLAG_HIDDEN` | 隐藏 |
| `LV_OBJ_FLAG_CLICKABLE` | 可点击（默认开启）|
| `LV_OBJ_FLAG_SCROLLABLE` | 可滚动（默认开启）|
| `LV_OBJ_FLAG_SCROLL_ELASTIC` | 弹性滚动 |
| `LV_OBJ_FLAG_SNAPPABLE` | 可吸附 |
| `LV_OBJ_FLAG_FLOATING` | 浮动（不参与布局）|

---

## 三、事件系统

### 事件注册

#### lv_obj_add_event_cb()

为对象注册事件回调。

```c
lv_event_dsc_t *lv_obj_add_event_cb(lv_obj_t *obj,
                                     lv_event_cb_t event_cb,
                                     lv_event_code_t filter,
                                     void *user_data);
```

**参数：**
- `event_cb`: 回调函数（`void func(lv_event_t *e)` 格式）
- `filter`: 事件过滤（`LV_EVENT_ALL` = 所有事件）
- `user_data`: 传给回调的自定义数据

---

### 常用事件代码

| 事件 | 说明 |
|------|------|
| `LV_EVENT_CLICKED` | 点击（按下后抬起）|
| `LV_EVENT_PRESSED` | 按下 |
| `LV_EVENT_RELEASED` | 抬起 |
| `LV_EVENT_LONG_PRESSED` | 长按 |
| `LV_EVENT_VALUE_CHANGED` | 值改变（滑块、开关等）|
| `LV_EVENT_FOCUSED` | 获得焦点 |
| `LV_EVENT_DEFOCUSED` | 失去焦点 |
| `LV_EVENT_SCROLL` | 滚动 |
| `LV_EVENT_DRAW_MAIN` | 绘制主体（自定义绘制）|

---

### 事件回调中获取信息

```c
void my_event_cb(lv_event_t *e)
{
    /* 获取触发事件的对象 */
    lv_obj_t *target = lv_event_get_target(e);

    /* 获取事件代码 */
    lv_event_code_t code = lv_event_get_code(e);

    /* 获取用户数据 */
    void *user_data = lv_event_get_user_data(e);

    /* 获取触摸点坐标（触摸事件）*/
    lv_indev_t *indev = lv_indev_get_act();
    lv_point_t point;
    lv_indev_get_point(indev, &point);
}
```

---

### 使用示例

```c
/* 按钮点击事件 */
static void btn_click_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);

    if (code == LV_EVENT_CLICKED) {
        static int count = 0;
        count++;
        /* 获取按钮上的标签并更新文字 */
        lv_obj_t *label = lv_obj_get_child(btn, 0);
        lv_label_set_text_fmt(label, "点击 %d 次", count);
    }
}

lv_obj_t *btn = lv_btn_create(lv_scr_act());
lv_obj_add_event_cb(btn, btn_click_cb, LV_EVENT_CLICKED, NULL);
```

---

## 四、样式系统

### 样式基础

LVGL 的样式通过 `lv_style_t` 结构体定义，可以复用到多个对象。

```c
/* 创建并初始化样式 */
static lv_style_t style;
lv_style_init(&style);

/* 设置属性 */
lv_style_set_bg_color(&style, lv_color_hex(0x1a1a2e));
lv_style_set_border_width(&style, 2);
lv_style_set_radius(&style, 10);

/* 应用到对象 */
lv_obj_add_style(obj, &style, LV_PART_MAIN | LV_STATE_DEFAULT);
```

---

### 内联样式（推荐，简单场景）

不需要复用时，直接用 `lv_obj_set_style_*` 系列函数：

```c
/* 背景颜色 */
lv_obj_set_style_bg_color(obj, lv_color_hex(0x2196F3), LV_PART_MAIN);

/* 文字颜色 */
lv_obj_set_style_text_color(obj, lv_color_white(), LV_PART_MAIN);

/* 字体 */
lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN);

/* 圆角 */
lv_obj_set_style_radius(obj, 8, LV_PART_MAIN);

/* 边框 */
lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN);
lv_obj_set_style_border_color(obj, lv_color_hex(0x0d47a1), LV_PART_MAIN);

/* 内边距 */
lv_obj_set_style_pad_all(obj, 10, LV_PART_MAIN);
lv_obj_set_style_pad_top(obj, 5, LV_PART_MAIN);

/* 阴影 */
lv_obj_set_style_shadow_width(obj, 10, LV_PART_MAIN);
lv_obj_set_style_shadow_color(obj, lv_color_hex(0x000000), LV_PART_MAIN);
lv_obj_set_style_shadow_opa(obj, LV_OPA_50, LV_PART_MAIN);

/* 透明度 */
lv_obj_set_style_opa(obj, LV_OPA_70, LV_PART_MAIN);  // 70% 不透明
```

---

### 部件（Part）

对象的不同部分可以独立设置样式：

| 部件 | 说明 |
|------|------|
| `LV_PART_MAIN` | 主体（背景、边框）|
| `LV_PART_SCROLLBAR` | 滚动条 |
| `LV_PART_INDICATOR` | 指示器（进度条填充部分）|
| `LV_PART_KNOB` | 旋钮（滑块的拖动点）|
| `LV_PART_SELECTED` | 选中项 |
| `LV_PART_ITEMS` | 列表项 |
| `LV_PART_CURSOR` | 光标（文本框）|

---

### 状态（State）

对象在不同状态下可以有不同样式：

| 状态 | 说明 |
|------|------|
| `LV_STATE_DEFAULT` | 默认状态 |
| `LV_STATE_FOCUSED` | 获得焦点 |
| `LV_STATE_PRESSED` | 按下 |
| `LV_STATE_CHECKED` | 选中（开关、复选框）|
| `LV_STATE_DISABLED` | 禁用 |

```c
/* 按下时变色 */
lv_obj_set_style_bg_color(btn, lv_color_hex(0x1565C0),
                           LV_PART_MAIN | LV_STATE_PRESSED);
```

---

## 五、颜色

### 颜色创建

```c
/* 从十六进制创建（最常用）*/
lv_color_t color = lv_color_hex(0xFF5722);

/* 从 RGB 分量创建 */
lv_color_t color = lv_color_make(255, 87, 34);

/* 预定义颜色 */
lv_color_white()
lv_color_black()

/* 透明度（0~255，255 = 完全不透明）*/
lv_opa_t opa = LV_OPA_50;   // 50% 不透明
lv_opa_t opa = LV_OPA_COVER; // 完全不透明（255）
lv_opa_t opa = LV_OPA_TRANSP; // 完全透明（0）
```

---

## 六、屏幕管理

### 屏幕操作

```c
/* 获取当前活动屏幕 */
lv_obj_t *scr = lv_scr_act();

/* 创建新屏幕 */
lv_obj_t *new_scr = lv_obj_create(NULL);  // parent = NULL

/* 切换屏幕（无动画）*/
lv_scr_load(new_scr);

/* 切换屏幕（带动画）*/
lv_scr_load_anim(new_scr, LV_SCR_LOAD_ANIM_FADE_ON, 300, 0, false);
```

**屏幕切换动画：**

| 动画 | 说明 |
|------|------|
| `LV_SCR_LOAD_ANIM_NONE` | 无动画 |
| `LV_SCR_LOAD_ANIM_FADE_ON` | 淡入 |
| `LV_SCR_LOAD_ANIM_OVER_LEFT` | 从右向左滑入 |
| `LV_SCR_LOAD_ANIM_OVER_RIGHT` | 从左向右滑入 |
| `LV_SCR_LOAD_ANIM_MOVE_LEFT` | 向左移动 |

---

## 七、定时器

### LVGL 内置定时器

```c
/* 创建周期定时器 */
lv_timer_t *timer = lv_timer_create(timer_cb, period_ms, user_data);

/* 回调函数 */
static void timer_cb(lv_timer_t *timer) {
    void *data = lv_timer_get_user_data(timer);
    /* 定时任务 */
}

/* 修改周期 */
lv_timer_set_period(timer, new_period_ms);

/* 暂停/恢复 */
lv_timer_pause(timer);
lv_timer_resume(timer);

/* 删除定时器 */
lv_timer_del(timer);

/* 单次执行（执行一次后自动删除）*/
lv_timer_set_repeat_count(timer, 1);
```

---

## 八、显示驱动注册

### 完整注册流程

```c
/* 1. 初始化 LVGL */
lv_init();

/* 2. 分配绘制缓冲区 */
static lv_color_t buf1[LCD_H_RES * 20];
static lv_color_t buf2[LCD_H_RES * 20];

static lv_disp_draw_buf_t draw_buf;
lv_disp_draw_buf_init(&draw_buf, buf1, buf2, LCD_H_RES * 20);

/* 3. 注册显示驱动 */
static lv_disp_drv_t disp_drv;
lv_disp_drv_init(&disp_drv);
disp_drv.hor_res  = LCD_H_RES;
disp_drv.ver_res  = LCD_V_RES;
disp_drv.flush_cb = my_flush_cb;   // 刷新回调（调用屏幕驱动）
disp_drv.draw_buf = &draw_buf;
lv_disp_drv_register(&disp_drv);

/* 4. 注册输入设备（触摸屏）*/
static lv_indev_drv_t indev_drv;
lv_indev_drv_init(&indev_drv);
indev_drv.type    = LV_INDEV_TYPE_POINTER;
indev_drv.read_cb = my_touch_read_cb;  // 触摸读取回调
lv_indev_drv_register(&indev_drv);
```

---

## ⚠️ 注意事项

### 1. 线程安全

LVGL 本身不是线程安全的。多任务环境中，所有 LVGL API 调用必须在同一任务中，
或者用互斥锁保护：

```c
/* 其他任务更新 UI 时 */
if (xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(100))) {
    lv_label_set_text(label, "新内容");
    xSemaphoreGive(lvgl_mutex);
}
```

### 2. lv_timer_handler 调用频率

`lv_timer_handler` 建议每 5~10ms 调用一次。
调用太频繁浪费 CPU，太少则动画卡顿。

### 3. 缓冲区大小

| 缓冲区大小 | 效果 | 内存占用 |
|-----------|------|---------|
| 屏幕宽度 × 10 行 | 一般，多次刷新 | 较小 |
| 屏幕宽度 × 屏幕高度 / 10 | 较好 | 中等 |
| 全屏（双缓冲）| 最流畅 | 最大（需 PSRAM）|

### 4. 对象生命周期

对象创建后由 LVGL 管理内存，不要手动 `free`。
删除对象用 `lv_obj_del()`，会自动释放内存和子对象。

---

## 📖 相关文档

- [LVGL_Widgets.md](LVGL_Widgets.md) — 常用控件
- [LVGL_Style.md](LVGL_Style.md) — 样式与主题
- [LVGL_Layout.md](LVGL_Layout.md) — 布局系统
- [LVGL_Animation.md](LVGL_Animation.md) — 动画系统
- [README.md](README.md) — 模块导航
