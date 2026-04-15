#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace auction::modules::payment {

inline constexpr std::string_view kPaymentChannelMockAlipay = "MOCK_ALIPAY";
inline constexpr std::string_view kPaymentChannelMockWechat = "MOCK_WECHAT";

inline constexpr std::string_view kPaymentStatusInitiated = "INITIATED";
inline constexpr std::string_view kPaymentStatusWaitingCallback = "WAITING_CALLBACK";
inline constexpr std::string_view kPaymentStatusSuccess = "SUCCESS";
inline constexpr std::string_view kPaymentStatusFailed = "FAILED";
inline constexpr std::string_view kPaymentStatusClosed = "CLOSED";

inline constexpr std::string_view kCallbackStatusReceived = "RECEIVED";
inline constexpr std::string_view kCallbackStatusVerified = "VERIFIED";
inline constexpr std::string_view kCallbackStatusRejected = "REJECTED";
inline constexpr std::string_view kCallbackStatusProcessed = "PROCESSED";

inline constexpr std::string_view kDefaultMerchantNo = "COURSE_AUCTION";

inline bool IsSupportedPaymentChannel(const std::string_view channel) {
    return channel == kPaymentChannelMockAlipay || channel == kPaymentChannelMockWechat;
}

inline bool IsSupportedCallbackResult(const std::string_view callback_result) {
    return callback_result == kPaymentStatusSuccess || callback_result == kPaymentStatusFailed;
}

struct PaymentRecord {
    std::uint64_t payment_id{0};
    std::string payment_no;
    std::uint64_t order_id{0};
    std::string pay_channel;
    std::string transaction_no;
    double pay_amount{0.0};
    std::string pay_status;
    std::string callback_payload_hash;
    std::string paid_at;
    std::string created_at;
    std::string updated_at;
};

struct InitiatePaymentRequest {
    std::string pay_channel;
};

struct InitiatePaymentResult {
    std::uint64_t payment_id{0};
    std::uint64_t order_id{0};
    std::string order_no;
    std::string payment_no;
    std::string pay_channel;
    double pay_amount{0.0};
    std::string pay_status;
    std::string merchant_no;
    std::string pay_url;
    std::string expire_at;
    bool reused_existing{false};
};

struct PaymentCallbackRequest {
    std::string payment_no;
    std::string order_no;
    std::string merchant_no;
    std::string transaction_no;
    std::string pay_status;
    double pay_amount{0.0};
    std::string signature;
};

struct PaymentCallbackResult {
    std::uint64_t payment_id{0};
    std::uint64_t order_id{0};
    std::string order_status;
    std::string pay_status;
    bool idempotent{false};
    std::string processed_at;
};

}  // namespace auction::modules::payment
