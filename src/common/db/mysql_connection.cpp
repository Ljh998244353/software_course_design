#include "common/db/mysql_connection.h"

#include <memory>
#include <stdexcept>

namespace auction::common::db {

namespace {

using MysqlResultPtr = std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)>;

MysqlResultPtr MakeResultPtr(MYSQL_RES* result) {
    return MysqlResultPtr(result, &mysql_free_result);
}

}  // namespace

MysqlConnection::MysqlConnection(
    const config::MysqlConfig& config,
    const std::filesystem::path& project_root
) : handle_(mysql_init(nullptr)), database_(config.database) {
    if (handle_ == nullptr) {
        throw std::runtime_error("failed to initialize mysql handle");
    }

    mysql_options(handle_, MYSQL_OPT_CONNECT_TIMEOUT, &config.connect_timeout_seconds);

    const auto socket_path = config.ResolveSocketPath(project_root);
    const std::string resolved_socket = socket_path.empty() ? "" : socket_path.string();
    const char* host = resolved_socket.empty() ? config.host.c_str() : "localhost";
    const char* socket = resolved_socket.empty() ? nullptr : resolved_socket.c_str();
    const unsigned int port = resolved_socket.empty() ? config.port : 0U;

    if (mysql_real_connect(
            handle_,
            host,
            config.username.c_str(),
            config.password.c_str(),
            config.database.c_str(),
            port,
            socket,
            0
        ) == nullptr) {
        const auto error_message = FormatError("failed to connect mysql");
        Close();
        throw std::runtime_error(error_message);
    }

    if (!config.charset.empty() && mysql_set_character_set(handle_, config.charset.c_str()) != 0) {
        const auto error_message = FormatError("failed to set mysql charset");
        Close();
        throw std::runtime_error(error_message);
    }

    server_version_ = mysql_get_server_info(handle_) != nullptr ? mysql_get_server_info(handle_) : "";
    endpoint_ = resolved_socket.empty() ? config.host + ":" + std::to_string(config.port)
                                        : resolved_socket;
}

MysqlConnection::~MysqlConnection() {
    Close();
}

MysqlConnection::MysqlConnection(MysqlConnection&& other) noexcept
    : handle_(other.handle_),
      database_(std::move(other.database_)),
      server_version_(std::move(other.server_version_)),
      endpoint_(std::move(other.endpoint_)) {
    other.handle_ = nullptr;
}

MysqlConnection& MysqlConnection::operator=(MysqlConnection&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    Close();
    handle_ = other.handle_;
    database_ = std::move(other.database_);
    server_version_ = std::move(other.server_version_);
    endpoint_ = std::move(other.endpoint_);
    other.handle_ = nullptr;
    return *this;
}

void MysqlConnection::Execute(const std::string& sql) {
    if (mysql_query(handle_, sql.c_str()) != 0) {
        throw std::runtime_error(FormatError("mysql execute failed"));
    }

    MYSQL_RES* result = mysql_store_result(handle_);
    if (result != nullptr) {
        mysql_free_result(result);
        return;
    }

    if (mysql_field_count(handle_) != 0) {
        throw std::runtime_error(FormatError("mysql execute failed to read result"));
    }
}

std::uint64_t MysqlConnection::QueryCount(const std::string& sql) {
    const auto result = MakeResultPtr(ExecuteQuery(sql));
    MYSQL_ROW row = mysql_fetch_row(result.get());
    if (row == nullptr || row[0] == nullptr) {
        return 0;
    }
    return std::stoull(row[0]);
}

std::string MysqlConnection::QueryString(const std::string& sql) {
    const auto result = MakeResultPtr(ExecuteQuery(sql));
    MYSQL_ROW row = mysql_fetch_row(result.get());
    if (row == nullptr || row[0] == nullptr) {
        return {};
    }
    return row[0];
}

std::string MysqlConnection::EscapeString(std::string_view value) const {
    std::string escaped(value.size() * 2 + 1, '\0');
    const auto escaped_length = mysql_real_escape_string(
        handle_,
        escaped.data(),
        value.data(),
        static_cast<unsigned long>(value.size())
    );
    escaped.resize(escaped_length);
    return escaped;
}

void MysqlConnection::BeginTransaction() {
    Execute("START TRANSACTION");
}

void MysqlConnection::Commit() {
    Execute("COMMIT");
}

void MysqlConnection::Rollback() {
    Execute("ROLLBACK");
}

const std::string& MysqlConnection::database() const {
    return database_;
}

const std::string& MysqlConnection::server_version() const {
    return server_version_;
}

const std::string& MysqlConnection::endpoint() const {
    return endpoint_;
}

MYSQL* MysqlConnection::native_handle() const {
    return handle_;
}

std::string MysqlConnection::FormatError(const std::string& prefix) const {
    if (handle_ == nullptr) {
        return prefix + ": mysql handle is null";
    }
    return prefix + ": " + mysql_error(handle_);
}

MYSQL_RES* MysqlConnection::ExecuteQuery(const std::string& sql) {
    if (mysql_query(handle_, sql.c_str()) != 0) {
        throw std::runtime_error(FormatError("mysql query failed"));
    }

    MYSQL_RES* result = mysql_store_result(handle_);
    if (result == nullptr) {
        if (mysql_field_count(handle_) == 0) {
            throw std::runtime_error("mysql query did not return a result set");
        }
        throw std::runtime_error(FormatError("mysql query failed to read result"));
    }
    return result;
}

void MysqlConnection::Close() noexcept {
    if (handle_ != nullptr) {
        mysql_close(handle_);
        handle_ = nullptr;
    }
}

}  // namespace auction::common::db
