#pragma once

#include <string>

#include "modules/statistics/statistics_service.h"
#include "modules/statistics/statistics_types.h"

namespace auction::jobs {

class StatisticsScheduler {
public:
    explicit StatisticsScheduler(modules::statistics::StatisticsService& statistics_service);

    [[nodiscard]] modules::statistics::DailyStatisticsTaskResult RunDailyAggregation(
        const std::string& stat_date
    );

private:
    modules::statistics::StatisticsService* statistics_service_;
};

}  // namespace auction::jobs
