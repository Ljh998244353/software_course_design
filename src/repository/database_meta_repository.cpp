#include "repository/database_meta_repository.h"

#include <array>
#include <sstream>

namespace auction::repository {

namespace {

constexpr std::array<const char*, 15> kExpectedTables = {
    "user_account",
    "item_category",
    "item",
    "item_image",
    "item_audit_log",
    "auction",
    "bid_record",
    "order_info",
    "payment_record",
    "payment_callback_log",
    "review",
    "notification",
    "statistics_daily",
    "task_log",
    "operation_log",
};

std::string BuildExpectedTablesSqlList() {
    std::ostringstream sql;
    for (std::size_t index = 0; index < kExpectedTables.size(); ++index) {
        if (index > 0) {
            sql << ", ";
        }
        sql << '\'' << kExpectedTables[index] << '\'';
    }
    return sql.str();
}

}  // namespace

DatabaseMetaSnapshot DatabaseMetaRepository::LoadSnapshot(const std::string& database_name) const {
    DatabaseMetaSnapshot snapshot;
    snapshot.expected_table_count = kExpectedTables.size();
    snapshot.server_version = connection().server_version();

    const auto escaped_database = EscapeString(database_name);
    const auto expected_tables = BuildExpectedTablesSqlList();

    snapshot.table_count = QueryCount(
        "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema = '" + escaped_database +
        "'"
    );
    snapshot.required_table_count = QueryCount(
        "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema = '" + escaped_database +
        "' AND table_name IN (" + expected_tables + ")"
    );

    if (snapshot.required_table_count != snapshot.expected_table_count) {
        return snapshot;
    }

    snapshot.admin_user_count = QueryCount(
        "SELECT COUNT(*) FROM user_account WHERE role_code = 'ADMIN'"
    );
    snapshot.category_count = QueryCount("SELECT COUNT(*) FROM item_category");
    return snapshot;
}

}  // namespace auction::repository
