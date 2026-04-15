#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "modules/order/order_types.h"
#include "repository/base_repository.h"

namespace auction::repository {

struct SettlementAuctionAggregate {
    std::uint64_t auction_id{0};
    std::uint64_t item_id{0};
    std::uint64_t seller_id{0};
    std::optional<std::uint64_t> highest_bidder_id;
    double current_price{0.0};
    std::string status;
    std::string end_time;
    std::string item_status;
    std::string item_title;
};

struct ExpiredOrderAggregate {
    modules::order::OrderRecord order;
    std::uint64_t auction_id{0};
    std::string auction_status;
    std::uint64_t item_id{0};
    std::string item_status;
    std::string item_title;
};

struct CreateOrderParams {
    std::string order_no;
    std::uint64_t auction_id{0};
    std::uint64_t buyer_id{0};
    std::uint64_t seller_id{0};
    std::string final_amount;
    std::string order_status;
    std::string pay_deadline_at;
};

struct OrderTaskLogParams {
    std::string task_type;
    std::string biz_key;
    std::string task_status;
    int retry_count{0};
    std::string last_error;
    std::string scheduled_at;
};

class OrderRepository : public BaseRepository {
public:
    using BaseRepository::BaseRepository;

    [[nodiscard]] std::vector<std::uint64_t> ListSettlingAuctionIds() const;
    [[nodiscard]] std::optional<SettlementAuctionAggregate> FindSettlementAuctionById(
        std::uint64_t auction_id
    ) const;
    [[nodiscard]] std::optional<SettlementAuctionAggregate> FindSettlementAuctionByIdForUpdate(
        std::uint64_t auction_id
    ) const;

    [[nodiscard]] std::optional<modules::order::OrderRecord> FindOrderByAuctionId(
        std::uint64_t auction_id
    ) const;
    [[nodiscard]] modules::order::OrderRecord CreateOrder(const CreateOrderParams& params) const;

    [[nodiscard]] std::vector<std::uint64_t> ListExpiredPendingOrderIds() const;
    [[nodiscard]] std::optional<ExpiredOrderAggregate> FindExpiredOrderAggregateByIdForUpdate(
        std::uint64_t order_id
    ) const;
    [[nodiscard]] bool IsOrderPaymentDeadlineExpired(std::uint64_t order_id) const;

    void UpdateOrderStatusToClosed(std::uint64_t order_id) const;
    void CloseOpenPaymentsByOrder(std::uint64_t order_id) const;
    void UpdateAuctionStatus(std::uint64_t auction_id, const std::string& status) const;
    void UpdateItemStatus(std::uint64_t item_id, const std::string& item_status) const;

    void InsertTaskLog(const OrderTaskLogParams& params) const;
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
