#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "common/config/app_config.h"
#include "common/db/mysql_connection.h"
#include "middleware/auth_middleware.h"
#include "modules/item/item_types.h"

namespace auction::modules::item {

class ItemService {
public:
    ItemService(
        const common::config::AppConfig& config,
        std::filesystem::path project_root,
        middleware::AuthMiddleware& auth_middleware
    );

    [[nodiscard]] CreateItemResult CreateDraftItem(
        std::string_view authorization_header,
        const CreateItemRequest& request
    );
    [[nodiscard]] UpdateItemResult UpdateItem(
        std::string_view authorization_header,
        std::uint64_t item_id,
        const UpdateItemRequest& request
    );
    [[nodiscard]] ItemImageRecord AddItemImage(
        std::string_view authorization_header,
        std::uint64_t item_id,
        const AddItemImageRequest& request
    );
    [[nodiscard]] DeleteItemImageResult DeleteItemImage(
        std::string_view authorization_header,
        std::uint64_t item_id,
        std::uint64_t image_id
    );
    [[nodiscard]] SubmitAuditResult SubmitForAudit(
        std::string_view authorization_header,
        std::uint64_t item_id
    );
    [[nodiscard]] std::vector<ItemSummary> ListMyItems(
        std::string_view authorization_header,
        const std::optional<std::string>& item_status
    );
    [[nodiscard]] ItemDetail GetItemDetail(
        std::string_view authorization_header,
        std::uint64_t item_id
    );

private:
    [[nodiscard]] common::db::MysqlConnection CreateConnection() const;
    [[nodiscard]] modules::auth::AuthContext RequireSeller(
        std::string_view authorization_header
    ) const;

    common::config::MysqlConfig mysql_config_;
    std::filesystem::path project_root_;
    middleware::AuthMiddleware* auth_middleware_;
};

}  // namespace auction::modules::item
