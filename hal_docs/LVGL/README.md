# LVGL 文档导航

本文档是 LVGL v8.x API 文档的导航与概览，聚焦于每个模块的**使用特点**和**适用场景**。

> **版本说明：** 本文档基于 **LVGL v8.3.x**，配合 **ESP-IDF 框架**使用。

---

## 目录

- [核心概念](#核心概念)
- [控件系统](#控件系统)
- [布局系统](#布局系统)
- [样式与主题](#样式与主题)
- [动画系统](#动画系统)
- [控件速查表](#控件速查表)
- [常见 UI 模式](#常见-ui-模式)

---

## 核心概念

### LVGL Core — 核心 API
**文档**: [LVGL_Core.md](LVGL_Core.md)

LVGL 的基础设施，包括初始化、对象系统、事件、样式基础、屏幕管理。

**核心特点**
- 所有 UI 元素都是 `lv_obj_t` 对象，构成父子树
- 事件系统：`lv_obj_add_event_cb` 注册回调，支持点击、长按、值变化等
- `lv_timer_handler` 必须在主循环中周期调用（每 5~10ms）
- `lv_tick_inc` 必须每毫秒调用（通常用 esp_timer 实现）
- 多任务环境中所有 LVGL API 调用需要互斥锁保护

**初始化流程**
```
lv_init()
→ 分配绘制缓冲区（lv_disp_draw_buf_init）
→ 注册显示驱动（lv_disp_drv_register）
→ 注册输入设备（lv_indev_drv_register，可选）
→ 启动 lv_tick_inc 定时器
→ 主循环中周期调用 lv_timer_handler
```

---

## 控件系统

### LVGL Widgets — 常用控件
**文档**: [LVGL_Widgets.md](LVGL_Widgets.md)

LVGL 内置丰富控件，覆盖常见 UI 需求。

**控件速查**

| 控件 | 函数前缀 | 主要用途 |
|------|---------|---------|
| 标签 | `lv_label_*` | 显示文本、数值 |
| 按钮 | `lv_btn_*` | 点击交互 |
| 图像 | `lv_img_*` | 显示图片、图标 |
| 进度条 | `lv_bar_*` | 显示进度、电量 |
| 滑块 | `lv_slider_*` | 调节数值 |
| 开关 | `lv_switch_*` | 布尔值切换 |
| 圆弧 | `lv_arc_*` | 圆形进度、旋钮 |
| 图表 | `lv_chart_*` | 折线图、柱状图 |
| 仪表盘 | `lv_meter_*` | 模拟仪表 |
| 文本框 | `lv_textarea_*` | 文本输入 |
| 下拉列表 | `lv_dropdown_*` | 选项选择 |
| 标签页 | `lv_tabview_*` | 多页面切换 |
| 消息框 | `lv_msgbox_*` | 弹出对话框 |
| 加载动画 | `lv_spinner_*` | 等待指示器 |

---

## 布局系统

### LVGL Layout — 布局
**文档**: [LVGL_Layout.md](LVGL_Layout.md)

类似 CSS Flexbox 和 Grid 的现代布局系统。

**Flex 布局适用场景**

| 场景 | 配置 |
|------|------|
| 水平按钮栏 | `LV_FLEX_FLOW_ROW` + `LV_FLEX_ALIGN_SPACE_EVENLY` |
| 垂直菜单列表 | `LV_FLEX_FLOW_COLUMN` + `LV_FLEX_ALIGN_START` |
| 图标 + 文字行 | `LV_FLEX_FLOW_ROW` + `lv_obj_set_flex_grow` |
| 自动换行标签云 | `LV_FLEX_FLOW_ROW_WRAP` |

**Grid 布局适用场景**

| 场景 | 配置 |
|------|------|
| 信息卡片网格 | 固定行列数，`LV_GRID_FR` 均分 |
| 表单（标签+输入框）| `LV_GRID_CONTENT` + `LV_GRID_FR(1)` |
| 仪表盘布局 | 混合固定和弹性列 |

---

## 样式与主题

### LVGL Style — 样式
**文档**: [LVGL_Style.md](LVGL_Style.md)

控制所有视觉外观的样式系统。

**常用样式属性速查**

| 属性 | 函数 |
|------|------|
| 背景颜色 | `lv_obj_set_style_bg_color` |
| 背景渐变 | `lv_obj_set_style_bg_grad_color` + `bg_grad_dir` |
| 边框 | `lv_obj_set_style_border_width` + `border_color` |
| 圆角 | `lv_obj_set_style_radius` |
| 阴影 | `lv_obj_set_style_shadow_width` + `shadow_color` |
| 文字颜色 | `lv_obj_set_style_text_color` |
| 字体 | `lv_obj_set_style_text_font` |
| 内边距 | `lv_obj_set_style_pad_all` |
| 透明度 | `lv_obj_set_style_opa` |

**内置主题**
- `lv_theme_default_init`：支持深色/浅色，Material Design 风格
- 调色板：`lv_palette_main(LV_PALETTE_BLUE)` 等 18 种颜色

---

## 动画系统

### LVGL Animation — 动画
**文档**: [LVGL_Animation.md](LVGL_Animation.md)

对任意数值属性做平滑过渡。

**常用动画模式**

| 场景 | 配置 |
|------|------|
| 滑入效果 | `lv_obj_set_x` + `lv_anim_path_ease_out` |
| 淡入淡出 | `lv_obj_set_style_opa` + playback |
| 进度条填充 | `lv_bar_set_value` 或自定义 exec_cb |
| 无限闪烁 | `LV_ANIM_REPEAT_INFINITE` + playback |
| 按钮点击反馈 | 按下缩小 + 释放弹性放大 |

**内置缓动函数**

| 函数 | 效果 |
|------|------|
| `lv_anim_path_linear` | 匀速 |
| `lv_anim_path_ease_out` | 快→慢（最自然）|
| `lv_anim_path_ease_in_out` | 慢→快→慢 |
| `lv_anim_path_overshoot` | 超出后回弹 |
| `lv_anim_path_bounce` | 弹跳 |

---

## 控件速查表

### 按需求找控件

| 需求 | 控件 |
|------|------|
| 显示文字/数值 | `lv_label` |
| 用户点击操作 | `lv_btn` |
| 显示图片/图标 | `lv_img` |
| 显示进度/电量 | `lv_bar` |
| 调节数值（拖动）| `lv_slider` |
| 开关切换 | `lv_switch` |
| 圆形进度/旋钮 | `lv_arc` |
| 实时数据曲线 | `lv_chart` |
| 模拟仪表盘 | `lv_meter` |
| 文本输入 | `lv_textarea` |
| 选项选择 | `lv_dropdown` |
| 多页面切换 | `lv_tabview` |
| 弹出确认框 | `lv_msgbox` |
| 加载等待 | `lv_spinner` |
| 容器/布局 | `lv_obj`（基础对象）|

---

## 常见 UI 模式

### 状态栏

```c
lv_obj_t *status_bar = lv_obj_create(lv_scr_act());
lv_obj_set_size(status_bar, LV_PCT(100), 30);
lv_obj_align(status_bar, LV_ALIGN_TOP_MID, 0, 0);
lv_obj_set_layout(status_bar, LV_LAYOUT_FLEX);
lv_obj_set_flex_flow(status_bar, LV_FLEX_FLOW_ROW);
lv_obj_set_flex_align(status_bar,
                       LV_FLEX_ALIGN_SPACE_BETWEEN,
                       LV_FLEX_ALIGN_CENTER,
                       LV_FLEX_ALIGN_CENTER);
```

### 底部导航栏

```c
lv_obj_t *nav = lv_obj_create(lv_scr_act());
lv_obj_set_size(nav, LV_PCT(100), 60);
lv_obj_align(nav, LV_ALIGN_BOTTOM_MID, 0, 0);
lv_obj_set_layout(nav, LV_LAYOUT_FLEX);
lv_obj_set_flex_flow(nav, LV_FLEX_FLOW_ROW);
lv_obj_set_flex_align(nav,
                       LV_FLEX_ALIGN_SPACE_EVENLY,
                       LV_FLEX_ALIGN_CENTER,
                       LV_FLEX_ALIGN_CENTER);
```

### 弹出层（Modal）

```c
/* 半透明遮罩 */
lv_obj_t *mask = lv_obj_create(lv_scr_act());
lv_obj_set_size(mask, LV_PCT(100), LV_PCT(100));
lv_obj_set_style_bg_color(mask, lv_color_black(), LV_PART_MAIN);
lv_obj_set_style_bg_opa(mask, LV_OPA_50, LV_PART_MAIN);
lv_obj_set_style_border_width(mask, 0, LV_PART_MAIN);

/* 弹出卡片 */
lv_obj_t *popup = lv_obj_create(mask);
lv_obj_set_size(popup, 250, 150);
lv_obj_center(popup);
```

---

## 文档列表

| 文件 | 模块 |
|------|------|
| [LVGL_Core.md](LVGL_Core.md) | 初始化、对象系统、事件、屏幕管理 |
| [LVGL_Label.md](LVGL_Label.md) | 文本标签、富文本、跑马灯 |
| [LVGL_Button.md](LVGL_Button.md) | 按钮、切换按钮、按钮组 |
| [LVGL_Bar_Slider.md](LVGL_Bar_Slider.md) | 进度条、滑块 |
| [LVGL_Arc_Meter.md](LVGL_Arc_Meter.md) | 圆弧、仪表盘 |
| [LVGL_Chart.md](LVGL_Chart.md) | 折线图、柱状图、散点图 |
| [LVGL_Input.md](LVGL_Input.md) | 文本框、下拉列表、虚拟键盘 |
| [LVGL_Container.md](LVGL_Container.md) | 标签页、列表、消息框、开关、复选框 |
| [LVGL_Image.md](LVGL_Image.md) | 图像显示、符号图标、旋转缩放 |
| [LVGL_Layout.md](LVGL_Layout.md) | Flex 布局、Grid 布局 |
| [LVGL_Style.md](LVGL_Style.md) | 样式属性、主题、调色板 |
| [LVGL_Animation.md](LVGL_Animation.md) | 动画系统、缓动函数 |
| [LVGL_Font.md](LVGL_Font.md) | 字体系统、中文支持、字体转换 |

---

## 参考资料

- [LVGL 官方文档](https://docs.lvgl.io/8.3)
- [LVGL GitHub](https://github.com/lvgl/lvgl)
- [SquareLine Studio（可视化设计工具）](https://squareline.io)
- [LVGL 字体转换工具](https://lvgl.io/tools/fontconverter)
- [ESP-IDF + LVGL 官方示例](https://github.com/lvgl/lv_port_esp32)
