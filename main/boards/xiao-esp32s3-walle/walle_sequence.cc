#include "walle_sequence.h"

#include <cctype>
#include <cstdlib>

namespace walle {
namespace {

// Reads an optionally signed integer at `pos`; advances `pos` past it.
bool ReadInt(const std::string& s, size_t& pos, int& value) {
    size_t start = pos;
    if (pos < s.size() && (s[pos] == '-' || s[pos] == '+')) {
        pos++;
    }
    size_t digits = pos;
    while (pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos]))) {
        pos++;
    }
    if (pos == digits || pos - digits > 5) {
        pos = start;
        return false;
    }
    value = std::atoi(s.substr(start, pos - start).c_str());
    return true;
}

bool InRange(int value, int low, int high) { return value >= low && value <= high; }

bool ParseStep(const std::string& token, SequenceStep& step, std::string& error) {
    if (token.empty()) {
        error = "empty step";
        return false;
    }
    char kind = static_cast<char>(std::toupper(static_cast<unsigned char>(token[0])));
    size_t pos = 1;
    int left = 0;
    int right = 0;
    if (kind == 'P') {
        // No speed.
    } else if (kind == 'T') {
        if (!ReadInt(token, pos, left) || pos >= token.size() || token[pos] != '/' ||
            !ReadInt(token, ++pos, right)) {
            error = "bad track speeds in '" + token + "'";
            return false;
        }
        if (!InRange(left, -100, 100) || !InRange(right, -100, 100)) {
            error = "track speed out of -100..100 in '" + token + "'";
            return false;
        }
    } else if (kind == 'F' || kind == 'B' || kind == 'L' || kind == 'R') {
        int speed = 0;
        if (!ReadInt(token, pos, speed) || !InRange(speed, 1, 100)) {
            error = "speed must be 1..100 in '" + token + "'";
            return false;
        }
        switch (kind) {
            case 'F':
                left = speed;
                right = speed;
                break;
            case 'B':
                left = -speed;
                right = -speed;
                break;
            case 'L':
                left = -speed;
                right = speed;
                break;
            default:
                left = speed;
                right = -speed;
                break;
        }
    } else {
        error = "unknown step '" + token + "'";
        return false;
    }

    int ms = 0;
    if (pos >= token.size() || token[pos] != ':' || !ReadInt(token, ++pos, ms) ||
        pos != token.size()) {
        error = "missing ':ms' in '" + token + "'";
        return false;
    }
    if (!InRange(ms, kMinStepMs, kMaxStepMs)) {
        error = "duration must be 20..3000 ms in '" + token + "'";
        return false;
    }
    step = {left, right, ms};
    return true;
}

}  // namespace

bool ParseSequence(const std::string& text, int repeat, std::vector<SequenceStep>& steps,
                   std::string& error) {
    steps.clear();
    if (!InRange(repeat, 1, kMaxSequenceRepeat)) {
        error = "repeat must be 1..5";
        return false;
    }

    std::vector<SequenceStep> once;
    std::string token;
    for (size_t i = 0; i <= text.size(); i++) {
        char c = i < text.size() ? text[i] : ',';
        if (c == ',') {
            if (token.empty() && i == text.size() && !once.empty()) {
                break;  // tolerate a trailing comma
            }
            SequenceStep step;
            if (!ParseStep(token, step, error)) {
                return false;
            }
            once.push_back(step);
            token.clear();
        } else if (!std::isspace(static_cast<unsigned char>(c))) {
            token.push_back(c);
        }
    }

    int total_ms = 0;
    for (int r = 0; r < repeat; r++) {
        for (const auto& step : once) {
            if (steps.size() >= kMaxSequenceSteps) {
                error = "too many steps (max 32)";
                steps.clear();
                return false;
            }
            total_ms += step.ms;
            if (total_ms > kMaxSequenceMs) {
                error = "sequence longer than 10 s";
                steps.clear();
                return false;
            }
            steps.push_back(step);
        }
    }
    return true;
}

}  // namespace walle
