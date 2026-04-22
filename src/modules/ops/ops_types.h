#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace auction::modules::ops {

inline constexpr std::string_view kExceptionSourceTaskLog = "TASK_LOG";
inline constexpr std::string_view kExceptionSourceNotification = "NOTIFICATION";
inline constexpr std::string_view kExceptionSourcePaymentCallback = "PAYMENT_CALLBACK";
inline constexpr std::string_view kExceptionSourceManualMark = "MANUAL_MARK";

inline constexpr std::string_view kCompensationTypeNotificationPush = "NOTIFICATION_PUSH";
inline constexpr std::string_view kCompensationTypeOrderSettlement = "ORDER_SETTLEMENT";
inline constexpr std::string_view kCompensationTypeOrderTimeoutClose = "ORDER_TIMEOUT_CLOSE";
inline constexpr std::string_view kCompensationTypeStatisticsDailyAggregation =
    "STATISTICS_DAILY_AGGREGATION";

struct OperationLogRecord {
    std::uint64_t operation_log_id{0};
    std::optional<std::uint64_t> operator_id;
    std::string operator_role;
    std::string module_name;
    std::string operation_name;
    std::string biz_key;
    std::string result;
    std::string detail;
    std::string created_at;
};

struct OperationLogQuery {
    std::optional<std::string> module_name;
    std::optional<std::string> result;
    int limit{50};
};

struct OperationLogQueryResult {
    std::vector<OperationLogRecord> records;
};

struct TaskLogRecord {
    std::uint64_t task_log_id{0};
    std::string task_type;
    std::string biz_key;
    std::string task_status;
    int retry_count{0};
    std::string last_error;
    std::string scheduled_at;
    std::string started_at;
    std::string finished_at;
    std::string created_at;
};

struct TaskLogQuery {
    std::optional<std::string> task_type;
    std::optional<std::string> task_status;
    int limit{50};
};

struct TaskLogQueryResult {
    std::vector<TaskLogRecord> records;
};

struct SystemExceptionRecord {
    std::string source_type;
    std::string source_id;
    std::string biz_key;
    std::string current_status;
    std::string summary;
    std::string detail;
    std::string occurred_at;
    bool retryable{false};
};

struct SystemExceptionQuery {
    int limit{50};
};

struct SystemExceptionQueryResult {
    std::vector<SystemExceptionRecord> records;
};

struct MarkExceptionRequest {
    std::string exception_type;
    std::string biz_key;
    std::string detail;
};

struct MarkExceptionResult {
    std::string exception_type;
    std::string biz_key;
    std::string detail;
    std::string created_at;
};

struct CompensationRequest {
    std::string compensation_type;
    std::string biz_key;
    int limit{20};
};

struct CompensationResult {
    std::string compensation_type;
    int scanned{0};
    int succeeded{0};
    int failed{0};
    int skipped{0};
    std::vector<std::string> affected_keys;
};

}  // namespace auction::modules::ops
