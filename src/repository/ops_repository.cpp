#include "repository/ops_repository.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

#include <mysql.h>

namespace auction::repository {

namespace {

using MysqlResultPtr = std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)>;

MysqlResultPtr ExecuteQuery(common::db::MysqlConnection& connection, const std::string& sql) {
    MYSQL* handle = connection.native_handle();
    if (mysql_query(handle, sql.c_str()) != 0) {
        throw std::runtime_error(std::string("mysql query failed: ") + mysql_error(handle));
    }

    MYSQL_RES* result = mysql_store_result(handle);
    if (result == nullptr) {
        throw std::runtime_error(std::string("mysql read result failed: ") + mysql_error(handle));
    }

    return MysqlResultPtr(result, &mysql_free_result);
}

std::string ReadRowString(MYSQL_ROW row, const unsigned int index) {
    return row[index] == nullptr ? "" : std::string(row[index]);
}

std::uint64_t ReadRowUint64(MYSQL_ROW row, const unsigned int index) {
    return row[index] == nullptr ? 0ULL : std::stoull(row[index]);
}

std::optional<std::uint64_t> ReadRowOptionalUint64(MYSQL_ROW row, const unsigned int index) {
    if (row[index] == nullptr) {
        return std::nullopt;
    }
    return std::stoull(row[index]);
}

int ReadRowInt(MYSQL_ROW row, const unsigned int index) {
    return row[index] == nullptr ? 0 : std::stoi(row[index]);
}

modules::ops::OperationLogRecord BuildOperationLogRecord(MYSQL_ROW row) {
    return modules::ops::OperationLogRecord{
        .operation_log_id = ReadRowUint64(row, 0),
        .operator_id = ReadRowOptionalUint64(row, 1),
        .operator_role = ReadRowString(row, 2),
        .module_name = ReadRowString(row, 3),
        .operation_name = ReadRowString(row, 4),
        .biz_key = ReadRowString(row, 5),
        .result = ReadRowString(row, 6),
        .detail = ReadRowString(row, 7),
        .created_at = ReadRowString(row, 8),
    };
}

modules::ops::TaskLogRecord BuildTaskLogRecord(MYSQL_ROW row) {
    return modules::ops::TaskLogRecord{
        .task_log_id = ReadRowUint64(row, 0),
        .task_type = ReadRowString(row, 1),
        .biz_key = ReadRowString(row, 2),
        .task_status = ReadRowString(row, 3),
        .retry_count = ReadRowInt(row, 4),
        .last_error = ReadRowString(row, 5),
        .scheduled_at = ReadRowString(row, 6),
        .started_at = ReadRowString(row, 7),
        .finished_at = ReadRowString(row, 8),
        .created_at = ReadRowString(row, 9),
    };
}

std::vector<modules::ops::SystemExceptionRecord> ListNotificationExceptions(
    common::db::MysqlConnection& connection,
    const int limit
) {
    const auto result = ExecuteQuery(
        connection,
        "SELECT notification_id, COALESCE(CAST(biz_id AS CHAR), ''), push_status, notice_type, "
        "content, CAST(created_at AS CHAR) "
        "FROM notification WHERE push_status = 'FAILED' "
        "ORDER BY created_at DESC, notification_id DESC LIMIT " +
            std::to_string(limit)
    );

    std::vector<modules::ops::SystemExceptionRecord> records;
    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(result.get())) != nullptr) {
        records.push_back(modules::ops::SystemExceptionRecord{
            .source_type = std::string(modules::ops::kExceptionSourceNotification),
            .source_id = ReadRowString(row, 0),
            .biz_key = ReadRowString(row, 1),
            .current_status = ReadRowString(row, 2),
            .summary = "notification push failed: " + ReadRowString(row, 3),
            .detail = ReadRowString(row, 4),
            .occurred_at = ReadRowString(row, 5),
            .retryable = true,
        });
    }
    return records;
}

