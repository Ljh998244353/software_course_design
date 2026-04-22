#include "modules/statistics/statistics_service.h"

#include <cctype>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

#include "common/errors/error_code.h"
#include "modules/auth/auth_types.h"
#include "modules/statistics/statistics_exception.h"
#include "repository/statistics_repository.h"

namespace auction::modules::statistics {

namespace {

void ThrowStatisticsError(const common::errors::ErrorCode code, const std::string_view message) {
    throw StatisticsException(code, std::string(message));
}

repository::StatisticsRepository MakeRepository(common::db::MysqlConnection& connection) {
    return repository::StatisticsRepository(connection);
}

void InsertTaskLogSafely(
    repository::StatisticsRepository& repository,
    const repository::StatisticsTaskLogParams& params
) {
    try {
        repository.InsertTaskLog(params);
    } catch (...) {
    }
}

void InsertOperationLogSafely(
    repository::StatisticsRepository& repository,
    const std::optional<std::uint64_t>& operator_id,
    const std::string& operator_role,
    const std::string& operation_name,
    const std::string& biz_key,
    const std::string& result,
    const std::string& detail
) {
    try {
        repository.InsertOperationLog(
            operator_id,
            operator_role,
            operation_name,
            biz_key,
            result,
            detail
        );
    } catch (...) {
    }
}

bool IsLeapYear(const int year) {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

int DaysInMonth(const int year, const int month) {
    switch (month) {
        case 1:
        case 3:
        case 5:
        case 7:
        case 8:
        case 10:
        case 12:
            return 31;
        case 4:
        case 6:
        case 9:
        case 11:
            return 30;
        case 2:
            return IsLeapYear(year) ? 29 : 28;
        default:
            return 0;
    }
}

bool IsValidDateText(const std::string& value) {
    if (value.size() != 10 || value[4] != '-' || value[7] != '-') {
        return false;
    }

    for (std::size_t index = 0; index < value.size(); ++index) {
        if (index == 4 || index == 7) {
            continue;
        }
        if (std::isdigit(static_cast<unsigned char>(value[index])) == 0) {
            return false;
        }
    }

    const auto year = std::stoi(value.substr(0, 4));
    const auto month = std::stoi(value.substr(5, 2));
    const auto day = std::stoi(value.substr(8, 2));

    if (year < 2000 || month < 1 || month > 12) {
        return false;
    }

    const auto max_day = DaysInMonth(year, month);
    return day >= 1 && day <= max_day;
}

void ValidateStatDate(const std::string& stat_date) {
    if (!IsValidDateText(stat_date)) {
        ThrowStatisticsError(
            common::errors::ErrorCode::kStatisticsDateInvalid,
            "statistics date must use YYYY-MM-DD"
        );
    }
}

void ValidateQuery(const DailyStatisticsQuery& query) {
    ValidateStatDate(query.start_date);
    ValidateStatDate(query.end_date);
    if (query.start_date > query.end_date) {
        ThrowStatisticsError(
            common::errors::ErrorCode::kStatisticsRangeInvalid,
            "statistics date range is invalid"
        );
    }
}

std::string FormatAmount(const double value) {
    std::ostringstream output;
    output.setf(std::ios::fixed);
    output.precision(2);
    output << value;
    return output.str();
}

std::string BuildCsvContent(const std::vector<DailyStatisticsRecord>& records) {
    std::ostringstream output;
    output << "stat_date,auction_count,sold_count,unsold_count,bid_count,gmv_amount\n";
    for (const auto& record : records) {
        output << record.stat_date << ',' << record.auction_count << ',' << record.sold_count << ','
               << record.unsold_count << ',' << record.bid_count << ','
               << FormatAmount(record.gmv_amount) << '\n';
    }
    return output.str();
}

std::string BuildExportFileName(const DailyStatisticsQuery& query) {
    return "statistics_daily_" + query.start_date + "_" + query.end_date + ".csv";
}

}  // namespace

StatisticsService::StatisticsService(
    const common::config::AppConfig& config,
    std::filesystem::path project_root,
    middleware::AuthMiddleware& auth_middleware
) : mysql_config_(config.mysql),
    project_root_(std::move(project_root)),
    auth_middleware_(&auth_middleware) {}

DailyStatisticsAggregationResult StatisticsService::RebuildDailyStatistics(
    const std::string& stat_date
) {
    ValidateStatDate(stat_date);

    auto connection = CreateConnection();
    auto repository = MakeRepository(connection);
    bool transaction_started = false;
    try {
        const auto existed_before = repository.FindDailyStatisticsByDate(stat_date).has_value();
        connection.BeginTransaction();
        transaction_started = true;

        const auto aggregate = repository.AggregateDailyStatistics(stat_date);
        const auto record = repository.UpsertDailyStatistics(aggregate);
        repository.InsertOperationLog(
            std::nullopt,
            "SYSTEM",
            "rebuild_daily_statistics",
            stat_date,
            "SUCCESS",
            "auction_count=" + std::to_string(record.auction_count) + ",sold_count=" +
                std::to_string(record.sold_count) + ",unsold_count=" +
                std::to_string(record.unsold_count) + ",bid_count=" +
                std::to_string(record.bid_count)
        );
        connection.Commit();
        transaction_started = false;

        InsertTaskLogSafely(
            repository,
            repository::StatisticsTaskLogParams{
                .task_type = std::string(kTaskTypeStatisticsDailyAggregation),
                .biz_key = stat_date,
                .task_status = std::string(kTaskStatusSuccess),
                .retry_count = 0,
                .last_error = "",
                .scheduled_at = stat_date + " 00:00:00",
            }
        );

        return DailyStatisticsAggregationResult{
            .record = record,
            .created = !existed_before,
        };
    } catch (const std::exception& exception) {
        if (transaction_started) {
            connection.Rollback();
        }
        InsertTaskLogSafely(
            repository,
            repository::StatisticsTaskLogParams{
                .task_type = std::string(kTaskTypeStatisticsDailyAggregation),
                .biz_key = stat_date,
                .task_status = std::string(kTaskStatusFailed),
                .retry_count = 0,
                .last_error = exception.what(),
                .scheduled_at = stat_date + " 00:00:00",
            }
        );
        InsertOperationLogSafely(
            repository,
            std::nullopt,
            "SYSTEM",
            "rebuild_daily_statistics",
            stat_date,
            "FAILED",
            exception.what()
        );
        throw;
    }
}

DailyStatisticsQueryResult StatisticsService::ListDailyStatistics(
    const std::string_view authorization_header,
    const DailyStatisticsQuery& query
) {
    [[maybe_unused]] const auto auth_context = auth_middleware_->RequireAnyRole(
        authorization_header,
        {modules::auth::kRoleAdmin}
    );
    ValidateQuery(query);

    auto connection = CreateConnection();
    auto repository = MakeRepository(connection);
    return DailyStatisticsQueryResult{
        .records = repository.ListDailyStatistics(query.start_date, query.end_date),
    };
}

DailyStatisticsExportResult StatisticsService::ExportDailyStatistics(
    const std::string_view authorization_header,
    const DailyStatisticsQuery& query
) {
    const auto list_result = ListDailyStatistics(authorization_header, query);
    auto connection = CreateConnection();
    auto repository = MakeRepository(connection);
    const auto auth_context = auth_middleware_->RequireAnyRole(
        authorization_header,
        {modules::auth::kRoleAdmin}
    );

    repository.InsertOperationLog(
        auth_context.user_id,
        auth_context.role_code,
        "export_daily_statistics",
        query.start_date + "_" + query.end_date,
        "SUCCESS",
        "row_count=" + std::to_string(list_result.records.size())
    );

    return DailyStatisticsExportResult{
        .file_name = BuildExportFileName(query),
        .content_type = "text/csv",
        .csv_content = BuildCsvContent(list_result.records),
        .row_count = list_result.records.size(),
    };
}

common::db::MysqlConnection StatisticsService::CreateConnection() const {
    return common::db::MysqlConnection(mysql_config_, project_root_);
}

}  // namespace auction::modules::statistics
