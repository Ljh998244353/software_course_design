#include "repository/notification_repository.h"

#include <cstring>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>

#include <mysql.h>

namespace auction::repository {

namespace {

using MysqlResultPtr = std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)>;

class PreparedStatement {
public:
    PreparedStatement(MYSQL* handle, const std::string_view sql) : handle_(mysql_stmt_init(handle)) {
        if (handle_ == nullptr) {
            throw std::runtime_error("failed to initialize mysql statement");
        }

        if (mysql_stmt_prepare(handle_, sql.data(), sql.size()) != 0) {
            const auto error_message =
                std::string("failed to prepare statement: ") + mysql_stmt_error(handle_);
            mysql_stmt_close(handle_);
            handle_ = nullptr;
            throw std::runtime_error(error_message);
        }
    }

    ~PreparedStatement() {
        if (handle_ != nullptr) {
            mysql_stmt_close(handle_);
        }
    }

    PreparedStatement(const PreparedStatement&) = delete;
    PreparedStatement& operator=(const PreparedStatement&) = delete;

    void BindParams(MYSQL_BIND* binds) {
        if (mysql_stmt_bind_param(handle_, binds) != 0) {
            throw std::runtime_error(std::string("failed to bind params: ") + mysql_stmt_error(handle_));
        }
    }

    void Execute() {
        if (mysql_stmt_execute(handle_) != 0) {
            throw std::runtime_error(std::string("failed to execute statement: ") +
                                     mysql_stmt_error(handle_));
        }
    }

    [[nodiscard]] std::uint64_t InsertId() const {
        return mysql_stmt_insert_id(handle_);
    }

private:
    MYSQL_STMT* handle_{nullptr};
};

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

NotificationRecord BuildNotificationRecord(MYSQL_ROW row) {
    return NotificationRecord{
        .notification_id = ReadRowUint64(row, 0),
        .user_id = ReadRowUint64(row, 1),
        .notice_type = ReadRowString(row, 2),
        .title = ReadRowString(row, 3),
        .content = ReadRowString(row, 4),
        .biz_type = ReadRowString(row, 5),
        .biz_id = ReadRowOptionalUint64(row, 6),
        .read_status = ReadRowString(row, 7),
        .push_status = ReadRowString(row, 8),
        .created_at = ReadRowString(row, 9),
    };
}

void BindStringParam(
    MYSQL_BIND& bind,
    const std::string& value,
    unsigned long& length,
    bool& is_null
) {
    std::memset(&bind, 0, sizeof(bind));
    length = static_cast<unsigned long>(value.size());
    is_null = false;
    bind.buffer_type = MYSQL_TYPE_STRING;
    bind.buffer = const_cast<char*>(value.data());
    bind.buffer_length = length;
    bind.length = &length;
    bind.is_null = &is_null;
}

void BindNullableStringParam(
    MYSQL_BIND& bind,
    const std::string& value,
    unsigned long& length,
    bool& is_null
) {
    std::memset(&bind, 0, sizeof(bind));
    length = static_cast<unsigned long>(value.size());
    is_null = value.empty();
    bind.buffer_type = MYSQL_TYPE_STRING;
    bind.buffer = is_null ? nullptr : const_cast<char*>(value.data());
    bind.buffer_length = length;
    bind.length = &length;
    bind.is_null = &is_null;
}

void BindUint64Param(MYSQL_BIND& bind, std::uint64_t& value, bool& is_null) {
    std::memset(&bind, 0, sizeof(bind));
    is_null = false;
    bind.buffer_type = MYSQL_TYPE_LONGLONG;
    bind.buffer = &value;
    bind.is_unsigned = true;
    bind.is_null = &is_null;
}

void BindNullableUint64Param(
    MYSQL_BIND& bind,
    std::uint64_t& value,
    bool& is_null,
    const bool present
) {
    std::memset(&bind, 0, sizeof(bind));
    is_null = !present;
    bind.buffer_type = MYSQL_TYPE_LONGLONG;
    bind.buffer = present ? &value : nullptr;
    bind.is_unsigned = true;
    bind.is_null = &is_null;
}

void BindIntParam(MYSQL_BIND& bind, int& value, bool& is_null) {
    std::memset(&bind, 0, sizeof(bind));
    is_null = false;
    bind.buffer_type = MYSQL_TYPE_LONG;
    bind.buffer = &value;
    bind.is_null = &is_null;
}

std::optional<NotificationRecord> FindNotificationById(
    common::db::MysqlConnection& connection,
    const std::uint64_t notification_id
) {
    const auto result = ExecuteQuery(
        connection,
        "SELECT notification_id, user_id, notice_type, title, content, COALESCE(biz_type, ''), "
        "biz_id, read_status, push_status, CAST(created_at AS CHAR) "
        "FROM notification WHERE notification_id = " +
            std::to_string(notification_id) + " LIMIT 1"
    );

    if (MYSQL_ROW row = mysql_fetch_row(result.get()); row != nullptr) {
        return BuildNotificationRecord(row);
    }
    return std::nullopt;
}

}  // namespace

