#include "modules/audit/item_audit_service.h"

#include <cctype>
#include <stdexcept>

#include "common/db/mysql_connection.h"
#include "common/errors/error_code.h"
#include "modules/auth/auth_types.h"
#include "modules/item/item_exception.h"
#include "repository/item_repository.h"

namespace auction::modules::audit {

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

void ThrowItemError(const common::errors::ErrorCode code, const std::string_view message) {
    throw modules::item::ItemException(code, std::string(message));
}

modules::item::ItemRecord LoadItemOrThrow(
    repository::ItemRepository& repository,
    const std::uint64_t item_id
) {
    const auto item = repository.FindItemById(item_id);
    if (!item.has_value()) {
        ThrowItemError(common::errors::ErrorCode::kItemNotFound, "item not found");
    }
    return *item;
}

}  // namespace

ItemAuditService::ItemAuditService(
    const common::config::AppConfig& config,
    std::filesystem::path project_root,
    middleware::AuthMiddleware& auth_middleware
) : mysql_config_(config.mysql),
    project_root_(std::move(project_root)),
    auth_middleware_(&auth_middleware) {}

std::vector<modules::item::PendingAuditItemSummary> ItemAuditService::ListPendingItems(
    const std::string_view authorization_header
) {
    const auto ignored = auth_middleware_->RequireAnyRole(
        authorization_header,
        {modules::auth::kRoleAdmin}
    );
    (void)ignored;

    auto connection = CreateConnection();
    repository::ItemRepository repository(connection);
    return repository.ListPendingAuditItems();
}

modules::item::AuditItemResult ItemAuditService::AuditItem(
    const std::string_view authorization_header,
    const std::uint64_t item_id,
    const modules::item::AuditItemRequest& request
) {
    const auto admin_context = auth_middleware_->RequireAnyRole(
        authorization_header,
        {modules::auth::kRoleAdmin}
    );

    const auto audit_result = TrimWhitespace(request.audit_result);
    const auto reason = TrimWhitespace(request.reason);
    if (!modules::item::IsValidAuditResult(audit_result)) {
        ThrowItemError(
            common::errors::ErrorCode::kItemAuditResultInvalid,
            "audit result is invalid"
        );
    }
    if (audit_result == modules::item::kAuditResultRejected && reason.empty()) {
        ThrowItemError(
            common::errors::ErrorCode::kItemAuditReasonRequired,
            "audit reject reason is required"
        );
    }

    auto connection = CreateConnection();
    repository::ItemRepository repository(connection);
    const auto item = LoadItemOrThrow(repository, item_id);
    if (!modules::item::IsValidAuditItemStatus(item.item_status)) {
        ThrowItemError(
            common::errors::ErrorCode::kItemAuditStatusInvalid,
            "item status does not allow audit"
        );
    }

    const std::string next_status = audit_result == modules::item::kAuditResultApproved
                                        ? std::string(modules::item::kItemStatusReadyForAuction)
                                        : std::string(modules::item::kItemStatusRejected);

    connection.BeginTransaction();
    try {
        repository.UpdateItemStatus(
            item_id,
            next_status,
            audit_result == modules::item::kAuditResultRejected ? reason : ""
        );
        repository.InsertAuditLog(repository::CreateItemAuditLogParams{
            .item_id = item_id,
            .admin_id = admin_context.user_id,
            .audit_result = audit_result,
            .audit_comment = reason,
        });
        connection.Commit();
    } catch (...) {
        connection.Rollback();
        throw;
    }

    const auto updated = LoadItemOrThrow(repository, item_id);
    return modules::item::AuditItemResult{
        .item_id = item_id,
        .old_status = item.item_status,
        .new_status = updated.item_status,
        .audit_result = audit_result,
        .audited_at = updated.updated_at,
    };
}

std::vector<modules::item::ItemAuditLogRecord> ItemAuditService::GetAuditLogs(
    const std::string_view authorization_header,
    const std::uint64_t item_id
) {
    const auto ignored = auth_middleware_->RequireAnyRole(
        authorization_header,
        {modules::auth::kRoleAdmin, modules::auth::kRoleSupport}
    );
    (void)ignored;

    auto connection = CreateConnection();
    repository::ItemRepository repository(connection);
    const auto item = LoadItemOrThrow(repository, item_id);
    (void)item;
    return repository.ListAuditLogs(item_id);
}

common::db::MysqlConnection ItemAuditService::CreateConnection() const {
    return common::db::MysqlConnection(mysql_config_, project_root_);
}

}  // namespace auction::modules::audit
