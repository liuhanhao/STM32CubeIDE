# lv_textarea & lv_dropdown & lv_keyboard — 输入控件 API 文档

## 📋 模块概述

LVGL 提供三种输入相关控件：
- **lv_textarea**：文本输入框（单行/多行，密码模式）
- **lv_dropdown**：下拉选择列表
- **lv_keyboard**：虚拟键盘（配合 textarea 使用）

---

## 📌 启用配置

```c
#define LV_USE_TEXTAREA  1
#define LV_USE_DROPDOWN  1
#define LV_USE_KEYBOARD  1
```

---

## 一、lv_textarea — 文本输入框

### 核心函数

#### lv_textarea_create()

```c
lv_obj_t *lv_textarea_create(lv_obj_t *parent);
```

---

#### lv_textarea_set_text()

设置文本内容。

```c
void lv_textarea_set_text(lv_obj_t *obj, const char *txt);
```

---

#### lv_textarea_get_text()

获取当前文本。

```c
const char *lv_textarea_get_text(const lv_obj_t *obj);
```

---

#### lv_textarea_add_text()

在光标位置追加文本。

```c
void lv_textarea_add_text(lv_obj_t *obj, const char *txt);
```

---

#### lv_textarea_add_char()

在光标位置追加单个字符。

```c
void lv_textarea_add_char(lv_obj_t *obj, uint32_t c);
```

---

#### lv_textarea_del_char()

删除光标前的字符（退格）。

```c
void lv_textarea_del_char(lv_obj_t *obj);
```

---

#### lv_textarea_set_placeholder_text()

设置占位符文字（输入框为空时显示）。

```c
void lv_textarea_set_placeholder_text(lv_obj_t *obj, const char *txt);
```

---

#### lv_textarea_set_one_line()

设置单行模式。

```c
void lv_textarea_set_one_line(lv_obj_t *obj, bool en);
```

---

#### lv_textarea_set_password_mode()

设置密码模式（显示 `*`）。

```c
void lv_textarea_set_password_mode(lv_obj_t *obj, bool en);
```

---

#### lv_textarea_set_max_length()

设置最大字符数。

```c
void lv_textarea_set_max_length(lv_obj_t *obj, uint32_t num);
```

---

#### lv_textarea_set_accepted_chars()

限制可输入的字符集。

```c
void lv_textarea_set_accepted_chars(lv_obj_t *obj, const char *list);
```

**使用示例：**
```c
/* 只允许输入数字 */
lv_textarea_set_accepted_chars(ta, "0123456789");

/* 只允许输入 IP 地址字符 */
lv_textarea_set_accepted_chars(ta, "0123456789.");
```

---

### 完整示例

#### 示例1：WiFi 配置表单

```c
static lv_obj_t *ssid_ta;
static lv_obj_t *pwd_ta;
static lv_obj_t *kb;

static void ta_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = lv_event_get_target(e);

    if (code == LV_EVENT_FOCUSED) {
        /* 点击输入框时显示键盘 */
        lv_keyboard_set_textarea(kb, ta);
        lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
    if (code == LV_EVENT_DEFOCUSED) {
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
}

void wifi_form_init(void)
{
    /* SSID 输入框 */
    lv_obj_t *ssid_label = lv_label_create(lv_scr_act());
    lv_label_set_text(ssid_label, "WiFi 名称:");
    lv_obj_align(ssid_label, LV_ALIGN_TOP_LEFT, 10, 20);

    ssid_ta = lv_textarea_create(lv_scr_act());
    lv_obj_set_size(ssid_ta, 200, 40);
    lv_obj_align(ssid_ta, LV_ALIGN_TOP_LEFT, 10, 45);
    lv_textarea_set_one_line(ssid_ta, true);
    lv_textarea_set_placeholder_text(ssid_ta, "输入 WiFi 名称");
    lv_textarea_set_max_length(ssid_ta, 32);
    lv_obj_add_event_cb(ssid_ta, ta_event_cb, LV_EVENT_ALL, NULL);

    /* 密码输入框 */
    lv_obj_t *pwd_label = lv_label_create(lv_scr_act());
    lv_label_set_text(pwd_label, "密码:");
    lv_obj_align(pwd_label, LV_ALIGN_TOP_LEFT, 10, 95);

    pwd_ta = lv_textarea_create(lv_scr_act());
    lv_obj_set_size(pwd_ta, 200, 40);
    lv_obj_align(pwd_ta, LV_ALIGN_TOP_LEFT, 10, 120);
    lv_textarea_set_one_line(pwd_ta, true);
    lv_textarea_set_password_mode(pwd_ta, true);
    lv_textarea_set_placeholder_text(pwd_ta, "输入密码");
    lv_textarea_set_max_length(pwd_ta, 64);
    lv_obj_add_event_cb(pwd_ta, ta_event_cb, LV_EVENT_ALL, NULL);

    /* 虚拟键盘（初始隐藏）*/
    kb = lv_keyboard_create(lv_scr_act());
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
}
```

