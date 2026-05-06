#include "access/http/ops_http.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

#include "common/http/http_utils.h"
#include "modules/notification/notification_service.h"
#include "modules/ops/ops_types.h"

#if AUCTION_HAS_DROGON
#include <drogon/drogon.h>
#endif

namespace auction::access::http {

namespace {

#if AUCTION_HAS_DROGON

int ParsePositiveInt(const std::string& raw_value, const std::string& field_name) {
    std::size_t parsed_length = 0;
    const auto value = std::stoll(raw_value, &parsed_length);
    if (parsed_length != raw_value.size() || value <= 0) {
        throw std::invalid_argument(field_name + " must be a positive integer");
    }
    return static_cast<int>(value);
}

std::uint64_t ParsePositiveId(const std::string& raw_value, const std::string& field_name) {
    std::size_t parsed_length = 0;
    const auto value = std::stoull(raw_value, &parsed_length);
    if (parsed_length != raw_value.size() || value == 0) {
        throw std::invalid_argument(field_name + " must be a positive integer");
    }
    return value;
}

std::optional<std::string> NonEmptyOptional(std::optional<std::string> value) {
    if (!value.has_value() || value->empty()) {
        return std::nullopt;
    }
    return value;
}

std::optional<std::string> OptionalParameterAny(
    const drogon::HttpRequestPtr& request,
    const std::string& camel_name,
    const std::string& snake_name
) {
    if (const auto value = request->getOptionalParameter<std::string>(camel_name);
        value.has_value() && !value->empty()) {
        return value;
    }
    return NonEmptyOptional(request->getOptionalParameter<std::string>(snake_name));
}

int ReadOptionalLimit(const drogon::HttpRequestPtr& request, const int default_limit) {
    const auto raw_limit = request->getOptionalParameter<std::string>("limit");
    if (!raw_limit.has_value() || raw_limit->empty()) {
        return default_limit;
    }
    return ParsePositiveInt(*raw_limit, "limit");
}

std::optional<std::uint64_t> ReadOptionalNotificationId(const Json::Value& body) {
    if (!body.isMember("notificationId") || body["notificationId"].isNull()) {
        return std::nullopt;
    }
    if (!body["notificationId"].isIntegral()) {
        throw std::invalid_argument("notificationId must be a positive integer");
    }
    Json::UInt64 value = 0;
    if (body["notificationId"].isUInt64()) {
        value = body["notificationId"].asUInt64();
    } else {
        const auto signed_value = body["notificationId"].asInt64();
        if (signed_value <= 0) {
            throw std::invalid_argument("notificationId must be a positive integer");
        }
        value = static_cast<Json::UInt64>(signed_value);
    }
    if (value == 0) {
        throw std::invalid_argument("notificationId must be a positive integer");
    }
    return static_cast<std::uint64_t>(value);
}

int ReadOptionalInt(const Json::Value& body, const std::string& field_name, const int fallback) {
    if (!body.isMember(field_name) || body[field_name].isNull()) {
        return fallback;
    }
    if (!body[field_name].isInt()) {
        throw std::invalid_argument(field_name + " must be an integer");
    }
    return body[field_name].asInt();
}

Json::Value ToOptionalUInt64Json(const std::optional<std::uint64_t>& value) {
    if (!value.has_value()) {
        return Json::Value(Json::nullValue);
    }
    return Json::Value(static_cast<Json::UInt64>(*value));
}

Json::Value StringVectorToJson(const std::vector<std::string>& values) {
    Json::Value list(Json::arrayValue);
    for (const auto& value : values) {
        list.append(value);
    }
    return list;
}

Json::Value UInt64VectorToJson(const std::vector<std::uint64_t>& values) {
    Json::Value list(Json::arrayValue);
    for (const auto value : values) {
        list.append(static_cast<Json::UInt64>(value));
    }
    return list;
}

modules::ops::OperationLogQuery BuildOperationLogQuery(
    const drogon::HttpRequestPtr& request
) {
    return modules::ops::OperationLogQuery{
        .module_name = OptionalParameterAny(request, "moduleName", "module_name"),
        .result = NonEmptyOptional(request->getOptionalParameter<std::string>("result")),
        .limit = ReadOptionalLimit(request, 50),
    };
}

modules::ops::TaskLogQuery BuildTaskLogQuery(const drogon::HttpRequestPtr& request) {
    return modules::ops::TaskLogQuery{
        .task_type = OptionalParameterAny(request, "taskType", "task_type"),
        .task_status = OptionalParameterAny(request, "taskStatus", "task_status"),
        .limit = ReadOptionalLimit(request, 50),
    };
}

modules::ops::SystemExceptionQuery BuildSystemExceptionQuery(
    const drogon::HttpRequestPtr& request
) {
    return modules::ops::SystemExceptionQuery{
        .limit = ReadOptionalLimit(request, 50),
    };
}

modules::ops::MarkExceptionRequest BuildMarkExceptionRequest(const Json::Value& body) {
    return modules::ops::MarkExceptionRequest{
        .exception_type = common::http::ReadRequiredString(body, "exceptionType"),
        .biz_key = common::http::ReadRequiredString(body, "bizKey"),
        .detail = common::http::ReadRequiredString(body, "detail"),
    };
}

modules::notification::NotificationRetryRequest BuildNotificationRetryRequest(
    const Json::Value& body
) {
    return modules::notification::NotificationRetryRequest{
        .notification_id = ReadOptionalNotificationId(body),
        .limit = ReadOptionalInt(body, "limit", 20),
    };
}

modules::ops::CompensationRequest BuildCompensationRequest(const Json::Value& body) {
    return modules::ops::CompensationRequest{
        .compensation_type = common::http::ReadRequiredString(body, "compensationType"),
        .biz_key = common::http::ReadOptionalString(body, "bizKey").value_or(""),
        .limit = ReadOptionalInt(body, "limit", 20),
    };
}

Json::Value ToOperationLogJson(const modules::ops::OperationLogRecord& record) {
    Json::Value json(Json::objectValue);
    json["operationLogId"] = static_cast<Json::UInt64>(record.operation_log_id);
    json["operatorId"] = ToOptionalUInt64Json(record.operator_id);
    json["operatorRole"] = record.operator_role;
    json["moduleName"] = record.module_name;
    json["operationName"] = record.operation_name;
    json["bizKey"] = record.biz_key;
    json["result"] = record.result;
    json["detail"] = record.detail;
    json["createdAt"] = record.created_at;
    return json;
}

Json::Value ToOperationLogListJson(const modules::ops::OperationLogQueryResult& result) {
    Json::Value data(Json::objectValue);
    Json::Value list(Json::arrayValue);
    for (const auto& record : result.records) {
        list.append(ToOperationLogJson(record));
    }
    data["list"] = std::move(list);
    data["total"] = static_cast<Json::UInt64>(result.records.size());
    return data;
}

Json::Value ToTaskLogJson(const modules::ops::TaskLogRecord& record) {
    Json::Value json(Json::objectValue);
    json["taskLogId"] = static_cast<Json::UInt64>(record.task_log_id);
    json["taskType"] = record.task_type;
    json["bizKey"] = record.biz_key;
    json["taskStatus"] = record.task_status;
    json["retryCount"] = record.retry_count;
    json["lastError"] = record.last_error;
    json["scheduledAt"] = record.scheduled_at;
    json["startedAt"] = record.started_at;
    json["finishedAt"] = record.finished_at;
    json["createdAt"] = record.created_at;
    return json;
}

Json::Value ToTaskLogListJson(const modules::ops::TaskLogQueryResult& result) {
    Json::Value data(Json::objectValue);
    Json::Value list(Json::arrayValue);
    for (const auto& record : result.records) {
        list.append(ToTaskLogJson(record));
    }
    data["list"] = std::move(list);
    data["total"] = static_cast<Json::UInt64>(result.records.size());
    return data;
}

Json::Value ToSystemExceptionJson(const modules::ops::SystemExceptionRecord& record) {
    Json::Value json(Json::objectValue);
    json["sourceType"] = record.source_type;
    json["sourceId"] = record.source_id;
    json["bizKey"] = record.biz_key;
    json["currentStatus"] = record.current_status;
    json["summary"] = record.summary;
    json["detail"] = record.detail;
    json["occurredAt"] = record.occurred_at;
    json["retryable"] = record.retryable;
    return json;
}

Json::Value ToSystemExceptionListJson(const modules::ops::SystemExceptionQueryResult& result) {
    Json::Value data(Json::objectValue);
    Json::Value list(Json::arrayValue);
    for (const auto& record : result.records) {
        list.append(ToSystemExceptionJson(record));
    }
    data["list"] = std::move(list);
    data["total"] = static_cast<Json::UInt64>(result.records.size());
    return data;
}

Json::Value ToMarkExceptionJson(const modules::ops::MarkExceptionResult& result) {
    Json::Value json(Json::objectValue);
    json["exceptionType"] = result.exception_type;
    json["bizKey"] = result.biz_key;
    json["detail"] = result.detail;
    json["createdAt"] = result.created_at;
    return json;
}

Json::Value ToNotificationRetryJson(
    const modules::notification::NotificationRetryResult& result
) {
    Json::Value json(Json::objectValue);
    json["scanned"] = result.scanned;
    json["succeeded"] = result.succeeded;
    json["failed"] = result.failed;
    json["skipped"] = result.skipped;
    json["affectedNotificationIds"] = UInt64VectorToJson(result.affected_notification_ids);
    return json;
}

Json::Value ToCompensationJson(const modules::ops::CompensationResult& result) {
    Json::Value json(Json::objectValue);
    json["compensationType"] = result.compensation_type;
    json["scanned"] = result.scanned;
    json["succeeded"] = result.succeeded;
    json["failed"] = result.failed;
    json["skipped"] = result.skipped;
    json["affectedKeys"] = StringVectorToJson(result.affected_keys);
    return json;
}

#endif

}  // namespace

void RegisterOpsHttpRoutes(const std::shared_ptr<common::http::HttpServiceContext> services) {
#if AUCTION_HAS_DROGON
    drogon::app().registerHandler(
        "/api/admin/ops/operation-logs",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            callback(common::http::ExecuteApi([&]() {
                const auto result = services->ops_service().ListOperationLogs(
                    request->getHeader("authorization"),
                    BuildOperationLogQuery(request)
                );
                return common::http::ApiResponse::Success(ToOperationLogListJson(result));
            }));
        },
        {drogon::Get}
    );

