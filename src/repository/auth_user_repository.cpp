#include "repository/auth_user_repository.h"

#include <array>
#include <cstring>
#include <stdexcept>
#include <string_view>

#include <mysql.h>

#include "common/errors/error_code.h"
#include "modules/auth/auth_exception.h"

namespace auction::repository {

namespace {

class PreparedStatement {
public:
    PreparedStatement(MYSQL* handle, const std::string_view sql) : handle_(mysql_stmt_init(handle)) {
        if (handle_ == nullptr) {
            throw std::runtime_error("failed to initialize mysql statement");
        }

        if (mysql_stmt_prepare(handle_, sql.data(), sql.size()) != 0) {
            const auto error_message = std::string("failed to prepare statement: ") +
                                       mysql_stmt_error(handle_);
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

    void BindParams(MYSQL_BIND* binds, const unsigned long count) {
        if (mysql_stmt_bind_param(handle_, binds) != 0) {
            throw std::runtime_error(std::string("failed to bind params: ") + mysql_stmt_error(handle_));
        }
        param_count_ = count;
    }

    void Execute() {
        if (mysql_stmt_execute(handle_) != 0) {
            throw std::runtime_error(std::string("failed to execute statement: ") +
                                     mysql_stmt_error(handle_));
        }
    }

    void StoreResult() {
        if (mysql_stmt_store_result(handle_) != 0) {
            throw std::runtime_error(std::string("failed to store result: ") + mysql_stmt_error(handle_));
        }
    }

    void BindResults(MYSQL_BIND* binds) {
        if (mysql_stmt_bind_result(handle_, binds) != 0) {
            throw std::runtime_error(std::string("failed to bind results: ") + mysql_stmt_error(handle_));
        }
    }

    [[nodiscard]] bool Fetch() {
        const int fetch_result = mysql_stmt_fetch(handle_);
        if (fetch_result == 0 || fetch_result == MYSQL_DATA_TRUNCATED) {
            return true;
        }
        if (fetch_result == MYSQL_NO_DATA) {
            return false;
        }
        throw std::runtime_error(std::string("failed to fetch row: ") + mysql_stmt_error(handle_));
    }

    [[nodiscard]] std::uint64_t InsertId() const {
        return mysql_stmt_insert_id(handle_);
    }

    [[nodiscard]] std::uint64_t AffectedRows() const {
        return mysql_stmt_affected_rows(handle_);
    }

    [[nodiscard]] unsigned int ErrorCode() const {
        return mysql_stmt_errno(handle_);
    }

private:
    MYSQL_STMT* handle_{nullptr};
    unsigned long param_count_{0};
};

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

void BindCountResult(
    MYSQL_BIND& bind,
    std::uint64_t& value,
    unsigned long& length,
    bool& is_null
) {
    std::memset(&bind, 0, sizeof(bind));
    value = 0;
    length = 0;
    is_null = false;
    bind.buffer_type = MYSQL_TYPE_LONGLONG;
    bind.buffer = &value;
    bind.buffer_length = sizeof(value);
    bind.is_unsigned = true;
    bind.length = &length;
    bind.is_null = &is_null;
}

void BindStringResult(
    MYSQL_BIND& bind,
    char* buffer,
    const std::size_t buffer_size,
    unsigned long& length,
    bool& is_null
) {
    std::memset(&bind, 0, sizeof(bind));
    std::memset(buffer, 0, buffer_size);
    length = 0;
    is_null = false;
    bind.buffer_type = MYSQL_TYPE_STRING;
    bind.buffer = buffer;
    bind.buffer_length = buffer_size;
    bind.length = &length;
    bind.is_null = &is_null;
}

std::optional<modules::auth::AuthUserRecord> FetchSingleUserRow(PreparedStatement& statement) {
    std::uint64_t user_id = 0;
    unsigned long user_id_length = 0;
    bool user_id_is_null = false;
    unsigned long username_length = 0;
    unsigned long password_hash_length = 0;
    unsigned long phone_length = 0;
    unsigned long email_length = 0;
    unsigned long role_length = 0;
    unsigned long status_length = 0;
    unsigned long nickname_length = 0;
    bool username_is_null = false;
    bool password_hash_is_null = false;
    bool phone_is_null = false;
    bool email_is_null = false;
    bool role_is_null = false;
    bool status_is_null = false;
    bool nickname_is_null = false;
    char username[64]{};
    char password_hash[256]{};
    char phone[32]{};
    char email[128]{};
    char role[32]{};
    char status[32]{};
    char nickname[64]{};

    MYSQL_BIND binds[8]{};
    std::memset(&binds[0], 0, sizeof(binds[0]));
    binds[0].buffer_type = MYSQL_TYPE_LONGLONG;
    binds[0].buffer = &user_id;
    binds[0].buffer_length = sizeof(user_id);
    binds[0].is_unsigned = true;
    binds[0].length = &user_id_length;
    binds[0].is_null = &user_id_is_null;

    BindStringResult(binds[1], username, sizeof(username), username_length, username_is_null);
    BindStringResult(
        binds[2],
        password_hash,
        sizeof(password_hash),
        password_hash_length,
        password_hash_is_null
    );
    BindStringResult(binds[3], phone, sizeof(phone), phone_length, phone_is_null);
    BindStringResult(binds[4], email, sizeof(email), email_length, email_is_null);
    BindStringResult(binds[5], role, sizeof(role), role_length, role_is_null);
    BindStringResult(binds[6], status, sizeof(status), status_length, status_is_null);
    BindStringResult(binds[7], nickname, sizeof(nickname), nickname_length, nickname_is_null);

    statement.BindResults(binds);
    if (!statement.Fetch()) {
        return std::nullopt;
    }

    return modules::auth::AuthUserRecord{
        .user_id = user_id,
        .username = std::string(username, username_length),
        .password_hash = std::string(password_hash, password_hash_length),
        .phone = std::string(phone, phone_length),
        .email = std::string(email, email_length),
        .role_code = std::string(role, role_length),
        .status = std::string(status, status_length),
        .nickname = std::string(nickname, nickname_length),
    };
}

bool ExistsByField(
    common::db::MysqlConnection& connection,
    const std::string_view field_name,
    const std::string& value
) {
    PreparedStatement statement(
        connection.native_handle(),
        "SELECT COUNT(*) FROM user_account WHERE " + std::string(field_name) + " = ?"
    );

    unsigned long value_length = 0;
    bool value_is_null = false;
    MYSQL_BIND params[1]{};
    BindStringParam(params[0], value, value_length, value_is_null);
    statement.BindParams(params, 1);
    statement.Execute();
    statement.StoreResult();

    std::uint64_t count = 0;
    unsigned long count_length = 0;
    bool count_is_null = false;
    MYSQL_BIND results[1]{};
    BindCountResult(results[0], count, count_length, count_is_null);
    statement.BindResults(results);
    if (!statement.Fetch()) {
        return false;
    }
    return count > 0;
}

std::optional<modules::auth::AuthUserRecord> FindSingleUserByField(
    common::db::MysqlConnection& connection,
    const std::string_view field_name,
    const std::string& value
) {
    PreparedStatement statement(
        connection.native_handle(),
        "SELECT user_id, username, password_hash, COALESCE(phone, ''), COALESCE(email, ''), "
        "role_code, status, COALESCE(nickname, '') "
        "FROM user_account WHERE " +
            std::string(field_name) + " = ? LIMIT 1"
    );

    unsigned long value_length = 0;
    bool value_is_null = false;
    MYSQL_BIND params[1]{};
    BindStringParam(params[0], value, value_length, value_is_null);
    statement.BindParams(params, 1);
    statement.Execute();
    statement.StoreResult();
    return FetchSingleUserRow(statement);
}

}  // namespace

bool AuthUserRepository::ExistsByUsername(const std::string& username) const {
    return ExistsByField(connection(), "username", username);
}

bool AuthUserRepository::ExistsByPhone(const std::string& phone) const {
    return ExistsByField(connection(), "phone", phone);
}

bool AuthUserRepository::ExistsByEmail(const std::string& email) const {
    return ExistsByField(connection(), "email", email);
}

std::optional<modules::auth::AuthUserRecord> AuthUserRepository::FindByUsername(
    const std::string& username
) const {
    return FindSingleUserByField(connection(), "username", username);
}

std::optional<modules::auth::AuthUserRecord> AuthUserRepository::FindByPhone(
    const std::string& phone
) const {
    return FindSingleUserByField(connection(), "phone", phone);
}

std::optional<modules::auth::AuthUserRecord> AuthUserRepository::FindByEmail(
    const std::string& email
) const {
    return FindSingleUserByField(connection(), "email", email);
}

std::optional<modules::auth::AuthUserRecord> AuthUserRepository::FindByUserId(
    std::uint64_t user_id
) const {
    PreparedStatement statement(
        connection().native_handle(),
        "SELECT user_id, username, password_hash, COALESCE(phone, ''), COALESCE(email, ''), "
        "role_code, status, COALESCE(nickname, '') "
        "FROM user_account WHERE user_id = ? LIMIT 1"
    );

    bool user_id_is_null = false;
    MYSQL_BIND params[1]{};
    BindUint64Param(params[0], user_id, user_id_is_null);
    statement.BindParams(params, 1);
    statement.Execute();
    statement.StoreResult();
    return FetchSingleUserRow(statement);
}

modules::auth::AuthUserRecord AuthUserRepository::CreateUser(const CreateAuthUserParams& params) const {
    PreparedStatement statement(
        connection().native_handle(),
        "INSERT INTO user_account (username, password_hash, phone, email, role_code, status, "
        "nickname) VALUES (?, ?, ?, ?, ?, ?, ?)"
    );

    unsigned long username_length = 0;
    unsigned long password_hash_length = 0;
    unsigned long phone_length = 0;
    unsigned long email_length = 0;
    unsigned long role_length = 0;
    unsigned long status_length = 0;
    unsigned long nickname_length = 0;
    bool username_is_null = false;
    bool password_hash_is_null = false;
    bool phone_is_null = false;
    bool email_is_null = false;
    bool role_is_null = false;
    bool status_is_null = false;
    bool nickname_is_null = false;
    MYSQL_BIND params_bind[7]{};
    BindStringParam(params_bind[0], params.username, username_length, username_is_null);
    BindStringParam(
        params_bind[1],
        params.password_hash,
        password_hash_length,
        password_hash_is_null
    );
    BindNullableStringParam(params_bind[2], params.phone, phone_length, phone_is_null);
    BindNullableStringParam(params_bind[3], params.email, email_length, email_is_null);
    BindStringParam(params_bind[4], params.role_code, role_length, role_is_null);
    BindStringParam(params_bind[5], params.status, status_length, status_is_null);
    BindNullableStringParam(params_bind[6], params.nickname, nickname_length, nickname_is_null);
    statement.BindParams(params_bind, 7);

    try {
        statement.Execute();
    } catch (const std::runtime_error&) {
        if (statement.ErrorCode() == 1062) {
            throw modules::auth::AuthException(
                common::errors::ErrorCode::kAuthIdentityAlreadyExists,
                "username, phone or email already exists"
            );
        }
        throw;
    }

    const auto user = FindByUserId(statement.InsertId());
    if (!user.has_value()) {
        throw std::runtime_error("created user could not be loaded");
    }
    return *user;
}

void AuthUserRepository::UpdateLastLoginAt(const std::uint64_t user_id) const {
    PreparedStatement statement(
        connection().native_handle(),
        "UPDATE user_account SET last_login_at = CURRENT_TIMESTAMP(3), "
        "updated_at = CURRENT_TIMESTAMP(3) WHERE user_id = ?"
    );

    auto mutable_user_id = user_id;
    bool user_id_is_null = false;
    MYSQL_BIND params[1]{};
    BindUint64Param(params[0], mutable_user_id, user_id_is_null);
    statement.BindParams(params, 1);
    statement.Execute();
}

void AuthUserRepository::UpdateUserStatus(
    std::uint64_t user_id,
    const std::string& new_status
) const {
    PreparedStatement statement(
        connection().native_handle(),
        "UPDATE user_account SET status = ?, updated_at = CURRENT_TIMESTAMP(3) WHERE user_id = ?"
    );

    unsigned long status_length = 0;
    bool status_is_null = false;
    bool user_id_is_null = false;
    MYSQL_BIND params[2]{};
    BindStringParam(params[0], new_status, status_length, status_is_null);
    BindUint64Param(params[1], user_id, user_id_is_null);
    statement.BindParams(params, 2);
    statement.Execute();
}

void AuthUserRepository::InsertOperationLog(
    std::uint64_t operator_id,
    const std::string& operator_role,
    const std::string& operation_name,
    const std::string& biz_key,
    const std::string& result,
    const std::string& detail
) const {
    PreparedStatement statement(
        connection().native_handle(),
        "INSERT INTO operation_log (operator_id, operator_role, module_name, operation_name, "
        "biz_key, result, detail) VALUES (?, ?, 'auth', ?, ?, ?, ?)"
    );

    bool operator_id_is_null = false;
    unsigned long role_length = 0;
    unsigned long operation_length = 0;
    unsigned long biz_key_length = 0;
    unsigned long result_length = 0;
    unsigned long detail_length = 0;
    bool role_is_null = false;
    bool operation_is_null = false;
    bool biz_key_is_null = false;
    bool result_is_null = false;
    bool detail_is_null = false;
    MYSQL_BIND params[6]{};
    BindUint64Param(params[0], operator_id, operator_id_is_null);
    BindStringParam(params[1], operator_role, role_length, role_is_null);
    BindStringParam(params[2], operation_name, operation_length, operation_is_null);
    BindStringParam(params[3], biz_key, biz_key_length, biz_key_is_null);
    BindStringParam(params[4], result, result_length, result_is_null);
    BindStringParam(params[5], detail, detail_length, detail_is_null);
    statement.BindParams(params, 6);
    statement.Execute();
}

}  // namespace auction::repository
