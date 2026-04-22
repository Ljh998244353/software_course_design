#pragma once

#include <filesystem>
#include <string_view>

#include "common/config/app_config.h"
#include "common/db/mysql_connection.h"
#include "middleware/auth_middleware.h"
#include "modules/notification/notification_service.h"
#include "modules/review/review_types.h"

namespace auction::modules::review {

class ReviewService {
public:
    ReviewService(
        const common::config::AppConfig& config,
        std::filesystem::path project_root,
        middleware::AuthMiddleware& auth_middleware,
        notification::NotificationService& notification_service
    );

    [[nodiscard]] SubmitReviewResult SubmitReview(
        std::string_view authorization_header,
        std::uint64_t order_id,
        const SubmitReviewRequest& request
    );
    [[nodiscard]] OrderReviewListResult ListOrderReviews(std::uint64_t order_id);
    [[nodiscard]] ReviewSummary GetUserReviewSummary(std::uint64_t user_id);

private:
    [[nodiscard]] common::db::MysqlConnection CreateConnection() const;
    [[nodiscard]] modules::auth::AuthContext RequireUser(
        std::string_view authorization_header
    ) const;

    common::config::MysqlConfig mysql_config_;
    std::filesystem::path project_root_;
    middleware::AuthMiddleware* auth_middleware_;
    notification::NotificationService* notification_service_;
};

}  // namespace auction::modules::review
