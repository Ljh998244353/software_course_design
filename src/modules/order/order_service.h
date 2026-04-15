#pragma once

#include <filesystem>

#include "common/config/app_config.h"
#include "common/db/mysql_connection.h"
#include "modules/notification/notification_service.h"
#include "modules/order/order_types.h"

namespace auction::modules::order {

class OrderService {
public:
    OrderService(
        const common::config::AppConfig& config,
        std::filesystem::path project_root,
        notification::NotificationService& notification_service
    );

    [[nodiscard]] OrderScheduleResult GenerateSettlementOrders();
    [[nodiscard]] OrderScheduleResult CloseExpiredOrders();

private:
    [[nodiscard]] common::db::MysqlConnection CreateConnection() const;

    common::config::MysqlConfig mysql_config_;
    std::filesystem::path project_root_;
    notification::NotificationService* notification_service_;
};

}  // namespace auction::modules::order
