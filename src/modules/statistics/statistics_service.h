#pragma once

#include <filesystem>
#include <string_view>

#include "common/config/app_config.h"
#include "common/db/mysql_connection.h"
#include "middleware/auth_middleware.h"
#include "modules/statistics/statistics_types.h"

namespace auction::modules::statistics {

class StatisticsService {
public:
    StatisticsService(
        const common::config::AppConfig& config,
        std::filesystem::path project_root,
        middleware::AuthMiddleware& auth_middleware
    );

    [[nodiscard]] DailyStatisticsAggregationResult RebuildDailyStatistics(
        const std::string& stat_date
    );
    [[nodiscard]] DailyStatisticsQueryResult ListDailyStatistics(
        std::string_view authorization_header,
        const DailyStatisticsQuery& query
    );
    [[nodiscard]] DailyStatisticsExportResult ExportDailyStatistics(
        std::string_view authorization_header,
        const DailyStatisticsQuery& query
    );

private:
    [[nodiscard]] common::db::MysqlConnection CreateConnection() const;

    common::config::MysqlConfig mysql_config_;
    std::filesystem::path project_root_;
    middleware::AuthMiddleware* auth_middleware_;
};

}  // namespace auction::modules::statistics
