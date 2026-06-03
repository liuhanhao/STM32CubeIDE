#ifndef __PLAYER_H
#define __PLAYER_H

#include "stm32f1xx_hal.h"
#include "songs.h"

/* 播放器状态 */
typedef enum {
    PLAYER_STOPPED = 0,
    PLAYER_PLAYING,
    PLAYER_PAUSED
} PlayerState;

/**
 * @brief 初始化播放器
 */
void Player_Init(void);

/**
 * @brief 开始播放指定歌曲
 * @param song_idx 歌曲索引（SONG_ODE_TO_JOY / SONG_LITTLE_STAR / SONG_MARIO）
 */
void Player_Play(uint8_t song_idx);

/**
 * @brief 暂停播放
 */
void Player_Pause(void);

/**
 * @brief 继续播放
 */
void Player_Resume(void);

/**
 * @brief 停止播放
 */
void Player_Stop(void);

/**
 * @brief 播放下一首
 */
void Player_Next(void);

/**
 * @brief 播放上一首
 */
void Player_Prev(void);

/**
 * @brief 获取当前播放器状态
 */
PlayerState Player_GetState(void);

/**
 * @brief 获取当前播放歌曲索引
 */
uint8_t Player_GetSongIdx(void);

/**
 * @brief 主循环中调用，推进音符播放
 *        （基于 HAL_GetTick 的非阻塞方式）
 */
void Player_Tick(void);

#endif /* __PLAYER_H */
