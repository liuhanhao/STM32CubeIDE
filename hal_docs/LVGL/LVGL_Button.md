# lv_btn — 按钮 API 文档

## 📋 模块概述

`lv_btn` 是可点击的矩形按钮控件，支持：
- 点击、长按、释放等事件
- 切换（Toggle）模式
- 禁用状态
- 自定义样式（圆角、颜色、阴影）

> **注意：** 按钮本身不显示文字，需要在按钮内创建 `lv_label` 子对象。

---

## 📌 启用配置

```c
#define LV_USE_BTN 1
```

---

## 🎯 核心函数

### lv_btn_create()

创建按钮。

```c
lv_obj_t *lv_btn_create(lv_obj_t *parent);
```

---

### 状态控制

```c
/* 禁用按钮（变灰，不响应点击）*/
lv_obj_add_state(btn, LV_STATE_DISABLED);

/* 恢复启用 */
lv_obj_clear_state(btn, LV_STATE_DISABLED);

/* 检查是否禁用 */
bool disabled = lv_obj_has_state(btn, LV_STATE_DISABLED);
```

---

### 切换模式（Toggle）

```c
/* 启用切换模式（点击后保持选中状态）*/
lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);

/* 检查是否选中 */
bool checked = lv_obj_has_state(btn, LV_STATE_CHECKED);

/* 手动设置选中状态 */
lv_obj_add_state(btn, LV_STATE_CHECKED);
lv_obj_clear_state(btn, LV_STATE_CHECKED);
```

---

### 事件注册

```c
lv_obj_add_event_cb(btn, callback, LV_EVENT_CLICKED, user_data);
```

**常用按钮事件：**

| 事件 | 说明 |
|------|------|
| `LV_EVENT_CLICKED` | 点击（按下后抬起）|
| `LV_EVENT_PRESSED` | 按下 |
| `LV_EVENT_RELEASED` | 抬起 |
| `LV_EVENT_LONG_PRESSED` | 长按（默认 400ms）|
| `LV_EVENT_LONG_PRESSED_REPEAT` | 长按重复触发 |
| `LV_EVENT_VALUE_CHANGED` | 切换模式下状态改变 |

---

## 💡 完整应用示例

### 示例1：基础按钮

```c
static void btn_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        ESP_LOGI("BTN", "按钮被点击");
    }
}

lv_obj_t *btn = lv_btn_create(lv_scr_act());
lv_obj_set_size(btn, 150, 50);
lv_obj_center(btn);

lv_obj_t *label = lv_label_create(btn);
lv_label_set_text(label, "点击我");
lv_obj_center(label);

lv_obj_add_event_cb(btn, btn_cb, LV_EVENT_CLICKED, NULL);
```

---

### 示例2：圆角按钮（Material Design 风格）

```c
lv_obj_t *btn = lv_btn_create(lv_scr_act());
lv_obj_set_size(btn, 160, 50);
lv_obj_center(btn);

/* 默认状态 */
lv_obj_set_style_bg_color(btn, lv_color_hex(0x2196F3), LV_PART_MAIN | LV_STATE_DEFAULT);
lv_obj_set_style_radius(btn, 25, LV_PART_MAIN);
lv_obj_set_style_shadow_width(btn, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
lv_obj_set_style_shadow_color(btn, lv_color_hex(0x1565C0), LV_PART_MAIN | LV_STATE_DEFAULT);
lv_obj_set_style_shadow_opa(btn, LV_OPA_50, LV_PART_MAIN | LV_STATE_DEFAULT);

/* 按下状态（颜色加深，阴影缩小）*/
lv_obj_set_style_bg_color(btn, lv_color_hex(0x1565C0), LV_PART_MAIN | LV_STATE_PRESSED);
lv_obj_set_style_shadow_width(btn, 2, LV_PART_MAIN | LV_STATE_PRESSED);

lv_obj_t *label = lv_label_create(btn);
lv_label_set_text(label, "确认");
lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
lv_obj_center(label);
```

---

### 示例3：切换按钮（开关效果）

