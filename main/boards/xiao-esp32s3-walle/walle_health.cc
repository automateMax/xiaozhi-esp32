#include "walle_health.h"

namespace walle {

void CountBoot(HealthCounters& counters, const std::string& reason) {
    counters.boots++;
    if (reason == "brownout") {
        counters.brownouts++;
    } else if (reason == "panic") {
        counters.panics++;
    } else if (reason == "task_wdt" || reason == "int_wdt" || reason == "wdt") {
        counters.watchdogs++;
    }
}

std::string AppendHealthJson(const std::string& board_json, const HealthCounters& counters,
                             const std::string& reason) {
    size_t end = board_json.find_last_of('}');
    if (board_json.empty() || board_json.front() != '{' || end == std::string::npos) {
        return board_json;
    }
    std::string health = R"("health":{"reset_reason":")" + reason + R"(","boots":)" +
                         std::to_string(counters.boots) + R"(,"brownouts":)" +
                         std::to_string(counters.brownouts) + R"(,"panics":)" +
                         std::to_string(counters.panics) + R"(,"watchdogs":)" +
                         std::to_string(counters.watchdogs) + "}";
    std::string head = board_json.substr(0, end);
    bool empty_object = head.find_first_not_of(" \t\r\n", 1) == std::string::npos;
    return head + (empty_object ? "" : ",") + health + board_json.substr(end);
}

}  // namespace walle
