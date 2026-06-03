# LVGL 布局系统 API 文档

## 📋 模块概述

LVGL 提供两种现代布局系统，类似 CSS 的 Flexbox 和 Grid：
- **Flex 布局**：一维排列（行或列），自动分配空间
- **Grid 布局**：二维网格排列，精确控制行列

---

## 一、Flex 布局

### 概念

Flex 布局将子对象沿主轴（行或列）排列，支持：
- 主轴方向（水平/垂直）
- 换行
- 对齐方式
- 子对象弹性伸缩

### 启用 Flex 布局

```c
lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
```

---

### 核心函数

#### lv_obj_set_flex_flow()

设置排列方向和换行方式。

```c
void lv_obj_set_flex_flow(lv_obj_t *obj, lv_flex_flow_t flow);
```

| 值 | 说明 |
|----|------|
| `LV_FLEX_FLOW_ROW` | 水平排列，不换行 |
| `LV_FLEX_FLOW_COLUMN` | 垂直排列，不换行 |
| `LV_FLEX_FLOW_ROW_WRAP` | 水平排列，自动换行 |
| `LV_FLEX_FLOW_COLUMN_WRAP` | 垂直排列，自动换行 |
| `LV_FLEX_FLOW_ROW_REVERSE` | 水平反向排列 |
| `LV_FLEX_FLOW_COLUMN_REVERSE` | 垂直反向排列 |

---

#### lv_obj_set_flex_align()

设置主轴、交叉轴和多行对齐方式。

```c
void lv_obj_set_flex_align(lv_obj_t *obj,
                            lv_flex_align_t main_place,
                            lv_flex_align_t cross_place,
                            lv_flex_align_t track_cross_place);
```

**参数：**
- `main_place`: 主轴对齐（子对象在主轴方向的分布）
- `cross_place`: 交叉轴对齐（子对象在交叉轴方向的位置）
- `track_cross_place`: 多行时行间对齐

**对齐值：**

| 值 | 说明 |
|----|------|
| `LV_FLEX_ALIGN_START` | 起始对齐（默认）|
| `LV_FLEX_ALIGN_END` | 末尾对齐 |
| `LV_FLEX_ALIGN_CENTER` | 居中 |
| `LV_FLEX_ALIGN_SPACE_EVENLY` | 均匀分布（含两端）|
| `LV_FLEX_ALIGN_SPACE_AROUND` | 均匀分布（两端半间距）|
| `LV_FLEX_ALIGN_SPACE_BETWEEN` | 均匀分布（两端贴边）|

---

#### lv_obj_set_flex_grow()

设置子对象的弹性伸缩比例（类似 CSS flex-grow）。

```c
void lv_obj_set_flex_grow(lv_obj_t *obj, uint8_t grow);
```

**说明：** `grow = 1` 表示该子对象占用剩余空间的 1 份，`grow = 2` 占 2 份，以此类推。

---

### 完整示例

#### 示例1：水平均匀分布的按钮栏

```c
/* 创建底部按钮栏 */
lv_obj_t *btn_bar = lv_obj_create(lv_scr_act());
lv_obj_set_size(btn_bar, 320, 60);
lv_obj_align(btn_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
lv_obj_set_style_pad_all(btn_bar, 5, LV_PART_MAIN);
lv_obj_set_style_pad_column(btn_bar, 5, LV_PART_MAIN);

/* 启用 Flex 布局，水平排列，均匀分布 */
lv_obj_set_layout(btn_bar, LV_LAYOUT_FLEX);
lv_obj_set_flex_flow(btn_bar, LV_FLEX_FLOW_ROW);
lv_obj_set_flex_align(btn_bar,
                       LV_FLEX_ALIGN_SPACE_EVENLY,  // 主轴：均匀分布
                       LV_FLEX_ALIGN_CENTER,          // 交叉轴：居中
                       LV_FLEX_ALIGN_CENTER);

/* 添加 3 个按钮，各占 1/3 */
const char *btn_labels[] = {"主页", "设置", "关于"};
for (int i = 0; i < 3; i++) {
    lv_obj_t *btn = lv_btn_create(btn_bar);
    lv_obj_set_flex_grow(btn, 1);  // 均等分配空间
    lv_obj_set_height(btn, LV_PCT(100));

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, btn_labels[i]);
    lv_obj_center(label);
}
```

---

#### 示例2：垂直列表（自动换行）

