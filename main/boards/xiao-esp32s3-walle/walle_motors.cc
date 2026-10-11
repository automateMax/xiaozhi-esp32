#include "walle_motors.h"

#include <esp_log.h>

#include <algorithm>
#include <cstdlib>
#include <mutex>

#define TAG "WalleMotors"

namespace {

constexpr ledc_mode_t kSpeedMode = LEDC_LOW_SPEED_MODE;
// Timer 0 is used by PwmBacklight and timer 1 by GpioLed; stay clear of both.
constexpr ledc_timer_t kTimer = LEDC_TIMER_2;
constexpr ledc_timer_bit_t kResolution = LEDC_TIMER_10_BIT;
constexpr uint32_t kFrequencyHz = 20000;  // above the audible range
constexpr uint32_t kMaxDuty = (1u << 10) - 1;
// N20 gear motors stall below roughly 40% duty, so non-zero speeds start here.
constexpr uint32_t kMinDuty = 400;

constexpr ledc_channel_t kLeftChannelA = LEDC_CHANNEL_2;
constexpr ledc_channel_t kLeftChannelB = LEDC_CHANNEL_3;
constexpr ledc_channel_t kRightChannelA = LEDC_CHANNEL_4;
constexpr ledc_channel_t kRightChannelB = LEDC_CHANNEL_5;

void ConfigureChannel(gpio_num_t gpio, ledc_channel_t channel) {
    ledc_channel_config_t channel_config = {};
    channel_config.gpio_num = gpio;
    channel_config.speed_mode = kSpeedMode;
    channel_config.channel = channel;
    channel_config.timer_sel = kTimer;
    channel_config.duty = 0;
    channel_config.hpoint = 0;
    ESP_ERROR_CHECK(ledc_channel_config(&channel_config));
}

}  // namespace

WalleMotors::WalleMotors(gpio_num_t left_in_a, gpio_num_t left_in_b, gpio_num_t right_in_a,
                         gpio_num_t right_in_b, bool left_invert, bool right_invert)
    : left_invert_(left_invert), right_invert_(right_invert) {
    ledc_timer_config_t timer_config = {};
    timer_config.speed_mode = kSpeedMode;
    timer_config.duty_resolution = kResolution;
    timer_config.timer_num = kTimer;
    timer_config.freq_hz = kFrequencyHz;
    timer_config.clk_cfg = LEDC_AUTO_CLK;
    ESP_ERROR_CHECK(ledc_timer_config(&timer_config));

    ConfigureChannel(left_in_a, kLeftChannelA);
    ConfigureChannel(left_in_b, kLeftChannelB);
    ConfigureChannel(right_in_a, kRightChannelA);
    ConfigureChannel(right_in_b, kRightChannelB);

    esp_timer_create_args_t stop_timer_args = {};
    stop_timer_args.callback = &WalleMotors::StopTimerCallback;
    stop_timer_args.arg = this;
    stop_timer_args.dispatch_method = ESP_TIMER_TASK;
    stop_timer_args.name = "walle_motor_stop";
    ESP_ERROR_CHECK(esp_timer_create(&stop_timer_args, &stop_timer_));

    SetOutputs(0, 0);
    ESP_LOGI(TAG, "Track motors ready");
}

WalleMotors::~WalleMotors() {
    if (stop_timer_ != nullptr) {
        esp_timer_stop(stop_timer_);
        esp_timer_delete(stop_timer_);
    }
    SetOutputs(0, 0);
}

// Runs on the esp_timer task. esp_timer_stop() cannot cancel a callback that
// has already started, so it only stops the command it was armed for.
void WalleMotors::StopTimerCallback(void* arg) {
    auto* motors = static_cast<WalleMotors*>(arg);
    std::lock_guard<std::mutex> lock(motors->mutex_);
    if (motors->command_ == motors->timed_command_) {
        motors->SetOutputs(0, 0);
    }
}

void WalleMotors::Drive(int left_speed, int right_speed) { DriveFor(left_speed, right_speed, 0); }

void WalleMotors::DriveFor(int left_speed, int right_speed, int duration_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    // Cancel a pending auto-stop so it cannot cut this new command short.
    esp_timer_stop(stop_timer_);
    command_++;
    SetOutputs(left_speed, right_speed);
    if (duration_ms > 0) {
        timed_command_ = command_;
        esp_timer_start_once(stop_timer_, static_cast<uint64_t>(duration_ms) * 1000);
    }
}

void WalleMotors::Stop() { DriveFor(0, 0, 0); }

void WalleMotors::SetOutputs(int left_speed, int right_speed) {
    SetMotor(kLeftChannelA, kLeftChannelB, left_invert_ ? -left_speed : left_speed);
    SetMotor(kRightChannelA, kRightChannelB, right_invert_ ? -right_speed : right_speed);
}

void WalleMotors::SetMotor(ledc_channel_t channel_a, ledc_channel_t channel_b, int speed) {
    uint32_t duty = 0;
    if (speed != 0) {
        uint32_t magnitude = static_cast<uint32_t>(std::min(std::abs(speed), 100));
        duty = kMinDuty + (kMaxDuty - kMinDuty) * magnitude / 100;
    }
    // Both inputs low lets the motor coast to a stop.
    ledc_set_duty(kSpeedMode, channel_a, speed > 0 ? duty : 0);
    ledc_update_duty(kSpeedMode, channel_a);
    ledc_set_duty(kSpeedMode, channel_b, speed < 0 ? duty : 0);
    ledc_update_duty(kSpeedMode, channel_b);
}