std::vector<modules::ops::SystemExceptionRecord> ListTaskExceptions(
    common::db::MysqlConnection& connection,
    const int limit
) {
    const auto result = ExecuteQuery(
        connection,
        "SELECT tl.task_log_id, tl.task_type, tl.biz_key, tl.task_status, "
        "COALESCE(tl.last_error, ''), CAST(tl.created_at AS CHAR) "
        "FROM task_log tl "
        "INNER JOIN ("
        "    SELECT task_type, biz_key, MAX(task_log_id) AS latest_task_log_id "
        "    FROM task_log GROUP BY task_type, biz_key"
        ") latest ON latest.latest_task_log_id = tl.task_log_id "
        "WHERE tl.task_status = 'FAILED' "
        "ORDER BY tl.created_at DESC, tl.task_log_id DESC LIMIT " +
            std::to_string(limit)
    );

    std::vector<modules::ops::SystemExceptionRecord> records;
    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(result.get())) != nullptr) {
        records.push_back(modules::ops::SystemExceptionRecord{
            .source_type = std::string(modules::ops::kExceptionSourceTaskLog),
            .source_id = ReadRowString(row, 0),
            .biz_key = ReadRowString(row, 2),
            .current_status = ReadRowString(row, 3),
            .summary = "task failed: " + ReadRowString(row, 1),
            .detail = ReadRowString(row, 4),
            .occurred_at = ReadRowString(row, 5),
            .retryable = true,
        });
    }
    return records;
}

std::vector<modules::ops::SystemExceptionRecord> ListPaymentCallbackExceptions(
    common::db::MysqlConnection& connection,
    const int limit
) {
    const auto result = ExecuteQuery(
        connection,
        "SELECT callback_log_id, COALESCE(CAST(order_id AS CHAR), COALESCE(transaction_no, '')), "
        "callback_status, COALESCE(process_result, ''), CAST(received_at AS CHAR) "
        "FROM payment_callback_log WHERE callback_status = 'REJECTED' "
        "ORDER BY received_at DESC, callback_log_id DESC LIMIT " +
            std::to_string(limit)
    );

    std::vector<modules::ops::SystemExceptionRecord> records;
    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(result.get())) != nullptr) {
        records.push_back(modules::ops::SystemExceptionRecord{
            .source_type = std::string(modules::ops::kExceptionSourcePaymentCallback),
            .source_id = ReadRowString(row, 0),
            .biz_key = ReadRowString(row, 1),
            .current_status = ReadRowString(row, 2),
            .summary = "payment callback rejected",
            .detail = ReadRowString(row, 3),
            .occurred_at = ReadRowString(row, 4),
            .retryable = true,
        });
    }
    return records;
}

std::vector<modules::ops::SystemExceptionRecord> ListManualExceptionMarks(
    common::db::MysqlConnection& connection,
    const int limit
) {
    const auto result = ExecuteQuery(
        connection,
        "SELECT operation_log_id, COALESCE(biz_key, ''), operation_name, COALESCE(detail, ''), "
        "CAST(created_at AS CHAR) "
        "FROM operation_log WHERE module_name = 'ops' "
        "AND operation_name LIKE 'mark_exception:%' "
        "ORDER BY created_at DESC, operation_log_id DESC LIMIT " +
            std::to_string(limit)
    );

    std::vector<modules::ops::SystemExceptionRecord> records;
    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(result.get())) != nullptr) {
        records.push_back(modules::ops::SystemExceptionRecord{
            .source_type = std::string(modules::ops::kExceptionSourceManualMark),
            .source_id = ReadRowString(row, 0),
            .biz_key = ReadRowString(row, 1),
            .current_status = "OPEN",
            .summary = ReadRowString(row, 2),
            .detail = ReadRowString(row, 3),
            .occurred_at = ReadRowString(row, 4),
            .retryable = false,
        });
    }
    return records;
}

}  // namespace

