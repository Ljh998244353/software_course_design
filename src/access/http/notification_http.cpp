#include "access/http/notification_http.h"

#include <algorithm>
#include <cstdint>
#include <cctype>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

#include "common/http/http_utils.h"
#include "modules/notification/notification_service.h"

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

int ParsePositiveInt(const std::string& raw_value, const std::string& field_name) {
    std::size_t parsed_length = 0;
    const auto value = std::stoll(raw_value, &parsed_length);
    if (parsed_length != raw_value.size() || value <= 0) {
        throw std::invalid_argument(field_name + " must be a positive integer");
    }
    return static_cast<int>(value);
}

std::optional<std::uint64_t> ParseOptionalPositiveId(
    const std::optional<std::string>& raw_value,
    const std::string& field_name
) {
    if (!raw_value.has_value() || raw_value->empty()) {
        return std::nullopt;
    }
    return ParsePositiveId(*raw_value, field_name);
}

bool ParseOptionalBool(
    std::optional<std::string> raw_value,
    const std::string& field_name
) {
    if (!raw_value.has_value() || raw_value->empty()) {
        return false;
    }

    std::transform(
        raw_value->begin(),
        raw_value->end(),
        raw_value->begin(),
        [](const unsigned char character) { return static_cast<char>(std::tolower(character)); }
    );
    if (*raw_value == "true" || *raw_value == "1") {
        return true;
    }
    if (*raw_value == "false" || *raw_value == "0") {
        return false;
    }
    throw std::invalid_argument(field_name + " must be true or false");
}

Json::Value ToOptionalUInt64Json(const std::optional<std::uint64_t>& value) {
    if (!value.has_value()) {
        return Json::Value(Json::nullValue);
    }
    return Json::Value(static_cast<Json::UInt64>(*value));
}

Json::Value ToNotificationJson(const modules::notification::UserNotificationEntry& entry) {
    Json::Value json(Json::objectValue);
    json["notificationId"] = static_cast<Json::UInt64>(entry.notification_id);
    json["userId"] = static_cast<Json::UInt64>(entry.user_id);
    json["noticeType"] = entry.notice_type;
    json["title"] = entry.title;
    json["content"] = entry.content;
    json["bizType"] = entry.biz_type;
    json["bizId"] = ToOptionalUInt64Json(entry.biz_id);
    json["readStatus"] = entry.read_status;
    json["pushStatus"] = entry.push_status;
    json["createdAt"] = entry.created_at;
    json["readAt"] = entry.read_at;
    return json;
}

Json::Value ToNotificationListJson(
    const modules::notification::UserNotificationListResult& result
) {
    Json::Value data(Json::objectValue);
    Json::Value list(Json::arrayValue);
    std::uint64_t unread_count = 0;
    for (const auto& entry : result.records) {
        if (entry.read_status == modules::notification::kReadStatusUnread) {
            ++unread_count;
        }
        list.append(ToNotificationJson(entry));
    }
    data["list"] = std::move(list);
    data["total"] = static_cast<Json::UInt64>(result.records.size());
    data["unreadCount"] = static_cast<Json::UInt64>(unread_count);
    data["limit"] = result.limit;
    return data;
}

Json::Value ToMarkReadResultJson(
    const modules::notification::MarkNotificationReadResult& result
) {
    Json::Value json(Json::objectValue);
    json["notificationId"] = static_cast<Json::UInt64>(result.notification_id);
    json["readStatus"] = result.read_status;
    json["readAt"] = result.read_at;
    return json;
}

modules::notification::NotificationQuery BuildNotificationQuery(
    const drogon::HttpRequestPtr& request
) {
    modules::notification::NotificationQuery query;
    const auto limit = request->getOptionalParameter<std::string>("limit");
    if (limit.has_value() && !limit->empty()) {
        query.limit = ParsePositiveInt(*limit, "limit");
    }
    query.unread_only = ParseOptionalBool(
        request->getOptionalParameter<std::string>("unreadOnly"),
        "unreadOnly"
    );
    query.biz_type = request->getOptionalParameter<std::string>("bizType").value_or("");
    query.biz_id = ParseOptionalPositiveId(
        request->getOptionalParameter<std::string>("bizId"),
        "bizId"
    );
    return query;
}

#endif

}  // namespace

void RegisterNotificationHttpRoutes(
    const std::shared_ptr<common::http::HttpServiceContext> services
) {
#if AUCTION_HAS_DROGON
    drogon::app().registerHandler(
        "/api/notifications",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            callback(common::http::ExecuteApi([&]() {
                const auto context =
                    common::http::RequireAuthContext(request, services->auth_middleware());
                const auto result = services->notification_service().ListUserNotifications(
                    context.user_id,
                    BuildNotificationQuery(request)
                );
                return common::http::ApiResponse::Success(ToNotificationListJson(result));
            }));
        },
        {drogon::Get}
    );

    drogon::app().registerHandler(
        "/api/notifications/{1}/read",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                   const std::string& raw_notification_id) {
            callback(common::http::ExecuteApi([&]() {
                const auto context =
                    common::http::RequireAuthContext(request, services->auth_middleware());
                const auto result = services->notification_service().MarkUserNotificationRead(
                    context.user_id,
                    ParsePositiveId(raw_notification_id, "notification id")
                );
                return common::http::ApiResponse::Success(ToMarkReadResultJson(result));
            }));
        },
        {drogon::Patch}
    );
#else
    static_cast<void>(services);
#endif
}

}  // namespace auction::access::http
