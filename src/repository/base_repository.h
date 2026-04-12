#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "common/db/mysql_connection.h"

namespace auction::repository {

class BaseRepository {
public:
    explicit BaseRepository(common::db::MysqlConnection& connection) : connection_(&connection) {}

protected:
    void Execute(const std::string& sql) const {
        connection_->Execute(sql);
    }

    [[nodiscard]] std::uint64_t QueryCount(const std::string& sql) const {
        return connection_->QueryCount(sql);
    }

    [[nodiscard]] std::string QueryString(const std::string& sql) const {
        return connection_->QueryString(sql);
    }

    [[nodiscard]] std::string EscapeString(std::string_view value) const {
        return connection_->EscapeString(value);
    }

    [[nodiscard]] common::db::MysqlConnection& connection() const {
        return *connection_;
    }

private:
    common::db::MysqlConnection* connection_;
};

}  // namespace auction::repository
