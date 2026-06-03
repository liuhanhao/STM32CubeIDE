/**
 * @file    weather_icons.h
 * @brief   天气图标字模数据（16×16 像素，列主序，SSD1306 页格式）
 *
 * 存储格式：每个图标 32 字节，按列主序排列
 *   data[col * 2 + 0] = 该列上半部分（page 0，bit0 在最上方）
 *   data[col * 2 + 1] = 该列下半部分（page 1，bit0 在中间行）
 *
 * 图标分类（12 类，覆盖和风天气全部 icon 码）：
 *   ICON_SUNNY       晴          100, 150
 *   ICON_PARTLY      晴间多云     101, 102, 103, 151, 152, 153
 *   ICON_CLOUDY      阴          104
 *   ICON_LIGHT_RAIN  小雨/阵雨   300, 301, 305, 306, 309, 314, 350, 351, 399
 *   ICON_HEAVY_RAIN  大雨/暴雨   307, 308, 310~318
 *   ICON_THUNDER     雷阵雨      302, 303, 304
 *   ICON_SLEET       雨夹雪      313, 404, 405, 406, 456
 *   ICON_SNOW        雪          400~403, 407~410, 457, 499
 *   ICON_FOG         雾          500, 501, 509, 510, 514, 515
 *   ICON_HAZE        霾          502, 511, 512, 513
 *   ICON_DUST        沙尘        503, 504, 507, 508
 *   ICON_UNKNOWN     未知/热/冷  900, 901, 999
 */

#ifndef WEATHER_ICONS_H
#define WEATHER_ICONS_H

#include <stdint.h>

/* ------------------------------------------------------------------ */
/*  图标索引枚举                                                        */
/* ------------------------------------------------------------------ */
typedef enum {
    ICON_SUNNY       = 0,
    ICON_PARTLY      = 1,
    ICON_CLOUDY      = 2,
    ICON_LIGHT_RAIN  = 3,
    ICON_HEAVY_RAIN  = 4,
    ICON_THUNDER     = 5,
    ICON_SLEET       = 6,
    ICON_SNOW        = 7,
    ICON_FOG         = 8,
    ICON_HAZE        = 9,
    ICON_DUST        = 10,
    ICON_UNKNOWN     = 11,
    ICON_COUNT       = 12
} WeatherIconIndex_t;

/* ------------------------------------------------------------------ */
/*  图标字模数据（16×16，列主序，每图标 32 字节）                        */
/* ------------------------------------------------------------------ */

/*
 * ☀️  晴（ICON_SUNNY）
 * 太阳：中心实心圆 + 8 方向射线
 *
 *   . . . . . . . . . . . . . . . .
 *   . . . . . . 1 . 1 . . . . . . .
 *   . . . . . . . 1 . . . . . . . .
 *   . . . . . 1 . . . 1 . . . . . .
 *   . . . . . . 1 1 1 . . . . . . .
 *   . . 1 . . 1 1 1 1 1 . . 1 . . .
 *   . . . 1 . 1 1 1 1 1 . 1 . . . .
 *   . . . . . . 1 1 1 . . . . . . .
 *   . . . . . 1 . . . 1 . . . . . .
 *   . . . . . . . 1 . . . . . . . .
 *   . . . . . . 1 . 1 . . . . . . .
 *   . . . . . . . . . . . . . . . .
 */
static const uint8_t WEATHER_ICON_SUNNY[32] = {
    0x00, 0x00,  /* col 0  */
    0x00, 0x00,  /* col 1  */
    0x40, 0x00,  /* col 2  */
    0x00, 0x00,  /* col 3  */
    0x00, 0x00,  /* col 4  */
    0x22, 0x00,  /* col 5  */
    0x9C, 0x00,  /* col 6  */
    0xF2, 0x00,  /* col 7  */
    0x9C, 0x00,  /* col 8  */
    0x22, 0x00,  /* col 9  */
    0x00, 0x00,  /* col 10 */
    0x00, 0x00,  /* col 11 */
    0x40, 0x00,  /* col 12 */
    0x00, 0x00,  /* col 13 */
    0x00, 0x00,  /* col 14 */
    0x00, 0x00,  /* col 15 */
};

/*
 * ⛅  晴间多云（ICON_PARTLY）
 * 上方小太阳 + 下方云朵
 */
