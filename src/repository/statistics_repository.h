#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "modules/statistics/statistics_types.h"
#include "repository/base_repository.h"

namespace auction::repository {

struct DailyStatisticsAggregate {
    std::string stat_date;
    int auction_count{0};
    int sold_count{0};
    int unsold_count{0};
    int bid_count{0};
    double gmv_amount{0.0};
};

struct StatisticsTaskLogParams {
    std::string task_type;
    std::string biz_key;
    std::string task_status;
    int retry_count{0};
    std::string last_error;
    std::string scheduled_at;
};

class StatisticsRepository : public BaseRepository {
public:
    using BaseRepository::BaseRepository;

    [[nodiscard]] DailyStatisticsAggregate AggregateDailyStatistics(
        const std::string& stat_date
    ) const;
    [[nodiscard]] std::optional<modules::statistics::DailyStatisticsRecord>
    FindDailyStatisticsByDate(const std::string& stat_date) const;
    [[nodiscard]] modules::statistics::DailyStatisticsRecord UpsertDailyStatistics(
        const DailyStatisticsAggregate& aggregate
    ) const;
    [[nodiscard]] std::vector<modules::statistics::DailyStatisticsRecord> ListDailyStatistics(
        const std::string& start_date,
        const std::string& end_date
    ) const;

    void InsertTaskLog(const StatisticsTaskLogParams& params) const;
    void InsertOperationLog(
        const std::optional<std::uint64_t>& operator_id,
        const std::string& operator_role,
        const std::string& operation_name,
        const std::string& biz_key,
        const std::string& result,
        const std::string& detail
    ) const;
};

}  // namespace auction::repository