```c
lv_obj_t *cont = lv_obj_create(lv_scr_act());
lv_obj_set_size(cont, 300, 200);
lv_obj_center(cont);
lv_obj_set_style_pad_all(cont, 10, LV_PART_MAIN);
lv_obj_set_style_pad_row(cont, 8, LV_PART_MAIN);

lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
lv_obj_set_flex_align(cont,
                       LV_FLEX_ALIGN_START,
                       LV_FLEX_ALIGN_START,
                       LV_FLEX_ALIGN_START);

/* 添加多个标签，自动垂直排列 */
const char *items[] = {"WiFi 已连接", "温度: 25.6°C", "湿度: 60%", "电量: 85%"};
for (int i = 0; i < 4; i++) {
    lv_obj_t *label = lv_label_create(cont);
    lv_label_set_text(label, items[i]);
    lv_obj_set_width(label, LV_PCT(100));
}
```

---

#### 示例3：弹性伸缩（一个固定，一个填充剩余）

```c
lv_obj_t *row = lv_obj_create(lv_scr_act());
lv_obj_set_size(row, 300, 50);
lv_obj_center(row);
lv_obj_set_style_pad_all(row, 5, LV_PART_MAIN);
lv_obj_set_style_pad_column(row, 5, LV_PART_MAIN);

lv_obj_set_layout(row, LV_LAYOUT_FLEX);
lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

/* 固定宽度的图标 */
lv_obj_t *icon = lv_label_create(row);
lv_label_set_text(icon, LV_SYMBOL_WIFI);
lv_obj_set_width(icon, 30);

/* 填充剩余空间的标签 */
lv_obj_t *label = lv_label_create(row);
lv_label_set_text(label, "WiFi 已连接");
lv_obj_set_flex_grow(label, 1);  // 占用剩余所有空间

/* 固定宽度的按钮 */
lv_obj_t *btn = lv_btn_create(row);
lv_obj_set_size(btn, 60, LV_PCT(100));
lv_obj_t *btn_lbl = lv_label_create(btn);
lv_label_set_text(btn_lbl, "断开");
lv_obj_center(btn_lbl);
```

---

## 二、Grid 布局

### 概念

Grid 布局将子对象放置在二维网格中，支持：
- 固定列宽/行高
- 弹性列宽/行高（`LV_GRID_FR`）
- 内容自适应（`LV_GRID_CONTENT`）
- 子对象跨列/跨行

### 启用 Grid 布局

```c
lv_obj_set_layout(cont, LV_LAYOUT_GRID);
```

---

### 核心函数

#### lv_obj_set_grid_dsc_array()

设置列宽和行高数组（必须以 `LV_GRID_TEMPLATE_LAST` 结尾）。

```c
void lv_obj_set_grid_dsc_array(lv_obj_t *obj,
                                const lv_coord_t col_dsc[],
                                const lv_coord_t row_dsc[]);
```

**列宽/行高值：**

| 值 | 说明 |
|----|------|
| 固定像素值（如 `100`）| 固定宽度 |
| `LV_GRID_FR(1)` | 弹性，占剩余空间的 1 份 |
| `LV_GRID_CONTENT` | 根据内容自适应 |
| `LV_GRID_TEMPLATE_LAST` | 数组结束标记 |

---

#### lv_obj_set_grid_cell()

设置子对象在网格中的位置。

```c
void lv_obj_set_grid_cell(lv_obj_t *obj,
                           lv_grid_align_t col_align,
                           uint8_t col_pos,
                           uint8_t col_span,
                           lv_grid_align_t row_align,
                           uint8_t row_pos,
                           uint8_t row_span);
```

**参数：**
- `col_align`: 单元格内水平对齐（`LV_GRID_ALIGN_START/CENTER/END/STRETCH`）
- `col_pos`: 列索引（从 0 开始）
- `col_span`: 跨列数（通常为 1）
- `row_align`: 单元格内垂直对齐
- `row_pos`: 行索引（从 0 开始）
- `row_span`: 跨行数（通常为 1）

---

#### lv_obj_set_grid_align()

设置整体对齐方式。

```c
void lv_obj_set_grid_align(lv_obj_t *obj,
                            lv_grid_align_t col_align,
                            lv_grid_align_t row_align);
```

---

### 完整示例

#### 示例1：2×2 信息卡片网格

