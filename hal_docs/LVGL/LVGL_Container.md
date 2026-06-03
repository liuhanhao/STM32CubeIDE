# lv_tabview & lv_list & lv_msgbox & lv_switch — 容器与交互控件 API 文档

## 📋 模块概述

本文档覆盖常用的容器和交互控件：
- **lv_tabview**：多标签页切换容器
- **lv_list**：可滚动列表
- **lv_msgbox**：弹出消息框/对话框
- **lv_switch**：拨动开关
- **lv_spinner**：加载动画
- **lv_checkbox**：复选框

---

## 📌 启用配置

```c
#define LV_USE_TABVIEW  1
#define LV_USE_LIST     1
#define LV_USE_MSGBOX   1
#define LV_USE_SWITCH   1
#define LV_USE_SPINNER  1
#define LV_USE_CHECKBOX 1
```

---

## 一、lv_tabview — 标签页

### 核心函数

#### lv_tabview_create()

```c
lv_obj_t *lv_tabview_create(lv_obj_t *parent, lv_dir_t tab_pos, lv_coord_t tab_size);
```

**参数：**
- `tab_pos`：标签栏位置（`LV_DIR_TOP/BOTTOM/LEFT/RIGHT`）
- `tab_size`：标签栏高度/宽度（像素）

---

#### lv_tabview_add_tab()

添加标签页，返回页面容器对象。

```c
lv_obj_t *lv_tabview_add_tab(lv_obj_t *obj, const char *name);
```

---

#### lv_tabview_set_act()

切换到指定标签页。

```c
void lv_tabview_set_act(lv_obj_t *obj, uint32_t id, lv_anim_enable_t anim_en);
```

---

#### lv_tabview_get_tab_act()

获取当前活动标签页索引。

```c
uint16_t lv_tabview_get_tab_act(const lv_obj_t *obj);
```

---

### 完整示例

#### 示例1：三标签页应用

```c
void tabview_demo(void)
{
    lv_obj_t *tv = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 45);

    /* 自定义标签栏颜色 */
    lv_obj_set_style_bg_color(lv_tabview_get_tab_btns(tv),
                               lv_color_hex(0x1a1a2e), LV_PART_MAIN);
    lv_obj_set_style_text_color(lv_tabview_get_tab_btns(tv),
                                 lv_color_hex(0x888888), LV_PART_MAIN);
    lv_obj_set_style_text_color(lv_tabview_get_tab_btns(tv),
                                 lv_color_hex(0x2196F3),
                                 LV_PART_MAIN | LV_STATE_CHECKED);

    /* 添加三个标签页 */
    lv_obj_t *tab1 = lv_tabview_add_tab(tv, LV_SYMBOL_HOME " 主页");
    lv_obj_t *tab2 = lv_tabview_add_tab(tv, LV_SYMBOL_SETTINGS " 设置");
    lv_obj_t *tab3 = lv_tabview_add_tab(tv, LV_SYMBOL_LIST " 日志");

    /* 主页内容 */
    lv_obj_t *label1 = lv_label_create(tab1);
    lv_label_set_text(label1, "主页内容");
    lv_obj_center(label1);

    /* 设置页内容 */
    lv_obj_t *label2 = lv_label_create(tab2);
    lv_label_set_text(label2, "设置内容");
    lv_obj_center(label2);

    /* 日志页内容 */
    lv_obj_t *label3 = lv_label_create(tab3);
    lv_label_set_text(label3, "日志内容");
    lv_obj_center(label3);
}
```

---

## 二、lv_list — 列表

### 核心函数

#### lv_list_create()

```c
lv_obj_t *lv_list_create(lv_obj_t *parent);
```

---

#### lv_list_add_btn()

添加带图标和文字的列表项（按钮）。

```c
lv_obj_t *lv_list_add_btn(lv_obj_t *list, const char *icon, const char *txt);
```

---

#### lv_list_add_text()

添加纯文字分组标题。

```c
lv_obj_t *lv_list_add_text(lv_obj_t *list, const char *txt);
```

