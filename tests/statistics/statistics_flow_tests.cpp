#include <algorithm>
#include <cassert>
#include <chrono>
#include <cctype>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "common/config/config_loader.h"
#include "common/db/mysql_connection.h"
#include "common/errors/error_code.h"
#include "jobs/auction_scheduler.h"
#include "jobs/order_scheduler.h"
#include "jobs/statistics_scheduler.h"
#include "middleware/auth_middleware.h"
#include "modules/audit/item_audit_service.h"
#include "modules/auction/auction_service.h"
#include "modules/auth/auth_exception.h"
#include "modules/auth/auth_service.h"
#include "modules/auth/auth_session_store.h"
#include "modules/bid/bid_cache_store.h"
#include "modules/bid/bid_service.h"
#include "modules/item/item_service.h"
#include "modules/notification/notification_service.h"
#include "modules/order/order_service.h"
#include "modules/payment/payment_service.h"
#include "modules/payment/payment_signature.h"
#include "modules/statistics/statistics_exception.h"
#include "modules/statistics/statistics_service.h"
#include "ws/auction_event_gateway.h"

namespace {

using auction::common::errors::ErrorCode;
using auction::modules::auth::AuthException;
using auction::modules::statistics::DailyStatisticsRecord;
using auction::modules::statistics::StatisticsException;

std::string BuildUniqueSuffix() {
    return std::to_string(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        )
            .count()
    );
}

std::string CurrentDateOffsetDays(const int day_offset) {
    const auto target_time =
        std::chrono::system_clock::now() + std::chrono::hours(24 * day_offset);
    const auto time_value = std::chrono::system_clock::to_time_t(target_time);
    std::tm local_tm{};
    localtime_r(&time_value, &local_tm);

    char buffer[16]{};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &local_tm);
    return buffer;
}

std::string FormatNowOffsetSeconds(const int offset_seconds) {
    const auto target_time =
        std::chrono::system_clock::now() + std::chrono::seconds(offset_seconds);
    const auto time_value = std::chrono::system_clock::to_time_t(target_time);
    std::tm local_tm{};
    localtime_r(&time_value, &local_tm);

    char buffer[32]{};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &local_tm);
    return buffer;
}

std::string FormatAmount(const double value) {
    char buffer[32]{};
    std::snprintf(buffer, sizeof(buffer), "%.2f", value);
    return buffer;
}

std::string BuildPhoneNumber(const std::string& suffix, const int index) {
    std::string digits;
    digits.reserve(suffix.size() + 4);
    for (const char character : suffix) {
        if (std::isdigit(static_cast<unsigned char>(character)) != 0) {
            digits.push_back(character);
        }
    }
    digits += std::to_string(index);
    if (digits.size() < 9) {
        digits.insert(digits.begin(), 9 - digits.size(), '0');
    }
    digits = digits.substr(digits.size() - 9);
    return "13" + digits;
}

template <typename T>
bool Contains(const std::vector<T>& values, const T& expected) {
    return std::find(values.begin(), values.end(), expected) != values.end();
}

template <typename Fn>
void ExpectAuthError(const ErrorCode expected_code, Fn&& function) {
    try {
        function();
        assert(false && "expected auth exception");
    } catch (const AuthException& exception) {
        assert(exception.code() == expected_code);
    }
}

template <typename Fn>
void ExpectStatisticsError(const ErrorCode expected_code, Fn&& function) {
    try {
        function();
        assert(false && "expected statistics exception");
    } catch (const StatisticsException& exception) {
        assert(exception.code() == expected_code);
    }
}

auction::common::db::MysqlConnection CreateConnection(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root
) {
    return auction::common::db::MysqlConnection(config.mysql, project_root);
}

void ExecuteSql(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::string& sql
) {
    auto connection = CreateConnection(config, project_root);
    connection.Execute(sql);
}

std::uint64_t QueryUint64(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::string& sql
) {
    auto connection = CreateConnection(config, project_root);
    return connection.QueryCount(sql);
}

std::string QueryString(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::string& sql
) {
    auto connection = CreateConnection(config, project_root);
    return connection.QueryString(sql);
}

std::uint64_t QueryOrderIdByAuction(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t auction_id
) {
    return QueryUint64(
        config,
        project_root,
        "SELECT COALESCE(MAX(order_id), 0) FROM order_info WHERE auction_id = " +
            std::to_string(auction_id)
    );
}

