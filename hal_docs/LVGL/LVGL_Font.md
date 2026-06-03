# LVGL 字体系统 API 文档

## 📋 模块概述

LVGL 的字体系统支持：
- 内置 Montserrat 字体（多种尺寸）
- 自定义字体（TTF 转换为 C 数组）
- 中文字体支持（Unicode 范围选择）
- 字体合并（ASCII + 中文）
- 符号图标字体

---

## 一、内置字体

### 启用内置字体

在 `lv_conf.h` 中启用需要的字体尺寸：

```c
/* Montserrat 字体（推荐）*/
#define LV_FONT_MONTSERRAT_8  1
#define LV_FONT_MONTSERRAT_10 1
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1   /* 默认字体 */
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_22 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_26 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_30 1
#define LV_FONT_MONTSERRAT_32 1
#define LV_FONT_MONTSERRAT_34 1
#define LV_FONT_MONTSERRAT_36 1
#define LV_FONT_MONTSERRAT_38 1
#define LV_FONT_MONTSERRAT_40 1
#define LV_FONT_MONTSERRAT_42 1
#define LV_FONT_MONTSERRAT_44 1
#define LV_FONT_MONTSERRAT_46 1
#define LV_FONT_MONTSERRAT_48 1

/* 默认字体 */
#define LV_FONT_DEFAULT &lv_font_montserrat_14
```

### 使用内置字体

```c
/* 设置标签字体 */
lv_obj_set_style_text_font(label, &lv_font_montserrat_16, LV_PART_MAIN);
lv_obj_set_style_text_font(label, &lv_font_montserrat_24, LV_PART_MAIN);

/* 全局设置默认字体 */
lv_style_set_text_font(&style, &lv_font_montserrat_14);
```

---

## 二、自定义字体（中文支持）

### 方法1：在线字体转换工具

