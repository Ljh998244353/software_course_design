#include "AntiSnipe.h"
#include "ConfigManager.h"

namespace auction {

AntiSnipe::AntiSnipe()
    : N_(ConfigManager::instance().auction().anti_snipe_N),
      M_(ConfigManager::instance().auction().anti_snipe_M),
      max_extensions_(ConfigManager::instance().auction().max_extensions) {
}

bool AntiSnipe::shouldExtend(int64_t remaining_time_ms, int current_extensions) const {
    if (remaining_time_ms <= 0) return false;
    if (current_extensions >= max_extensions_) return false;

    auto remaining_seconds = remaining_time_ms / 1000;
    return remaining_seconds <= N_;
}

int64_t AntiSnipe::calculateNewEndTime(int64_t current_end_time, int current_extensions) {
    if (current_extensions >= max_extensions_) {
        return current_end_time;
    }
    return current_end_time + M_ * 1000;
}

} // namespace auction
