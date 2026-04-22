#pragma once

#include <filesystem>
#include <string_view>

#include "common/config/app_config.h"
#include "common/db/mysql_connection.h"
#include "middleware/auth_middleware.h"
#include "modules/notification/notification_service.h"
#include "modules/ops/ops_types.h"
#include "modules/order/order_service.h"
#include "modules/statistics/statistics_service.h"

namespace auction::modules::ops {

class OpsService {
public:
    OpsService(
        const common::config::AppConfig& config,
        std::filesystem::path project_root,
        middleware::AuthMiddleware& auth_middleware,
        notification::NotificationService& notification_service,
        order::OrderService& order_service,
        statistics::StatisticsService& statistics_service
    );

    [[nodiscard]] OperationLogQueryResult ListOperationLogs(
        std::string_view authorization_header,
        const OperationLogQuery& query
    );
    [[nodiscard]] TaskLogQueryResult ListTaskLogs(
        std::string_view authorization_header,
        const TaskLogQuery& query
    );
    [[nodiscard]] SystemExceptionQueryResult ListSystemExceptions(
        std::string_view authorization_header,
        const SystemExceptionQuery& query
    );
    [[nodiscard]] MarkExceptionResult MarkException(
        std::string_view authorization_header,
        const MarkExceptionRequest& request
    );
    [[nodiscard]] notification::NotificationRetryResult RetryFailedNotifications(
        std::string_view authorization_header,
        const notification::NotificationRetryRequest& request
    );
    [[nodiscard]] CompensationResult RunCompensation(
        std::string_view authorization_header,
        const CompensationRequest& request
    );

private:
    [[nodiscard]] common::db::MysqlConnection CreateConnection() const;

    common::config::MysqlConfig mysql_config_;
    std::filesystem::path project_root_;
    middleware::AuthMiddleware* auth_middleware_;
    notification::NotificationService* notification_service_;
    order::OrderService* order_service_;
    statistics::StatisticsService* statistics_service_;
};

}  // namespace auction::modules::ops
