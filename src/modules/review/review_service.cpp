#include "modules/review/review_service.h"

#include <algorithm>
#include <cctype>
#include <optional>
#include <stdexcept>
#include <string>

#include "common/errors/error_code.h"
#include "common/logging/logger.h"
#include "modules/auth/auth_types.h"
#include "modules/order/order_types.h"
#include "modules/review/review_exception.h"
#include "repository/review_repository.h"

namespace auction::modules::review {

namespace {

std::string TrimWhitespace(std::string value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) {
        value.erase(value.begin());
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
        value.pop_back();
    }
    return value;
}

void ThrowReviewError(const common::errors::ErrorCode code, const std::string_view message) {
    throw ReviewException(code, std::string(message));
}

repository::ReviewRepository MakeRepository(common::db::MysqlConnection& connection) {
    return repository::ReviewRepository(connection);
}

void InsertOperationLogSafely(
    repository::ReviewRepository& repository,
    const std::optional<std::uint64_t>& operator_id,
    const std::string& operator_role,
    const std::string& operation_name,
    const std::string& biz_key,
    const std::string& result,
    const std::string& detail
) {
    try {
        repository.InsertOperationLog(
            operator_id,
            operator_role,
            operation_name,
            biz_key,
            result,
            detail
        );
    } catch (...) {
    }
}

void CreateStationNoticeSafely(
    notification::NotificationService& notification_service,
    const notification::StationNoticeRequest& request
) {
    try {
        notification_service.CreateStationNotice(request);
    } catch (const std::exception& exception) {
        common::logging::Logger::Instance().Warn(
            "review station notice create failed: " + std::string(exception.what())
        );
    }
}

std::string ResolveReviewType(
    const repository::OrderReviewAggregate& aggregate,
    const std::uint64_t reviewer_id
) {
    if (aggregate.order.buyer_id == reviewer_id) {
        return std::string(kReviewTypeBuyerToSeller);
    }
    if (aggregate.order.seller_id == reviewer_id) {
        return std::string(kReviewTypeSellerToBuyer);
    }
    ThrowReviewError(
        common::errors::ErrorCode::kReviewPermissionDenied,
        "user does not participate in this order"
    );
    return {};
}

std::uint64_t ResolveRevieweeId(
    const repository::OrderReviewAggregate& aggregate,
    const std::uint64_t reviewer_id
) {
    if (aggregate.order.buyer_id == reviewer_id) {
        return aggregate.order.seller_id;
    }
    if (aggregate.order.seller_id == reviewer_id) {
        return aggregate.order.buyer_id;
    }
    ThrowReviewError(
        common::errors::ErrorCode::kReviewPermissionDenied,
        "user does not participate in this order"
    );
    return 0;
}

void ValidateSubmitReviewRequest(const SubmitReviewRequest& request) {
    if (request.rating < kReviewRatingMin || request.rating > kReviewRatingMax) {
        ThrowReviewError(
            common::errors::ErrorCode::kReviewRatingInvalid,
            "review rating must be between 1 and 5"
        );
    }
    if (TrimWhitespace(request.content).size() > kReviewContentMaxLength) {
        ThrowReviewError(
            common::errors::ErrorCode::kReviewContentTooLong,
            "review content must not exceed 500 characters"
        );
    }
}

bool IsReviewableOrderStatus(const std::string_view order_status) {
    return order_status == modules::order::kOrderStatusCompleted ||
           order_status == modules::order::kOrderStatusReviewed;
}

std::string BuildNoticeActor(const std::string_view review_type) {
    if (review_type == kReviewTypeBuyerToSeller) {
        return "buyer";
    }
    return "seller";
}

}  // namespace

ReviewService::ReviewService(
    const common::config::AppConfig& config,
    std::filesystem::path project_root,
    middleware::AuthMiddleware& auth_middleware,
    notification::NotificationService& notification_service
) : mysql_config_(config.mysql),
    project_root_(std::move(project_root)),
    auth_middleware_(&auth_middleware),
    notification_service_(&notification_service) {}

