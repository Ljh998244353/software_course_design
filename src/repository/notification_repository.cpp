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

double ReadRowDouble(MYSQL_ROW row, const unsigned int index) {
    return row[index] == nullptr ? 0.0 : std::stod(row[index]);
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
        .read_at = ReadRowString(row, 10),
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
        "biz_id, read_status, push_status, CAST(created_at AS CHAR), "
        "COALESCE(CAST(read_at AS CHAR), '') "
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

    const auto created =
        ::auction::repository::FindNotificationById(connection(), statement.InsertId());
    if (!created.has_value()) {
        throw std::runtime_error("created notification could not be loaded");
    }
    return *created;
}

std::optional<NotificationRecord> NotificationRepository::FindNotificationById(
    const std::uint64_t notification_id
) const {
    return ::auction::repository::FindNotificationById(connection(), notification_id);
}

std::optional<NotificationRecord> NotificationRepository::FindNotificationByIdAndUser(
    const std::uint64_t notification_id,
    const std::uint64_t user_id
) const {
    const auto result = ExecuteQuery(
        connection(),
        "SELECT notification_id, user_id, notice_type, title, content, COALESCE(biz_type, ''), "
        "biz_id, read_status, push_status, CAST(created_at AS CHAR), "
        "COALESCE(CAST(read_at AS CHAR), '') "
        "FROM notification WHERE notification_id = " +
            std::to_string(notification_id) + " AND user_id = " + std::to_string(user_id) +
            " LIMIT 1"
    );

    if (MYSQL_ROW row = mysql_fetch_row(result.get()); row != nullptr) {
        return BuildNotificationRecord(row);
    }
    return std::nullopt;
}

std::vector<NotificationRecord> NotificationRepository::ListUserNotifications(
    const std::uint64_t user_id,
    const int limit,
    const bool unread_only,
    const std::string& biz_type,
    const std::optional<std::uint64_t> biz_id
) const {
    std::string where_clause = "WHERE user_id = " + std::to_string(user_id);
    if (unread_only) {
        where_clause += " AND read_status = 'UNREAD'";
    }
    if (!biz_type.empty()) {
        where_clause += " AND biz_type = '" + EscapeString(biz_type) + "'";
    }
    if (biz_id.has_value()) {
        where_clause += " AND biz_id = " + std::to_string(*biz_id);
    }

    const auto result = ExecuteQuery(
        connection(),
        "SELECT notification_id, user_id, notice_type, title, content, COALESCE(biz_type, ''), "
        "biz_id, read_status, push_status, CAST(created_at AS CHAR), "
        "COALESCE(CAST(read_at AS CHAR), '') "
        "FROM notification " +
            where_clause + " ORDER BY created_at DESC, notification_id DESC LIMIT " +
            std::to_string(limit)
    );

    std::vector<NotificationRecord> records;
    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(result.get())) != nullptr) {
        records.push_back(BuildNotificationRecord(row));
    }
    return records;
}

std::vector<std::uint64_t> NotificationRepository::ListFailedNotificationIds(const int limit) const {
    const auto result = ExecuteQuery(
        connection(),
        "SELECT notification_id FROM notification WHERE push_status = 'FAILED' "
        "ORDER BY created_at ASC, notification_id ASC LIMIT " +
            std::to_string(limit)
    );

    std::vector<std::uint64_t> notification_ids;
    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(result.get())) != nullptr) {
        notification_ids.push_back(ReadRowUint64(row, 0));
    }
    return notification_ids;
}

int NotificationRepository::QueryMaxTaskRetryCount(
    const std::string& task_type,
    const std::string& biz_key
) const {
    return std::stoi(QueryString(
        "SELECT COALESCE(CAST(MAX(retry_count) AS CHAR), '0') FROM task_log "
        "WHERE task_type = '" +
        EscapeString(task_type) + "' AND biz_key = '" + EscapeString(biz_key) + "'"
    ));
}

std::optional<NotificationRetryCandidate> NotificationRepository::FindAuctionRetryCandidate(
    const std::uint64_t notification_id
) const {
    const auto result = ExecuteQuery(
        connection(),
        "SELECT n.notification_id, n.user_id, n.notice_type, n.title, n.content, "
        "COALESCE(n.biz_type, ''), n.biz_id, n.read_status, n.push_status, "
        "CAST(n.created_at AS CHAR), COALESCE(CAST(n.read_at AS CHAR), ''), "
        "COALESCE(a.auction_id, 0), "
        "COALESCE(CAST(a.current_price AS CHAR), '0'), COALESCE(u.username, ''), "
        "COALESCE(CAST(a.end_time AS CHAR), '') "
        "FROM notification n "
        "LEFT JOIN auction a ON n.biz_type = 'AUCTION' AND n.biz_id = a.auction_id "
        "LEFT JOIN user_account u ON a.highest_bidder_id = u.user_id "
        "WHERE n.notification_id = " +
            std::to_string(notification_id) + " LIMIT 1"
    );

    if (MYSQL_ROW row = mysql_fetch_row(result.get()); row != nullptr) {
        return NotificationRetryCandidate{
            .notification = BuildNotificationRecord(row),
            .auction_id = ReadRowUint64(row, 11),
            .current_price = ReadRowDouble(row, 12),
            .highest_bidder_username = ReadRowString(row, 13),
            .end_time = ReadRowString(row, 14),
        };
    }
    return std::nullopt;
}

void NotificationRepository::MarkNotificationRead(
    const std::uint64_t notification_id,
    const std::uint64_t user_id
) const {
    PreparedStatement statement(
        connection().native_handle(),
        "UPDATE notification SET read_status = 'READ', read_at = CURRENT_TIMESTAMP(3) "
        "WHERE notification_id = ? AND user_id = ?"
    );

    auto mutable_notification_id = notification_id;
    auto mutable_user_id = user_id;
    bool notification_id_is_null = false;
    bool user_id_is_null = false;
    MYSQL_BIND params_bind[2]{};
    BindUint64Param(params_bind[0], mutable_notification_id, notification_id_is_null);
    BindUint64Param(params_bind[1], mutable_user_id, user_id_is_null);
    statement.BindParams(params_bind);
    statement.Execute();
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
