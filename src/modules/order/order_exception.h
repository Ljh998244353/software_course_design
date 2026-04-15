#pragma once

#include <stdexcept>
#include <string>

#include "common/errors/error_code.h"

namespace auction::modules::order {

class OrderException : public std::runtime_error {
public:
    OrderException(common::errors::ErrorCode code, std::string message)
        : std::runtime_error(std::move(message)), code_(code) {}

    [[nodiscard]] common::errors::ErrorCode code() const noexcept {
        return code_;
    }

private:
    common::errors::ErrorCode code_;
};

}  // namespace auction::modules::order