---

### 完整示例

#### 示例2：设置菜单列表

```c
static void list_item_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    const char *txt = lv_list_get_btn_text(lv_obj_get_parent(btn), btn);
    ESP_LOGI("LIST", "点击: %s", txt);
}

void settings_list_init(void)
{
    lv_obj_t *list = lv_list_create(lv_scr_act());
    lv_obj_set_size(list, 240, 280);
    lv_obj_center(list);

    /* 分组标题 */
    lv_list_add_text(list, "网络设置");
    lv_obj_t *btn;

    btn = lv_list_add_btn(list, LV_SYMBOL_WIFI, "WiFi 设置");
    lv_obj_add_event_cb(btn, list_item_cb, LV_EVENT_CLICKED, NULL);

    btn = lv_list_add_btn(list, LV_SYMBOL_BLUETOOTH, "蓝牙设置");
    lv_obj_add_event_cb(btn, list_item_cb, LV_EVENT_CLICKED, NULL);

    /* 分组标题 */
    lv_list_add_text(list, "系统设置");

    btn = lv_list_add_btn(list, LV_SYMBOL_BELL, "通知设置");
    lv_obj_add_event_cb(btn, list_item_cb, LV_EVENT_CLICKED, NULL);

    btn = lv_list_add_btn(list, LV_SYMBOL_BATTERY_FULL, "电源管理");
    lv_obj_add_event_cb(btn, list_item_cb, LV_EVENT_CLICKED, NULL);

    btn = lv_list_add_btn(list, LV_SYMBOL_SETTINGS, "关于设备");
    lv_obj_add_event_cb(btn, list_item_cb, LV_EVENT_CLICKED, NULL);
}
```

---

## 三、lv_msgbox — 消息框

### 核心函数

#### lv_msgbox_create()

```c
lv_obj_t *lv_msgbox_create(lv_obj_t *parent,
                             const char *title,
                             const char *txt,
                             const char *btn_txts[],
                             bool add_close_btn);
```

**参数：**
- `parent`：通常传 `NULL`（显示在最顶层）
- `btn_txts`：按钮文字数组，以空字符串 `""` 结尾
- `add_close_btn`：是否添加右上角关闭按钮

---

#### lv_msgbox_get_active_btn()

获取点击的按钮索引。

```c
uint16_t lv_msgbox_get_active_btn(lv_obj_t *mbox);
```

---

#### lv_msgbox_get_active_btn_text()

获取点击的按钮文字。

```c
const char *lv_msgbox_get_active_btn_text(lv_obj_t *mbox);
```

---

#### lv_msgbox_close()

关闭消息框。

```c
void lv_msgbox_close(lv_obj_t *mbox);
```

---

### 完整示例

#### 示例3：确认对话框

```c
static void confirm_cb(lv_event_t *e)
{
    lv_obj_t *mbox = lv_event_get_current_target(e);
    const char *btn = lv_msgbox_get_active_btn_text(mbox);

    if (strcmp(btn, "确认") == 0) {
        ESP_LOGI("MSGBOX", "用户确认");
        /* 执行操作 */
    } else {
        ESP_LOGI("MSGBOX", "用户取消");
    }
    lv_msgbox_close(mbox);
}

void show_confirm_dialog(const char *message)
{
    static const char *btns[] = {"确认", "取消", ""};
    lv_obj_t *mbox = lv_msgbox_create(NULL, "确认操作", message, btns, false);
    lv_obj_center(mbox);
    lv_obj_add_event_cb(mbox, confirm_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

/* 使用 */
show_confirm_dialog("确定要删除所有数据吗？\n此操作不可撤销。");
```

---

#### 示例4：信息提示框（自动关闭）