std::uint64_t CountTaskLogByType(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::string& task_type
) {
    return QueryUint64(
        config,
        project_root,
        "SELECT COUNT(*) FROM task_log WHERE task_type = '" + task_type + "'"
    );
}

std::uint64_t CountStatisticsRowsByDate(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::string& stat_date
) {
    return QueryUint64(
        config,
        project_root,
        "SELECT COUNT(*) FROM statistics_daily WHERE stat_date = '" + stat_date + "'"
    );
}

std::string QueryAuctionStatus(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t auction_id
) {
    return QueryString(
        config,
        project_root,
        "SELECT status FROM auction WHERE auction_id = " + std::to_string(auction_id)
    );
}

void PrepareRunningAuctionWindow(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t auction_id
) {
    ExecuteSql(
        config,
        project_root,
        "UPDATE auction SET start_time = DATE_SUB(CURRENT_TIMESTAMP(3), INTERVAL 5 SECOND), "
        "end_time = DATE_ADD(CURRENT_TIMESTAMP(3), INTERVAL 300 SECOND) WHERE auction_id = " +
            std::to_string(auction_id)
    );
}

void UpdateAuctionWindow(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t auction_id,
    const int day_offset
) {
    ExecuteSql(
        config,
        project_root,
        "UPDATE auction SET start_time = DATE_ADD(DATE_SUB(CURRENT_TIMESTAMP(3), INTERVAL 10 "
        "MINUTE), INTERVAL " +
            std::to_string(day_offset) +
            " DAY), end_time = DATE_ADD(DATE_SUB(CURRENT_TIMESTAMP(3), INTERVAL 5 SECOND), "
            "INTERVAL " +
            std::to_string(day_offset) + " DAY) WHERE auction_id = " + std::to_string(auction_id)
    );
}

void UpdateBidTimes(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t auction_id,
    const int day_offset
) {
    ExecuteSql(
        config,
        project_root,
        "UPDATE bid_record SET bid_time = DATE_ADD(DATE_SUB(CURRENT_TIMESTAMP(3), INTERVAL 30 "
        "SECOND), INTERVAL " +
            std::to_string(day_offset) +
            " DAY), created_at = DATE_ADD(DATE_SUB(CURRENT_TIMESTAMP(3), INTERVAL 30 SECOND), "
            "INTERVAL " +
            std::to_string(day_offset) + " DAY) WHERE auction_id = " + std::to_string(auction_id)
    );
}

std::uint64_t CreateApprovedItem(
    auction::modules::item::ItemService& item_service,
    auction::modules::audit::ItemAuditService& item_audit_service,
    const std::string& seller_header,
    const std::string& admin_header,
    const std::string& unique_suffix
) {
    const auto created = item_service.CreateDraftItem(
        seller_header,
        {
            .title = "Statistics test item " + unique_suffix,
            .description = "used for S10 statistics tests",
            .category_id = 1,
            .start_price = 100.0,
            .cover_image_url = "",
        }
    );
    const auto image = item_service.AddItemImage(
        seller_header,
        created.item_id,
        {
            .image_url = "/images/statistics/" + unique_suffix + ".jpg",
            .sort_no = std::nullopt,
            .is_cover = true,
        }
    );
    assert(image.image_id > 0);
    const auto submitted = item_service.SubmitForAudit(seller_header, created.item_id);
    assert(submitted.item_status == "PENDING_AUDIT");
    const auto approved = item_audit_service.AuditItem(
        admin_header,
        created.item_id,
        {
            .audit_result = "APPROVED",
            .reason = "approved for statistics tests",
        }
    );
    assert(approved.new_status == "READY_FOR_AUCTION");
    return created.item_id;
}

struct RegisteredUser {
    std::uint64_t user_id{0};
    std::string username;
    std::string header;
};