    drogon::app().registerHandler(
        "/api/admin/ops/task-logs",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            callback(common::http::ExecuteApi([&]() {
                const auto result = services->ops_service().ListTaskLogs(
                    request->getHeader("authorization"),
                    BuildTaskLogQuery(request)
                );
                return common::http::ApiResponse::Success(ToTaskLogListJson(result));
            }));
        },
        {drogon::Get}
    );

    drogon::app().registerHandler(
        "/api/admin/ops/exceptions",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            callback(common::http::ExecuteApi([&]() {
                const auto result = services->ops_service().ListSystemExceptions(
                    request->getHeader("authorization"),
                    BuildSystemExceptionQuery(request)
                );
                return common::http::ApiResponse::Success(ToSystemExceptionListJson(result));
            }));
        },
        {drogon::Get}
    );

    drogon::app().registerHandler(
        "/api/admin/ops/exceptions/mark",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            callback(common::http::ExecuteApi([&]() {
                const auto result = services->ops_service().MarkException(
                    request->getHeader("authorization"),
                    BuildMarkExceptionRequest(common::http::RequireJsonBody(request))
                );
                return common::http::ApiResponse::Success(ToMarkExceptionJson(result));
            }));
        },
        {drogon::Post}
    );

    drogon::app().registerHandler(
        "/api/admin/ops/notifications/retry",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            callback(common::http::ExecuteApi([&]() {
                const auto result = services->ops_service().RetryFailedNotifications(
                    request->getHeader("authorization"),
                    BuildNotificationRetryRequest(common::http::RequireJsonBody(request))
                );
                return common::http::ApiResponse::Success(ToNotificationRetryJson(result));
            }));
        },
        {drogon::Post}
    );

    drogon::app().registerHandler(
        "/api/admin/ops/compensations",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            callback(common::http::ExecuteApi([&]() {
                const auto result = services->ops_service().RunCompensation(
                    request->getHeader("authorization"),
                    BuildCompensationRequest(common::http::RequireJsonBody(request))
                );
                return common::http::ApiResponse::Success(ToCompensationJson(result));
            }));
        },
        {drogon::Post}
    );
#else
    static_cast<void>(services);
#endif
}

}  // namespace auction::access::http
