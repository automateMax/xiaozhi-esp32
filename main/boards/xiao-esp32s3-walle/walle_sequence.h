#ifndef WALLE_SEQUENCE_H
#define WALLE_SEQUENCE_H

#include <cstdint>
#include <string>
#include <vector>

// A short choreography for the two tracks, written as comma-separated steps:
//
//   F75:600      forward at speed 75 for 600 ms
//   B70:300      backward
//   L70:300      turn left in place (left track back, right track forward)
//   R70:300      turn right in place
//   T40/90:1200  raw track speeds, left/right, -100..100 (arcs and curves)
//   P:400        pause with the motors off
//
// Kept free of ESP-IDF so it can be unit-tested on the host.
namespace walle {

struct SequenceStep {
    int left;   // -100..100
    int right;  // -100..100
    int ms;
};

constexpr size_t kMaxSequenceSteps = 32;
constexpr int kMinStepMs = 20;
constexpr int kMaxStepMs = 3000;
constexpr int kMaxSequenceMs = 10000;
constexpr int kMaxSequenceRepeat = 5;

// Parses `text` and repeats it `repeat` times. On failure returns false and
// sets `error` to a short reason that is safe to show to the model.
bool ParseSequence(const std::string& text, int repeat, std::vector<SequenceStep>& steps,
                   std::string& error);

}  // namespace walle

#endif  // WALLE_SEQUENCE_H
