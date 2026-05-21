#ifndef __SONGS_H
#define __SONGS_H

#include <stdint.h>

/* ===== 音符频率定义 (Hz) ===== */
#define NOTE_REST  0
#define NOTE_G3   196
#define NOTE_B3   247
#define NOTE_C4   262
#define NOTE_D4   294
#define NOTE_E4   330
#define NOTE_F4   349
#define NOTE_G4   392
#define NOTE_A4   440
#define NOTE_B4   494
#define NOTE_C5   523
#define NOTE_D5   587
#define NOTE_E5   659
#define NOTE_F5   698
#define NOTE_G5   784
#define NOTE_A5   880

/* ===== 音符结构体 ===== */
typedef struct {
    uint16_t freq;      /* 频率 Hz，0 = 休止符 */
    uint16_t duration;  /* 持续时间 ms */
} Note;

/* ===== 歌曲总数 ===== */
#define NUM_SONGS  3

/* ===== 歌曲数据声明（定义在 songs.c） ===== */
extern const Note song_twinkle[];
extern const Note song_birthday[];
extern const Note song_ode[];
extern const Note * const song_list[NUM_SONGS];
extern const uint16_t song_list_len[NUM_SONGS];
extern const char * const song_names[NUM_SONGS];  /* 歌曲名称，用于 OLED 显示 */

#endif /* __SONGS_H */
