#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "modules/order/order_types.h"
#include "modules/payment/payment_types.h"
#include "repository/base_repository.h"

namespace auction::repository {

struct OrderPaymentSnapshot {
    modules::order::OrderRecord order;
    std::uint64_t auction_id{0};
    std::string auction_status;
    std::uint64_t item_id{0};
    std::string item_status;
    std::string item_title;
};

struct PaymentAggregate {
    modules::payment::PaymentRecord payment;
    modules::order::OrderRecord order;
    std::uint64_t auction_id{0};
    std::string auction_status;
    std::uint64_t item_id{0};
    std::string item_status;
    std::string item_title;
};

struct CreatePaymentParams {
    std::string payment_no;
    std::uint64_t order_id{0};
    std::string pay_channel;
    std::string pay_amount;
    std::string pay_status;
};

struct CreatePaymentCallbackLogParams {
    std::optional<std::uint64_t> payment_id;
    std::optional<std::uint64_t> order_id;
    std::string transaction_no;
    std::string callback_status;
    std::string raw_payload;
};

class PaymentRepository : public BaseRepository {
public:
    using BaseRepository::BaseRepository;

    [[nodiscard]] std::optional<OrderPaymentSnapshot> FindOrderPaymentSnapshotByIdForUpdate(
        std::uint64_t order_id
    ) const;
    [[nodiscard]] bool IsOrderPaymentDeadlineExpired(std::uint64_t order_id) const;
    [[nodiscard]] std::optional<modules::payment::PaymentRecord> FindLatestActivePaymentByOrderId(
        std::uint64_t order_id
    ) const;
    [[nodiscard]] modules::payment::PaymentRecord CreatePayment(const CreatePaymentParams& params) const;

    [[nodiscard]] std::optional<PaymentAggregate> FindPaymentAggregateByPaymentNoForUpdate(
        std::string_view payment_no
    ) const;

    void MarkPaymentSuccess(
        std::uint64_t payment_id,
        const std::string& transaction_no,
        const std::string& callback_payload_hash
    ) const;
    void MarkPaymentFailed(
        std::uint64_t payment_id,
        const std::string& transaction_no,
        const std::string& callback_payload_hash
    ) const;
    void UpdateOrderStatusToPaid(std::uint64_t order_id) const;
    void UpdateAuctionStatus(std::uint64_t auction_id, const std::string& status) const;
    void UpdateItemStatus(std::uint64_t item_id, const std::string& item_status) const;

    [[nodiscard]] std::uint64_t CreatePaymentCallbackLog(
        const CreatePaymentCallbackLogParams& params
    ) const;
    void UpdatePaymentCallbackLog(
        std::uint64_t callback_log_id,
        const std::string& callback_status,
        const std::string& process_result
    ) const;
    void InsertOperationLog(
        const std::optional<std::uint64_t>& operator_id,
        const std::string& operator_role,
        const std::string& operation_name,
        const std::string& biz_key,
        const std::string& result,
        const std::string& detail
    ) const;
};

}  // namespace auction::repository