static const uint8_t WEATHER_ICON_PARTLY[32] = {
    0x00, 0x00,  /* col 0  */
    0x00, 0x00,  /* col 1  */
    0x08, 0x00,  /* col 2  */
    0x24, 0x00,  /* col 3  */
    0x1C, 0x00,  /* col 4  */
    0x3E, 0x00,  /* col 5  */
    0x1C, 0x00,  /* col 6  */
    0x24, 0x00,  /* col 7  */
    0x08, 0x00,  /* col 8  */
    0x00, 0x3E,  /* col 9  */
    0x00, 0x7F,  /* col 10 */
    0x00, 0xFF,  /* col 11 */
    0x00, 0xFF,  /* col 12 */
    0x00, 0x7F,  /* col 13 */
    0x00, 0x3E,  /* col 14 */
    0x00, 0x00,  /* col 15 */
};

/*
 * ☁️  阴（ICON_CLOUDY）
 * 大云朵居中
 */
static const uint8_t WEATHER_ICON_CLOUDY[32] = {
    0x00, 0x00,  /* col 0  */
    0x00, 0x1C,  /* col 1  */
    0x00, 0x3E,  /* col 2  */
    0xC0, 0x7F,  /* col 3  */
    0xF0, 0xFF,  /* col 4  */
    0xF8, 0xFF,  /* col 5  */
    0xF8, 0xFF,  /* col 6  */
    0xF8, 0xFF,  /* col 7  */
    0xF8, 0xFF,  /* col 8  */
    0xF8, 0xFF,  /* col 9  */
    0xF0, 0xFF,  /* col 10 */
    0xC0, 0x7F,  /* col 11 */
    0x00, 0x3E,  /* col 12 */
    0x00, 0x1C,  /* col 13 */
    0x00, 0x00,  /* col 14 */
    0x00, 0x00,  /* col 15 */
};

/*
 * 🌦️  小雨（ICON_LIGHT_RAIN）
 * 云朵 + 2 条雨线
 */
static const uint8_t WEATHER_ICON_LIGHT_RAIN[32] = {
    0x00, 0x00,  /* col 0  */
    0x00, 0x08,  /* col 1  */
    0xC0, 0x1C,  /* col 2  */
    0xF0, 0x3E,  /* col 3  */
    0xF8, 0x7F,  /* col 4  */
    0xF8, 0xFF,  /* col 5  */
    0xF8, 0xF6,  /* col 6  */
    0xF8, 0xC0,  /* col 7  */
    0xF8, 0xFF,  /* col 8  */
    0xF8, 0xF6,  /* col 9  */
    0xF0, 0xC0,  /* col 10 */
    0xC0, 0x3E,  /* col 11 */
    0x00, 0x1C,  /* col 12 */
    0x00, 0x08,  /* col 13 */
    0x00, 0x00,  /* col 14 */
    0x00, 0x00,  /* col 15 */
};

/*
 * 🌧️  大雨（ICON_HEAVY_RAIN）
 * 云朵 + 4 条密集雨线
 */
static const uint8_t WEATHER_ICON_HEAVY_RAIN[32] = {
    0x00, 0x00,  /* col 0  */
    0x00, 0x24,  /* col 1  */
    0xC0, 0x5A,  /* col 2  */
    0xF0, 0xBE,  /* col 3  */
    0xF8, 0xFF,  /* col 4  */
    0xF8, 0xDB,  /* col 5  */
    0xF8, 0x24,  /* col 6  */
    0xF8, 0xDB,  /* col 7  */
    0xF8, 0xFF,  /* col 8  */
    0xF8, 0xDB,  /* col 9  */
    0xF0, 0x24,  /* col 10 */
    0xC0, 0xBE,  /* col 11 */
    0x00, 0x5A,  /* col 12 */
    0x00, 0x24,  /* col 13 */
    0x00, 0x00,  /* col 14 */
    0x00, 0x00,  /* col 15 */
};

/*
 * ⛈️  雷阵雨（ICON_THUNDER）
 * 云朵 + 闪电符号
 */
static const uint8_t WEATHER_ICON_THUNDER[32] = {
    0x00, 0x00,  /* col 0  */
    0x00, 0x00,  /* col 1  */
    0xC0, 0x1C,  /* col 2  */
    0xF0, 0x3E,  /* col 3  */
    0xF8, 0x7F,  /* col 4  */
    0xF8, 0xFF,  /* col 5  */
    0xF8, 0xF8,  /* col 6  */
    0xF8, 0xFC,  /* col 7  */
    0xF8, 0xFE,  /* col 8  */
    0xF8, 0xF8,  /* col 9  */
    0xF0, 0x7F,  /* col 10 */
    0xC0, 0x3E,  /* col 11 */
    0x00, 0x1C,  /* col 12 */
    0x00, 0x00,  /* col 13 */
    0x00, 0x00,  /* col 14 */
    0x00, 0x00,  /* col 15 */
};