RegisteredUser RegisterAndLoginUser(
    auction::modules::auth::AuthService& auth_service,
    const std::string& unique_suffix,
    const std::string& username_prefix,
    const int index
) {
    RegisteredUser result;
    const auto compact_suffix = std::to_string(std::hash<std::string>{}(unique_suffix));
    const auto max_prefix_length = static_cast<std::size_t>(50 - 1 - compact_suffix.size());
    const auto compact_prefix =
        username_prefix.size() > max_prefix_length ? username_prefix.substr(0, max_prefix_length)
                                                   : username_prefix;
    result.username = compact_prefix + "_" + compact_suffix;
    const auto password = "Password@1";
    const auto registered = auth_service.RegisterUser({
        .username = result.username,
        .password = password,
        .phone = BuildPhoneNumber(unique_suffix, index),
        .email = result.username + "@auction.local",
        .nickname = username_prefix,
    });
    const auto login = auth_service.Login({
        .principal = result.username,
        .password = password,
    });
    result.user_id = registered.user_id;
    result.header = "Bearer " + login.token;
    return result;
}

std::uint64_t CreateRunningAuction(
    auction::modules::item::ItemService& item_service,
    auction::modules::audit::ItemAuditService& item_audit_service,
    auction::modules::auction::AuctionService& auction_service,
    auction::jobs::AuctionScheduler& auction_scheduler,
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::string& seller_header,
    const std::string& admin_header,
    const std::string& unique_suffix
) {
    const auto item_id = CreateApprovedItem(
        item_service,
        item_audit_service,
        seller_header,
        admin_header,
        unique_suffix
    );
    const auto created = auction_service.CreateAuction(
        admin_header,
        {
            .item_id = item_id,
            .start_time = FormatNowOffsetSeconds(600),
            .end_time = FormatNowOffsetSeconds(1200),
            .start_price = 100.0,
            .bid_step = 10.0,
            .anti_sniping_window_seconds = 0,
            .extend_seconds = 0,
        }
    );
    PrepareRunningAuctionWindow(config, project_root, created.auction_id);
    const auto started = auction_scheduler.RunStartCycle();
    assert(Contains(started.affected_auction_ids, created.auction_id));
    return created.auction_id;
}

struct PaidAuctionContext {
    std::uint64_t auction_id{0};
    std::uint64_t order_id{0};
    RegisteredUser seller;
    RegisteredUser buyer;
    double final_amount{0.0};
};

PaidAuctionContext CreatePaidAuction(
    auction::modules::auth::AuthService& auth_service,
    auction::modules::item::ItemService& item_service,
    auction::modules::audit::ItemAuditService& item_audit_service,
    auction::modules::auction::AuctionService& auction_service,
    auction::modules::bid::BidService& bid_service,
    auction::jobs::AuctionScheduler& auction_scheduler,
    auction::jobs::OrderScheduler& order_scheduler,
    auction::modules::payment::PaymentService& payment_service,
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::string& admin_header,
    const std::string& unique_suffix,
    const int start_index,
    const int day_offset,
    const std::vector<double>& bid_amounts
) {
    PaidAuctionContext context;
    context.seller = RegisterAndLoginUser(
        auth_service,
        unique_suffix,
        "statistics_seller",
        start_index
    );
    context.buyer = RegisterAndLoginUser(
        auth_service,
        unique_suffix,
        "statistics_buyer",
        start_index + 1
    );

    context.auction_id = CreateRunningAuction(
        item_service,
        item_audit_service,
        auction_service,
        auction_scheduler,
        config,
        project_root,
        context.seller.header,
        admin_header,
        unique_suffix
    );

    for (std::size_t index = 0; index < bid_amounts.size(); ++index) {
        const auto bid = bid_service.PlaceBid(
            context.buyer.header,
            context.auction_id,
            {
                .request_id =
                    "statistics-bid-" + unique_suffix + "-" + std::to_string(index + 1),
                .bid_amount = bid_amounts[index],
            }
        );
        assert(bid.bid_status == "WINNING");
        context.final_amount = bid.bid_amount;
    }

    UpdateAuctionWindow(config, project_root, context.auction_id, day_offset);
    UpdateBidTimes(config, project_root, context.auction_id, day_offset);

    const auto finished = auction_scheduler.RunFinishCycle();
    assert(Contains(finished.affected_auction_ids, context.auction_id));
    const auto settled = order_scheduler.RunSettlementCycle();
    assert(settled.succeeded >= 1);

    context.order_id = QueryOrderIdByAuction(config, project_root, context.auction_id);
    assert(context.order_id > 0);

    const auto initiated = payment_service.InitiatePayment(
        context.buyer.header,
        context.order_id,
        {
            .pay_channel = "MOCK_ALIPAY",
        }
    );
    auction::modules::payment::PaymentCallbackRequest callback{
        .payment_no = initiated.payment_no,
        .order_no = initiated.order_no,
        .merchant_no = initiated.merchant_no,
        .transaction_no = "TXN_STATISTICS_" + unique_suffix,
        .pay_status = "SUCCESS",
        .pay_amount = initiated.pay_amount,
        .signature = "",
    };
    callback.signature =
        auction::modules::payment::SignMockPaymentCallback(config.auth.token_secret, callback);
    const auto callback_result = payment_service.HandlePaymentCallback(callback);
    assert(callback_result.order_status == "PAID");

    return context;
}

