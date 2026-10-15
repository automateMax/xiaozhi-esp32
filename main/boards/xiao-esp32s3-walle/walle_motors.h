#ifndef WALLE_MOTORS_H
#define WALLE_MOTORS_H

#include <driver/gpio.h>
#include <driver/ledc.h>
#include <esp_timer.h>

#include <cstdint>
#include <mutex>
#include <vector>

#include "walle_sequence.h"

// Two DC track motors on an MX1508-style dual H-bridge. Each motor uses two
// PWM inputs: PWM on input A drives forward, PWM on input B drives backward.
class WalleMotors {
public:
    WalleMotors(gpio_num_t left_in_a, gpio_num_t left_in_b, gpio_num_t right_in_a,
                gpio_num_t right_in_b, bool left_invert, bool right_invert);
    ~WalleMotors();

    // Speeds are -100..100. Runs until Stop() or another Drive call.
    void Drive(int left_speed, int right_speed);
    // Drives for duration_ms, then stops automatically.
    void DriveFor(int left_speed, int right_speed, int duration_ms);
    void Stop();
    // Plays the steps one after another, timed on the robot, then stops.
    // Any Drive, DriveFor, Stop or new sequence cancels it.
    void RunSequence(std::vector<walle::SequenceStep> steps);

private:
    static void StopTimerCallback(void* arg);
    void StartStepLocked();
    void ArmTimerLocked(int duration_ms);
    void SetOutputs(int left_speed, int right_speed);
    void SetMotor(ledc_channel_t channel_a, ledc_channel_t channel_b, int speed);

    bool left_invert_;
    bool right_invert_;
    esp_timer_handle_t stop_timer_ = nullptr;
    std::mutex mutex_;
    uint32_t command_ = 0;                    // bumped by every Drive, DriveFor and Stop
    uint32_t timed_command_ = 0;              // the command the auto-stop timer belongs to
    std::vector<walle::SequenceStep> steps_;  // the running sequence, if any
    size_t next_step_ = 0;
    int64_t deadline_us_ = 0;  // when the armed timer is due

    // A timer callback earlier than this before its deadline is a stale one.
    static constexpr int64_t kTimerSlackUs = 2000;
};

#endif  // WALLE_MOTORS_H
