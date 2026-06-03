# lv_label — 文本标签 API 文档

## 📋 模块概述

`lv_label` 是 LVGL 最基础的控件，用于显示文本，支持：
- 静态文本和动态格式化文本
- 长文本处理（换行、滚动、省略号）
- 富文本（颜色标记）
- 文字对齐

---

## 📌 启用配置

在 `lv_conf.h` 中确认：
```c
#define LV_USE_LABEL 1
```

---

## 🎯 核心函数

### lv_label_create()

创建标签对象。

```c
lv_obj_t *lv_label_create(lv_obj_t *parent);
```

---

### lv_label_set_text()

设置文本（复制字符串到内部缓冲区）。

```c
void lv_label_set_text(lv_obj_t *obj, const char *text);
```

---

### lv_label_set_text_fmt()

格式化文本（类似 `printf`）。

```c
void lv_label_set_text_fmt(lv_obj_t *obj, const char *fmt, ...);
```

**使用示例：**
```c
lv_label_set_text_fmt(label, "温度: %.1f°C", 25.6f);
lv_label_set_text_fmt(label, "计数: %d / %d", current, total);
lv_label_set_text_fmt(label, "IP: %d.%d.%d.%d", 192, 168, 1, 100);
```

---

### lv_label_set_text_static()

设置静态文本（不复制，直接引用指针）。

```c
void lv_label_set_text_static(lv_obj_t *obj, const char *text);
```

> ⚠️ 字符串必须在标签整个生命周期内有效（全局或静态变量）。
> 节省内存，适合常量字符串。

---

### lv_label_get_text()

获取当前文本。

```c
char *lv_label_get_text(const lv_obj_t *obj);
```

---

### lv_label_set_long_mode()

设置长文本处理模式。

```c
void lv_label_set_long_mode(lv_obj_t *obj, lv_label_long_mode_t long_mode);
```

**长文本模式：**

| 模式 | 说明 | 适用场景 |
|------|------|---------|
| `LV_LABEL_LONG_WRAP` | 自动换行（默认）| 多行文本 |
| `LV_LABEL_LONG_DOT` | 超出显示 `...` | 单行截断 |
| `LV_LABEL_LONG_SCROLL` | 水平滚动 | 跑马灯效果 |
| `LV_LABEL_LONG_SCROLL_CIRCULAR` | 循环滚动 | 无缝跑马灯 |
| `LV_LABEL_LONG_CLIP` | 直接裁剪 | 固定区域显示 |

---

### lv_label_set_recolor()

启用富文本颜色标记。

```c
void lv_label_set_recolor(lv_obj_t *obj, bool en);
```

**颜色标记语法：** `#RRGGBB 文字#`

```c
lv_label_set_recolor(label, true);
lv_label_set_text(label, "#FF0000 红色# 普通 #00FF00 绿色#");
```

---

## 💡 完整应用示例

### 示例1：基础文本显示

```c
/* 标题标签 */
lv_obj_t *title = lv_label_create(lv_scr_act());
lv_label_set_text(title, "ESP32-S3 监控");
lv_obj_set_style_text_font(title, &lv_font_montserrat_16, LV_PART_MAIN);
lv_obj_set_style_text_color(title, lv_color_hex(0x00d4ff), LV_PART_MAIN);
lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
```

---

### 示例2：实时数值更新

```c
static lv_obj_t *temp_label;
static lv_obj_t *humi_label;

void sensor_ui_init(void)
{
    temp_label = lv_label_create(lv_scr_act());
    lv_label_set_text(temp_label, "温度: --°C");
    lv_obj_align(temp_label, LV_ALIGN_CENTER, 0, -20);

    humi_label = lv_label_create(lv_scr_act());
    lv_label_set_text(humi_label, "湿度: --%");
    lv_obj_align(humi_label, LV_ALIGN_CENTER, 0, 20);
}

/* 在定时器或任务中调用（需持有 LVGL 互斥锁）*/
void sensor_ui_update(float temp, float humi)
{
    lv_label_set_text_fmt(temp_label, "温度: %.1f°C", temp);
    lv_label_set_text_fmt(humi_label, "湿度: %.1f%%", humi);
}
```

---

### 示例3：跑马灯效果

```c
lv_obj_t *ticker = lv_label_create(lv_scr_act());
lv_obj_set_width(ticker, 200);  // 固定宽度
lv_label_set_long_mode(ticker, LV_LABEL_LONG_SCROLL_CIRCULAR);
lv_label_set_text(ticker, "ESP32-S3 实时监控系统 | 温度: 25.6°C | 湿度: 60%");
lv_obj_set_style_text_color(ticker, lv_color_hex(0xffffff), LV_PART_MAIN);
lv_obj_align(ticker, LV_ALIGN_BOTTOM_MID, 0, -10);
```

---

### 示例4：省略号截断

```c
lv_obj_t *label = lv_label_create(lv_scr_act());
lv_obj_set_width(label, 150);  // 必须设置宽度
lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
lv_label_set_text(label, "这是一段很长的文字，超出部分会显示省略号");
lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
```

---

### 示例5：富文本颜色

```c
lv_obj_t *label = lv_label_create(lv_scr_act());
lv_label_set_recolor(label, true);
lv_label_set_text(label,
    "#FF5722 错误# | #4CAF50 正常# | #FFC107 警告#");
lv_obj_center(label);
```

---

### 示例6：状态指示标签

```c
typedef enum { STATUS_OK, STATUS_WARN, STATUS_ERROR } status_t;

void set_status_label(lv_obj_t *label, status_t status, const char *msg)
{
    lv_label_set_recolor(label, true);

    char buf[128];
    switch (status) {
        case STATUS_OK:
            snprintf(buf, sizeof(buf), "#4CAF50 ● # %s", msg);
            break;
        case STATUS_WARN:
            snprintf(buf, sizeof(buf), "#FFC107 ● # %s", msg);
            break;
        case STATUS_ERROR:
            snprintf(buf, sizeof(buf), "#F44336 ● # %s", msg);
            break;
    }
    lv_label_set_text(label, buf);
}
```

---

## ⚠️ 注意事项

### 1. set_text vs set_text_static

- `set_text`：复制字符串，安全但占用内存
- `set_text_static`：不复制，节省内存，但字符串必须持久有效
- `set_text_fmt`：内部使用 `set_text`，格式化后复制

### 2. 长文本模式需要固定宽度

`LV_LABEL_LONG_DOT`、`LV_LABEL_LONG_SCROLL` 等模式需要先设置标签宽度：
```c
lv_obj_set_width(label, 200);
lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
```

### 3. 换行符

文本中可以使用 `\n` 换行：
```c
lv_label_set_text(label, "第一行\n第二行\n第三行");
```

---

## 📖 相关文档

- [LVGL_Core.md](LVGL_Core.md) — 核心概念
- [LVGL_Style.md](LVGL_Style.md) — 文字样式
- [LVGL_Font.md](LVGL_Font.md) — 字体与中文支持
- [README.md](README.md) — 模块导航
