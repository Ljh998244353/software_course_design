#include "application/demo_dashboard_service.h"

#include <chrono>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

#include <mysql.h>

#include "common/db/mysql_connection.h"
#include "common/errors/error_code.h"

namespace auction::application {

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

std::string QueryString(common::db::MysqlConnection& connection, const std::string& sql) {
    const auto result = ExecuteQuery(connection, sql);
    MYSQL_ROW row = mysql_fetch_row(result.get());
    if (row == nullptr || row[0] == nullptr) {
        return {};
    }
    return row[0];
}

Json::Value QueryRows(common::db::MysqlConnection& connection, const std::string& sql) {
    const auto result = ExecuteQuery(connection, sql);
    MYSQL_FIELD* fields = mysql_fetch_fields(result.get());
    const auto field_count = mysql_num_fields(result.get());

    Json::Value rows(Json::arrayValue);
    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(result.get())) != nullptr) {
        unsigned long* lengths = mysql_fetch_lengths(result.get());
        Json::Value item(Json::objectValue);
        for (unsigned int index = 0; index < field_count; ++index) {
            const std::string field_name = fields[index].name != nullptr ? fields[index].name : "";
            if (row[index] == nullptr) {
                item[field_name] = Json::nullValue;
            } else {
                item[field_name] =
                    std::string(row[index], lengths == nullptr ? std::strlen(row[index]) : lengths[index]);
            }
        }
        rows.append(item);
    }
    return rows;
}

std::string ToIsoTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm{};
    localtime_r(&time, &local_tm);

    std::ostringstream output;
    output << std::put_time(&local_tm, "%Y-%m-%dT%H:%M:%S%z");
    return output.str();
}

Json::Value BuildRiskScenarios() {
    Json::Value scenarios(Json::arrayValue);

    Json::Value bid(Json::objectValue);
    bid["title"] = "并发出价最高价一致";
    bid["evidence"] = "auction_high_risk_flow";
    bid["description"] = "多个买家同时出价时，数据库事务保证一个拍卖活动只有一个当前最高价和最高出价者。";
    scenarios.append(bid);

    Json::Value finish(Json::objectValue);
    finish["title"] = "拍卖结束竞争";
    finish["evidence"] = "auction_high_risk_flow";
    finish["description"] = "结束调度与晚到出价并发时，晚到出价被拒绝，活动进入结算或流拍状态。";
    scenarios.append(finish);

    Json::Value payment(Json::objectValue);
    payment["title"] = "支付回调幂等";
    payment["evidence"] = "auction_high_risk_flow";
    payment["description"] = "重复成功回调只产生一次有效状态变更，避免订单重复记账。";
    scenarios.append(payment);

    Json::Value cache(Json::objectValue);
    cache["title"] = "缓存降级不破坏事实";
    cache["evidence"] = "auction_high_risk_flow";
    cache["description"] = "缓存刷新失败不会回滚已成功提交的出价事务，MySQL 仍是权威事实源。";
    scenarios.append(cache);

    Json::Value security(Json::objectValue);
    security["title"] = "安全负向路径";
    security["evidence"] = "auction_security_negative";
    security["description"] = "未认证、越权、冻结账号、非法金额和支付回调篡改均已被自动化测试覆盖。";
    scenarios.append(security);

    return scenarios;
}

Json::Value BuildSummary(common::db::MysqlConnection& connection) {
    Json::Value summary(Json::objectValue);
    summary["demoUserCount"] = QueryString(
        connection,
        "SELECT COUNT(*) FROM user_account "
        "WHERE username IN ('admin','support','seller_demo','buyer_demo','buyer_demo_2')"
    );
    summary["demoItemCount"] =
        QueryString(connection, "SELECT COUNT(*) FROM item WHERE title LIKE '演示%'");
    summary["demoAuctionCount"] = QueryString(
        connection,
        "SELECT COUNT(*) FROM auction a "
        "JOIN item i ON i.item_id = a.item_id "
        "WHERE i.title LIKE '演示%'"
    );
    summary["demoBidCount"] = QueryString(
        connection,
        "SELECT COUNT(*) FROM bid_record b "
        "JOIN auction a ON a.auction_id = b.auction_id "
        "JOIN item i ON i.item_id = a.item_id "
        "WHERE i.title LIKE '演示%'"
    );
    summary["demoOrderCount"] =
        QueryString(connection, "SELECT COUNT(*) FROM order_info WHERE order_no LIKE 'DEMO-%'");
    summary["demoGmvAmount"] = QueryString(
        connection,
        "SELECT COALESCE(FORMAT(SUM(final_amount), 2), '0.00') "
        "FROM order_info WHERE order_no LIKE 'DEMO-%'"
    );
    summary["demoNotificationCount"] = QueryString(
        connection,
        "SELECT COUNT(*) FROM notification n "
        "LEFT JOIN order_info o ON n.biz_type = 'ORDER' AND n.biz_id = o.order_id "
        "LEFT JOIN auction a ON n.biz_type = 'AUCTION' AND n.biz_id = a.auction_id "
        "LEFT JOIN item i ON i.item_id = a.item_id "
        "WHERE o.order_no LIKE 'DEMO-%' OR i.title LIKE '演示%'"
    );
    return summary;
}

}  // namespace

