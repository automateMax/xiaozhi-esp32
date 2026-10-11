// Deterministic stand-in for ESP-IDF's esp_random() in host tests.
#pragma once

#include <cstdint>

inline uint32_t g_random_state = 2463534242u;

inline uint32_t esp_random() {
    g_random_state ^= g_random_state << 13;
    g_random_state ^= g_random_state >> 17;
    g_random_state ^= g_random_state << 5;
    return g_random_state;
}
