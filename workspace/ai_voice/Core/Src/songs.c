/**
 * @file    songs.c
 * @brief   歌曲数据：欢乐颂、小星星、超级马里奥
 *
 * 时值说明（BPM ≈ 120，一拍 = 500ms）：
 *   全音符 = 2000ms   二分音符 = 1000ms
 *   四分音符 = 500ms  八分音符 = 250ms
 *   十六分音符 = 125ms
 */

#include "songs.h"

/* ------------------------------------------------------------------ */
/*  欢乐颂（Ode to Joy）                                                 */
/* ------------------------------------------------------------------ */
static const Note s_ode_to_joy[] = {
    /* 主旋律第一段 */
    {NOTE_M3, 500}, {NOTE_M3, 500}, {NOTE_M4, 500}, {NOTE_M5, 500},
    {NOTE_M5, 500}, {NOTE_M4, 500}, {NOTE_M3, 500}, {NOTE_M2, 500},
    {NOTE_M1, 500}, {NOTE_M1, 500}, {NOTE_M2, 500}, {NOTE_M3, 500},
    {NOTE_M3, 750}, {NOTE_M2, 250}, {NOTE_M2, 1000},
    /* 第二段 */
    {NOTE_M3, 500}, {NOTE_M3, 500}, {NOTE_M4, 500}, {NOTE_M5, 500},
    {NOTE_M5, 500}, {NOTE_M4, 500}, {NOTE_M3, 500}, {NOTE_M2, 500},
    {NOTE_M1, 500}, {NOTE_M1, 500}, {NOTE_M2, 500}, {NOTE_M3, 500},
    {NOTE_M2, 750}, {NOTE_M1, 250}, {NOTE_M1, 1000},
    /* 第三段 */
    {NOTE_M2, 500}, {NOTE_M2, 500}, {NOTE_M3, 500}, {NOTE_M1, 500},
    {NOTE_M2, 500}, {NOTE_M3, 250}, {NOTE_M4, 250}, {NOTE_M3, 500}, {NOTE_M1, 500},
    {NOTE_M2, 500}, {NOTE_M3, 250}, {NOTE_M4, 250}, {NOTE_M3, 500}, {NOTE_M2, 500},
    {NOTE_M1, 500}, {NOTE_M2, 500}, {NOTE_L5, 1000},
    /* 第四段（重复第一段旋律） */
    {NOTE_M3, 500}, {NOTE_M3, 500}, {NOTE_M4, 500}, {NOTE_M5, 500},
    {NOTE_M5, 500}, {NOTE_M4, 500}, {NOTE_M3, 500}, {NOTE_M2, 500},
    {NOTE_M1, 500}, {NOTE_M1, 500}, {NOTE_M2, 500}, {NOTE_M3, 500},
    {NOTE_M2, 750}, {NOTE_M1, 250}, {NOTE_M1, 1000},
    {NOTE_REST, 500},
};

/* ------------------------------------------------------------------ */
/*  小星星（Twinkle Twinkle Little Star）                                 */
/* ------------------------------------------------------------------ */
static const Note s_little_star[] = {
    {NOTE_M1, 500}, {NOTE_M1, 500}, {NOTE_M5, 500}, {NOTE_M5, 500},
    {NOTE_M6, 500}, {NOTE_M6, 500}, {NOTE_M5, 1000},
    {NOTE_M4, 500}, {NOTE_M4, 500}, {NOTE_M3, 500}, {NOTE_M3, 500},
    {NOTE_M2, 500}, {NOTE_M2, 500}, {NOTE_M1, 1000},
    {NOTE_M5, 500}, {NOTE_M5, 500}, {NOTE_M4, 500}, {NOTE_M4, 500},
    {NOTE_M3, 500}, {NOTE_M3, 500}, {NOTE_M2, 1000},
    {NOTE_M5, 500}, {NOTE_M5, 500}, {NOTE_M4, 500}, {NOTE_M4, 500},
    {NOTE_M3, 500}, {NOTE_M3, 500}, {NOTE_M2, 1000},
    {NOTE_M1, 500}, {NOTE_M1, 500}, {NOTE_M5, 500}, {NOTE_M5, 500},
    {NOTE_M6, 500}, {NOTE_M6, 500}, {NOTE_M5, 1000},
    {NOTE_M4, 500}, {NOTE_M4, 500}, {NOTE_M3, 500}, {NOTE_M3, 500},
    {NOTE_M2, 500}, {NOTE_M2, 500}, {NOTE_M1, 1000},
    {NOTE_REST, 500},
};

