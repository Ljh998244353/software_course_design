#pragma once

#include <string>

namespace auction {

class PaymentService {
public:
    static PaymentService& instance();

    bool processPaymentCallback(const std::string& order_no,
                              const std::string& txn_id,
                              const std::string& status,
                              const std::string& sign);

private:
    PaymentService() = default;

    bool verifySign(const std::string& order_no,
                   const std::string& txn_id,
                   const std::string& status,
                   const std::string& sign);

    std::string calculateSign(const std::string& order_no,
                              const std::string& txn_id,
                              const std::string& status);

    std::string extractOrderIdFromNo(const std::string& order_no);
    std::string md5(const std::string& input);

    static const std::string SECRET;
};

} // namespace auction