common::http::ApiResponse BuildDemoDashboardResponse(
    const common::config::AppConfig& config,
    const std::filesystem::path& project_root
) {
    Json::Value payload(Json::objectValue);
    payload["service"] = "auction_app";
    payload["view"] = "demo_dashboard";
    payload["database"] = config.mysql.database;
    payload["generatedAt"] = ToIsoTimestamp();
    payload["mode"] = "read_only_demo";

    try {
        common::db::MysqlConnection connection(config.mysql, project_root);

        payload["summary"] = BuildSummary(connection);
        payload["accounts"] = QueryRows(
            connection,
            "SELECT username, role_code, status, nickname, "
            "CASE username "
            "WHEN 'admin' THEN '审核、拍卖配置、统计与异常处理展示' "
            "WHEN 'support' THEN '日志、通知、异常辅助处理展示' "
            "WHEN 'seller_demo' THEN '卖家发布、成交、评价展示' "
            "WHEN 'buyer_demo' THEN '买家出价、支付、评价展示' "
            "WHEN 'buyer_demo_2' THEN '竞争出价和被超越提醒展示' "
            "ELSE '演示账号' END AS demo_usage "
            "FROM user_account "
            "WHERE username IN ('admin','support','seller_demo','buyer_demo','buyer_demo_2') "
            "ORDER BY FIELD(username, 'admin', 'support', 'seller_demo', 'buyer_demo', 'buyer_demo_2')"
        );
        payload["auctions"] = QueryRows(
            connection,
            "SELECT CAST(a.auction_id AS CHAR) AS auction_id, "
            "i.title, i.description, i.cover_image_url, i.item_status, "
            "seller.nickname AS seller_nickname, seller.username AS seller_username, "
            "a.status AS auction_status, DATE_FORMAT(a.start_time, '%Y-%m-%d %H:%i:%s') AS start_time, "
            "DATE_FORMAT(a.end_time, '%Y-%m-%d %H:%i:%s') AS end_time, "
            "FORMAT(a.start_price, 2) AS start_price, FORMAT(a.current_price, 2) AS current_price, "
            "FORMAT(a.bid_step, 2) AS bid_step, "
            "COALESCE(bidder.nickname, '暂无') AS highest_bidder_nickname, "
            "COALESCE(bidder.username, '') AS highest_bidder_username, "
            "CAST(a.anti_sniping_window_seconds AS CHAR) AS anti_sniping_window_seconds, "
            "CAST(a.extend_seconds AS CHAR) AS extend_seconds, "
            "CAST((SELECT COUNT(*) FROM bid_record b WHERE b.auction_id = a.auction_id) AS CHAR) AS bid_count "
            "FROM auction a "
            "JOIN item i ON i.item_id = a.item_id "
            "JOIN user_account seller ON seller.user_id = a.seller_id "
            "LEFT JOIN user_account bidder ON bidder.user_id = a.highest_bidder_id "
            "WHERE i.title LIKE '演示%' "
            "ORDER BY FIELD(a.status, 'RUNNING', 'PENDING_START', 'SOLD', 'UNSOLD'), a.auction_id"
        );
        payload["bids"] = QueryRows(
            connection,
            "SELECT i.title, bidder.nickname AS bidder_nickname, bidder.username AS bidder_username, "
            "FORMAT(b.bid_amount, 2) AS bid_amount, b.bid_status, b.request_id, "
            "DATE_FORMAT(b.bid_time, '%Y-%m-%d %H:%i:%s') AS bid_time "
            "FROM bid_record b "
            "JOIN auction a ON a.auction_id = b.auction_id "
            "JOIN item i ON i.item_id = a.item_id "
            "JOIN user_account bidder ON bidder.user_id = b.bidder_id "
            "WHERE i.title LIKE '演示%' "
            "ORDER BY b.bid_time DESC, b.bid_id DESC LIMIT 8"
        );
        payload["orders"] = QueryRows(
            connection,
            "SELECT o.order_no, i.title, buyer.nickname AS buyer_nickname, seller.nickname AS seller_nickname, "
            "FORMAT(o.final_amount, 2) AS final_amount, o.order_status, "
            "DATE_FORMAT(o.paid_at, '%Y-%m-%d %H:%i:%s') AS paid_at, "
            "DATE_FORMAT(o.shipped_at, '%Y-%m-%d %H:%i:%s') AS shipped_at, "
            "DATE_FORMAT(o.completed_at, '%Y-%m-%d %H:%i:%s') AS completed_at "
            "FROM order_info o "
            "JOIN auction a ON a.auction_id = o.auction_id "
            "JOIN item i ON i.item_id = a.item_id "
            "JOIN user_account buyer ON buyer.user_id = o.buyer_id "
            "JOIN user_account seller ON seller.user_id = o.seller_id "
            "WHERE o.order_no LIKE 'DEMO-%' ORDER BY o.created_at DESC"
        );
        payload["payments"] = QueryRows(
            connection,
            "SELECT p.payment_no, o.order_no, p.pay_channel, FORMAT(p.pay_amount, 2) AS pay_amount, "
            "p.pay_status, p.transaction_no, DATE_FORMAT(p.paid_at, '%Y-%m-%d %H:%i:%s') AS paid_at "
            "FROM payment_record p "
            "JOIN order_info o ON o.order_id = p.order_id "
            "WHERE o.order_no LIKE 'DEMO-%' ORDER BY p.created_at DESC"
        );
        payload["reviews"] = QueryRows(
            connection,
            "SELECT o.order_no, reviewer.nickname AS reviewer_nickname, reviewee.nickname AS reviewee_nickname, "
            "r.review_type, CAST(r.rating AS CHAR) AS rating, r.content, "
            "DATE_FORMAT(r.created_at, '%Y-%m-%d %H:%i:%s') AS created_at "
            "FROM review r "
            "JOIN order_info o ON o.order_id = r.order_id "
            "JOIN user_account reviewer ON reviewer.user_id = r.reviewer_id "
            "JOIN user_account reviewee ON reviewee.user_id = r.reviewee_id "
            "WHERE o.order_no LIKE 'DEMO-%' ORDER BY r.created_at DESC"
        );
        payload["notifications"] = QueryRows(
            connection,
            "SELECT u.nickname AS user_nickname, n.notice_type, n.title, n.content, n.read_status, n.push_status, "
            "DATE_FORMAT(n.created_at, '%Y-%m-%d %H:%i:%s') AS created_at "
            "FROM notification n "
            "JOIN user_account u ON u.user_id = n.user_id "
            "LEFT JOIN order_info o ON n.biz_type = 'ORDER' AND n.biz_id = o.order_id "
            "LEFT JOIN auction a ON n.biz_type = 'AUCTION' AND n.biz_id = a.auction_id "
            "LEFT JOIN item i ON i.item_id = a.item_id "
            "WHERE o.order_no LIKE 'DEMO-%' OR i.title LIKE '演示%' "
            "ORDER BY n.created_at DESC LIMIT 6"
        );
        payload["taskLogs"] = QueryRows(
            connection,
            "SELECT task_type, biz_key, task_status, CAST(retry_count AS CHAR) AS retry_count, "
            "last_error, DATE_FORMAT(finished_at, '%Y-%m-%d %H:%i:%s') AS finished_at "
            "FROM task_log WHERE biz_key LIKE 'DEMO-%' ORDER BY created_at DESC LIMIT 6"
        );
        payload["operationLogs"] = QueryRows(
            connection,
            "SELECT operator_role, module_name, operation_name, biz_key, result, detail, "
            "DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s') AS created_at "
            "FROM operation_log WHERE biz_key LIKE 'DEMO-%' OR detail LIKE '%DEMO-%' "
            "ORDER BY created_at DESC LIMIT 6"
        );
        payload["statisticsDaily"] = QueryRows(
            connection,
            "SELECT DATE_FORMAT(stat_date, '%Y-%m-%d') AS stat_date, "
            "CAST(auction_count AS CHAR) AS auction_count, CAST(sold_count AS CHAR) AS sold_count, "
            "CAST(unsold_count AS CHAR) AS unsold_count, CAST(bid_count AS CHAR) AS bid_count, "
            "FORMAT(gmv_amount, 2) AS gmv_amount "
            "FROM statistics_daily ORDER BY stat_date DESC LIMIT 5"
        );
        payload["riskScenarios"] = BuildRiskScenarios();

        return common::http::ApiResponse::Success(payload);
    } catch (const std::exception& exception) {
        payload["status"] = "down";
        return common::http::ApiResponse::Failure(
            common::errors::ErrorCode::kDatabaseUnavailable,
            exception.what(),
            payload
        );
    }
}

}  // namespace auction::application
