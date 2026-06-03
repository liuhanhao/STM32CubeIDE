# lv_chart — 图表 API 文档

## 📋 模块概述

`lv_chart` 提供数据可视化图表，支持：
- 折线图（Line）
- 柱状图（Bar）
- 散点图（Scatter）
- 多数据系列
- 实时滚动数据
- 自定义坐标轴和网格

---

## 📌 启用配置

```c
#define LV_USE_CHART 1
```

---

## 🎯 核心函数

### lv_chart_create()

```c
lv_obj_t *lv_chart_create(lv_obj_t *parent);
```

---

### lv_chart_set_type()

设置图表类型。

```c
void lv_chart_set_type(lv_obj_t *obj, lv_chart_type_t type);
```

| 类型 | 说明 |
|------|------|
| `LV_CHART_TYPE_NONE` | 无（隐藏数据）|
| `LV_CHART_TYPE_LINE` | 折线图 |
| `LV_CHART_TYPE_BAR` | 柱状图 |
| `LV_CHART_TYPE_SCATTER` | 散点图 |

---

### lv_chart_set_point_count()

设置数据点数量（X 轴分辨率）。

```c
void lv_chart_set_point_count(lv_obj_t *obj, uint16_t cnt);
```

---

### lv_chart_set_range()

设置 Y 轴范围。

```c
void lv_chart_set_range(lv_obj_t *obj, lv_chart_axis_t axis,
                         lv_coord_t min, lv_coord_t max);
```

**axis 参数：**
- `LV_CHART_AXIS_PRIMARY_Y`：主 Y 轴（左侧）
- `LV_CHART_AXIS_SECONDARY_Y`：副 Y 轴（右侧）

---

### lv_chart_add_series()

添加数据系列。

```c
lv_chart_series_t *lv_chart_add_series(lv_obj_t *obj,
                                        lv_color_t color,
                                        lv_chart_axis_t axis);
```

---

### lv_chart_set_next_value()

追加一个数据点（旧数据自动滚动）。

```c
void lv_chart_set_next_value(lv_obj_t *obj,
                              lv_chart_series_t *ser,
                              lv_coord_t value);
```

---

### lv_chart_set_value_by_id()

设置指定索引的数据点。

```c
void lv_chart_set_value_by_id(lv_obj_t *obj,
                               lv_chart_series_t *ser,
                               uint16_t id,
                               lv_coord_t value);
```

---

### lv_chart_refresh()

强制刷新图表（修改数据后调用）。

```c
void lv_chart_refresh(lv_obj_t *obj);
```

---

### lv_chart_set_div_line_count()

设置网格线数量。

```c
void lv_chart_set_div_line_count(lv_obj_t *obj, uint8_t hdiv, uint8_t vdiv);
```

---

### lv_chart_set_zoom_x() / lv_chart_set_zoom_y()

设置缩放（256 = 1:1，512 = 2倍）。

```c
void lv_chart_set_zoom_x(lv_obj_t *obj, uint16_t zoom_x);
void lv_chart_set_zoom_y(lv_obj_t *obj, uint16_t zoom_y);
```

---

## 💡 完整应用示例

### 示例1：实时折线图（传感器数据）

```c
#include "lvgl.h"
#include "esp_log.h"

static lv_obj_t *chart;
static lv_chart_series_t *temp_ser;
static lv_chart_series_t *humi_ser;

void chart_init(void)
{
    chart = lv_chart_create(lv_scr_act());
    lv_obj_set_size(chart, 300, 150);
    lv_obj_align(chart, LV_ALIGN_CENTER, 0, 0);

    /* 折线图，60 个数据点 */
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, 60);

    /* Y 轴范围 */
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);

    /* 网格线 */
    lv_chart_set_div_line_count(chart, 5, 5);

    /* 隐藏数据点（只显示线）*/
    lv_obj_set_style_size(chart, 0, LV_PART_INDICATOR);

    /* 添加两条数据线 */
    temp_ser = lv_chart_add_series(chart, lv_color_hex(0xFF5722),
                                    LV_CHART_AXIS_PRIMARY_Y);
    humi_ser = lv_chart_add_series(chart, lv_color_hex(0x2196F3),
                                    LV_CHART_AXIS_PRIMARY_Y);

    /* 图表背景 */
    lv_obj_set_style_bg_color(chart, lv_color_hex(0x1a1a2e), LV_PART_MAIN);
    lv_obj_set_style_border_color(chart, lv_color_hex(0x333355), LV_PART_MAIN);
}

/* 在定时器中调用，添加新数据 */
void chart_add_data(float temp, float humi)
{
    lv_chart_set_next_value(chart, temp_ser, (lv_coord_t)temp);
    lv_chart_set_next_value(chart, humi_ser, (lv_coord_t)humi);
    lv_chart_refresh(chart);
}
```

---

### 示例2：柱状图（统计数据）