/*
 * 🌨️  雨夹雪（ICON_SLEET）
 * 云朵 + 交替雨点和雪花点
 */
static const uint8_t WEATHER_ICON_SLEET[32] = {
    0x00, 0x00,  /* col 0  */
    0x00, 0x10,  /* col 1  */
    0xC0, 0x28,  /* col 2  */
    0xF0, 0x54,  /* col 3  */
    0xF8, 0xAA,  /* col 4  */
    0xF8, 0xFF,  /* col 5  */
    0xF8, 0xAA,  /* col 6  */
    0xF8, 0x54,  /* col 7  */
    0xF8, 0xAA,  /* col 8  */
    0xF8, 0xFF,  /* col 9  */
    0xF0, 0xAA,  /* col 10 */
    0xC0, 0x54,  /* col 11 */
    0x00, 0x28,  /* col 12 */
    0x00, 0x10,  /* col 13 */
    0x00, 0x00,  /* col 14 */
    0x00, 0x00,  /* col 15 */
};

/*
 * ❄️  雪（ICON_SNOW）
 * 六角雪花形状
 */
static const uint8_t WEATHER_ICON_SNOW[32] = {
    0x00, 0x00,  /* col 0  */
    0x00, 0x00,  /* col 1  */
    0x20, 0x04,  /* col 2  */
    0x60, 0x06,  /* col 3  */
    0xC0, 0x03,  /* col 4  */
    0xFC, 0xFF,  /* col 5  */
    0xC0, 0x03,  /* col 6  */
    0x60, 0x06,  /* col 7  */
    0xC0, 0x03,  /* col 8  */
    0xFC, 0xFF,  /* col 9  */
    0xC0, 0x03,  /* col 10 */
    0x60, 0x06,  /* col 11 */
    0x20, 0x04,  /* col 12 */
    0x00, 0x00,  /* col 13 */
    0x00, 0x00,  /* col 14 */
    0x00, 0x00,  /* col 15 */
};

/*
 * 🌫️  雾（ICON_FOG）
 * 3 条水平虚线
 */
static const uint8_t WEATHER_ICON_FOG[32] = {
    0x00, 0x00,  /* col 0  */
    0x24, 0x24,  /* col 1  */
    0x24, 0x24,  /* col 2  */
    0x24, 0x24,  /* col 3  */
    0x24, 0x24,  /* col 4  */
    0x24, 0x24,  /* col 5  */
    0x24, 0x24,  /* col 6  */
    0x24, 0x24,  /* col 7  */
    0x24, 0x24,  /* col 8  */
    0x24, 0x24,  /* col 9  */
    0x24, 0x24,  /* col 10 */
    0x24, 0x24,  /* col 11 */
    0x24, 0x24,  /* col 12 */
    0x24, 0x24,  /* col 13 */
    0x24, 0x24,  /* col 14 */
    0x00, 0x00,  /* col 15 */
};

/*
 * 😶‍🌫️  霾（ICON_HAZE）
 * 3 条实线（比雾更密）
 */
static const uint8_t WEATHER_ICON_HAZE[32] = {
    0x00, 0x00,  /* col 0  */
    0x7E, 0x7E,  /* col 1  */
    0x7E, 0x7E,  /* col 2  */
    0x00, 0x00,  /* col 3  */
    0x7E, 0x7E,  /* col 4  */
    0x7E, 0x7E,  /* col 5  */
    0x00, 0x00,  /* col 6  */
    0x7E, 0x7E,  /* col 7  */
    0x7E, 0x7E,  /* col 8  */
    0x00, 0x00,  /* col 9  */
    0x7E, 0x7E,  /* col 10 */
    0x7E, 0x7E,  /* col 11 */
    0x00, 0x00,  /* col 12 */
    0x7E, 0x7E,  /* col 13 */
    0x7E, 0x7E,  /* col 14 */
    0x00, 0x00,  /* col 15 */
};

/*
 * 💨  沙尘（ICON_DUST）
 * 波浪线 + 颗粒点
 */
static const uint8_t WEATHER_ICON_DUST[32] = {
    0x00, 0x00,  /* col 0  */
    0x10, 0x01,  /* col 1  */
    0x28, 0x02,  /* col 2  */
    0x44, 0x04,  /* col 3  */
    0x82, 0x08,  /* col 4  */
    0x44, 0x10,  /* col 5  */
    0x28, 0x20,  /* col 6  */
    0x10, 0x40,  /* col 7  */
    0x28, 0x20,  /* col 8  */
    0x44, 0x10,  /* col 9  */
    0x82, 0x08,  /* col 10 */
    0x44, 0x04,  /* col 11 */
    0x28, 0x02,  /* col 12 */
    0x10, 0x01,  /* col 13 */
    0x00, 0x00,  /* col 14 */
    0x00, 0x00,  /* col 15 */
};

