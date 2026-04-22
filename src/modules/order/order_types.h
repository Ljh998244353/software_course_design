#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace auction::modules::order {

inline constexpr std::string_view kOrderStatusPendingPayment = "PENDING_PAYMENT";
inline constexpr std::string_view kOrderStatusPaid = "PAID";
inline constexpr std::string_view kOrderStatusWaitingDelivery = "WAITING_DELIVERY";
inline constexpr std::string_view kOrderStatusShipped = "SHIPPED";
inline constexpr std::string_view kOrderStatusCompleted = "COMPLETED";
inline constexpr std::string_view kOrderStatusReviewed = "REVIEWED";
inline constexpr std::string_view kOrderStatusClosed = "CLOSED";

inline constexpr std::string_view kTaskTypeOrderSettlement = "ORDER_SETTLEMENT";
inline constexpr std::string_view kTaskTypeOrderTimeoutClose = "ORDER_TIMEOUT_CLOSE";
inline constexpr std::string_view kTaskStatusSuccess = "SUCCESS";
inline constexpr std::string_view kTaskStatusFailed = "FAILED";
inline constexpr std::string_view kTaskStatusSkipped = "SKIPPED";

inline constexpr int kDefaultPayTimeoutMinutes = 30;

inline bool IsKnownOrderStatus(const std::string_view status) {
    return status == kOrderStatusPendingPayment || status == kOrderStatusPaid ||
           status == kOrderStatusWaitingDelivery || status == kOrderStatusShipped ||
           status == kOrderStatusCompleted || status == kOrderStatusReviewed ||
           status == kOrderStatusClosed;
}

struct OrderRecord {
    std::uint64_t order_id{0};
    std::string order_no;
    std::uint64_t auction_id{0};
    std::uint64_t buyer_id{0};
    std::uint64_t seller_id{0};
    double final_amount{0.0};
    std::string order_status;
    std::string pay_deadline_at;
    std::string paid_at;
    std::string shipped_at;
    std::string completed_at;
    std::string closed_at;
    std::string created_at;
    std::string updated_at;
};

struct OrderScheduleResult {
    int scanned{0};
    int succeeded{0};
    int failed{0};
    int skipped{0};
    std::vector<std::uint64_t> affected_order_ids;
};

struct OrderTransitionResult {
    std::uint64_t order_id{0};
    std::string old_status;
    std::string new_status;
};

}  // namespace auction::modules::order
