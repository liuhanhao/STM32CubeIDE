#ifndef __SONGS_H
#define __SONGS_H

#include "stm32f1xx_hal.h"

/* ------------------------------------------------------------------ */
/*  音符频率定义（标准12平均律，Hz）                                       */
/* ------------------------------------------------------------------ */
#define NOTE_REST  0     /* 休止符 */

/* 低音区（第3组）*/
#define NOTE_L1  262
#define NOTE_L2  294
#define NOTE_L3  330
#define NOTE_L4  349
#define NOTE_L5  392
#define NOTE_L6  440
#define NOTE_L7  494

/* 中音区（第4组）*/
#define NOTE_M1  523
#define NOTE_M2  587
#define NOTE_M3  659
#define NOTE_M4  698
#define NOTE_M5  784
#define NOTE_M6  880
#define NOTE_M7  988

/* 高音区（第5组）*/
#define NOTE_H1  1047
#define NOTE_H2  1175
#define NOTE_H3  1319
#define NOTE_H4  1397
#define NOTE_H5  1568
#define NOTE_H6  1760
#define NOTE_H7  1976

/* ------------------------------------------------------------------ */
/*  音符结构体：频率 + 时值（ms）                                          */
/* ------------------------------------------------------------------ */
typedef struct {
    uint32_t freq;    /* 频率 Hz，0 = 休止符 */
    uint32_t ms;      /* 持续时间 ms */
} Note;

/* ------------------------------------------------------------------ */
/*  歌曲索引                                                             */
/* ------------------------------------------------------------------ */
#define SONG_COUNT  4

#define SONG_ODE_TO_JOY    0   /* 欢乐颂 */
#define SONG_LITTLE_STAR   1   /* 小星星 */
#define SONG_MARIO         2   /* 超级马里奥 */
#define SONG_TWO_TIGERS    3   /* 两只老虎 */

/* 歌曲信息结构 */
typedef struct {
    const char   *name;        /* 显示名称（最多16字符）*/
    const Note   *notes;       /* 音符数组 */
    uint16_t      count;       /* 音符数量 */
} SongInfo;

extern const SongInfo g_songs[SONG_COUNT];

#endif /* __SONGS_H */
