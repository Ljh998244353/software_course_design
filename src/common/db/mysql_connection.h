#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

#include <mysql.h>

#include "common/config/app_config.h"

namespace auction::common::db {

class MysqlConnection {
public:
    MysqlConnection(
        const config::MysqlConfig& config,
        const std::filesystem::path& project_root
    );
    ~MysqlConnection();

    MysqlConnection(const MysqlConnection&) = delete;
    MysqlConnection& operator=(const MysqlConnection&) = delete;
    MysqlConnection(MysqlConnection&& other) noexcept;
    MysqlConnection& operator=(MysqlConnection&& other) noexcept;

    void Execute(const std::string& sql);
    [[nodiscard]] std::uint64_t QueryCount(const std::string& sql);
    [[nodiscard]] std::string QueryString(const std::string& sql);
    [[nodiscard]] std::string EscapeString(std::string_view value) const;

    void BeginTransaction();
    void Commit();
    void Rollback();

    [[nodiscard]] const std::string& database() const;
    [[nodiscard]] const std::string& server_version() const;
    [[nodiscard]] const std::string& endpoint() const;
    [[nodiscard]] MYSQL* native_handle() const;

private:
    [[nodiscard]] std::string FormatError(const std::string& prefix) const;
    [[nodiscard]] MYSQL_RES* ExecuteQuery(const std::string& sql);
    void Close() noexcept;

    MYSQL* handle_{nullptr};
    std::string database_;
    std::string server_version_;
    std::string endpoint_;
};

}  // namespace auction::common::db