```c
void show_toast(const char *message, uint32_t duration_ms)
{
    static const char *btns[] = {""};  // 无按钮
    lv_obj_t *mbox = lv_msgbox_create(NULL, NULL, message, btns, false);
    lv_obj_align(mbox, LV_ALIGN_BOTTOM_MID, 0, -20);

    /* 定时自动关闭 */
    lv_timer_t *timer = lv_timer_create([](lv_timer_t *t) {
        lv_msgbox_close((lv_obj_t *)lv_timer_get_user_data(t));
        lv_timer_del(t);
    }, duration_ms, mbox);
}

/* 使用 */
show_toast("WiFi 连接成功", 2000);
```

---

## 四、lv_switch — 拨动开关

### 核心函数

```c
/* 创建 */
lv_obj_t *sw = lv_switch_create(parent);

/* 设置状态 */
lv_obj_add_state(sw, LV_STATE_CHECKED);    // 打开
lv_obj_clear_state(sw, LV_STATE_CHECKED);  // 关闭

/* 获取状态 */
bool is_on = lv_obj_has_state(sw, LV_STATE_CHECKED);

/* 监听变化 */
lv_obj_add_event_cb(sw, switch_cb, LV_EVENT_VALUE_CHANGED, NULL);
```

### 完整示例

#### 示例5：设置项（标签 + 开关）

```c
lv_obj_t *create_setting_row(lv_obj_t *parent,
                               const char *label_text,
                               bool initial_state,
                               lv_event_cb_t cb)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_PCT(100), 50);
    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                           LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *label = lv_label_create(row);
    lv_label_set_text(label, label_text);

    lv_obj_t *sw = lv_switch_create(row);
    if (initial_state) lv_obj_add_state(sw, LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(sw, lv_color_hex(0x4CAF50),
                               LV_PART_INDICATOR | LV_STATE_CHECKED);
    if (cb) lv_obj_add_event_cb(sw, cb, LV_EVENT_VALUE_CHANGED, NULL);

    return sw;
}
```

---

## 五、lv_spinner — 加载动画

```c
/* 创建旋转加载动画（1000ms 周期，60° 弧长）*/
lv_obj_t *spinner = lv_spinner_create(parent, 1000, 60);
lv_obj_set_size(spinner, 50, 50);
lv_obj_center(spinner);

/* 自定义颜色 */
lv_obj_set_style_arc_color(spinner, lv_color_hex(0x2196F3), LV_PART_INDICATOR);
lv_obj_set_style_arc_width(spinner, 4, LV_PART_INDICATOR);
lv_obj_set_style_arc_width(spinner, 4, LV_PART_MAIN);
lv_obj_set_style_arc_color(spinner, lv_color_hex(0x333333), LV_PART_MAIN);
```

---

## 六、lv_checkbox — 复选框

```c
/* 创建 */
lv_obj_t *cb = lv_checkbox_create(parent);
lv_checkbox_set_text(cb, "同意用户协议");

/* 设置选中状态 */
lv_obj_add_state(cb, LV_STATE_CHECKED);

/* 获取状态 */
bool checked = lv_obj_has_state(cb, LV_STATE_CHECKED);

/* 监听变化 */
lv_obj_add_event_cb(cb, [](lv_event_t *e) {
    lv_obj_t *cb = lv_event_get_target(e);
    bool checked = lv_obj_has_state(cb, LV_STATE_CHECKED);
    ESP_LOGI("CB", "复选框: %s", checked ? "选中" : "未选中");
}, LV_EVENT_VALUE_CHANGED, NULL);
```

---

## ⚠️ 注意事项

### 1. msgbox 的 parent 参数

传 `NULL` 时消息框显示在最顶层（覆盖所有内容）。
传具体父对象时，消息框在父对象范围内显示。

### 2. tabview 内容区域滚动

标签页内容区域默认可滚动，如果不需要滚动：
```c
lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE);
```

### 3. list 按钮文字获取

```c
const char *txt = lv_list_get_btn_text(list, btn);
```

---

## 📖 相关文档

- [LVGL_Core.md](LVGL_Core.md) — 事件系统
- [LVGL_Layout.md](LVGL_Layout.md) — 布局系统
- [LVGL_Style.md](LVGL_Style.md) — 样式
- [README.md](README.md) — 模块导航
