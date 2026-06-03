# LVGL 样式与主题 API 文档

## 📋 模块概述

LVGL 的样式系统控制所有视觉外观，包括：
- 背景颜色、渐变、透明度
- 边框、圆角、阴影
- 文字颜色、字体、行间距
- 内边距、轮廓
- 主题（全局统一风格）

---

## 一、样式属性速查

### 背景

```c
/* 背景颜色 */
lv_obj_set_style_bg_color(obj, lv_color_hex(0x2196F3), selector);

/* 背景透明度（0=透明，255=不透明）*/
lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, selector);  // 完全不透明
lv_obj_set_style_bg_opa(obj, LV_OPA_50, selector);     // 50% 透明

/* 渐变颜色（从 bg_color 渐变到 bg_grad_color）*/
lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0x0d47a1), selector);
lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, selector);  // 垂直渐变
lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_HOR, selector);  // 水平渐变

/* 渐变起止位置（0~255，对应 0%~100%）*/
lv_obj_set_style_bg_main_stop(obj, 0, selector);    // 渐变起始位置
lv_obj_set_style_bg_grad_stop(obj, 255, selector);  // 渐变结束位置
```

---

### 边框

```c
/* 边框宽度 */
lv_obj_set_style_border_width(obj, 2, selector);

/* 边框颜色 */
lv_obj_set_style_border_color(obj, lv_color_hex(0x0d47a1), selector);

/* 边框透明度 */
lv_obj_set_style_border_opa(obj, LV_OPA_COVER, selector);

/* 边框位置（哪些边显示边框）*/
lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_FULL, selector);    // 四边
lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_BOTTOM, selector);  // 仅底边
lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_TOP | LV_BORDER_SIDE_BOTTOM, selector);
```

---

### 圆角

```c
/* 圆角半径 */
lv_obj_set_style_radius(obj, 10, selector);

/* 完全圆形（半径设为最大值）*/
lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, selector);
```

---

### 阴影

```c
/* 阴影宽度（模糊半径）*/
lv_obj_set_style_shadow_width(obj, 15, selector);

/* 阴影颜色 */
lv_obj_set_style_shadow_color(obj, lv_color_hex(0x000000), selector);

/* 阴影透明度 */
lv_obj_set_style_shadow_opa(obj, LV_OPA_30, selector);

/* 阴影偏移 */
lv_obj_set_style_shadow_ofs_x(obj, 3, selector);
lv_obj_set_style_shadow_ofs_y(obj, 3, selector);

/* 阴影扩散 */
lv_obj_set_style_shadow_spread(obj, 2, selector);
```

---

### 文字

```c
/* 文字颜色 */
lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), selector);

/* 文字透明度 */
lv_obj_set_style_text_opa(obj, LV_OPA_COVER, selector);

/* 字体 */
lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, selector);

/* 字间距 */
lv_obj_set_style_text_letter_space(obj, 2, selector);

/* 行间距 */
lv_obj_set_style_text_line_space(obj, 5, selector);

/* 文字对齐 */
lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, selector);
lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_LEFT, selector);
lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, selector);

/* 文字装饰 */
lv_obj_set_style_text_decor(obj, LV_TEXT_DECOR_UNDERLINE, selector);
lv_obj_set_style_text_decor(obj, LV_TEXT_DECOR_STRIKETHROUGH, selector);
```

---

### 内边距

```c
/* 四边相同 */
lv_obj_set_style_pad_all(obj, 10, selector);

/* 单边 */
lv_obj_set_style_pad_top(obj, 5, selector);
lv_obj_set_style_pad_bottom(obj, 5, selector);
lv_obj_set_style_pad_left(obj, 10, selector);
lv_obj_set_style_pad_right(obj, 10, selector);

/* 水平/垂直 */
lv_obj_set_style_pad_hor(obj, 10, selector);  // 左右
lv_obj_set_style_pad_ver(obj, 5, selector);   // 上下

/* 子对象间距（布局中）*/
lv_obj_set_style_pad_row(obj, 8, selector);
lv_obj_set_style_pad_column(obj, 8, selector);
```

---

### 轮廓（Outline）

轮廓在边框外侧绘制，不影响布局。

```c
lv_obj_set_style_outline_width(obj, 2, selector);
lv_obj_set_style_outline_color(obj, lv_color_hex(0x2196F3), selector);
lv_obj_set_style_outline_opa(obj, LV_OPA_COVER, selector);
lv_obj_set_style_outline_pad(obj, 3, selector);  // 轮廓与边框的间距
```

---

### 整体透明度

```c
/* 对象整体透明度（包括子对象）*/
lv_obj_set_style_opa(obj, LV_OPA_70, selector);  // 70% 不透明
```

---

### 圆弧（Arc 控件专用）

