#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace auction::modules::statistics {

inline constexpr std::string_view kTaskTypeStatisticsDailyAggregation =
    "STATISTICS_DAILY_AGGREGATION";
inline constexpr std::string_view kTaskStatusSuccess = "SUCCESS";
inline constexpr std::string_view kTaskStatusFailed = "FAILED";

struct DailyStatisticsRecord {
    std::uint64_t stat_id{0};
    std::string stat_date;
    int auction_count{0};
    int sold_count{0};
    int unsold_count{0};
    int bid_count{0};
    double gmv_amount{0.0};
    std::string created_at;
    std::string updated_at;
};

struct DailyStatisticsQuery {
    std::string start_date;
    std::string end_date;
};

struct DailyStatisticsQueryResult {
    std::vector<DailyStatisticsRecord> records;
};

struct DailyStatisticsExportResult {
    std::string file_name;
    std::string content_type;
    std::string csv_content;
    std::size_t row_count{0};
};

struct DailyStatisticsAggregationResult {
    DailyStatisticsRecord record;
    bool created{false};
};

struct DailyStatisticsTaskResult {
    int scanned{0};
    int succeeded{0};
    int failed{0};
    std::vector<std::string> affected_dates;
};

}  // namespace auction::modules::statistics