```c
/* 定义列宽：两列各占 1 份 */
static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
/* 定义行高：两行各 80px */
static lv_coord_t row_dsc[] = {80, 80, LV_GRID_TEMPLATE_LAST};

lv_obj_t *grid = lv_obj_create(lv_scr_act());
lv_obj_set_size(grid, 300, 180);
lv_obj_center(grid);
lv_obj_set_style_pad_all(grid, 5, LV_PART_MAIN);
lv_obj_set_style_pad_gap(grid, 5, LV_PART_MAIN);

lv_obj_set_layout(grid, LV_LAYOUT_GRID);
lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);

/* 创建 4 个卡片 */
const char *titles[] = {"温度", "湿度", "气压", "光照"};
const char *values[] = {"25.6°C", "60%", "1013hPa", "850lux"};

for (int i = 0; i < 4; i++) {
    lv_obj_t *card = lv_obj_create(grid);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x1e1e2e), LV_PART_MAIN);
    lv_obj_set_style_radius(card, 8, LV_PART_MAIN);

    /* 放置在网格中 */
    lv_obj_set_grid_cell(card,
                          LV_GRID_ALIGN_STRETCH, i % 2, 1,  // 列
                          LV_GRID_ALIGN_STRETCH, i / 2, 1); // 行

    /* 卡片内容 */
    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *title = lv_label_create(card);
    lv_label_set_text(title, titles[i]);
    lv_obj_set_style_text_color(title, lv_color_hex(0x888888), LV_PART_MAIN);

    lv_obj_t *value = lv_label_create(card);
    lv_label_set_text(value, values[i]);
    lv_obj_set_style_text_color(value, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(value, &lv_font_montserrat_16, LV_PART_MAIN);
}
```

---

#### 示例2：表单布局（标签 + 输入框）

```c
static lv_coord_t col_dsc[] = {LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
static lv_coord_t row_dsc[] = {50, 50, 50, LV_GRID_TEMPLATE_LAST};

lv_obj_t *form = lv_obj_create(lv_scr_act());
lv_obj_set_size(form, 300, 180);
lv_obj_center(form);
lv_obj_set_style_pad_all(form, 10, LV_PART_MAIN);
lv_obj_set_style_pad_gap(form, 8, LV_PART_MAIN);

lv_obj_set_layout(form, LV_LAYOUT_GRID);
lv_obj_set_grid_dsc_array(form, col_dsc, row_dsc);

const char *field_names[] = {"SSID:", "密码:", "端口:"};
const char *placeholders[] = {"WiFi 名称", "WiFi 密码", "8080"};

for (int i = 0; i < 3; i++) {
    /* 标签（第 0 列）*/
    lv_obj_t *label = lv_label_create(form);
    lv_label_set_text(label, field_names[i]);
    lv_obj_set_grid_cell(label,
                          LV_GRID_ALIGN_END, 0, 1,
                          LV_GRID_ALIGN_CENTER, i, 1);

    /* 输入框（第 1 列）*/
    lv_obj_t *ta = lv_textarea_create(form);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_placeholder_text(ta, placeholders[i]);
    lv_obj_set_grid_cell(ta,
                          LV_GRID_ALIGN_STRETCH, 1, 1,
                          LV_GRID_ALIGN_CENTER, i, 1);
}
```

---

## 三、间距与内边距

布局中的间距通过样式属性控制：

```c
/* 内边距（容器内边距）*/
lv_obj_set_style_pad_all(cont, 10, LV_PART_MAIN);    // 四边相同
lv_obj_set_style_pad_top(cont, 5, LV_PART_MAIN);
lv_obj_set_style_pad_bottom(cont, 5, LV_PART_MAIN);
lv_obj_set_style_pad_left(cont, 10, LV_PART_MAIN);
lv_obj_set_style_pad_right(cont, 10, LV_PART_MAIN);

/* 子对象间距（Flex/Grid 布局中）*/
lv_obj_set_style_pad_row(cont, 8, LV_PART_MAIN);     // 行间距
lv_obj_set_style_pad_column(cont, 8, LV_PART_MAIN);  // 列间距
lv_obj_set_style_pad_gap(cont, 8, LV_PART_MAIN);     // 行列间距相同
```

---

## ⚠️ 注意事项

### 1. 布局触发重新计算

修改子对象的尺寸或位置后，父容器会自动重新计算布局。
如果需要手动触发，调用 `lv_obj_update_layout(obj)`。

### 2. 固定位置与布局冲突

在启用了 Flex/Grid 布局的容器中，子对象的 `lv_obj_set_pos` 会被布局覆盖。
如果需要某个子对象不参与布局，添加 `LV_OBJ_FLAG_FLOATING` 标志：
```c
lv_obj_add_flag(child, LV_OBJ_FLAG_FLOATING);
```

### 3. Grid 数组必须是静态的

`lv_obj_set_grid_dsc_array` 传入的数组指针在对象生命周期内必须有效，
使用 `static` 修饰：
```c
static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
```

### 4. 容器需要足够大

Flex/Grid 布局需要容器有足够空间容纳子对象。
如果容器尺寸设为 `LV_SIZE_CONTENT`，会根据子对象自动调整。

---

## 📖 相关文档

- [LVGL_Core.md](LVGL_Core.md) — 核心概念（对象位置与尺寸）
- [LVGL_Widgets.md](LVGL_Widgets.md) — 常用控件
- [LVGL_Style.md](LVGL_Style.md) — 样式与主题
- [README.md](README.md) — 模块导航
