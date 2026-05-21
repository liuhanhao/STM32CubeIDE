/**
  ******************************************************************************
  * @file    songs.c
  * @brief   八音盒歌曲数据
  *          歌曲1：小星星 (Twinkle Twinkle Little Star)
  *          歌曲2：生日快乐 (Happy Birthday)
  *          歌曲3：欢乐颂 (Ode to Joy)
  ******************************************************************************
  */
#include "songs.h"

/* ===================================================
 * 歌曲1：小星星
 * =================================================== */
const Note song_twinkle[] = {
    /* Twinkle twinkle little star */
    {NOTE_C4, 400}, {NOTE_C4, 400}, {NOTE_G4, 400}, {NOTE_G4, 400},
    {NOTE_A4, 400}, {NOTE_A4, 400}, {NOTE_G4, 800},
    /* How I wonder what you are */
    {NOTE_F4, 400}, {NOTE_F4, 400}, {NOTE_E4, 400}, {NOTE_E4, 400},
    {NOTE_D4, 400}, {NOTE_D4, 400}, {NOTE_C4, 800},
    /* Up above the world so high */
    {NOTE_G4, 400}, {NOTE_G4, 400}, {NOTE_F4, 400}, {NOTE_F4, 400},
    {NOTE_E4, 400}, {NOTE_E4, 400}, {NOTE_D4, 800},
    /* Like a diamond in the sky */
    {NOTE_G4, 400}, {NOTE_G4, 400}, {NOTE_F4, 400}, {NOTE_F4, 400},
    {NOTE_E4, 400}, {NOTE_E4, 400}, {NOTE_D4, 800},
    /* Twinkle twinkle little star */
    {NOTE_C4, 400}, {NOTE_C4, 400}, {NOTE_G4, 400}, {NOTE_G4, 400},
    {NOTE_A4, 400}, {NOTE_A4, 400}, {NOTE_G4, 800},
    /* How I wonder what you are */
    {NOTE_F4, 400}, {NOTE_F4, 400}, {NOTE_E4, 400}, {NOTE_E4, 400},
    {NOTE_D4, 400}, {NOTE_D4, 400}, {NOTE_C4, 800},
    {NOTE_REST, 400},
};

/* ===================================================
 * 歌曲2：生日快乐
 * =================================================== */
const Note song_birthday[] = {
    /* 祝你生日快乐 x2 */
    {NOTE_G4, 300}, {NOTE_REST, 100},
    {NOTE_G4, 300}, {NOTE_REST, 100},
    {NOTE_A4, 600}, {NOTE_G4, 600},
    {NOTE_C5, 600}, {NOTE_B4, 800}, {NOTE_REST, 200},

    {NOTE_G4, 300}, {NOTE_REST, 100},
    {NOTE_G4, 300}, {NOTE_REST, 100},
    {NOTE_A4, 600}, {NOTE_G4, 600},
    {NOTE_D5, 600}, {NOTE_C5, 800}, {NOTE_REST, 200},

    /* 祝你生日快乐 */
    {NOTE_G4, 300}, {NOTE_REST, 100},
    {NOTE_G4, 300}, {NOTE_REST, 100},
    {NOTE_G5, 600}, {NOTE_E5, 600},
    {NOTE_C5, 600}, {NOTE_B4, 600}, {NOTE_A4, 800}, {NOTE_REST, 200},

    /* 生日快乐 */
    {NOTE_F5, 300}, {NOTE_REST, 100},
    {NOTE_F5, 300}, {NOTE_REST, 100},
    {NOTE_E5, 600}, {NOTE_C5, 600},
    {NOTE_D5, 600}, {NOTE_C5, 800},
    {NOTE_REST, 400},
};

/* ===================================================
 * 歌曲3：欢乐颂 (Ode to Joy - Beethoven)
 * =================================================== */
const Note song_ode[] = {
    /* 第一句 */
    {NOTE_E4, 400}, {NOTE_E4, 400}, {NOTE_F4, 400}, {NOTE_G4, 400},
    {NOTE_G4, 400}, {NOTE_F4, 400}, {NOTE_E4, 400}, {NOTE_D4, 400},
    {NOTE_C4, 400}, {NOTE_C4, 400}, {NOTE_D4, 400}, {NOTE_E4, 400},
    {NOTE_E4, 600}, {NOTE_D4, 200}, {NOTE_D4, 800},
    /* 第二句 */
    {NOTE_E4, 400}, {NOTE_E4, 400}, {NOTE_F4, 400}, {NOTE_G4, 400},
    {NOTE_G4, 400}, {NOTE_F4, 400}, {NOTE_E4, 400}, {NOTE_D4, 400},
    {NOTE_C4, 400}, {NOTE_C4, 400}, {NOTE_D4, 400}, {NOTE_E4, 400},
    {NOTE_D4, 600}, {NOTE_C4, 200}, {NOTE_C4, 800},
    /* 第三句 */
    {NOTE_D4, 400}, {NOTE_D4, 400}, {NOTE_E4, 400}, {NOTE_C4, 400},
    {NOTE_D4, 400}, {NOTE_E4, 200}, {NOTE_F4, 200}, {NOTE_E4, 400}, {NOTE_C4, 400},
    {NOTE_D4, 400}, {NOTE_E4, 200}, {NOTE_F4, 200}, {NOTE_E4, 400}, {NOTE_D4, 400},
    {NOTE_C4, 400}, {NOTE_D4, 400}, {NOTE_G3, 800},
    /* 第四句 */
    {NOTE_E4, 400}, {NOTE_E4, 400}, {NOTE_F4, 400}, {NOTE_G4, 400},
    {NOTE_G4, 400}, {NOTE_F4, 400}, {NOTE_E4, 400}, {NOTE_D4, 400},
    {NOTE_C4, 400}, {NOTE_C4, 400}, {NOTE_D4, 400}, {NOTE_E4, 400},
    {NOTE_D4, 600}, {NOTE_C4, 200}, {NOTE_C4, 800},
    {NOTE_REST, 400},
};

/* ===== 歌曲列表 ===== */
const Note * const song_list[NUM_SONGS] = {
    song_twinkle,
    song_birthday,
    song_ode,
};

const uint16_t song_list_len[NUM_SONGS] = {
    sizeof(song_twinkle)  / sizeof(song_twinkle[0]),
    sizeof(song_birthday) / sizeof(song_birthday[0]),
    sizeof(song_ode)      / sizeof(song_ode[0]),
};

/* ===== 歌曲名称（每首最多 16 个 ASCII 字符，对应 OLED 一行）===== */
const char * const song_names[NUM_SONGS] = {
    "Twinkle Star",
    "Happy Birthday",
    "Ode to Joy",
};
