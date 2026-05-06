#pragma once

#include <filesystem>
#include <string_view>

#include "common/config/app_config.h"
#include "common/db/mysql_connection.h"
#include "middleware/auth_middleware.h"
#include "modules/notification/notification_service.h"
#include "modules/order/order_types.h"

namespace auction::modules::order {

class OrderService {
public:
    OrderService(
        const common::config::AppConfig& config,
        std::filesystem::path project_root,
        middleware::AuthMiddleware& auth_middleware,
        notification::NotificationService& notification_service
    );

    [[nodiscard]] OrderScheduleResult GenerateSettlementOrders();
    [[nodiscard]] OrderScheduleResult CloseExpiredOrders();
    [[nodiscard]] UserOrderListResult ListMyOrders(
        std::string_view authorization_header,
        const UserOrderQuery& query
    );
    [[nodiscard]] UserOrderEntry GetMyOrder(
        std::string_view authorization_header,
        std::uint64_t order_id
    );
    [[nodiscard]] OrderTransitionResult ShipOrder(
        std::string_view authorization_header,
        std::uint64_t order_id
    );
    [[nodiscard]] OrderTransitionResult ConfirmReceipt(
        std::string_view authorization_header,
        std::uint64_t order_id
    );

private:
    [[nodiscard]] common::db::MysqlConnection CreateConnection() const;

    common::config::MysqlConfig mysql_config_;
    std::filesystem::path project_root_;
    middleware::AuthMiddleware* auth_middleware_;
    notification::NotificationService* notification_service_;
};

}  // namespace auction::modules::order