---

## 二、lv_dropdown — 下拉列表

### 核心函数

#### lv_dropdown_create()

```c
lv_obj_t *lv_dropdown_create(lv_obj_t *parent);
```

---

#### lv_dropdown_set_options()

设置选项列表（`\n` 分隔）。

```c
void lv_dropdown_set_options(lv_obj_t *obj, const char *options);
```

---

#### lv_dropdown_set_options_static()

设置静态选项（不复制，字符串必须持久有效）。

```c
void lv_dropdown_set_options_static(lv_obj_t *obj, const char *options);
```

---

#### lv_dropdown_add_option()

追加一个选项。

```c
void lv_dropdown_add_option(lv_obj_t *obj, const char *option, uint32_t pos);
```

---

#### lv_dropdown_set_selected()

设置选中项（索引从 0 开始）。

```c
void lv_dropdown_set_selected(lv_obj_t *obj, uint16_t sel_opt);
```

---

#### lv_dropdown_get_selected()

获取选中项索引。

```c
uint16_t lv_dropdown_get_selected(const lv_obj_t *obj);
```

---

#### lv_dropdown_get_selected_str()

获取选中项文本。

```c
void lv_dropdown_get_selected_str(const lv_obj_t *obj, char *buf, uint32_t buf_size);
```

---

#### lv_dropdown_set_dir()

设置下拉方向。

```c
void lv_dropdown_set_dir(lv_obj_t *obj, lv_dir_t dir);
```

| 方向 | 说明 |
|------|------|
| `LV_DIR_BOTTOM` | 向下（默认）|
| `LV_DIR_TOP` | 向上 |
| `LV_DIR_LEFT` | 向左 |
| `LV_DIR_RIGHT` | 向右 |

---

### 完整示例

#### 示例2：语言选择下拉框

```c
static void lang_cb(lv_event_t *e)
{
    lv_obj_t *dd = lv_event_get_target(e);
    char buf[32];
    lv_dropdown_get_selected_str(dd, buf, sizeof(buf));
    ESP_LOGI("DD", "选择: %s (索引: %d)", buf, lv_dropdown_get_selected(dd));
}

lv_obj_t *dd = lv_dropdown_create(lv_scr_act());
lv_obj_set_width(dd, 150);
lv_obj_align(dd, LV_ALIGN_CENTER, 0, 0);

lv_dropdown_set_options(dd,
    "中文\n"
    "English\n"
    "日本語\n"
    "한국어");

lv_dropdown_set_selected(dd, 0);  // 默认选中第一项
lv_obj_add_event_cb(dd, lang_cb, LV_EVENT_VALUE_CHANGED, NULL);
```

---

#### 示例3：动态更新选项

