#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief 初始化音频播放器（包括SD卡挂载）
 */
esp_err_t audio_player_init(void);

/**
 * @brief 播放URL音频（MP3格式）
 * @param url 音频URL
 * @return ESP_OK成功
 */
esp_err_t audio_player_play_url(const char *url);

/**
 * @brief 播放SD卡上的OGG文件
 * @param filename 文件名（不含路径，文件应在/sdcard/ogg/目录下）
 * @return ESP_OK成功
 */
esp_err_t audio_player_play_ogg(const char *filename);

/**
 * @brief 停止播放
 */
esp_err_t audio_player_stop(void);

/**
 * @brief 检查是否正在播放
 */
bool audio_player_is_playing(void);

/**
 * @brief 检查SD卡是否已挂载
 */
bool audio_player_sd_mounted(void);

/**
 * @brief 设置音量 (0-100)
 */
void audio_player_set_volume(int volume);

/**
 * @brief 获取当前音量
 */
int audio_player_get_volume(void);

#endif // AUDIO_PLAYER_H
