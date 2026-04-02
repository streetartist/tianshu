#ifndef BAIDU_TTS_H
#define BAIDU_TTS_H

#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief 百度语音合成并播放（异步，事件驱动）
 *
 * Starts TTS playback in background task. When playback completes,
 * sends SIG_TTS_PLAYBACK_DONE to the specified entity.
 *
 * @param text 要合成的文本
 * @param notify_entity_id Entity ID to receive SIG_TTS_PLAYBACK_DONE when done (0 to disable)
 * @return ESP_OK成功，其他失败
 */
esp_err_t baidu_tts_speak_async(const char *text, uint16_t notify_entity_id);

/**
 * @brief 百度语音合成并播放（异步，旧API兼容）
 * @param text 要合成的文本
 * @return ESP_OK成功，其他失败
 */
esp_err_t baidu_tts_speak(const char *text);

/**
 * @brief 停止TTS播放
 * @return ESP_OK成功
 */
esp_err_t baidu_tts_stop(void);

/**
 * @brief 检查TTS是否正在播放
 * @return true正在播放，false未播放
 */
bool baidu_tts_is_playing(void);

/**
 * @brief 等待TTS播放完成（阻塞）
 */
void baidu_tts_wait(void);

/**
 * @brief 生成提示音并保存到SD卡（如果不存在）
 * @param text 要合成的文本
 * @param filename 保存的文件名（不含路径）
 * @return ESP_OK成功
 */
esp_err_t baidu_tts_generate_prompt(const char *text, const char *filename);

/**
 * @brief 播放SD卡上的提示音（阻塞）
 * @param filename 文件名（不含路径）
 * @return ESP_OK成功
 */
esp_err_t baidu_tts_play_prompt(const char *filename);

#endif // BAIDU_TTS_H