```c
lv_obj_set_style_arc_color(arc, lv_color_hex(0x2196F3), LV_PART_INDICATOR);
lv_obj_set_style_arc_width(arc, 8, LV_PART_INDICATOR);
lv_obj_set_style_arc_rounded(arc, true, LV_PART_INDICATOR);  // 圆头
```

---

## 二、可复用样式对象

当多个对象需要相同样式时，使用 `lv_style_t` 对象复用：

```c
/* 定义样式（必须是静态或全局变量）*/
static lv_style_t style_card;

void style_init(void)
{
    lv_style_init(&style_card);

    lv_style_set_bg_color(&style_card, lv_color_hex(0x1e1e2e));
    lv_style_set_bg_opa(&style_card, LV_OPA_COVER);
    lv_style_set_border_width(&style_card, 1);
    lv_style_set_border_color(&style_card, lv_color_hex(0x333355));
    lv_style_set_radius(&style_card, 10);
    lv_style_set_pad_all(&style_card, 12);
    lv_style_set_shadow_width(&style_card, 10);
    lv_style_set_shadow_color(&style_card, lv_color_hex(0x000000));
    lv_style_set_shadow_opa(&style_card, LV_OPA_30);
}

/* 应用到多个对象 */
lv_obj_t *card1 = lv_obj_create(lv_scr_act());
lv_obj_add_style(card1, &style_card, LV_PART_MAIN | LV_STATE_DEFAULT);

lv_obj_t *card2 = lv_obj_create(lv_scr_act());
lv_obj_add_style(card2, &style_card, LV_PART_MAIN | LV_STATE_DEFAULT);
```

---

## 三、状态样式

不同状态下显示不同样式：

```c
/* 默认状态 */
lv_obj_set_style_bg_color(btn, lv_color_hex(0x2196F3),
                           LV_PART_MAIN | LV_STATE_DEFAULT);

/* 按下状态（颜色加深）*/
lv_obj_set_style_bg_color(btn, lv_color_hex(0x1565C0),
                           LV_PART_MAIN | LV_STATE_PRESSED);

/* 禁用状态（变灰）*/
lv_obj_set_style_bg_color(btn, lv_color_hex(0x555555),
                           LV_PART_MAIN | LV_STATE_DISABLED);
lv_obj_set_style_text_color(btn, lv_color_hex(0x888888),
                             LV_PART_MAIN | LV_STATE_DISABLED);

/* 选中状态（开关、复选框）*/
lv_obj_set_style_bg_color(sw, lv_color_hex(0x4CAF50),
                           LV_PART_INDICATOR | LV_STATE_CHECKED);
```

---

## 四、主题

### 使用内置主题

LVGL 内置两个主题：默认主题（支持深色/浅色）和简单主题。

```c
/* 在 lv_conf.h 中启用 */
#define LV_USE_THEME_DEFAULT 1

/* 初始化主题（在 lv_init() 后调用）*/
lv_theme_t *theme = lv_theme_default_init(
    lv_disp_get_default(),   // 显示设备
    lv_palette_main(LV_PALETTE_BLUE),   // 主色
    lv_palette_main(LV_PALETTE_RED),    // 次色
    true,                    // true = 深色主题，false = 浅色主题
    &lv_font_montserrat_14   // 默认字体
);
lv_disp_set_theme(lv_disp_get_default(), theme);
```

---

### 调色板

LVGL 内置 Material Design 调色板：

```c
/* 主色（500 色调）*/
lv_palette_main(LV_PALETTE_RED)
lv_palette_main(LV_PALETTE_PINK)
lv_palette_main(LV_PALETTE_PURPLE)
lv_palette_main(LV_PALETTE_DEEP_PURPLE)
lv_palette_main(LV_PALETTE_INDIGO)
lv_palette_main(LV_PALETTE_BLUE)
lv_palette_main(LV_PALETTE_LIGHT_BLUE)
lv_palette_main(LV_PALETTE_CYAN)
lv_palette_main(LV_PALETTE_TEAL)
lv_palette_main(LV_PALETTE_GREEN)
lv_palette_main(LV_PALETTE_LIGHT_GREEN)
lv_palette_main(LV_PALETTE_LIME)
lv_palette_main(LV_PALETTE_YELLOW)
lv_palette_main(LV_PALETTE_AMBER)
lv_palette_main(LV_PALETTE_ORANGE)
lv_palette_main(LV_PALETTE_DEEP_ORANGE)
lv_palette_main(LV_PALETTE_BROWN)
lv_palette_main(LV_PALETTE_BLUE_GREY)
lv_palette_main(LV_PALETTE_GREY)

/* 浅色调（lighten）*/
lv_palette_lighten(LV_PALETTE_BLUE, 3)  // 3 = 最浅

/* 深色调（darken）*/
lv_palette_darken(LV_PALETTE_BLUE, 3)   // 3 = 最深
```

