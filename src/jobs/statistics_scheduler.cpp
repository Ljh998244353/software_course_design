#include "jobs/statistics_scheduler.h"

namespace auction::jobs {

StatisticsScheduler::StatisticsScheduler(
    modules::statistics::StatisticsService& statistics_service
) : statistics_service_(&statistics_service) {}

modules::statistics::DailyStatisticsTaskResult StatisticsScheduler::RunDailyAggregation(
    const std::string& stat_date
) {
    modules::statistics::DailyStatisticsTaskResult result;
    result.scanned = 1;

    const auto aggregation_result = statistics_service_->RebuildDailyStatistics(stat_date);
    result.succeeded = 1;
    result.affected_dates.push_back(aggregation_result.record.stat_date);
    return result;
}

}  // namespace auction::jobs
