#include "access/http/audit_http.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "common/http/http_utils.h"
#include "modules/item/item_types.h"

#if AUCTION_HAS_DROGON
#include <drogon/drogon.h>
#endif

namespace auction::access::http {

namespace {

#if AUCTION_HAS_DROGON

std::uint64_t ParsePositiveId(const std::string& raw_value, const std::string& field_name) {
    std::size_t parsed_length = 0;
    const auto value = std::stoull(raw_value, &parsed_length);
    if (parsed_length != raw_value.size() || value == 0) {
        throw std::invalid_argument(field_name + " must be a positive integer");
    }
    return value;
}

Json::Value ToPendingAuditItemJson(const modules::item::PendingAuditItemSummary& item) {
    Json::Value json(Json::objectValue);
    json["itemId"] = static_cast<Json::UInt64>(item.item_id);
    json["sellerId"] = static_cast<Json::UInt64>(item.seller_id);
    json["sellerUsername"] = item.seller_username;
    json["sellerNickname"] = item.seller_nickname;
    json["categoryId"] = static_cast<Json::UInt64>(item.category_id);
    json["title"] = item.title;
    json["startPrice"] = item.start_price;
    json["itemStatus"] = std::string(modules::item::kItemStatusPendingAudit);
    json["createdAt"] = item.created_at;
    return json;
}

Json::Value ToAuditLogJson(const modules::item::ItemAuditLogRecord& log) {
    Json::Value json(Json::objectValue);
    json["auditLogId"] = static_cast<Json::UInt64>(log.audit_log_id);
    json["itemId"] = static_cast<Json::UInt64>(log.item_id);
    json["adminId"] = static_cast<Json::UInt64>(log.admin_id);
    json["adminUsername"] = log.admin_username;
    json["auditResult"] = log.audit_result;
    json["auditComment"] = log.audit_comment;
    json["createdAt"] = log.created_at;
    return json;
}

Json::Value ToPendingAuditListJson(
    const std::vector<modules::item::PendingAuditItemSummary>& items
) {
    Json::Value data(Json::objectValue);
    Json::Value list(Json::arrayValue);
    for (const auto& item : items) {
        list.append(ToPendingAuditItemJson(item));
    }
    data["list"] = std::move(list);
    data["total"] = static_cast<Json::UInt64>(items.size());
    data["pageNo"] = 1;
    data["pageSize"] = static_cast<Json::UInt64>(items.size());
    return data;
}

Json::Value ToAuditLogListJson(const std::vector<modules::item::ItemAuditLogRecord>& logs) {
    Json::Value data(Json::objectValue);
    Json::Value list(Json::arrayValue);
    for (const auto& log : logs) {
        list.append(ToAuditLogJson(log));
    }
    data["list"] = std::move(list);
    data["total"] = static_cast<Json::UInt64>(logs.size());
    data["pageNo"] = 1;
    data["pageSize"] = static_cast<Json::UInt64>(logs.size());
    return data;
}

Json::Value ToAuditItemResultJson(const modules::item::AuditItemResult& result) {
    Json::Value json(Json::objectValue);
    json["itemId"] = static_cast<Json::UInt64>(result.item_id);
    json["oldStatus"] = result.old_status;
    json["newStatus"] = result.new_status;
    json["auditStatus"] = result.audit_result;
    json["auditResult"] = result.audit_result;
    json["auditedAt"] = result.audited_at;
    return json;
}

std::string ReadAuditStatus(const Json::Value& body) {
    const auto audit_status = common::http::ReadOptionalString(body, "auditStatus");
    if (audit_status.has_value()) {
        return *audit_status;
    }
    return common::http::ReadRequiredString(body, "auditResult");
}

modules::item::AuditItemRequest BuildAuditItemRequest(const Json::Value& body) {
    return modules::item::AuditItemRequest{
        .audit_result = ReadAuditStatus(body),
        .reason = common::http::ReadOptionalString(body, "reason").value_or(""),
    };
}

#endif

}  // namespace

void RegisterAuditHttpRoutes(const std::shared_ptr<common::http::HttpServiceContext> services) {
#if AUCTION_HAS_DROGON
    drogon::app().registerHandler(
        "/api/admin/items/pending",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            callback(common::http::ExecuteApi([&]() {
                const auto items = services->item_audit_service().ListPendingItems(
                    request->getHeader("authorization")
                );
                return common::http::ApiResponse::Success(ToPendingAuditListJson(items));
            }));
        },
        {drogon::Get}
    );

    drogon::app().registerHandler(
        "/api/admin/items/{1}/audit-logs",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                   const std::string& raw_item_id) {
            callback(common::http::ExecuteApi([&]() {
                const auto logs = services->item_audit_service().GetAuditLogs(
                    request->getHeader("authorization"),
                    ParsePositiveId(raw_item_id, "item id")
                );
                return common::http::ApiResponse::Success(ToAuditLogListJson(logs));
            }));
        },
        {drogon::Get}
    );

    drogon::app().registerHandler(
        "/api/admin/items/{1}/audit",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                   const std::string& raw_item_id) {
            callback(common::http::ExecuteApi([&]() {
                const auto body = common::http::RequireJsonBody(request);
                const auto result = services->item_audit_service().AuditItem(
                    request->getHeader("authorization"),
                    ParsePositiveId(raw_item_id, "item id"),
                    BuildAuditItemRequest(body)
                );
                return common::http::ApiResponse::Success(ToAuditItemResultJson(result));
            }));
        },
        {drogon::Post}
    );
#else
    static_cast<void>(services);
#endif
}

}  // namespace auction::access::http