SubmitReviewResult ReviewService::SubmitReview(
    const std::string_view authorization_header,
    const std::uint64_t order_id,
    const SubmitReviewRequest& request
) {
    const auto auth_context = RequireUser(authorization_header);
    ValidateSubmitReviewRequest(request);

    auto connection = CreateConnection();
    auto repository = MakeRepository(connection);
    bool transaction_started = false;
    try {
        connection.BeginTransaction();
        transaction_started = true;

        const auto aggregate = repository.FindOrderReviewAggregateByIdForUpdate(order_id);
        if (!aggregate.has_value()) {
            ThrowReviewError(common::errors::ErrorCode::kOrderNotFound, "order not found");
        }
        if (!IsReviewableOrderStatus(aggregate->order.order_status)) {
            ThrowReviewError(
                common::errors::ErrorCode::kReviewOrderStatusInvalid,
                "order status does not allow review"
            );
        }

        const auto review_type = ResolveReviewType(*aggregate, auth_context.user_id);
        const auto reviewee_id = ResolveRevieweeId(*aggregate, auth_context.user_id);

        if (repository.FindReviewByOrderAndReviewer(order_id, auth_context.user_id).has_value()) {
            ThrowReviewError(
                common::errors::ErrorCode::kReviewDuplicateSubmitted,
                "review already submitted for this order"
            );
        }

        const auto created_review = repository.CreateReview(repository::CreateReviewParams{
            .order_id = order_id,
            .reviewer_id = auth_context.user_id,
            .reviewee_id = reviewee_id,
            .review_type = review_type,
            .rating = request.rating,
            .content = TrimWhitespace(request.content),
        });
        const auto review_count = repository.CountReviewsByOrder(order_id);
        const auto order_marked_reviewed =
            review_count >= 2 &&
            aggregate->order.order_status != modules::order::kOrderStatusReviewed;
        if (order_marked_reviewed) {
            repository.UpdateOrderStatus(
                order_id,
                std::string(modules::order::kOrderStatusReviewed)
            );
        }

        repository.InsertOperationLog(
            auth_context.user_id,
            auth_context.role_code,
            "submit_review",
            std::to_string(order_id),
            "SUCCESS",
            "review_id=" + std::to_string(created_review.review_id)
        );
        connection.Commit();
        transaction_started = false;

        CreateStationNoticeSafely(
            *notification_service_,
            notification::StationNoticeRequest{
                .user_id = reviewee_id,
                .notice_type = "ORDER_REVIEW_RECEIVED",
                .title = "New order review received",
                .content = "Order " + aggregate->order.order_no + " received a " +
                           std::to_string(request.rating) + "-star review from the " +
                           BuildNoticeActor(review_type) + ".",
                .biz_type = "ORDER",
                .biz_id = order_id,
            }
        );

        return SubmitReviewResult{
            .review = created_review,
            .order_status_after = order_marked_reviewed
                                      ? std::string(modules::order::kOrderStatusReviewed)
                                      : aggregate->order.order_status,
            .order_marked_reviewed = order_marked_reviewed,
        };
    } catch (...) {
        if (transaction_started) {
            connection.Rollback();
        }
        InsertOperationLogSafely(
            repository,
            auth_context.user_id,
            auth_context.role_code,
            "submit_review",
            std::to_string(order_id),
            "FAILED",
            "review submit failed"
        );
        throw;
    }
}

OrderReviewListResult ReviewService::ListOrderReviews(const std::uint64_t order_id) {
    auto connection = CreateConnection();
    auto repository = MakeRepository(connection);
    const auto aggregate = repository.FindOrderReviewAggregateById(order_id);
    if (!aggregate.has_value()) {
        ThrowReviewError(common::errors::ErrorCode::kOrderNotFound, "order not found");
    }

    return OrderReviewListResult{
        .order_id = order_id,
        .order_status = aggregate->order.order_status,
        .reviews = repository.ListReviewsByOrder(order_id),
    };
}

ReviewSummary ReviewService::GetUserReviewSummary(const std::uint64_t user_id) {
    auto connection = CreateConnection();
    auto repository = MakeRepository(connection);
    return repository.GetReviewSummaryByReviewee(user_id);
}

common::db::MysqlConnection ReviewService::CreateConnection() const {
    return common::db::MysqlConnection(mysql_config_, project_root_);
}

modules::auth::AuthContext ReviewService::RequireUser(
    const std::string_view authorization_header
) const {
    return auth_middleware_->RequireAnyRole(
        authorization_header,
        {modules::auth::kRoleUser}
    );
}

}  // namespace auction::modules::review
