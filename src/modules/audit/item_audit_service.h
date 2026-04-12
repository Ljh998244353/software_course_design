#pragma once

#include <filesystem>
#include <string_view>
#include <vector>

#include "common/config/app_config.h"
#include "common/db/mysql_connection.h"
#include "middleware/auth_middleware.h"
#include "modules/item/item_types.h"

namespace auction::modules::audit {

class ItemAuditService {
public:
    ItemAuditService(
        const common::config::AppConfig& config,
        std::filesystem::path project_root,
        middleware::AuthMiddleware& auth_middleware
    );

    [[nodiscard]] std::vector<modules::item::PendingAuditItemSummary> ListPendingItems(
        std::string_view authorization_header
    );
    [[nodiscard]] modules::item::AuditItemResult AuditItem(
        std::string_view authorization_header,
        std::uint64_t item_id,
        const modules::item::AuditItemRequest& request
    );
    [[nodiscard]] std::vector<modules::item::ItemAuditLogRecord> GetAuditLogs(
        std::string_view authorization_header,
        std::uint64_t item_id
    );

private:
    [[nodiscard]] common::db::MysqlConnection CreateConnection() const;

    common::config::MysqlConfig mysql_config_;
    std::filesystem::path project_root_;
    middleware::AuthMiddleware* auth_middleware_;
};

}  // namespace auction::modules::audit
