#ifndef WALLE_HEALTH_H
#define WALLE_HEALTH_H

#include <cstdint>
#include <string>

// Lifetime reset counters, reported to the server with every OTA check so
// brown-outs and crashes show up in the console instead of passing silently.
// Kept free of ESP-IDF so it can be unit-tested on the host.
namespace walle {

struct HealthCounters {
    int32_t boots = 0;
    int32_t brownouts = 0;
    int32_t panics = 0;
    int32_t watchdogs = 0;
};

// Counts one boot whose reset reason is `reason`: "brownout", "panic",
// "task_wdt", "int_wdt", "wdt", or anything else for an ordinary reset.
void CountBoot(HealthCounters& counters, const std::string& reason);

// Adds "health":{...} to the board JSON object, before its closing brace.
// Returns `board_json` unchanged if it isn't a JSON object.
std::string AppendHealthJson(const std::string& board_json, const HealthCounters& counters,
                             const std::string& reason);

}  // namespace walle

#endif  // WALLE_HEALTH_H
