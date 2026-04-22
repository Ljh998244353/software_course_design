#include "repository/statistics_repository.h"

#include <cmath>
#include <iomanip>
#include <memory>
#include <optional>
#include <sstream>
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

double ReadRowDouble(MYSQL_ROW row, const unsigned int index) {
    return row[index] == nullptr ? 0.0 : std::stod(row[index]);
}

std::string FormatAmount(const double value) {
    std::ostringstream output;
    output << std::fixed << std::setprecision(2) << (std::round(value * 100.0) / 100.0);
    return output.str();
}

modules::statistics::DailyStatisticsRecord BuildDailyStatisticsRecord(
    MYSQL_ROW row,
    const unsigned int offset
) {
    return modules::statistics::DailyStatisticsRecord{
        .stat_id = ReadRowUint64(row, offset + 0),
        .stat_date = ReadRowString(row, offset + 1),
        .auction_count = static_cast<int>(ReadRowUint64(row, offset + 2)),
        .sold_count = static_cast<int>(ReadRowUint64(row, offset + 3)),
        .unsold_count = static_cast<int>(ReadRowUint64(row, offset + 4)),
        .bid_count = static_cast<int>(ReadRowUint64(row, offset + 5)),
        .gmv_amount = ReadRowDouble(row, offset + 6),
        .created_at = ReadRowString(row, offset + 7),
        .updated_at = ReadRowString(row, offset + 8),
    };
}

}  // namespace

DailyStatisticsAggregate StatisticsRepository::AggregateDailyStatistics(
    const std::string& stat_date
) const {
    const auto escaped_date = EscapeString(stat_date);
    const auto ended_auction_filter =
        "DATE(a.end_time) = '" + escaped_date + "' AND a.end_time <= CURRENT_TIMESTAMP(3)";

    DailyStatisticsAggregate aggregate;
    aggregate.stat_date = stat_date;
    aggregate.auction_count = static_cast<int>(QueryCount(
        "SELECT COUNT(*) FROM auction a WHERE " + ended_auction_filter
    ));
    aggregate.sold_count = static_cast<int>(QueryCount(
        "SELECT COUNT(*) FROM order_info o "
        "INNER JOIN auction a ON a.auction_id = o.auction_id "
        "WHERE " +
        ended_auction_filter +
        " AND o.order_status <> 'CLOSED' "
        "AND EXISTS (SELECT 1 FROM payment_record p WHERE p.order_id = o.order_id "
        "AND p.pay_status = 'SUCCESS')"
    ));
    aggregate.unsold_count = static_cast<int>(QueryCount(
        "SELECT COUNT(*) FROM auction a "
        "LEFT JOIN order_info o ON o.auction_id = a.auction_id "
        "WHERE " +
        ended_auction_filter +
        " AND (a.status = 'UNSOLD' OR (o.order_id IS NOT NULL AND o.order_status = 'CLOSED'))"
    ));
    aggregate.bid_count = static_cast<int>(QueryCount(
        "SELECT COUNT(*) FROM bid_record WHERE DATE(bid_time) = '" + escaped_date + "'"
    ));
    aggregate.gmv_amount = std::stod(QueryString(
        "SELECT COALESCE(CAST(SUM(o.final_amount) AS CHAR), '0') FROM order_info o "
        "INNER JOIN auction a ON a.auction_id = o.auction_id "
        "WHERE " +
        ended_auction_filter +
        " AND o.order_status <> 'CLOSED' "
        "AND EXISTS (SELECT 1 FROM payment_record p WHERE p.order_id = o.order_id "
        "AND p.pay_status = 'SUCCESS')"
    ));
    return aggregate;
}

std::optional<modules::statistics::DailyStatisticsRecord>
StatisticsRepository::FindDailyStatisticsByDate(const std::string& stat_date) const {
    const auto result = ExecuteQuery(
        connection(),
        "SELECT stat_id, CAST(stat_date AS CHAR), auction_count, sold_count, unsold_count, "
        "bid_count, CAST(gmv_amount AS CHAR), CAST(created_at AS CHAR), CAST(updated_at AS CHAR) "
        "FROM statistics_daily WHERE stat_date = '" +
            EscapeString(stat_date) + "' LIMIT 1"
    );

    if (MYSQL_ROW row = mysql_fetch_row(result.get()); row != nullptr) {
        return BuildDailyStatisticsRecord(row, 0);
    }
    return std::nullopt;
}

modules::statistics::DailyStatisticsRecord StatisticsRepository::UpsertDailyStatistics(
    const DailyStatisticsAggregate& aggregate
) const {
    Execute(
        "INSERT INTO statistics_daily (stat_date, auction_count, sold_count, unsold_count, "
        "bid_count, gmv_amount) VALUES ('" +
        EscapeString(aggregate.stat_date) + "', " + std::to_string(aggregate.auction_count) +
        ", " + std::to_string(aggregate.sold_count) + ", " +
        std::to_string(aggregate.unsold_count) + ", " + std::to_string(aggregate.bid_count) +
        ", " + FormatAmount(aggregate.gmv_amount) +
        ") ON DUPLICATE KEY UPDATE auction_count = VALUES(auction_count), "
        "sold_count = VALUES(sold_count), unsold_count = VALUES(unsold_count), "
        "bid_count = VALUES(bid_count), gmv_amount = VALUES(gmv_amount), "
        "updated_at = CURRENT_TIMESTAMP(3)"
    );

    const auto saved = FindDailyStatisticsByDate(aggregate.stat_date);
    if (!saved.has_value()) {
        throw std::runtime_error("saved statistics daily row could not be loaded");
    }
    return *saved;
}

std::vector<modules::statistics::DailyStatisticsRecord> StatisticsRepository::ListDailyStatistics(
    const std::string& start_date,
    const std::string& end_date
) const {
    const auto result = ExecuteQuery(
        connection(),
        "SELECT stat_id, CAST(stat_date AS CHAR), auction_count, sold_count, unsold_count, "
        "bid_count, CAST(gmv_amount AS CHAR), CAST(created_at AS CHAR), CAST(updated_at AS CHAR) "
        "FROM statistics_daily WHERE stat_date >= '" +
            EscapeString(start_date) + "' AND stat_date <= '" + EscapeString(end_date) +
            "' ORDER BY stat_date ASC, stat_id ASC"
    );

    std::vector<modules::statistics::DailyStatisticsRecord> records;
    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(result.get())) != nullptr) {
        records.push_back(BuildDailyStatisticsRecord(row, 0));
    }
    return records;
}

void StatisticsRepository::InsertTaskLog(const StatisticsTaskLogParams& params) const {
    Execute(
        "INSERT INTO task_log (task_type, biz_key, task_status, retry_count, last_error, "
        "scheduled_at, started_at, finished_at) VALUES ('" +
        EscapeString(params.task_type) + "', '" + EscapeString(params.biz_key) + "', '" +
        EscapeString(params.task_status) + "', " + std::to_string(params.retry_count) + ", " +
        (params.last_error.empty() ? "NULL" : "'" + EscapeString(params.last_error) + "'") +
        ", " +
        (params.scheduled_at.empty() ? "NULL" : "'" + EscapeString(params.scheduled_at) + "'") +
        ", CURRENT_TIMESTAMP(3), CURRENT_TIMESTAMP(3))"
    );
}

void StatisticsRepository::InsertOperationLog(
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
        EscapeString(operator_role) + "', 'statistics', '" + EscapeString(operation_name) +
        "', '" + EscapeString(biz_key) + "', '" + EscapeString(result) + "', '" +
        EscapeString(detail) + "')"
    );
}

}  // namespace auction::repository