struct UnsoldAuctionContext {
    std::uint64_t auction_id{0};
    RegisteredUser seller;
};

UnsoldAuctionContext CreateUnsoldAuction(
    auction::modules::auth::AuthService& auth_service,
    auction::modules::item::ItemService& item_service,
    auction::modules::audit::ItemAuditService& item_audit_service,
    auction::modules::auction::AuctionService& auction_service,
    auction::jobs::AuctionScheduler& auction_scheduler,
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::string& admin_header,
    const std::string& unique_suffix,
    const int start_index,
    const int day_offset
) {
    UnsoldAuctionContext context;
    context.seller = RegisterAndLoginUser(
        auth_service,
        unique_suffix,
        "statistics_unsold_seller",
        start_index
    );
    context.auction_id = CreateRunningAuction(
        item_service,
        item_audit_service,
        auction_service,
        auction_scheduler,
        config,
        project_root,
        context.seller.header,
        admin_header,
        unique_suffix
    );

    UpdateAuctionWindow(config, project_root, context.auction_id, day_offset);
    const auto finished = auction_scheduler.RunFinishCycle();
    assert(Contains(finished.affected_auction_ids, context.auction_id));
    assert(QueryAuctionStatus(config, project_root, context.auction_id) == "UNSOLD");
    return context;
}

const DailyStatisticsRecord* FindRecordByDate(
    const std::vector<DailyStatisticsRecord>& records,
    const std::string& stat_date
) {
    const auto iterator = std::find_if(
        records.begin(),
        records.end(),
        [&](const DailyStatisticsRecord& record) { return record.stat_date == stat_date; }
    );
    return iterator == records.end() ? nullptr : &(*iterator);
}

}  // namespace