/* ------------------------------------------------------------------ */
/*  超级马里奥主题曲（Super Mario Bros. Theme — 简化版）                    */
/* ------------------------------------------------------------------ */
static const Note s_mario[] = {
    /* 经典开头 riff */
    {NOTE_M3, 125}, {NOTE_REST, 125},
    {NOTE_M3, 125}, {NOTE_REST, 375},
    {NOTE_M3, 125}, {NOTE_REST, 375},
    {NOTE_M1, 125}, {NOTE_REST, 125},
    {NOTE_M3, 250}, {NOTE_REST, 250},
    {NOTE_M5, 500}, {NOTE_REST, 500},
    {NOTE_L5, 500}, {NOTE_REST, 500},

    /* 主旋律第一句 */
    {NOTE_M1, 375}, {NOTE_L6, 125}, {NOTE_REST, 250},
    {NOTE_L6, 250}, {NOTE_L3, 500},
    {NOTE_L5, 500}, {NOTE_REST, 250},
    {NOTE_M1, 250}, {NOTE_L5, 250},

    /* 主旋律第二句 */
    {NOTE_M1, 375}, {NOTE_M2, 125}, {NOTE_REST, 250},
    {NOTE_M3, 250}, {NOTE_REST, 250},
    {NOTE_M2, 500}, {NOTE_REST, 250},
    {NOTE_M1, 250}, {NOTE_REST, 250},
    {NOTE_L6, 250}, {NOTE_REST, 250},
    {NOTE_L5, 500},

    /* 高潮段 */
    {NOTE_M5, 250}, {NOTE_M6, 250}, {NOTE_H1, 250}, {NOTE_REST, 125},
    {NOTE_M6, 125}, {NOTE_REST, 250},
    {NOTE_M5, 250}, {NOTE_M4, 250}, {NOTE_REST, 125},
    {NOTE_M3, 125}, {NOTE_REST, 125},
    {NOTE_M2, 500},

    /* 结尾 */
    {NOTE_M1, 375}, {NOTE_L6, 125}, {NOTE_REST, 250},
    {NOTE_L6, 250}, {NOTE_L3, 500},
    {NOTE_L5, 500}, {NOTE_REST, 250},
    {NOTE_M1, 250}, {NOTE_L5, 250},
    {NOTE_M1, 750}, {NOTE_REST, 250},

    {NOTE_REST, 500},
};

/* ------------------------------------------------------------------ */
/*  两只老虎（Are You Sleeping）                                          */
/* ------------------------------------------------------------------ */
static const Note s_two_tigers[] = {
    /* 两只老虎，两只老虎 */
    {NOTE_M1, 500}, {NOTE_M2, 500}, {NOTE_M3, 500}, {NOTE_M1, 500},
    {NOTE_M1, 500}, {NOTE_M2, 500}, {NOTE_M3, 500}, {NOTE_M1, 500},
    /* 跑得快，跑得快 */
    {NOTE_M3, 500}, {NOTE_M4, 500}, {NOTE_M5, 1000},
    {NOTE_M3, 500}, {NOTE_M4, 500}, {NOTE_M5, 1000},
    /* 一只没有眼睛，一只没有尾巴 */
    {NOTE_M5, 250}, {NOTE_M6, 250}, {NOTE_M5, 250}, {NOTE_M4, 250}, {NOTE_M3, 500}, {NOTE_M1, 500},
    {NOTE_M5, 250}, {NOTE_M6, 250}, {NOTE_M5, 250}, {NOTE_M4, 250}, {NOTE_M3, 500}, {NOTE_M1, 500},
    /* 真奇怪，真奇怪 */
    {NOTE_M2, 500}, {NOTE_L5, 500}, {NOTE_M1, 1000},
    {NOTE_M2, 500}, {NOTE_L5, 500}, {NOTE_M1, 1000},
    {NOTE_REST, 500},
};

/* ------------------------------------------------------------------ */
/*  歌曲目录                                                              */
/* ------------------------------------------------------------------ */
const SongInfo g_songs[SONG_COUNT] = {
    [SONG_ODE_TO_JOY]  = {
        .name  = "Ode to Joy",
        .notes = s_ode_to_joy,
        .count = sizeof(s_ode_to_joy) / sizeof(s_ode_to_joy[0]),
    },
    [SONG_LITTLE_STAR] = {
        .name  = "Little Star",
        .notes = s_little_star,
        .count = sizeof(s_little_star) / sizeof(s_little_star[0]),
    },
    [SONG_MARIO]       = {
        .name  = "Super Mario",
        .notes = s_mario,
        .count = sizeof(s_mario) / sizeof(s_mario[0]),
    },
    [SONG_TWO_TIGERS]  = {
        .name  = "Two Tigers",
        .notes = s_two_tigers,
        .count = sizeof(s_two_tigers) / sizeof(s_two_tigers[0]),
    },
};
