#include "modules/ops/ops_service.h"

#include <cctype>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

#include "common/errors/error_code.h"
#include "modules/auth/auth_types.h"
#include "modules/ops/ops_exception.h"
#include "repository/ops_repository.h"

namespace auction::modules::ops {

namespace {

void ThrowOpsError(const common::errors::ErrorCode code, const std::string_view message) {
    throw OpsException(code, std::string(message));
}

repository::OpsRepository MakeRepository(common::db::MysqlConnection& connection) {
    return repository::OpsRepository(connection);
}

std::string CurrentTimestampText() {
    const auto now = std::chrono::system_clock::now();
    const auto time_value = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm{};
    localtime_r(&time_value, &local_tm);

    std::ostringstream output;
    output << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
    return output.str();
}

void InsertOperationLogSafely(
    repository::OpsRepository& repository,
    const std::optional<std::uint64_t>& operator_id,
    const std::string& operator_role,
    const std::string& operation_name,
    const std::string& biz_key,
    const std::string& result,
    const std::string& detail
) {
    try {
        repository.InsertOperationLog(
            operator_id,
            operator_role,
            operation_name,
            biz_key,
            result,
            detail
        );
    } catch (...) {
    }
}

bool IsValidQueryToken(const std::string& value) {
    if (value.empty() || value.size() > 64) {
        return false;
    }
    for (const char character : value) {
        if (std::isalnum(static_cast<unsigned char>(character)) == 0 && character != '_' &&
            character != '-') {
            return false;
        }
    }
    return true;
}

bool IsValidExceptionType(const std::string& value) {
    if (value.empty() || value.size() > 40) {
        return false;
    }
    for (const char character : value) {
        if (std::isalnum(static_cast<unsigned char>(character)) == 0 && character != '_') {
            return false;
        }
    }
    return true;
}

void ValidateLimit(const int limit) {
    if (limit <= 0 || limit > 100) {
        ThrowOpsError(
            common::errors::ErrorCode::kOpsQueryLimitInvalid,
            "ops query limit must be between 1 and 100"
        );
    }
}

void ValidateOperationLogQuery(const OperationLogQuery& query) {
    ValidateLimit(query.limit);
    if (query.module_name.has_value() && !query.module_name->empty() &&
        !IsValidQueryToken(*query.module_name)) {
        ThrowOpsError(
            common::errors::ErrorCode::kOpsQueryTokenInvalid,
            "operation log module_name is invalid"
        );
    }
    if (query.result.has_value() && !query.result->empty() && !IsValidQueryToken(*query.result)) {
        ThrowOpsError(
            common::errors::ErrorCode::kOpsQueryTokenInvalid,
            "operation log result is invalid"
        );
    }
}

void ValidateTaskLogQuery(const TaskLogQuery& query) {
    ValidateLimit(query.limit);
    if (query.task_type.has_value() && !query.task_type->empty() &&
        !IsValidQueryToken(*query.task_type)) {
        ThrowOpsError(
            common::errors::ErrorCode::kOpsQueryTokenInvalid,
            "task log task_type is invalid"
        );
    }
    if (query.task_status.has_value() && !query.task_status->empty() &&
        !IsValidQueryToken(*query.task_status)) {
        ThrowOpsError(
            common::errors::ErrorCode::kOpsQueryTokenInvalid,
            "task log task_status is invalid"
        );
    }
}

void ValidateSystemExceptionQuery(const SystemExceptionQuery& query) {
    ValidateLimit(query.limit);
}

void ValidateMarkExceptionRequest(const MarkExceptionRequest& request) {
    if (!IsValidExceptionType(request.exception_type)) {
        ThrowOpsError(
            common::errors::ErrorCode::kOpsExceptionTypeInvalid,
            "ops exception type is invalid"
        );
    }
    if (request.biz_key.empty() || request.biz_key.size() > 128) {
        ThrowOpsError(
            common::errors::ErrorCode::kOpsBizKeyInvalid,
            "ops biz_key is invalid"
        );
    }
    if (request.detail.empty() || request.detail.size() > 500) {
        ThrowOpsError(
            common::errors::ErrorCode::kOpsMarkDetailInvalid,
            "ops exception detail is invalid"
        );
    }
}

void ValidateRetryRequest(const notification::NotificationRetryRequest& request) {
    if (request.notification_id.has_value()) {
        return;
    }
    ValidateLimit(request.limit);
}

void ValidateCompensationRequest(const CompensationRequest& request) {
    if (request.compensation_type != kCompensationTypeNotificationPush &&
        request.compensation_type != kCompensationTypeOrderSettlement &&
        request.compensation_type != kCompensationTypeOrderTimeoutClose &&
        request.compensation_type != kCompensationTypeStatisticsDailyAggregation) {
        ThrowOpsError(
            common::errors::ErrorCode::kOpsCompensationTypeInvalid,
            "ops compensation type is invalid"
        );
    }

    if (request.compensation_type == kCompensationTypeNotificationPush) {
        if (request.biz_key.empty()) {
            ValidateLimit(request.limit);
            return;
        }
        for (const char character : request.biz_key) {
            if (std::isdigit(static_cast<unsigned char>(character)) == 0) {
                ThrowOpsError(
                    common::errors::ErrorCode::kOpsBizKeyInvalid,
                    "notification compensation biz_key must be numeric"
                );
            }
        }
        return;
    }

    if (request.compensation_type == kCompensationTypeStatisticsDailyAggregation &&
        request.biz_key.empty()) {
        ThrowOpsError(
            common::errors::ErrorCode::kOpsBizKeyInvalid,
            "statistics compensation requires a stat date"
        );
    }
}

notification::NotificationRetryResult RetryNotificationsAndLog(
    repository::OpsRepository& repository,
    const modules::auth::AuthContext& auth_context,
    notification::NotificationService& notification_service,
    const notification::NotificationRetryRequest& request
) {
    try {
        const auto result = notification_service.RetryFailedNotifications(request);
        InsertOperationLogSafely(
            repository,
            auth_context.user_id,
            auth_context.role_code,
            "retry_failed_notifications",
            request.notification_id.has_value() ? std::to_string(*request.notification_id) : "",
            "SUCCESS",
            "scanned=" + std::to_string(result.scanned) + ",succeeded=" +
                std::to_string(result.succeeded) + ",failed=" + std::to_string(result.failed) +
                ",skipped=" + std::to_string(result.skipped)
        );
        return result;
    } catch (const std::exception& exception) {
        InsertOperationLogSafely(
            repository,
            auth_context.user_id,
            auth_context.role_code,
            "retry_failed_notifications",
            request.notification_id.has_value() ? std::to_string(*request.notification_id) : "",
            "FAILED",
            exception.what()
        );
        throw;
    }
}

CompensationResult BuildCompensationResult(
    const std::string& compensation_type,
    const notification::NotificationRetryResult& result
) {
    CompensationResult compensation;
    compensation.compensation_type = compensation_type;
    compensation.scanned = result.scanned;
    compensation.succeeded = result.succeeded;
    compensation.failed = result.failed;
    compensation.skipped = result.skipped;
    for (const auto notification_id : result.affected_notification_ids) {
        compensation.affected_keys.push_back(std::to_string(notification_id));
    }
    return compensation;
}

CompensationResult BuildCompensationResult(
    const std::string& compensation_type,
    const order::OrderScheduleResult& result
) {
    CompensationResult compensation;
    compensation.compensation_type = compensation_type;
    compensation.scanned = result.scanned;
    compensation.succeeded = result.succeeded;
    compensation.failed = result.failed;
    compensation.skipped = result.skipped;
    for (const auto order_id : result.affected_order_ids) {
        compensation.affected_keys.push_back(std::to_string(order_id));
    }
    return compensation;
}

}  // namespace

OpsService::OpsService(
    const common::config::AppConfig& config,
    std::filesystem::path project_root,
    middleware::AuthMiddleware& auth_middleware,
    notification::NotificationService& notification_service,
    order::OrderService& order_service,
    statistics::StatisticsService& statistics_service
) : mysql_config_(config.mysql),
    project_root_(std::move(project_root)),
    auth_middleware_(&auth_middleware),
    notification_service_(&notification_service),
    order_service_(&order_service),
    statistics_service_(&statistics_service) {}

OperationLogQueryResult OpsService::ListOperationLogs(
    const std::string_view authorization_header,
    const OperationLogQuery& query
) {
    [[maybe_unused]] const auto auth_context = auth_middleware_->RequireAnyRole(
        authorization_header,
        {modules::auth::kRoleAdmin, modules::auth::kRoleSupport}
    );
    ValidateOperationLogQuery(query);

    auto connection = CreateConnection();
    auto repository = MakeRepository(connection);
    return OperationLogQueryResult{
        .records = repository.ListOperationLogs(query),
    };
}

TaskLogQueryResult OpsService::ListTaskLogs(
    const std::string_view authorization_header,
    const TaskLogQuery& query
) {
    [[maybe_unused]] const auto auth_context = auth_middleware_->RequireAnyRole(
        authorization_header,
        {modules::auth::kRoleAdmin, modules::auth::kRoleSupport}
    );
    ValidateTaskLogQuery(query);

    auto connection = CreateConnection();
    auto repository = MakeRepository(connection);
    return TaskLogQueryResult{
        .records = repository.ListTaskLogs(query),
    };
}

SystemExceptionQueryResult OpsService::ListSystemExceptions(
    const std::string_view authorization_header,
    const SystemExceptionQuery& query
) {
    [[maybe_unused]] const auto auth_context = auth_middleware_->RequireAnyRole(
        authorization_header,
        {modules::auth::kRoleAdmin, modules::auth::kRoleSupport}
    );
    ValidateSystemExceptionQuery(query);

    auto connection = CreateConnection();
    auto repository = MakeRepository(connection);
    return SystemExceptionQueryResult{
        .records = repository.ListSystemExceptions(query.limit),
    };
}

MarkExceptionResult OpsService::MarkException(
    const std::string_view authorization_header,
    const MarkExceptionRequest& request
) {
    const auto auth_context = auth_middleware_->RequireAnyRole(
        authorization_header,
        {modules::auth::kRoleAdmin, modules::auth::kRoleSupport}
    );
    ValidateMarkExceptionRequest(request);

    auto connection = CreateConnection();
    auto repository = MakeRepository(connection);
    const auto created_at = CurrentTimestampText();
    repository.InsertOperationLog(
        auth_context.user_id,
        auth_context.role_code,
        "mark_exception:" + request.exception_type,
        request.biz_key,
        "SUCCESS",
        request.detail
    );

    return MarkExceptionResult{
        .exception_type = request.exception_type,
        .biz_key = request.biz_key,
        .detail = request.detail,
        .created_at = created_at,
    };
}

notification::NotificationRetryResult OpsService::RetryFailedNotifications(
    const std::string_view authorization_header,
    const notification::NotificationRetryRequest& request
) {
    const auto auth_context = auth_middleware_->RequireAnyRole(
        authorization_header,
        {modules::auth::kRoleAdmin}
    );
    ValidateRetryRequest(request);

    auto connection = CreateConnection();
    auto repository = MakeRepository(connection);
    return RetryNotificationsAndLog(repository, auth_context, *notification_service_, request);
}

CompensationResult OpsService::RunCompensation(
    const std::string_view authorization_header,
    const CompensationRequest& request
) {
    const auto auth_context = auth_middleware_->RequireAnyRole(
        authorization_header,
        {modules::auth::kRoleAdmin}
    );
    ValidateCompensationRequest(request);

    auto connection = CreateConnection();
    auto repository = MakeRepository(connection);

    try {
        if (request.compensation_type == kCompensationTypeNotificationPush) {
            notification::NotificationRetryRequest retry_request;
            retry_request.limit = request.limit;
            if (!request.biz_key.empty()) {
                retry_request.notification_id = std::stoull(request.biz_key);
            }
            const auto retry_result = RetryNotificationsAndLog(
                repository,
                auth_context,
                *notification_service_,
                retry_request
            );
            return BuildCompensationResult(request.compensation_type, retry_result);
        }

        if (request.compensation_type == kCompensationTypeOrderSettlement) {
            const auto result = order_service_->GenerateSettlementOrders();
            InsertOperationLogSafely(
                repository,
                auth_context.user_id,
                auth_context.role_code,
                "run_compensation:" + request.compensation_type,
                request.biz_key,
                "SUCCESS",
                "scanned=" + std::to_string(result.scanned) + ",succeeded=" +
                    std::to_string(result.succeeded) + ",failed=" +
                    std::to_string(result.failed) + ",skipped=" +
                    std::to_string(result.skipped)
            );
            return BuildCompensationResult(request.compensation_type, result);
        }

        if (request.compensation_type == kCompensationTypeOrderTimeoutClose) {
            const auto result = order_service_->CloseExpiredOrders();
            InsertOperationLogSafely(
                repository,
                auth_context.user_id,
                auth_context.role_code,
                "run_compensation:" + request.compensation_type,
                request.biz_key,
                "SUCCESS",
                "scanned=" + std::to_string(result.scanned) + ",succeeded=" +
                    std::to_string(result.succeeded) + ",failed=" +
                    std::to_string(result.failed) + ",skipped=" +
                    std::to_string(result.skipped)
            );
            return BuildCompensationResult(request.compensation_type, result);
        }

        const auto statistics_result = statistics_service_->RebuildDailyStatistics(request.biz_key);
        InsertOperationLogSafely(
            repository,
            auth_context.user_id,
            auth_context.role_code,
            "run_compensation:" + request.compensation_type,
            request.biz_key,
            "SUCCESS",
            "created=" + std::string(statistics_result.created ? "true" : "false")
        );

        CompensationResult compensation;
        compensation.compensation_type = request.compensation_type;
        compensation.scanned = 1;
        compensation.succeeded = 1;
        compensation.affected_keys.push_back(statistics_result.record.stat_date);
        return compensation;
    } catch (const std::exception& exception) {
        InsertOperationLogSafely(
            repository,
            auth_context.user_id,
            auth_context.role_code,
            "run_compensation:" + request.compensation_type,
            request.biz_key,
            "FAILED",
            exception.what()
        );
        throw;
    }
}

common::db::MysqlConnection OpsService::CreateConnection() const {
    return common::db::MysqlConnection(mysql_config_, project_root_);
}

}  // namespace auction::modules::ops
