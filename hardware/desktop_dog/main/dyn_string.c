#include "dyn_string.h"
#include <string.h>
#include <stdlib.h>
#include "esp_heap_caps.h"

#define MIN_CAP 64

// 使用PSRAM分配
static void *psram_alloc(size_t size)
{
    return heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
}

static void *psram_realloc(void *ptr, size_t size)
{
    return heap_caps_realloc(ptr, size, MALLOC_CAP_SPIRAM);
}

dyn_string_t *dyn_string_create(size_t init_cap)
{
    dyn_string_t *s = psram_alloc(sizeof(dyn_string_t));
    if (!s) return NULL;

    if (init_cap < MIN_CAP) init_cap = MIN_CAP;
    s->data = psram_alloc(init_cap);
    if (!s->data) {
        free(s);
        return NULL;
    }
    s->data[0] = '\0';
    s->len = 0;
    s->cap = init_cap;
    return s;
}

dyn_string_t *dyn_string_from(const char *str)
{
    size_t len = str ? strlen(str) : 0;
    dyn_string_t *s = dyn_string_create(len + 1);
    if (s && str) {
        memcpy(s->data, str, len);
        s->data[len] = '\0';
        s->len = len;
    }
    return s;
}

void dyn_string_free(dyn_string_t *s)
{
    if (s) {
        if (s->data) heap_caps_free(s->data);
        heap_caps_free(s);
    }
}

void dyn_string_clear(dyn_string_t *s)
{
    if (s && s->data) {
        s->data[0] = '\0';
        s->len = 0;
    }
}

const char *dyn_string_cstr(dyn_string_t *s)
{
    return s ? s->data : NULL;
}

size_t dyn_string_len(dyn_string_t *s)
{
    return s ? s->len : 0;
}

bool dyn_string_reserve(dyn_string_t *s, size_t cap)
{
    if (!s) return false;
    if (cap <= s->cap) return true;

    // 扩展为2倍或目标容量
    size_t new_cap = s->cap * 2;
    if (new_cap < cap) new_cap = cap;

    char *new_data = psram_realloc(s->data, new_cap);
    if (!new_data) return false;

    s->data = new_data;
    s->cap = new_cap;
    return true;
}

bool dyn_string_append_n(dyn_string_t *s, const char *str, size_t n)
{
    if (!s || !str || n == 0) return false;

    size_t need = s->len + n + 1;
    if (need > s->cap && !dyn_string_reserve(s, need)) {
        return false;
    }

    memcpy(s->data + s->len, str, n);
    s->len += n;
    s->data[s->len] = '\0';
    return true;
}

bool dyn_string_append(dyn_string_t *s, const char *str)
{
    if (!str) return false;
    return dyn_string_append_n(s, str, strlen(str));
}

bool dyn_string_append_char(dyn_string_t *s, char c)
{
    return dyn_string_append_n(s, &c, 1);
}