1. 访问 [https://lvgl.io/tools/fontconverter](https://lvgl.io/tools/fontconverter)
2. 上传 TTF 字体文件（推荐思源黑体 `SourceHanSansCN-Regular.ttf`）
3. 配置参数：
   - **Name**：字体变量名（如 `lv_font_cn_16`）
   - **Size**：字体大小（像素）
   - **Bpp**：抗锯齿位数（4 = 推荐，1 = 最小体积）
   - **Range**：字符范围（见下方）
4. 下载生成的 `.c` 文件

**常用字符范围：**

| 范围 | 说明 | 字符数 |
|------|------|--------|
| `0x20-0x7E` | ASCII 可打印字符 | 95 |
| `0x4E00-0x9FFF` | 常用汉字（CJK 统一表意文字）| 20,902 |
| `0x3000-0x303F` | CJK 标点符号 | 64 |
| `0xFF00-0xFFEF` | 全角字符 | 240 |

**推荐配置（常用汉字 + ASCII）：**
```
Range: 0x20-0x7E, 0x4E00-0x9FFF, 0x3000-0x303F
```

**精简配置（只包含项目用到的汉字）：**
```
Range: 0x20-0x7E, 温度湿度电量连接断开设置确认取消
```

---

### 方法2：命令行工具（lv_font_conv）

```bash
# 安装
npm install -g lv_font_conv

# 生成中文字体（16px，常用汉字 + ASCII）
lv_font_conv \
  --font SourceHanSansCN-Regular.ttf \
  --size 16 \
  --bpp 4 \
  --format lvgl \
  --range 0x20-0x7E \
  --range 0x4E00-0x9FFF \
  -o lv_font_cn_16.c
```

---

### 在项目中使用自定义字体

#### 1. 添加到 CMakeLists.txt

```cmake
idf_component_register(
    SRCS
        "main.c"
        "fonts/lv_font_cn_16.c"   # 添加字体文件
    INCLUDE_DIRS "." "fonts"
    ...
)
```

#### 2. 在 lv_conf.h 中声明

```c
/* 声明外部字体 */
LV_FONT_DECLARE(lv_font_cn_16);
LV_FONT_DECLARE(lv_font_cn_24);
```

#### 3. 使用字体

```c
lv_obj_t *label = lv_label_create(lv_scr_act());
lv_obj_set_style_text_font(label, &lv_font_cn_16, LV_PART_MAIN);
lv_label_set_text(label, "你好，世界！Hello World!");
```

---

## 三、字体合并（ASCII + 中文）

将多个字体合并为一个，实现 ASCII 用 Montserrat，中文用思源黑体：

```bash
lv_font_conv \
  --font Montserrat-Regular.ttf \
  --size 16 \
  --bpp 4 \
  --range 0x20-0x7E \
  --font SourceHanSansCN-Regular.ttf \
  --size 16 \
  --bpp 4 \
  --range 0x4E00-0x9FFF \
  --format lvgl \
  -o lv_font_mixed_16.c
```

---

## 四、字体 API

### lv_obj_set_style_text_font()

为对象设置字体。

```c
void lv_obj_set_style_text_font(lv_obj_t *obj,
                                 const lv_font_t *value,
                                 lv_style_selector_t selector);
```

---

### lv_font_get_line_height()

获取字体行高。

```c
lv_coord_t lv_font_get_line_height(const lv_font_t *font_p);
```

---

### lv_txt_get_size()

计算文本渲染后的尺寸。

```c
void lv_txt_get_size(lv_point_t *size_res,
                     const char *text,
                     const lv_font_t *font,
                     lv_coord_t letter_space,
                     lv_coord_t line_space,
                     lv_coord_t max_width,
                     lv_text_flag_t flag);
```

---

## 💡 完整应用示例

### 示例1：多字体混合显示

```c
/* lv_conf.h 中启用 */
LV_FONT_DECLARE(lv_font_cn_16);

void multi_font_demo(void)
{
    /* 英文标题（Montserrat 24）*/
    lv_obj_t *title = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_label_set_text(title, "ESP32-S3 Monitor");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    /* 中文内容（思源黑体 16）*/
    lv_obj_t *content = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(content, &lv_font_cn_16, LV_PART_MAIN);
    lv_label_set_text(content, "当前温度: 25.6°C\n当前湿度: 60%\n系统状态: 正常");
    lv_obj_align(content, LV_ALIGN_CENTER, 0, 0);
}
```

---

### 示例2：全局设置中文字体

```c
/* 在初始化时设置全局默认字体 */
void set_global_font(void)
{
    /* 方法1：通过主题设置 */
    lv_theme_t *theme = lv_theme_default_init(
        lv_disp_get_default(),
        lv_palette_main(LV_PALETTE_BLUE),
        lv_palette_main(LV_PALETTE_RED),
        true,
        &lv_font_cn_16  // 使用中文字体作为默认字体
    );
    lv_disp_set_theme(lv_disp_get_default(), theme);

    /* 方法2：通过全局样式 */
    static lv_style_t style_global;
    lv_style_init(&style_global);
    lv_style_set_text_font(&style_global, &lv_font_cn_16);
    lv_obj_add_style(lv_scr_act(), &style_global, LV_PART_MAIN);
}
```

---

### 示例3：动态字体大小（响应式）

```c
const lv_font_t *get_font_for_size(int size_px)
{
    if (size_px <= 12) return &lv_font_montserrat_12;
    if (size_px <= 14) return &lv_font_montserrat_14;
    if (size_px <= 16) return &lv_font_montserrat_16;
    if (size_px <= 20) return &lv_font_montserrat_20;
    if (size_px <= 24) return &lv_font_montserrat_24;
    return &lv_font_montserrat_32;
}
```

---

## 五、字体文件大小参考

字体文件大小影响固件体积，选择时需要权衡：

| 配置 | 大约大小 |
|------|---------|
| ASCII 16px，4bpp | ~10KB |
| ASCII 24px，4bpp | ~20KB |
| 常用汉字 2500 字，16px，4bpp | ~200KB |
| 常用汉字 2500 字，24px，4bpp | ~400KB |
| 全部汉字 20902 字，16px，4bpp | ~1.5MB |

> **建议：** 只包含项目实际用到的汉字，可以大幅减小字体文件体积。
> 使用 lv_font_conv 的 `--symbols` 参数指定具体字符。

---

## ⚠️ 注意事项

### 1. 字体文件存储在 Flash

字体 C 数组存储在 Flash 中，不占用 RAM。
但大字体文件（>1MB）会占用大量 Flash 空间，注意分区大小。

### 2. 字符范围要完整

如果文本中包含字体范围外的字符，会显示为方块（□）。
确保字体包含所有需要显示的字符。

### 3. 字体大小与显示效果

- 小字体（<14px）：可读性差，建议最小 14px
- 中文字体：建议 16px 以上，否则笔画太细难以辨认
- 高分辨率屏幕：可以使用更大字体

### 4. 抗锯齿（Bpp）

- `1bpp`：无抗锯齿，体积最小，边缘锯齿明显
- `2bpp`：轻微抗锯齿
- `4bpp`：良好抗锯齿（推荐）
- `8bpp`：最佳抗锯齿，体积最大

---

## 📖 相关文档

- [LVGL_Core.md](LVGL_Core.md) — 核心概念
- [LVGL_Label.md](LVGL_Label.md) — 文本标签
- [LVGL_Style.md](LVGL_Style.md) — 样式（文字颜色、间距）
- [README.md](README.md) — 模块导航
