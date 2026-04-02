#ifndef MIMO_AI_H
#define MIMO_AI_H

#include "esp_err.h"
#include <stdbool.h>
#include "dyn_string.h"

/**
 * @brief 初始化Mimo AI模块
 * @return ESP_OK成功
 */
esp_err_t mimo_ai_init(void);

/**
 * @brief 调用Mimo AI获取回答（动态字符串版本）
 * @param question 用户问题
 * @param answer 动态字符串，会自动扩展
 * @return ESP_OK成功，其他失败
 */
esp_err_t mimo_ai_chat(const char *question, dyn_string_t *answer);

/**
 * @brief 执行待执行的动作（TTS完成后调用）
 * @return ESP_OK成功
 */
esp_err_t mimo_ai_execute_pending_actions(void);

/**
 * @brief 清除待执行的动作（TTS被打断时调用）
 */
void mimo_ai_clear_pending_actions(void);

/**
 * @brief 重置对话上下文
 */
void mimo_ai_reset_context(void);

/**
 * @brief 检查是否有连续对话pending
 * @return true有，false没有
 */
bool mimo_ai_is_continue_chat_pending(void);

/**
 * @brief 工具调用结果结构
 */
typedef struct {
    char name[32];       // 工具名称
    char arguments[512]; // 工具参数JSON
    char call_id[64];    // 调用ID
} mimo_tool_call_t;

/**
 * @brief 工具执行结果
 */
typedef struct {
    char call_id[64];
    char result[1024];
    bool success;
} mimo_tool_result_t;

#endif // MIMO_AI_H
