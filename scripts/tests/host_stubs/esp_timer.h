// Host stand-in for ESP-IDF's esp_timer: one-shot timers that the test fires by hand.
#pragma once

#include <cstdint>

typedef int esp_err_t;
typedef void (*esp_timer_cb_t)(void* arg);
typedef enum { ESP_TIMER_TASK } esp_timer_dispatch_t;

typedef struct {
    esp_timer_cb_t callback;
    void* arg;
    esp_timer_dispatch_t dispatch_method;
    const char* name;
    bool skip_unhandled_events;
} esp_timer_create_args_t;

struct esp_timer {
    esp_timer_cb_t callback;
    void* arg;
    bool armed;
    uint64_t timeout_us;
};
typedef esp_timer* esp_timer_handle_t;

// The fake clock, advanced by the test.
inline int64_t g_now_us = 0;
inline int64_t esp_timer_get_time() { return g_now_us; }

inline esp_err_t esp_timer_create(const esp_timer_create_args_t* args, esp_timer_handle_t* out) {
    *out = new esp_timer{args->callback, args->arg, false, 0};
    return 0;
}
inline esp_err_t esp_timer_start_once(esp_timer_handle_t timer, uint64_t timeout_us) {
    timer->armed = true;
    timer->timeout_us = timeout_us;
    return 0;
}
inline esp_err_t esp_timer_stop(esp_timer_handle_t timer) {
    timer->armed = false;
    return 0;
}
inline esp_err_t esp_timer_delete(esp_timer_handle_t timer) {
    delete timer;
    return 0;
}
// Test helper: run the callback as the esp_timer task would.
inline void esp_timer_fire(esp_timer_handle_t timer) {
    timer->armed = false;
    timer->callback(timer->arg);
}