```c
lv_obj_t *dd = lv_dropdown_create(lv_scr_act());
lv_obj_set_width(dd, 180);
lv_obj_center(dd);

/* 初始选项 */
lv_dropdown_set_options(dd, "加载中...");

/* 异步加载后更新 */
void update_wifi_list(const char **ssids, int count)
{
    char options[512] = "";
    for (int i = 0; i < count; i++) {
        strncat(options, ssids[i], sizeof(options) - strlen(options) - 2);
        if (i < count - 1) strncat(options, "\n", 2);
    }
    lv_dropdown_set_options(dd, options);
}
```

---

## 三、lv_keyboard — 虚拟键盘

### 核心函数

#### lv_keyboard_create()

```c
lv_obj_t *lv_keyboard_create(lv_obj_t *parent);
```

---

#### lv_keyboard_set_textarea()

绑定到文本输入框。

```c
void lv_keyboard_set_textarea(lv_obj_t *kb, lv_obj_t *ta);
```

---

#### lv_keyboard_set_mode()

设置键盘模式。

```c
void lv_keyboard_set_mode(lv_obj_t *kb, lv_keyboard_mode_t mode);
```

| 模式 | 说明 |
|------|------|
| `LV_KEYBOARD_MODE_TEXT_LOWER` | 小写字母（默认）|
| `LV_KEYBOARD_MODE_TEXT_UPPER` | 大写字母 |
| `LV_KEYBOARD_MODE_SPECIAL` | 特殊字符 |
| `LV_KEYBOARD_MODE_NUMBER` | 数字 |
| `LV_KEYBOARD_MODE_USER_1~4` | 自定义键盘 |

---

### 完整示例

#### 示例4：数字键盘输入

```c
static lv_obj_t *num_ta;
static lv_obj_t *num_kb;

static void kb_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *kb = lv_event_get_target(e);

    if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        /* 按下确认或取消键 */
        const char *text = lv_textarea_get_text(num_ta);
        ESP_LOGI("KB", "输入值: %s", text);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
}

void number_input_init(void)
{
    num_ta = lv_textarea_create(lv_scr_act());
    lv_obj_set_size(num_ta, 150, 45);
    lv_obj_align(num_ta, LV_ALIGN_TOP_MID, 0, 20);
    lv_textarea_set_one_line(num_ta, true);
    lv_textarea_set_accepted_chars(num_ta, "0123456789.");
    lv_textarea_set_placeholder_text(num_ta, "输入数值");

    num_kb = lv_keyboard_create(lv_scr_act());
    lv_keyboard_set_mode(num_kb, LV_KEYBOARD_MODE_NUMBER);
    lv_keyboard_set_textarea(num_kb, num_ta);
    lv_obj_add_event_cb(num_kb, kb_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_flag(num_kb, LV_OBJ_FLAG_HIDDEN);

    /* 点击输入框时显示键盘 */
    lv_obj_add_event_cb(num_ta, [](lv_event_t *e) {
        lv_obj_clear_flag(num_kb, LV_OBJ_FLAG_HIDDEN);
    }, LV_EVENT_FOCUSED, NULL);
}
```

---

## ⚠️ 注意事项

### 1. textarea 需要焦点才能接收键盘输入

触摸屏点击 textarea 会自动获得焦点。
如果没有触摸屏，需要手动调用 `lv_group_focus_obj(ta)`。

### 2. 键盘占用大量屏幕空间

虚拟键盘通常占屏幕下半部分，需要合理安排布局。
可以在键盘显示时滚动或移动输入框到可见区域。

### 3. dropdown 选项字符串

`lv_dropdown_set_options` 会复制字符串，
`lv_dropdown_set_options_static` 不复制（字符串必须持久有效）。

---

## 📖 相关文档

- [LVGL_Core.md](LVGL_Core.md) — 事件系统
- [LVGL_Container.md](LVGL_Container.md) — 容器控件
- [LVGL_Font.md](LVGL_Font.md) — 字体（中文输入）
- [README.md](README.md) — 模块导航