```c
static void toggle_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    bool checked = lv_obj_has_state(btn, LV_STATE_CHECKED);

    lv_obj_t *label = lv_obj_get_child(btn, 0);
    lv_label_set_text(label, checked ? "已开启" : "已关闭");

    gpio_set_level(GPIO_NUM_4, checked ? 1 : 0);
}

lv_obj_t *btn = lv_btn_create(lv_scr_act());
lv_obj_set_size(btn, 120, 45);
lv_obj_center(btn);
lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);

/* 选中状态颜色 */
lv_obj_set_style_bg_color(btn, lv_color_hex(0x4CAF50),
                           LV_PART_MAIN | LV_STATE_CHECKED);

lv_obj_t *label = lv_label_create(btn);
lv_label_set_text(label, "已关闭");
lv_obj_center(label);

lv_obj_add_event_cb(btn, toggle_cb, LV_EVENT_VALUE_CHANGED, NULL);
```

---

### 示例4：图标 + 文字按钮

```c
lv_obj_t *btn = lv_btn_create(lv_scr_act());
lv_obj_set_size(btn, 140, 50);
lv_obj_center(btn);
lv_obj_set_style_radius(btn, 8, LV_PART_MAIN);

/* 使用 Flex 布局排列图标和文字 */
lv_obj_set_layout(btn, LV_LAYOUT_FLEX);
lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_ROW);
lv_obj_set_flex_align(btn,
                       LV_FLEX_ALIGN_CENTER,
                       LV_FLEX_ALIGN_CENTER,
                       LV_FLEX_ALIGN_CENTER);
lv_obj_set_style_pad_column(btn, 8, LV_PART_MAIN);

lv_obj_t *icon = lv_label_create(btn);
lv_label_set_text(icon, LV_SYMBOL_WIFI);

lv_obj_t *text = lv_label_create(btn);
lv_label_set_text(text, "连接 WiFi");
```

---

### 示例5：按钮组（单选）

```c
static lv_obj_t *selected_btn = NULL;

static void radio_btn_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);

    /* 取消上一个选中 */
    if (selected_btn && selected_btn != btn) {
        lv_obj_clear_state(selected_btn, LV_STATE_CHECKED);
    }
    selected_btn = btn;
    lv_obj_add_state(btn, LV_STATE_CHECKED);
}

void create_radio_group(lv_obj_t *parent)
{
    const char *options[] = {"选项 A", "选项 B", "选项 C"};

    for (int i = 0; i < 3; i++) {
        lv_obj_t *btn = lv_btn_create(parent);
        lv_obj_set_size(btn, 100, 40);
        lv_obj_set_pos(btn, 10, 10 + i * 50);

        lv_obj_set_style_bg_color(btn, lv_color_hex(0x333333),
                                   LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x2196F3),
                                   LV_PART_MAIN | LV_STATE_CHECKED);

        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, options[i]);
        lv_obj_center(label);

        lv_obj_add_event_cb(btn, radio_btn_cb, LV_EVENT_CLICKED, NULL);
    }
}
```

---

## ⚠️ 注意事项

### 1. 按钮不自带文字

按钮本身只是一个可点击的矩形，文字需要创建子标签：
```c
lv_obj_t *btn = lv_btn_create(parent);
lv_obj_t *label = lv_label_create(btn);  // 子标签
lv_label_set_text(label, "文字");
lv_obj_center(label);
```

### 2. 获取按钮上的标签

```c
/* 获取第一个子对象（通常是标签）*/
lv_obj_t *label = lv_obj_get_child(btn, 0);
```

### 3. 长按时间配置

```c
/* 修改长按触发时间（默认 400ms）*/
lv_obj_set_style_anim_time(btn, 200, LV_PART_MAIN);  // 不是这个
/* 通过 indev 配置全局长按时间 */
lv_indev_set_long_press_time(lv_indev_get_act(), 600);  // 600ms
```

---

## 📖 相关文档

- [LVGL_Core.md](LVGL_Core.md) — 事件系统
- [LVGL_Label.md](LVGL_Label.md) — 文本标签
- [LVGL_Style.md](LVGL_Style.md) — 样式
- [README.md](README.md) — 模块导航