std::vector<modules::ops::OperationLogRecord> OpsRepository::ListOperationLogs(
    const modules::ops::OperationLogQuery& query
) const {
    std::string sql =
        "SELECT operation_log_id, operator_id, operator_role, module_name, operation_name, "
        "COALESCE(biz_key, ''), result, COALESCE(detail, ''), CAST(created_at AS CHAR) "
        "FROM operation_log WHERE 1 = 1";

    if (query.module_name.has_value() && !query.module_name->empty()) {
        sql += " AND module_name = '" + EscapeString(*query.module_name) + "'";
    }
    if (query.result.has_value() && !query.result->empty()) {
        sql += " AND result = '" + EscapeString(*query.result) + "'";
    }
    sql += " ORDER BY created_at DESC, operation_log_id DESC LIMIT " + std::to_string(query.limit);

    const auto result = ExecuteQuery(connection(), sql);
    std::vector<modules::ops::OperationLogRecord> records;
    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(result.get())) != nullptr) {
        records.push_back(BuildOperationLogRecord(row));
    }
    return records;
}

std::vector<modules::ops::TaskLogRecord> OpsRepository::ListTaskLogs(
    const modules::ops::TaskLogQuery& query
) const {
    std::string sql =
        "SELECT task_log_id, task_type, biz_key, task_status, retry_count, "
        "COALESCE(last_error, ''), COALESCE(CAST(scheduled_at AS CHAR), ''), "
        "COALESCE(CAST(started_at AS CHAR), ''), COALESCE(CAST(finished_at AS CHAR), ''), "
        "CAST(created_at AS CHAR) FROM task_log WHERE 1 = 1";

    if (query.task_type.has_value() && !query.task_type->empty()) {
        sql += " AND task_type = '" + EscapeString(*query.task_type) + "'";
    }
    if (query.task_status.has_value() && !query.task_status->empty()) {
        sql += " AND task_status = '" + EscapeString(*query.task_status) + "'";
    }
    sql += " ORDER BY created_at DESC, task_log_id DESC LIMIT " + std::to_string(query.limit);

    const auto result = ExecuteQuery(connection(), sql);
    std::vector<modules::ops::TaskLogRecord> records;
    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(result.get())) != nullptr) {
        records.push_back(BuildTaskLogRecord(row));
    }
    return records;
}

std::vector<modules::ops::SystemExceptionRecord> OpsRepository::ListSystemExceptions(
    const int limit
) const {
    auto notification_records = ListNotificationExceptions(connection(), limit);
    auto task_records = ListTaskExceptions(connection(), limit);
    auto payment_records = ListPaymentCallbackExceptions(connection(), limit);
    auto manual_records = ListManualExceptionMarks(connection(), limit);

    std::vector<modules::ops::SystemExceptionRecord> records;
    records.reserve(
        notification_records.size() + task_records.size() + payment_records.size() +
        manual_records.size()
    );

    records.insert(records.end(), notification_records.begin(), notification_records.end());
    records.insert(records.end(), task_records.begin(), task_records.end());
    records.insert(records.end(), payment_records.begin(), payment_records.end());
    records.insert(records.end(), manual_records.begin(), manual_records.end());

    std::sort(
        records.begin(),
        records.end(),
        [](const modules::ops::SystemExceptionRecord& left,
           const modules::ops::SystemExceptionRecord& right) {
            if (left.occurred_at == right.occurred_at) {
                return left.source_id > right.source_id;
            }
            return left.occurred_at > right.occurred_at;
        }
    );

    if (records.size() > static_cast<std::size_t>(limit)) {
        records.resize(static_cast<std::size_t>(limit));
    }
    return records;
}

void OpsRepository::InsertOperationLog(
    const std::optional<std::uint64_t>& operator_id,
    const std::string& operator_role,
    const std::string& operation_name,
    const std::string& biz_key,
    const std::string& result,
    const std::string& detail
) const {
    Execute(
        "INSERT INTO operation_log (operator_id, operator_role, module_name, operation_name, "
        "biz_key, result, detail) VALUES (" +
        (operator_id.has_value() ? std::to_string(*operator_id) : "NULL") + ", '" +
        EscapeString(operator_role) + "', 'ops', '" + EscapeString(operation_name) + "', " +
        (biz_key.empty() ? "NULL" : "'" + EscapeString(biz_key) + "'") + ", '" +
        EscapeString(result) + "', '" + EscapeString(detail) + "')"
    );
}

}  // namespace auction::repository
