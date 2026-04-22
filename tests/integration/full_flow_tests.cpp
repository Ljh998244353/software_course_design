#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cctype>
#include <ctime>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "common/config/config_loader.h"
#include "common/db/mysql_connection.h"
#include "jobs/auction_scheduler.h"
#include "jobs/order_scheduler.h"
#include "middleware/auth_middleware.h"
#include "modules/audit/item_audit_service.h"
#include "modules/auction/auction_service.h"
#include "modules/auth/auth_service.h"
#include "modules/auth/auth_session_store.h"
#include "modules/bid/bid_cache_store.h"
#include "modules/bid/bid_service.h"
#include "modules/item/item_service.h"
#include "modules/notification/notification_service.h"
#include "modules/ops/ops_service.h"
#include "modules/order/order_service.h"
#include "modules/payment/payment_service.h"
#include "modules/payment/payment_signature.h"
#include "modules/review/review_service.h"
#include "modules/statistics/statistics_service.h"
#include "ws/auction_event_gateway.h"

namespace {

struct RegisteredUser {
    std::uint64_t user_id{0};
    std::string username;
    std::string header;
};

std::string BuildUniqueSuffix() {
    return std::to_string(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        )
            .count()
    );
}

std::string CurrentDateText() {
    const auto time_value = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
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

std::string QueryOrderStatus(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t order_id
) {
    return QueryString(
        config,
        project_root,
        "SELECT order_status FROM order_info WHERE order_id = " + std::to_string(order_id)
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

std::string QueryItemStatusByAuction(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t auction_id
) {
    return QueryString(
        config,
        project_root,
        "SELECT i.item_status FROM item i INNER JOIN auction a ON a.item_id = i.item_id "
        "WHERE a.auction_id = " +
            std::to_string(auction_id)
    );
}

std::uint64_t CountNotificationsByOrderAndType(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t order_id,
    const std::string& notice_type
) {
    return QueryUint64(
        config,
        project_root,
        "SELECT COUNT(*) FROM notification WHERE biz_type = 'ORDER' AND biz_id = " +
            std::to_string(order_id) + " AND notice_type = '" + notice_type + "'"
    );
}

std::uint64_t QueryNotificationIdByAuctionAndUser(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t auction_id,
    const std::uint64_t user_id
) {
    return QueryUint64(
        config,
        project_root,
        "SELECT COALESCE(MAX(notification_id), 0) FROM notification WHERE biz_type = 'AUCTION' "
        "AND biz_id = " +
            std::to_string(auction_id) + " AND user_id = " + std::to_string(user_id)
    );
}

std::string QueryNotificationPushStatus(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t notification_id
) {
    return QueryString(
        config,
        project_root,
        "SELECT push_status FROM notification WHERE notification_id = " +
            std::to_string(notification_id)
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

void UpdateAuctionEndToPast(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t auction_id
) {
    ExecuteSql(
        config,
        project_root,
        "UPDATE auction SET end_time = DATE_SUB(CURRENT_TIMESTAMP(3), INTERVAL 5 SECOND) "
        "WHERE auction_id = " +
            std::to_string(auction_id)
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
            .title = "Integration item " + unique_suffix,
            .description = "used for S12 full flow integration",
            .category_id = 1,
            .start_price = 100.0,
            .cover_image_url = "",
        }
    );
    const auto image = item_service.AddItemImage(
        seller_header,
        created.item_id,
        {
            .image_url = "/images/integration/" + unique_suffix + ".jpg",
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
            .reason = "approved for S12 integration flow",
        }
    );
    assert(approved.new_status == "READY_FOR_AUCTION");
    return created.item_id;
}

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

bool HasPublishedEvent(
    const std::vector<auction::ws::PublishedAuctionEvent>& events,
    const std::optional<std::uint64_t> user_id,
    const std::optional<std::uint64_t> auction_channel_id,
    const std::string& event_name
) {
    return std::any_of(
        events.begin(),
        events.end(),
        [&](const auction::ws::PublishedAuctionEvent& event) {
            return event.user_id == user_id && event.auction_channel_id == auction_channel_id &&
                   event.message.event == event_name;
        }
    );
}

bool HasOperationLog(
    const std::vector<auction::modules::ops::OperationLogRecord>& records,
    const std::string& operation_name,
    const std::string& biz_key
) {
    return std::any_of(
        records.begin(),
        records.end(),
        [&](const auction::modules::ops::OperationLogRecord& record) {
            return record.operation_name == operation_name && record.biz_key == biz_key &&
                   record.result == "SUCCESS";
        }
    );
}

bool HasTaskLog(
    const std::vector<auction::modules::ops::TaskLogRecord>& records,
    const std::string& task_type,
    const std::string& biz_key,
    const std::string& task_status
) {
    return std::any_of(
        records.begin(),
        records.end(),
        [&](const auction::modules::ops::TaskLogRecord& record) {
            return record.task_type == task_type && record.biz_key == biz_key &&
                   record.task_status == task_status;
        }
    );
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
        auth_middleware,
        notification_service
    );
    auction::modules::payment::PaymentService payment_service(
        config,
        project_root,
        auth_middleware,
        notification_service
    );
    auction::modules::review::ReviewService review_service(
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
    auction::modules::ops::OpsService ops_service(
        config,
        project_root,
        auth_middleware,
        notification_service,
        order_service,
        statistics_service
    );
    auction::jobs::AuctionScheduler auction_scheduler(auction_service);
    auction::jobs::OrderScheduler order_scheduler(order_service);

    const auto admin_login = auth_service.Login({
        .principal = "admin",
        .password = "Admin@123",
    });
    const auto support_login = auth_service.Login({
        .principal = "support",
        .password = "Support@123",
    });
    const auto admin_header = "Bearer " + admin_login.token;
    const auto support_header = "Bearer " + support_login.token;
    const auto unique_suffix = BuildUniqueSuffix();
    const auto today_date = CurrentDateText();

    (void)statistics_service.RebuildDailyStatistics(today_date);
    const auto baseline_stats = statistics_service.ListDailyStatistics(
        admin_header,
        {
            .start_date = today_date,
            .end_date = today_date,
        }
    );
    assert(baseline_stats.records.size() == 1);
    const auto baseline_record = baseline_stats.records.front();

    const auto seller =
        RegisterAndLoginUser(auth_service, unique_suffix, "integration_seller", 10);
    const auto first_bidder =
        RegisterAndLoginUser(auth_service, unique_suffix, "integration_bidder_a", 20);
    const auto second_bidder =
        RegisterAndLoginUser(auth_service, unique_suffix, "integration_bidder_b", 30);

    const auto item_id = CreateApprovedItem(
        item_service,
        item_audit_service,
        seller.header,
        admin_header,
        unique_suffix
    );

    const auto created_auction = auction_service.CreateAuction(
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
    assert(created_auction.status == "PENDING_START");

    const auto admin_detail =
        auction_service.GetAdminAuctionDetail(admin_header, created_auction.auction_id);
    assert(admin_detail.auction.item_id == item_id);
    assert(admin_detail.item.item_status == "READY_FOR_AUCTION");

    PrepareRunningAuctionWindow(config, project_root, created_auction.auction_id);
    const auto started = auction_scheduler.RunStartCycle();
    assert(Contains(started.affected_auction_ids, created_auction.auction_id));
    assert(QueryAuctionStatus(config, project_root, created_auction.auction_id) == "RUNNING");

    const auto public_detail = auction_service.GetPublicAuctionDetail(created_auction.auction_id);
    assert(public_detail.status == "RUNNING");
    assert(std::fabs(public_detail.current_price - 100.0) < 0.0001);

    const auto public_list = auction_service.ListPublicAuctions({
        .keyword = std::nullopt,
        .status = std::optional<std::string>("RUNNING"),
        .page_no = 1,
        .page_size = 20,
    });
    assert(std::any_of(
        public_list.begin(),
        public_list.end(),
        [&](const auction::modules::auction::PublicAuctionSummary& summary) {
            return summary.auction_id == created_auction.auction_id;
        }
    ));

    const auto first_bid = bid_service.PlaceBid(
        first_bidder.header,
        created_auction.auction_id,
        {
            .request_id = "integration-bid-" + unique_suffix + "-1",
            .bid_amount = 120.0,
        }
    );
    assert(first_bid.bid_status == "WINNING");

    const auto second_bid = bid_service.PlaceBid(
        second_bidder.header,
        created_auction.auction_id,
        {
            .request_id = "integration-bid-" + unique_suffix + "-2",
            .bid_amount = 140.0,
        }
    );
    assert(second_bid.bid_status == "WINNING");
    assert(std::fabs(second_bid.current_price - 140.0) < 0.0001);

    const auto history = bid_service.ListAuctionBidHistory(
        created_auction.auction_id,
        {
            .page_no = 1,
            .page_size = 10,
        }
    );
    assert(history.records.size() == 2);
    assert(std::fabs(history.records.front().bid_amount - 140.0) < 0.0001);

    const auto first_bidder_status =
        bid_service.GetMyBidStatus(first_bidder.header, created_auction.auction_id);
    const auto second_bidder_status =
        bid_service.GetMyBidStatus(second_bidder.header, created_auction.auction_id);
    assert(!first_bidder_status.is_current_highest);
    assert(second_bidder_status.is_current_highest);
    assert(std::fabs(second_bidder_status.my_highest_bid - 140.0) < 0.0001);

    const auto outbid_notification_id = QueryNotificationIdByAuctionAndUser(
        config,
        project_root,
        created_auction.auction_id,
        first_bidder.user_id
    );
    assert(outbid_notification_id > 0);
    assert(QueryNotificationPushStatus(config, project_root, outbid_notification_id) == "SENT");

    const auto published_events = event_gateway.SnapshotPublishedEvents();
    assert(HasPublishedEvent(published_events, std::nullopt, created_auction.auction_id, "PRICE_UPDATED"));
    assert(HasPublishedEvent(published_events, first_bidder.user_id, std::nullopt, "OUTBID"));

    UpdateAuctionEndToPast(config, project_root, created_auction.auction_id);
    const auto finished = auction_scheduler.RunFinishCycle();
    assert(Contains(finished.affected_auction_ids, created_auction.auction_id));
    assert(QueryAuctionStatus(config, project_root, created_auction.auction_id) == "SETTLING");

    const auto settled = order_scheduler.RunSettlementCycle();
    assert(settled.succeeded >= 1);

    const auto order_id =
        QueryOrderIdByAuction(config, project_root, created_auction.auction_id);
    assert(order_id > 0);
    assert(QueryOrderStatus(config, project_root, order_id) == "PENDING_PAYMENT");
    assert(CountNotificationsByOrderAndType(config, project_root, order_id, "ORDER_PENDING_PAYMENT") == 1);
    assert(CountNotificationsByOrderAndType(config, project_root, order_id, "ORDER_CREATED") == 1);

    const auto initiated = payment_service.InitiatePayment(
        second_bidder.header,
        order_id,
        {
            .pay_channel = "MOCK_ALIPAY",
        }
    );
    assert(initiated.pay_status == "WAITING_CALLBACK");
    assert(std::fabs(initiated.pay_amount - 140.0) < 0.0001);

    auction::modules::payment::PaymentCallbackRequest callback{
        .payment_no = initiated.payment_no,
        .order_no = initiated.order_no,
        .merchant_no = initiated.merchant_no,
        .transaction_no = "TXN_INTEGRATION_" + unique_suffix,
        .pay_status = "SUCCESS",
        .pay_amount = initiated.pay_amount,
        .signature = "",
    };
    callback.signature =
        auction::modules::payment::SignMockPaymentCallback(config.auth.token_secret, callback);
    const auto callback_result = payment_service.HandlePaymentCallback(callback);
    assert(callback_result.order_status == "PAID");
    assert(QueryOrderStatus(config, project_root, order_id) == "PAID");
    assert(QueryAuctionStatus(config, project_root, created_auction.auction_id) == "SOLD");
    assert(QueryItemStatusByAuction(config, project_root, created_auction.auction_id) == "SOLD");
    assert(CountNotificationsByOrderAndType(config, project_root, order_id, "PAYMENT_SUCCESS") == 1);
    assert(CountNotificationsByOrderAndType(config, project_root, order_id, "ORDER_PAID") == 1);

    const auto shipped = order_service.ShipOrder(seller.header, order_id);
    assert(shipped.old_status == "PAID");
    assert(shipped.new_status == "SHIPPED");
    assert(QueryOrderStatus(config, project_root, order_id) == "SHIPPED");
    assert(CountNotificationsByOrderAndType(config, project_root, order_id, "ORDER_SHIPPED") == 1);

    const auto completed = order_service.ConfirmReceipt(second_bidder.header, order_id);
    assert(completed.old_status == "SHIPPED");
    assert(completed.new_status == "COMPLETED");
    assert(QueryOrderStatus(config, project_root, order_id) == "COMPLETED");
    assert(CountNotificationsByOrderAndType(config, project_root, order_id, "ORDER_COMPLETED") == 1);

    const auto buyer_review = review_service.SubmitReview(
        second_bidder.header,
        order_id,
        {
            .rating = 5,
            .content = "Seller shipped quickly and item matched the description.",
        }
    );
    assert(buyer_review.review.review_type == "BUYER_TO_SELLER");
    assert(!buyer_review.order_marked_reviewed);
    assert(buyer_review.order_status_after == "COMPLETED");

    const auto seller_review = review_service.SubmitReview(
        seller.header,
        order_id,
        {
            .rating = 4,
            .content = "Buyer completed payment and confirmation on time.",
        }
    );
    assert(seller_review.review.review_type == "SELLER_TO_BUYER");
    assert(seller_review.order_marked_reviewed);
    assert(seller_review.order_status_after == "REVIEWED");
    assert(QueryOrderStatus(config, project_root, order_id) == "REVIEWED");
    assert(CountNotificationsByOrderAndType(config, project_root, order_id, "ORDER_REVIEW_RECEIVED") == 2);

    const auto order_reviews = review_service.ListOrderReviews(order_id);
    assert(order_reviews.reviews.size() == 2);
    assert(order_reviews.order_status == "REVIEWED");

    const auto seller_summary = review_service.GetUserReviewSummary(seller.user_id);
    const auto buyer_summary = review_service.GetUserReviewSummary(second_bidder.user_id);
    assert(seller_summary.total_reviews == 1);
    assert(std::fabs(seller_summary.average_rating - 5.0) < 0.0001);
    assert(buyer_summary.total_reviews == 1);
    assert(std::fabs(buyer_summary.average_rating - 4.0) < 0.0001);

    (void)statistics_service.RebuildDailyStatistics(today_date);
    const auto today_stats = statistics_service.ListDailyStatistics(
        admin_header,
        {
            .start_date = today_date,
            .end_date = today_date,
        }
    );
    assert(today_stats.records.size() == 1);
    const auto today_record = today_stats.records.front();
    assert(today_record.auction_count >= baseline_record.auction_count + 1);
    assert(today_record.sold_count >= baseline_record.sold_count + 1);
    assert(today_record.bid_count >= baseline_record.bid_count + 2);
    assert(today_record.gmv_amount >= baseline_record.gmv_amount + 140.0);

    const auto order_logs = ops_service.ListOperationLogs(
        support_header,
        {
            .module_name = std::optional<std::string>("order"),
            .result = std::optional<std::string>("SUCCESS"),
            .limit = 100,
        }
    );
    const auto payment_logs = ops_service.ListOperationLogs(
        support_header,
        {
            .module_name = std::optional<std::string>("payment"),
            .result = std::optional<std::string>("SUCCESS"),
            .limit = 100,
        }
    );
    const auto review_logs = ops_service.ListOperationLogs(
        support_header,
        {
            .module_name = std::optional<std::string>("review"),
            .result = std::optional<std::string>("SUCCESS"),
            .limit = 100,
        }
    );
    const auto settlement_logs = ops_service.ListTaskLogs(
        support_header,
        {
            .task_type = std::optional<std::string>("ORDER_SETTLEMENT"),
            .task_status = std::optional<std::string>("SUCCESS"),
            .limit = 100,
        }
    );

    assert(HasOperationLog(order_logs.records, "ship_order", std::to_string(order_id)));
    assert(HasOperationLog(order_logs.records, "confirm_receipt", std::to_string(order_id)));
    assert(HasOperationLog(payment_logs.records, "payment_callback", std::to_string(order_id)));
    assert(HasOperationLog(review_logs.records, "submit_review", std::to_string(order_id)));
    assert(HasTaskLog(
        settlement_logs.records,
        "ORDER_SETTLEMENT",
        std::to_string(created_auction.auction_id),
        "SUCCESS"
    ));

    return 0;
}