---

## 五、完整主题示例

### 深色科技风格

```c
void apply_dark_theme(void)
{
    /* 屏幕背景 */
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x0a0a1a), LV_PART_MAIN);

    /* 全局按钮样式 */
    static lv_style_t style_btn;
    lv_style_init(&style_btn);
    lv_style_set_bg_color(&style_btn, lv_color_hex(0x1a237e));
    lv_style_set_bg_grad_color(&style_btn, lv_color_hex(0x283593));
    lv_style_set_bg_grad_dir(&style_btn, LV_GRAD_DIR_VER);
    lv_style_set_border_color(&style_btn, lv_color_hex(0x3949ab));
    lv_style_set_border_width(&style_btn, 1);
    lv_style_set_radius(&style_btn, 6);
    lv_style_set_shadow_width(&style_btn, 8);
    lv_style_set_shadow_color(&style_btn, lv_color_hex(0x1a237e));
    lv_style_set_shadow_opa(&style_btn, LV_OPA_50);
    lv_style_set_text_color(&style_btn, lv_color_hex(0xe8eaf6));

    /* 按下状态 */
    static lv_style_t style_btn_pressed;
    lv_style_init(&style_btn_pressed);
    lv_style_set_bg_color(&style_btn_pressed, lv_color_hex(0x0d47a1));
    lv_style_set_shadow_width(&style_btn_pressed, 2);

    /* 创建示例按钮 */
    lv_obj_t *btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn, 150, 50);
    lv_obj_center(btn);
    lv_obj_add_style(btn, &style_btn, LV_STATE_DEFAULT);
    lv_obj_add_style(btn, &style_btn_pressed, LV_STATE_PRESSED);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, "科技按钮");
    lv_obj_center(label);
}
```

---

### 卡片组件样式

```c
lv_obj_t *create_info_card(lv_obj_t *parent,
                             const char *title,
                             const char *value,
                             lv_color_t accent_color)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, 130, 80);

    /* 卡片背景 */
    lv_obj_set_style_bg_color(card, lv_color_hex(0x1e1e2e), LV_PART_MAIN);
    lv_obj_set_style_border_color(card, accent_color, LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(card, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 10, LV_PART_MAIN);

    /* 左侧强调色条 */
    lv_obj_t *bar = lv_obj_create(card);
    lv_obj_set_size(bar, 3, 40);
    lv_obj_align(bar, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_color(bar, accent_color, LV_PART_MAIN);
    lv_obj_set_style_radius(bar, 2, LV_PART_MAIN);
    lv_obj_set_style_border_width(bar, 0, LV_PART_MAIN);

    /* 标题 */
    lv_obj_t *title_lbl = lv_label_create(card);
    lv_label_set_text(title_lbl, title);
    lv_obj_set_style_text_color(title_lbl, lv_color_hex(0x888888), LV_PART_MAIN);
    lv_obj_align(title_lbl, LV_ALIGN_TOP_LEFT, 10, 5);

    /* 数值 */
    lv_obj_t *value_lbl = lv_label_create(card);
    lv_label_set_text(value_lbl, value);
    lv_obj_set_style_text_color(value_lbl, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(value_lbl, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(value_lbl, LV_ALIGN_BOTTOM_LEFT, 10, -5);

    return card;
}
```

---

## ⚠️ 注意事项

### 1. 样式对象必须持久

`lv_style_t` 对象必须是静态或全局变量，不能是局部变量（函数返回后会被销毁）：

```c
/* 错误：局部变量，函数返回后失效 */
void bad_example(void) {
    lv_style_t style;  // ❌
    lv_style_init(&style);
    lv_obj_add_style(obj, &style, 0);
}

/* 正确：静态变量 */
void good_example(void) {
    static lv_style_t style;  // ✅
    lv_style_init(&style);
    lv_obj_add_style(obj, &style, 0);
}
```

### 2. 内联样式 vs 样式对象

- 内联样式（`lv_obj_set_style_*`）：简单，每个对象独立，不可复用
- 样式对象（`lv_style_t`）：可复用，修改一处影响所有使用该样式的对象

### 3. 样式优先级

后添加的样式优先级更高，内联样式优先级最高。

### 4. 清除样式

```c
/* 移除特定样式 */
lv_obj_remove_style(obj, &my_style, selector);

/* 移除所有样式（恢复默认）*/
lv_obj_remove_style_all(obj);
```

---

## 📖 相关文档

- [LVGL_Core.md](LVGL_Core.md) — 核心概念
- [LVGL_Widgets.md](LVGL_Widgets.md) — 常用控件
- [LVGL_Layout.md](LVGL_Layout.md) — 布局系统
- [LVGL_Animation.md](LVGL_Animation.md) — 动画系统
- [README.md](README.md) — 模块导航