int main() {
    namespace fs = std::filesystem;

    const fs::path project_root(AUCTION_PROJECT_ROOT);
    const fs::path config_path =
        auction::common::config::ConfigLoader::ResolveConfigPath(project_root);
    const auto config = auction::common::config::ConfigLoader::LoadFromFile(config_path);

    auction::modules::auth::InMemoryAuthSessionStore session_store;
    auction::modules::auth::AuthService auth_service(config, project_root, session_store);
    auction::middleware::AuthMiddleware auth_middleware(auth_service);
    auction::modules::item::ItemService item_service(config, project_root, auth_middleware);
    auction::modules::audit::ItemAuditService item_audit_service(
        config,
        project_root,
        auth_middleware
    );
    auction::modules::auction::AuctionService auction_service(
        config,
        project_root,
        auth_middleware
    );
    auction::ws::InMemoryAuctionEventGateway event_gateway;
    auction::modules::notification::NotificationService notification_service(
        config,
        project_root,
        event_gateway
    );
    auction::modules::bid::InMemoryBidCacheStore cache_store;
    auction::modules::bid::BidService bid_service(
        config,
        project_root,
        auth_middleware,
        notification_service,
        cache_store
    );
    auction::modules::order::OrderService order_service(
        config,
        project_root,
        notification_service
    );
    auction::modules::payment::PaymentService payment_service(
        config,
        project_root,
        auth_middleware,
        notification_service
    );
    auction::modules::statistics::StatisticsService statistics_service(
        config,
        project_root,
        auth_middleware
    );
    auction::jobs::AuctionScheduler auction_scheduler(auction_service);
    auction::jobs::OrderScheduler order_scheduler(order_service);
    auction::jobs::StatisticsScheduler statistics_scheduler(statistics_service);

    const auto admin_login = auth_service.Login({
        .principal = "admin",
        .password = "Admin@123",
    });
    const auto admin_header = "Bearer " + admin_login.token;
    const auto unique_suffix = BuildUniqueSuffix();
    const auto today_date = CurrentDateOffsetDays(0);
    const auto yesterday_date = CurrentDateOffsetDays(-1);

    (void)statistics_scheduler.RunDailyAggregation(today_date);
    (void)statistics_scheduler.RunDailyAggregation(yesterday_date);

    const auto baseline_today_result = statistics_service.ListDailyStatistics(
        admin_header,
        {
            .start_date = today_date,
            .end_date = today_date,
        }
    );
    const auto baseline_yesterday_result = statistics_service.ListDailyStatistics(
        admin_header,
        {
            .start_date = yesterday_date,
            .end_date = yesterday_date,
        }
    );
    assert(baseline_today_result.records.size() == 1);
    assert(baseline_yesterday_result.records.size() == 1);
    const auto baseline_today = baseline_today_result.records.front();
    const auto baseline_yesterday = baseline_yesterday_result.records.front();

    const auto paid_today = CreatePaidAuction(
        auth_service,
        item_service,
        item_audit_service,
        auction_service,
        bid_service,
        auction_scheduler,
        order_scheduler,
        payment_service,
        config,
        project_root,
        admin_header,
        unique_suffix + "_today_paid",
        10,
        0,
        {150.0}
    );
    (void)paid_today;

    const auto unsold_today = CreateUnsoldAuction(
        auth_service,
        item_service,
        item_audit_service,
        auction_service,
        auction_scheduler,
        config,
        project_root,
        admin_header,
        unique_suffix + "_today_unsold",
        20,
        0
    );
    (void)unsold_today;

    const auto paid_yesterday = CreatePaidAuction(
        auth_service,
        item_service,
        item_audit_service,
        auction_service,
        bid_service,
        auction_scheduler,
        order_scheduler,
        payment_service,
        config,
        project_root,
        admin_header,
        unique_suffix + "_yesterday_paid",
        30,
        -1,
        {140.0, 180.0}
    );
    (void)paid_yesterday;

    const auto task_log_before = CountTaskLogByType(
        config,
        project_root,
        "STATISTICS_DAILY_AGGREGATION"
    );

    const auto today_task_1 = statistics_scheduler.RunDailyAggregation(today_date);
    assert(today_task_1.scanned == 1);
    assert(today_task_1.succeeded == 1);
    assert(today_task_1.failed == 0);
    assert(Contains(today_task_1.affected_dates, today_date));

    const auto today_result_1 = statistics_service.ListDailyStatistics(
        admin_header,
        {
            .start_date = today_date,
            .end_date = today_date,
        }
    );
    assert(today_result_1.records.size() == 1);
    const auto& today_record_1 = today_result_1.records.front();
    assert(today_record_1.stat_date == today_date);
    assert(today_record_1.auction_count == baseline_today.auction_count + 2);
    assert(today_record_1.sold_count == baseline_today.sold_count + 1);
    assert(today_record_1.unsold_count == baseline_today.unsold_count + 1);
    assert(today_record_1.bid_count == baseline_today.bid_count + 1);
    assert(today_record_1.gmv_amount == baseline_today.gmv_amount + 150.0);

    const auto yesterday_task = statistics_scheduler.RunDailyAggregation(yesterday_date);
    assert(yesterday_task.scanned == 1);
    assert(yesterday_task.succeeded == 1);
    assert(yesterday_task.failed == 0);
    assert(Contains(yesterday_task.affected_dates, yesterday_date));

    const auto yesterday_result = statistics_service.ListDailyStatistics(
        admin_header,
        {
            .start_date = yesterday_date,
            .end_date = yesterday_date,
        }
    );
    assert(yesterday_result.records.size() == 1);
    const auto& yesterday_record = yesterday_result.records.front();
    assert(yesterday_record.stat_date == yesterday_date);
    assert(yesterday_record.auction_count == baseline_yesterday.auction_count + 1);
    assert(yesterday_record.sold_count == baseline_yesterday.sold_count + 1);
    assert(yesterday_record.unsold_count == baseline_yesterday.unsold_count);
    assert(yesterday_record.bid_count == baseline_yesterday.bid_count + 2);
    assert(yesterday_record.gmv_amount == baseline_yesterday.gmv_amount + 180.0);

    const auto extra_unsold_today = CreateUnsoldAuction(
        auth_service,
        item_service,
        item_audit_service,
        auction_service,
        auction_scheduler,
        config,
        project_root,
        admin_header,
        unique_suffix + "_today_unsold_extra",
        40,
        0
    );
    (void)extra_unsold_today;

    const auto today_task_2 = statistics_scheduler.RunDailyAggregation(today_date);
    assert(today_task_2.scanned == 1);
    assert(today_task_2.succeeded == 1);
    assert(today_task_2.failed == 0);
    assert(Contains(today_task_2.affected_dates, today_date));
    assert(CountStatisticsRowsByDate(config, project_root, today_date) == 1);

    const auto today_result_2 = statistics_service.ListDailyStatistics(
        admin_header,
        {
            .start_date = today_date,
            .end_date = today_date,
        }
    );
    assert(today_result_2.records.size() == 1);
    const auto& today_record_2 = today_result_2.records.front();
    assert(today_record_2.auction_count == baseline_today.auction_count + 3);
    assert(today_record_2.sold_count == baseline_today.sold_count + 1);
    assert(today_record_2.unsold_count == baseline_today.unsold_count + 2);
    assert(today_record_2.bid_count == baseline_today.bid_count + 1);
    assert(today_record_2.gmv_amount == baseline_today.gmv_amount + 150.0);

    const auto range_result = statistics_service.ListDailyStatistics(
        admin_header,
        {
            .start_date = yesterday_date,
            .end_date = today_date,
        }
    );
    assert(range_result.records.size() == 2);
    assert(range_result.records.front().stat_date == yesterday_date);
    assert(range_result.records.back().stat_date == today_date);
    const auto yesterday_from_range = FindRecordByDate(range_result.records, yesterday_date);
    const auto today_from_range = FindRecordByDate(range_result.records, today_date);
    assert(yesterday_from_range != nullptr);
    assert(today_from_range != nullptr);
    assert(yesterday_from_range->auction_count == baseline_yesterday.auction_count + 1);
    assert(today_from_range->auction_count == baseline_today.auction_count + 3);

    const auto export_result = statistics_service.ExportDailyStatistics(
        admin_header,
        {
            .start_date = yesterday_date,
            .end_date = today_date,
        }
    );
    assert(export_result.file_name == "statistics_daily_" + yesterday_date + "_" + today_date + ".csv");
    assert(export_result.content_type == "text/csv");
    assert(export_result.row_count == 2);
    assert(
        export_result.csv_content.find(
            "stat_date,auction_count,sold_count,unsold_count,bid_count,gmv_amount\n"
        ) != std::string::npos
    );
    assert(
        export_result.csv_content.find(
            yesterday_date + "," +
            std::to_string(baseline_yesterday.auction_count + 1) + "," +
            std::to_string(baseline_yesterday.sold_count + 1) + "," +
            std::to_string(baseline_yesterday.unsold_count) + "," +
            std::to_string(baseline_yesterday.bid_count + 2) + "," +
            FormatAmount(baseline_yesterday.gmv_amount + 180.0) + "\n"
        ) != std::string::npos
    );
    assert(
        export_result.csv_content.find(
            today_date + "," + std::to_string(baseline_today.auction_count + 3) + "," +
            std::to_string(baseline_today.sold_count + 1) + "," +
            std::to_string(baseline_today.unsold_count + 2) + "," +
            std::to_string(baseline_today.bid_count + 1) + "," +
            FormatAmount(baseline_today.gmv_amount + 150.0) + "\n"
        ) != std::string::npos
    );

    assert(
        CountTaskLogByType(config, project_root, "STATISTICS_DAILY_AGGREGATION") >=
        task_log_before + 3
    );

    ExpectAuthError(
        ErrorCode::kAuthPermissionDenied,
        [&]() {
            (void)statistics_service.ListDailyStatistics(
                paid_today.buyer.header,
                {
                    .start_date = today_date,
                    .end_date = today_date,
                }
            );
        }
    );

    ExpectStatisticsError(
        ErrorCode::kStatisticsRangeInvalid,
        [&]() {
            (void)statistics_service.ListDailyStatistics(
                admin_header,
                {
                    .start_date = today_date,
                    .end_date = yesterday_date,
                }
            );
        }
    );

    return 0;
}