NotificationRecord NotificationRepository::CreateNotification(
    const CreateNotificationParams& params
) const {
    PreparedStatement statement(
        connection().native_handle(),
        "INSERT INTO notification (user_id, notice_type, title, content, biz_type, biz_id, "
        "read_status, push_status) VALUES (?, ?, ?, ?, ?, ?, ?, ?)"
    );

    auto user_id = params.user_id;
    auto biz_id = params.biz_id.value_or(0);
    const bool has_biz_id = params.biz_id.has_value();
    unsigned long notice_type_length = 0;
    unsigned long title_length = 0;
    unsigned long content_length = 0;
    unsigned long biz_type_length = 0;
    unsigned long read_status_length = 0;
    unsigned long push_status_length = 0;
    bool user_id_is_null = false;
    bool notice_type_is_null = false;
    bool title_is_null = false;
    bool content_is_null = false;
    bool biz_type_is_null = false;
    bool biz_id_is_null = false;
    bool read_status_is_null = false;
    bool push_status_is_null = false;
    MYSQL_BIND params_bind[8]{};
    BindUint64Param(params_bind[0], user_id, user_id_is_null);
    BindStringParam(params_bind[1], params.notice_type, notice_type_length, notice_type_is_null);
    BindStringParam(params_bind[2], params.title, title_length, title_is_null);
    BindStringParam(params_bind[3], params.content, content_length, content_is_null);
    BindStringParam(params_bind[4], params.biz_type, biz_type_length, biz_type_is_null);
    BindNullableUint64Param(params_bind[5], biz_id, biz_id_is_null, has_biz_id);
    BindStringParam(params_bind[6], params.read_status, read_status_length, read_status_is_null);
    BindStringParam(params_bind[7], params.push_status, push_status_length, push_status_is_null);
    statement.BindParams(params_bind);
    statement.Execute();

    const auto created = FindNotificationById(connection(), statement.InsertId());
    if (!created.has_value()) {
        throw std::runtime_error("created notification could not be loaded");
    }
    return *created;
}

void NotificationRepository::UpdatePushStatus(
    const std::uint64_t notification_id,
    const std::string& push_status
) const {
    PreparedStatement statement(
        connection().native_handle(),
        "UPDATE notification SET push_status = ? WHERE notification_id = ?"
    );

    auto mutable_notification_id = notification_id;
    unsigned long push_status_length = 0;
    bool push_status_is_null = false;
    bool notification_id_is_null = false;
    MYSQL_BIND params_bind[2]{};
    BindStringParam(params_bind[0], push_status, push_status_length, push_status_is_null);
    BindUint64Param(params_bind[1], mutable_notification_id, notification_id_is_null);
    statement.BindParams(params_bind);
    statement.Execute();
}

void NotificationRepository::InsertTaskLog(const NotificationTaskLogParams& params) const {
    PreparedStatement statement(
        connection().native_handle(),
        "INSERT INTO task_log (task_type, biz_key, task_status, retry_count, last_error, "
        "scheduled_at, started_at, finished_at) "
        "VALUES (?, ?, ?, ?, ?, ?, CURRENT_TIMESTAMP(3), CURRENT_TIMESTAMP(3))"
    );

    auto retry_count = params.retry_count;
    unsigned long task_type_length = 0;
    unsigned long biz_key_length = 0;
    unsigned long task_status_length = 0;
    unsigned long last_error_length = 0;
    unsigned long scheduled_at_length = 0;
    bool task_type_is_null = false;
    bool biz_key_is_null = false;
    bool task_status_is_null = false;
    bool retry_count_is_null = false;
    bool last_error_is_null = false;
    bool scheduled_at_is_null = false;
    MYSQL_BIND params_bind[6]{};
    BindStringParam(params_bind[0], params.task_type, task_type_length, task_type_is_null);
    BindStringParam(params_bind[1], params.biz_key, biz_key_length, biz_key_is_null);
    BindStringParam(
        params_bind[2],
        params.task_status,
        task_status_length,
        task_status_is_null
    );
    BindIntParam(params_bind[3], retry_count, retry_count_is_null);
    BindNullableStringParam(params_bind[4], params.last_error, last_error_length, last_error_is_null);
    BindNullableStringParam(
        params_bind[5],
        params.scheduled_at,
        scheduled_at_length,
        scheduled_at_is_null
    );
    statement.BindParams(params_bind);
    statement.Execute();
}

}  // namespace auction::repository