```c
void bar_chart_init(void)
{
    lv_obj_t *chart = lv_chart_create(lv_scr_act());
    lv_obj_set_size(chart, 280, 160);
    lv_obj_center(chart);

    lv_chart_set_type(chart, LV_CHART_TYPE_BAR);
    lv_chart_set_point_count(chart, 7);  // 7 天
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);

    lv_chart_series_t *ser = lv_chart_add_series(chart,
                                                   lv_color_hex(0x4CAF50),
                                                   LV_CHART_AXIS_PRIMARY_Y);

    /* 设置每天的数据 */
    int daily_data[] = {65, 72, 58, 80, 45, 90, 70};
    for (int i = 0; i < 7; i++) {
        lv_chart_set_value_by_id(chart, ser, i, daily_data[i]);
    }
    lv_chart_refresh(chart);
}
```

---

### 示例3：双 Y 轴图表

```c
void dual_axis_chart(void)
{
    lv_obj_t *chart = lv_chart_create(lv_scr_act());
    lv_obj_set_size(chart, 280, 150);
    lv_obj_center(chart);

    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, 30);

    /* 主 Y 轴：温度（0~50°C）*/
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 50);
    lv_chart_series_t *temp_ser = lv_chart_add_series(chart,
                                                        lv_color_hex(0xFF5722),
                                                        LV_CHART_AXIS_PRIMARY_Y);

    /* 副 Y 轴：湿度（0~100%）*/
    lv_chart_set_range(chart, LV_CHART_AXIS_SECONDARY_Y, 0, 100);
    lv_chart_series_t *humi_ser = lv_chart_add_series(chart,
                                                        lv_color_hex(0x2196F3),
                                                        LV_CHART_AXIS_SECONDARY_Y);

    /* 填充测试数据 */
    for (int i = 0; i < 30; i++) {
        lv_chart_set_next_value(chart, temp_ser, 20 + (i % 10));
        lv_chart_set_next_value(chart, humi_ser, 50 + (i % 20));
    }
    lv_chart_refresh(chart);
}
```

---

### 示例4：带图例的图表

```c
void chart_with_legend(lv_obj_t *parent)
{
    /* 图表 */
    lv_obj_t *chart = lv_chart_create(parent);
    lv_obj_set_size(chart, 260, 130);
    lv_obj_align(chart, LV_ALIGN_TOP_MID, 0, 10);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, 20);
    lv_obj_set_style_size(chart, 0, LV_PART_INDICATOR);

    lv_chart_series_t *s1 = lv_chart_add_series(chart, lv_color_hex(0xFF5722),
                                                  LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_series_t *s2 = lv_chart_add_series(chart, lv_color_hex(0x4CAF50),
                                                  LV_CHART_AXIS_PRIMARY_Y);

    /* 图例 */
    lv_obj_t *legend = lv_obj_create(parent);
    lv_obj_set_size(legend, 260, 30);
    lv_obj_align(legend, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_set_layout(legend, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(legend, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(legend, LV_FLEX_ALIGN_CENTER,
                           LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(legend, 20, LV_PART_MAIN);
    lv_obj_set_style_border_width(legend, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(legend, LV_OPA_TRANSP, LV_PART_MAIN);

    /* 图例项 */
    const char *names[] = {"温度", "湿度"};
    lv_color_t colors[] = {lv_color_hex(0xFF5722), lv_color_hex(0x4CAF50)};

    for (int i = 0; i < 2; i++) {
        lv_obj_t *dot = lv_obj_create(legend);
        lv_obj_set_size(dot, 10, 10);
        lv_obj_set_style_bg_color(dot, colors[i], LV_PART_MAIN);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_border_width(dot, 0, LV_PART_MAIN);

        lv_obj_t *lbl = lv_label_create(legend);
        lv_label_set_text(lbl, names[i]);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xaaaaaa), LV_PART_MAIN);
    }
}
```

---

## ⚠️ 注意事项

### 1. 数据点数量影响内存

每个数据系列的每个点占用 `sizeof(lv_coord_t)` 字节（通常 2 字节）。
60 点 × 2 系列 = 240 字节，不大，但多系列时注意。

### 2. 实时数据必须调用 refresh

修改数据后必须调用 `lv_chart_refresh(chart)` 才会重绘。

### 3. 隐藏数据点标记

折线图默认在每个数据点显示小圆点，通常需要隐藏：
```c
lv_obj_set_style_size(chart, 0, LV_PART_INDICATOR);
```

### 4. 线宽设置

```c
lv_obj_set_style_line_width(chart, 2, LV_PART_ITEMS);
```

---

## 📖 相关文档

- [LVGL_Core.md](LVGL_Core.md) — 核心概念
- [LVGL_Style.md](LVGL_Style.md) — 样式
- [LVGL_Animation.md](LVGL_Animation.md) — 动画
- [README.md](README.md) — 模块导航
