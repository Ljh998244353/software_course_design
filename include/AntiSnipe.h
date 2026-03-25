#pragma once

#include <cstdint>

namespace auction {

class AntiSnipe {
public:
    AntiSnipe();

    bool shouldExtend(int64_t remaining_time_ms, int current_extensions) const;
    int64_t calculateNewEndTime(int64_t current_end_time, int current_extensions);

    int getN() const { return N_; }
    int getM() const { return M_; }
    int getMaxExtensions() const { return max_extensions_; }

private:
    int N_;
    int M_;
    int max_extensions_;
};

} // namespace auction
