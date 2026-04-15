#include "modules/payment/payment_signature.h"

#include <array>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

namespace auction::modules::payment {

namespace {

std::string BuildSignaturePayload(const PaymentCallbackRequest& request) {
    return request.merchant_no + "|" + request.order_no + "|" + request.payment_no + "|" +
           request.transaction_no + "|" + request.pay_status + "|" +
           FormatPaymentAmount(request.pay_amount);
}

std::string BytesToHex(const unsigned char* input, const std::size_t length) {
    std::ostringstream output;
    output << std::hex << std::setfill('0');
    for (std::size_t index = 0; index < length; ++index) {
        output << std::setw(2) << static_cast<int>(input[index]);
    }
    return output.str();
}

std::string HmacSha256Hex(std::string_view secret, std::string_view payload) {
    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    unsigned int digest_length = 0;

    const unsigned char* signed_result = HMAC(
        EVP_sha256(),
        secret.data(),
        static_cast<int>(secret.size()),
        reinterpret_cast<const unsigned char*>(payload.data()),
        payload.size(),
        digest.data(),
        &digest_length
    );
    if (signed_result == nullptr) {
        throw std::runtime_error("failed to calculate payment signature");
    }

    return BytesToHex(digest.data(), digest_length);
}

bool ConstantTimeEquals(std::string_view left, std::string_view right) {
    if (left.size() != right.size()) {
        return false;
    }
    return CRYPTO_memcmp(left.data(), right.data(), left.size()) == 0;
}

}  // namespace

std::string FormatPaymentAmount(const double amount) {
    std::ostringstream output;
    output << std::fixed << std::setprecision(2) << (std::round(amount * 100.0) / 100.0);
    return output.str();
}

std::string SignMockPaymentCallback(
    const std::string_view secret,
    const PaymentCallbackRequest& request
) {
    return HmacSha256Hex(secret, BuildSignaturePayload(request));
}

bool VerifyMockPaymentCallbackSignature(
    const std::string_view secret,
    const PaymentCallbackRequest& request
) {
    return ConstantTimeEquals(request.signature, SignMockPaymentCallback(secret, request));
}

std::string Sha256Hex(const std::string_view input) {
    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    unsigned int digest_length = 0;

    EVP_MD_CTX* context = EVP_MD_CTX_new();
    if (context == nullptr) {
        throw std::runtime_error("failed to allocate sha256 context");
    }

    if (EVP_DigestInit_ex(context, EVP_sha256(), nullptr) != 1 ||
        EVP_DigestUpdate(context, input.data(), input.size()) != 1 ||
        EVP_DigestFinal_ex(context, digest.data(), &digest_length) != 1) {
        EVP_MD_CTX_free(context);
        throw std::runtime_error("failed to calculate sha256");
    }

    EVP_MD_CTX_free(context);
    return BytesToHex(digest.data(), digest_length);
}

}  // namespace auction::modules::payment
