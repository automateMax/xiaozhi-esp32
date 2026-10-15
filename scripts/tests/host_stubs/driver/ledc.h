// Host stand-in for ESP-IDF's LEDC PWM driver: records the duty of each channel.
#pragma once

#include <cstdint>

#include "driver/gpio.h"

typedef int esp_err_t;
#define ESP_OK 0
#define ESP_ERROR_CHECK(x) (void)(x)

typedef enum { LEDC_LOW_SPEED_MODE } ledc_mode_t;
typedef enum { LEDC_TIMER_0, LEDC_TIMER_1, LEDC_TIMER_2, LEDC_TIMER_3 } ledc_timer_t;
typedef enum { LEDC_TIMER_10_BIT = 10 } ledc_timer_bit_t;
typedef enum { LEDC_AUTO_CLK } ledc_clk_cfg_t;
typedef enum {
    LEDC_CHANNEL_0,
    LEDC_CHANNEL_1,
    LEDC_CHANNEL_2,
    LEDC_CHANNEL_3,
    LEDC_CHANNEL_4,
    LEDC_CHANNEL_5,
    LEDC_CHANNEL_MAX
} ledc_channel_t;

typedef struct {
    ledc_mode_t speed_mode;
    ledc_timer_bit_t duty_resolution;
    ledc_timer_t timer_num;
    uint32_t freq_hz;
    ledc_clk_cfg_t clk_cfg;
} ledc_timer_config_t;

typedef struct {
    gpio_num_t gpio_num;
    ledc_mode_t speed_mode;
    ledc_channel_t channel;
    ledc_timer_t timer_sel;
    uint32_t duty;
    int hpoint;
} ledc_channel_config_t;

inline uint32_t g_ledc_duty[LEDC_CHANNEL_MAX] = {};

inline esp_err_t ledc_timer_config(const ledc_timer_config_t*) { return ESP_OK; }
inline esp_err_t ledc_channel_config(const ledc_channel_config_t*) { return ESP_OK; }
inline esp_err_t ledc_set_duty(ledc_mode_t, ledc_channel_t channel, uint32_t duty) {
    g_ledc_duty[channel] = duty;
    return ESP_OK;
}
inline esp_err_t ledc_update_duty(ledc_mode_t, ledc_channel_t) { return ESP_OK; }
