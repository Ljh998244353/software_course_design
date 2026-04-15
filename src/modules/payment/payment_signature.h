#pragma once

#include <string>
#include <string_view>

#include "modules/payment/payment_types.h"

namespace auction::modules::payment {

[[nodiscard]] std::string FormatPaymentAmount(double amount);
[[nodiscard]] std::string SignMockPaymentCallback(
    std::string_view secret,
    const PaymentCallbackRequest& request
);
[[nodiscard]] bool VerifyMockPaymentCallbackSignature(
    std::string_view secret,
    const PaymentCallbackRequest& request
);
[[nodiscard]] std::string Sha256Hex(std::string_view input);

}  // namespace auction::modules::payment
