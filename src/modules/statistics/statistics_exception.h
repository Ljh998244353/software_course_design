#pragma once

#include <stdexcept>
#include <string>

#include "common/errors/error_code.h"

namespace auction::modules::statistics {

class StatisticsException : public std::runtime_error {
public:
    StatisticsException(common::errors::ErrorCode code, std::string message)
        : std::runtime_error(std::move(message)), code_(code) {}

    [[nodiscard]] common::errors::ErrorCode code() const noexcept {
        return code_;
    }

private:
    common::errors::ErrorCode code_;
};

}  // namespace auction::modules::statistics
