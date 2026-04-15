#pragma once

#include <filesystem>
#include <string_view>

#include "common/config/app_config.h"
#include "common/db/mysql_connection.h"
#include "middleware/auth_middleware.h"
#include "modules/notification/notification_service.h"
#include "modules/payment/payment_types.h"

namespace auction::modules::payment {

class PaymentService {
public:
    PaymentService(
        const common::config::AppConfig& config,
        std::filesystem::path project_root,
        middleware::AuthMiddleware& auth_middleware,
        notification::NotificationService& notification_service
    );

    [[nodiscard]] InitiatePaymentResult InitiatePayment(
        std::string_view authorization_header,
        std::uint64_t order_id,
        const InitiatePaymentRequest& request
    );
    [[nodiscard]] PaymentCallbackResult HandlePaymentCallback(
        const PaymentCallbackRequest& request
    );

private:
    [[nodiscard]] common::db::MysqlConnection CreateConnection() const;

    common::config::MysqlConfig mysql_config_;
    std::filesystem::path project_root_;
    std::string callback_secret_;
    middleware::AuthMiddleware* auth_middleware_;
    notification::NotificationService* notification_service_;
};

}  // namespace auction::modules::payment
