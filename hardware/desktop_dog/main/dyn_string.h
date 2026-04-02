#ifndef DYN_STRING_H
#define DYN_STRING_H

#include <stddef.h>
#include <stdbool.h>

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} dyn_string_t;

// 创建动态字符串（初始容量）
dyn_string_t *dyn_string_create(size_t init_cap);

// 从C字符串创建
dyn_string_t *dyn_string_from(const char *str);

// 释放动态字符串
void dyn_string_free(dyn_string_t *s);

// 追加字符串
bool dyn_string_append(dyn_string_t *s, const char *str);

// 追加指定长度的数据
bool dyn_string_append_n(dyn_string_t *s, const char *str, size_t n);

// 追加单个字符
bool dyn_string_append_char(dyn_string_t *s, char c);

// 清空内容（不释放内存）
void dyn_string_clear(dyn_string_t *s);

// 获取C字符串
const char *dyn_string_cstr(dyn_string_t *s);

// 获取长度
size_t dyn_string_len(dyn_string_t *s);

// 确保容量
bool dyn_string_reserve(dyn_string_t *s, size_t cap);

#endif // DYN_STRING_H
