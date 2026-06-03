/**
 * @file    player.c
 * @brief   非阻塞歌曲播放器
 *
 * 使用 HAL_GetTick() 计时，在主循环中调用 Player_Tick()，
 * 不占用任何额外定时器，也不阻塞 CPU。
 */

#include "player.h"
#include "buzzer.h"
#include "songs.h"

/* ------------------------------------------------------------------ */
/*  内部状态                                                              */
/* ------------------------------------------------------------------ */
static PlayerState  s_state     = PLAYER_STOPPED;
static uint8_t      s_song_idx  = 0;       /* 当前歌曲索引 */
static uint16_t     s_note_idx  = 0;       /* 当前音符索引 */
static uint32_t     s_next_tick = 0;       /* 下一个音符的 HAL tick */

/* 音符间隙比例（防止连音糊在一起）：实际发声 85%，静音 15% */
#define NOTE_GAP_RATIO  85

/* ------------------------------------------------------------------ */
/*  内部工具                                                              */
/* ------------------------------------------------------------------ */
static void _start_note(const Note *n)
{
    if (n->freq == NOTE_REST) {
        Buzzer_SetFreq(0);
    } else {
        /* 发声时长 = 85%，剩余 15% 由下个 tick 静音体现 */
        Buzzer_SetFreq(n->freq);
    }
    s_next_tick = HAL_GetTick() + n->ms;
}

/* ------------------------------------------------------------------ */
/*  公共接口                                                              */
/* ------------------------------------------------------------------ */

void Player_Init(void)
{
    s_state    = PLAYER_STOPPED;
    s_song_idx = 0;
    s_note_idx = 0;
    s_next_tick = 0;
}

void Player_Play(uint8_t song_idx)
{
    if (song_idx >= SONG_COUNT) return;

    /* 停止当前播放 */
    Buzzer_SetFreq(0);

    s_song_idx  = song_idx;
    s_note_idx  = 0;
    s_state     = PLAYER_PLAYING;

    /* 立即开始第一个音符 */
    _start_note(&g_songs[s_song_idx].notes[0]);
}

void Player_Pause(void)
{
    if (s_state != PLAYER_PLAYING) return;
    Buzzer_SetFreq(0);
    s_state = PLAYER_PAUSED;
}

void Player_Resume(void)
{
    if (s_state != PLAYER_PAUSED) return;
    s_state = PLAYER_PLAYING;
    /* 重新开始当前音符 */
    _start_note(&g_songs[s_song_idx].notes[s_note_idx]);
}

void Player_Stop(void)
{
    Buzzer_SetFreq(0);
    s_state    = PLAYER_STOPPED;
    s_note_idx = 0;
}

void Player_Next(void)
{
    uint8_t next = (s_song_idx + 1) % SONG_COUNT;
    Player_Play(next);
}

void Player_Prev(void)
{
    uint8_t prev = (s_song_idx == 0) ? (SONG_COUNT - 1) : (s_song_idx - 1);
    Player_Play(prev);
}

PlayerState Player_GetState(void)
{
    return s_state;
}

uint8_t Player_GetSongIdx(void)
{
    return s_song_idx;
}

/**
 * @brief 主循环中调用，推进音符播放（非阻塞）
 */
void Player_Tick(void)
{
    if (s_state != PLAYER_PLAYING) return;

    uint32_t now = HAL_GetTick();
    if (now < s_next_tick) return;   /* 当前音符未到期 */

    /* 先静音 15ms 间隙（音符断开感）*/
    Buzzer_SetFreq(0);

    /* 推进到下一个音符 */
    s_note_idx++;

    const SongInfo *song = &g_songs[s_song_idx];

    if (s_note_idx >= song->count) {
        /* 播放完毕，自动停止 */
        s_state    = PLAYER_STOPPED;
        s_note_idx = 0;
        return;
    }

    /* 稍微延迟一个间隙再发声 */
    HAL_Delay(15);
    _start_note(&song->notes[s_note_idx]);
}
