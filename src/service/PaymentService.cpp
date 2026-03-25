#include "PaymentService.h"
#include "MySQLPool.h"
#include "OrderService.h"
#include "Logger.h"
#include <openssl/evp.h>
#include <sstream>
#include <iomanip>

namespace auction {

const std::string PaymentService::SECRET = "your-secret-key";

PaymentService& PaymentService::instance() {
    static PaymentService instance;
    return instance;
}

bool PaymentService::processPaymentCallback(const std::string& order_no,
                                          const std::string& txn_id,
                                          const std::string& status,
                                          const std::string& sign) {
    if (!verifySign(order_no, txn_id, status, sign)) {
        AUCTION_LOG_ERROR("Payment callback verification failed");
        return false;
    }

    AUCTION_LOG_INFO("Processing payment callback for order: %s", order_no.c_str());

    if (status == "success") {
        return true;
    }

    return true;
}

bool PaymentService::verifySign(const std::string& order_no,
                              const std::string& txn_id,
                              const std::string& status,
                              const std::string& sign) {
    std::string expected_sign = calculateSign(order_no, txn_id, status);
    return expected_sign == sign;
}

std::string PaymentService::calculateSign(const std::string& order_no,
                                        const std::string& txn_id,
                                        const std::string& status) {
    std::string data = order_no + "|" + txn_id + "|" + status + "|" + SECRET;
    return md5(data);
}

std::string PaymentService::extractOrderIdFromNo(const std::string& order_no) {
    return order_no.substr(3);
}

std::string PaymentService::md5(const std::string& input) {
    unsigned char digest[16];
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_md5(), nullptr);
    EVP_DigestUpdate(ctx, input.c_str(), input.length());
    EVP_DigestFinal_ex(ctx, digest, nullptr);
    EVP_MD_CTX_free(ctx);

    char hex_str[33];
    for (int i = 0; i < 16; ++i) {
        sprintf(hex_str + i * 2, "%02x", digest[i]);
    }
    hex_str[32] = '\0';

    return std::string(hex_str);
}

} // namespace auction