/*
 * ❓  未知（ICON_UNKNOWN）
 * 问号
 */
static const uint8_t WEATHER_ICON_UNKNOWN[32] = {
    0x00, 0x00,  /* col 0  */
    0x00, 0x00,  /* col 1  */
    0x00, 0x00,  /* col 2  */
    0xF0, 0x00,  /* col 3  */
    0xF8, 0x00,  /* col 4  */
    0x08, 0x06,  /* col 5  */
    0x08, 0x06,  /* col 6  */
    0xF0, 0x03,  /* col 7  */
    0xE0, 0x03,  /* col 8  */
    0x00, 0x06,  /* col 9  */
    0x00, 0x06,  /* col 10 */
    0x00, 0x00,  /* col 11 */
    0x00, 0x00,  /* col 12 */
    0x00, 0x00,  /* col 13 */
    0x00, 0x00,  /* col 14 */
    0x00, 0x00,  /* col 15 */
};

/* ------------------------------------------------------------------ */
/*  图标指针表（按 WeatherIconIndex_t 索引）                            */
/* ------------------------------------------------------------------ */
static const uint8_t * const WEATHER_ICONS[ICON_COUNT] = {
    WEATHER_ICON_SUNNY,       /* ICON_SUNNY      */
    WEATHER_ICON_PARTLY,      /* ICON_PARTLY     */
    WEATHER_ICON_CLOUDY,      /* ICON_CLOUDY     */
    WEATHER_ICON_LIGHT_RAIN,  /* ICON_LIGHT_RAIN */
    WEATHER_ICON_HEAVY_RAIN,  /* ICON_HEAVY_RAIN */
    WEATHER_ICON_THUNDER,     /* ICON_THUNDER    */
    WEATHER_ICON_SLEET,       /* ICON_SLEET      */
    WEATHER_ICON_SNOW,        /* ICON_SNOW       */
    WEATHER_ICON_FOG,         /* ICON_FOG        */
    WEATHER_ICON_HAZE,        /* ICON_HAZE       */
    WEATHER_ICON_DUST,        /* ICON_DUST       */
    WEATHER_ICON_UNKNOWN,     /* ICON_UNKNOWN    */
};

/* ------------------------------------------------------------------ */
/*  icon 码 → 图标索引映射函数                                          */
/* ------------------------------------------------------------------ */
/**
 * @brief  将和风天气 icon 码映射到本地图标索引
 * @param  icon_code  和风天气返回的 icon 字段值（如 100, 300, 502 等）
 * @return WeatherIconIndex_t 枚举值
 */
static inline WeatherIconIndex_t weather_icon_index(uint16_t icon_code)
{
    /* 晴 */
    if (icon_code == 100 || icon_code == 150)
        return ICON_SUNNY;

    /* 晴间多云 / 少云 */
    if (icon_code == 101 || icon_code == 102 || icon_code == 103 ||
        icon_code == 151 || icon_code == 152 || icon_code == 153)
        return ICON_PARTLY;

    /* 阴 */
    if (icon_code == 104)
        return ICON_CLOUDY;

    /* 雷阵雨 */
    if (icon_code >= 302 && icon_code <= 304)
        return ICON_THUNDER;

    /* 大雨 / 暴雨 */
    if (icon_code == 307 || icon_code == 308 ||
        (icon_code >= 310 && icon_code <= 318))
        return ICON_HEAVY_RAIN;

    /* 小雨 / 阵雨（其余 3xx） */
    if (icon_code >= 300 && icon_code <= 399)
        return ICON_LIGHT_RAIN;

    /* 雨夹雪 */
    if (icon_code == 313 || icon_code == 404 || icon_code == 405 ||
        icon_code == 406 || icon_code == 456)
        return ICON_SLEET;

    /* 雪 */
    if ((icon_code >= 400 && icon_code <= 410) ||
        icon_code == 457 || icon_code == 499)
        return ICON_SNOW;

    /* 霾 */
    if (icon_code == 502 || icon_code == 511 ||
        icon_code == 512 || icon_code == 513)
        return ICON_HAZE;

    /* 沙尘 */
    if (icon_code == 503 || icon_code == 504 ||
        icon_code == 507 || icon_code == 508)
        return ICON_DUST;

    /* 雾 */
    if (icon_code >= 500 && icon_code <= 515)
        return ICON_FOG;

    /* 其余（热/冷/未知） */
    return ICON_UNKNOWN;
}

#endif /* WEATHER_ICONS_H */
