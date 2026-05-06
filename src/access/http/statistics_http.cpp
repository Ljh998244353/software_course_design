#include "access/http/statistics_http.h"

#include <functional>
#include <memory>
#include <string>

#include "common/http/http_utils.h"
#include "modules/auth/auth_types.h"
#include "modules/statistics/statistics_types.h"

#if AUCTION_HAS_DROGON
#include <drogon/drogon.h>
#endif

namespace auction::access::http {

namespace {

#if AUCTION_HAS_DROGON

Json::Value ToDailyStatisticsJson(
    const modules::statistics::DailyStatisticsRecord& record
) {
    Json::Value json(Json::objectValue);
    json["statId"] = static_cast<Json::UInt64>(record.stat_id);
    json["statDate"] = record.stat_date;
    json["auctionCount"] = record.auction_count;
    json["soldCount"] = record.sold_count;
    json["unsoldCount"] = record.unsold_count;
    json["bidCount"] = record.bid_count;
    json["gmvAmount"] = record.gmv_amount;
    json["createdAt"] = record.created_at;
    json["updatedAt"] = record.updated_at;
    return json;
}

Json::Value ToDailyStatisticsListJson(
    const modules::statistics::DailyStatisticsQueryResult& result
) {
    Json::Value data(Json::objectValue);
    Json::Value list(Json::arrayValue);
    for (const auto& record : result.records) {
        list.append(ToDailyStatisticsJson(record));
    }
    data["list"] = std::move(list);
    data["total"] = static_cast<Json::UInt64>(result.records.size());
    return data;
}

Json::Value ToDailyStatisticsExportJson(
    const modules::statistics::DailyStatisticsExportResult& result
) {
    Json::Value json(Json::objectValue);
    json["fileName"] = result.file_name;
    json["contentType"] = result.content_type;
    json["csvContent"] = result.csv_content;
    json["rowCount"] = static_cast<Json::UInt64>(result.row_count);
    return json;
}

Json::Value ToDailyStatisticsAggregationJson(
    const modules::statistics::DailyStatisticsAggregationResult& result
) {
    Json::Value json = ToDailyStatisticsJson(result.record);
    json["created"] = result.created;
    return json;
}

modules::statistics::DailyStatisticsQuery BuildDailyStatisticsQuery(
    const drogon::HttpRequestPtr& request
) {
    return modules::statistics::DailyStatisticsQuery{
        .start_date = request->getOptionalParameter<std::string>("startDate").value_or(""),
        .end_date = request->getOptionalParameter<std::string>("endDate").value_or(""),
    };
}

#endif

}  // namespace

void RegisterStatisticsHttpRoutes(
    const std::shared_ptr<common::http::HttpServiceContext> services
) {
#if AUCTION_HAS_DROGON
    drogon::app().registerHandler(
        "/api/admin/statistics/daily",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            callback(common::http::ExecuteApi([&]() {
                const auto result = services->statistics_service().ListDailyStatistics(
                    request->getHeader("authorization"),
                    BuildDailyStatisticsQuery(request)
                );
                return common::http::ApiResponse::Success(ToDailyStatisticsListJson(result));
            }));
        },
        {drogon::Get}
    );

    drogon::app().registerHandler(
        "/api/admin/statistics/daily/export",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            callback(common::http::ExecuteApi([&]() {
                const auto result = services->statistics_service().ExportDailyStatistics(
                    request->getHeader("authorization"),
                    BuildDailyStatisticsQuery(request)
                );
                return common::http::ApiResponse::Success(ToDailyStatisticsExportJson(result));
            }));
        },
        {drogon::Get}
    );

    drogon::app().registerHandler(
        "/api/admin/statistics/daily/rebuild",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            callback(common::http::ExecuteApi([&]() {
                (void)common::http::RequireAnyRoleContext(
                    request,
                    services->auth_middleware(),
                    {modules::auth::kRoleAdmin}
                );
                const auto body = common::http::RequireJsonBody(request);
                const auto result = services->statistics_service().RebuildDailyStatistics(
                    common::http::ReadRequiredString(body, "statDate")
                );
                return common::http::ApiResponse::Success(
                    ToDailyStatisticsAggregationJson(result)
                );
            }));
        },
        {drogon::Post}
    );
#else
    static_cast<void>(services);
#endif
}

}  // namespace auction::access::http
