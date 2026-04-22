#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "modules/order/order_types.h"
#include "modules/review/review_types.h"
#include "repository/base_repository.h"

namespace auction::repository {

struct OrderReviewAggregate {
    modules::order::OrderRecord order;
    std::string item_title;
};

struct CreateReviewParams {
    std::uint64_t order_id{0};
    std::uint64_t reviewer_id{0};
    std::uint64_t reviewee_id{0};
    std::string review_type;
    int rating{0};
    std::string content;
};

class ReviewRepository : public BaseRepository {
public:
    using BaseRepository::BaseRepository;

    [[nodiscard]] std::optional<OrderReviewAggregate> FindOrderReviewAggregateById(
        std::uint64_t order_id
    ) const;
    [[nodiscard]] std::optional<OrderReviewAggregate> FindOrderReviewAggregateByIdForUpdate(
        std::uint64_t order_id
    ) const;

    [[nodiscard]] std::optional<modules::review::ReviewRecord> FindReviewByOrderAndReviewer(
        std::uint64_t order_id,
        std::uint64_t reviewer_id
    ) const;
    [[nodiscard]] modules::review::ReviewRecord CreateReview(const CreateReviewParams& params) const;
    [[nodiscard]] std::vector<modules::review::ReviewRecord> ListReviewsByOrder(
        std::uint64_t order_id
    ) const;
    [[nodiscard]] std::uint64_t CountReviewsByOrder(std::uint64_t order_id) const;
    [[nodiscard]] modules::review::ReviewSummary GetReviewSummaryByReviewee(
        std::uint64_t user_id
    ) const;

    void UpdateOrderStatus(std::uint64_t order_id, const std::string& order_status) const;
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
