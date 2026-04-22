#include <algorithm>
#include <cassert>
#include <chrono>
#include <cctype>
#include <ctime>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "common/config/config_loader.h"
#include "common/db/mysql_connection.h"
#include "common/errors/error_code.h"
#include "jobs/auction_scheduler.h"
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
#include "modules/ops/ops_exception.h"
#include "modules/ops/ops_service.h"
#include "modules/order/order_service.h"
#include "modules/payment/payment_exception.h"
#include "modules/payment/payment_service.h"
#include "modules/statistics/statistics_service.h"
#include "ws/auction_event_gateway.h"

namespace {

using auction::common::errors::ErrorCode;
using auction::modules::auth::AuthException;
using auction::modules::ops::OpsException;
using auction::modules::payment::PaymentException;

std::string BuildUniqueSuffix() {
    return std::to_string(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        )
            .count()
    );
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
void ExpectOpsError(const ErrorCode expected_code, Fn&& function) {
    try {
        function();
        assert(false && "expected ops exception");
    } catch (const OpsException& exception) {
        assert(exception.code() == expected_code);
    }
}

template <typename Fn>
void ExpectPaymentError(const ErrorCode expected_code, Fn&& function) {
    try {
        function();
        assert(false && "expected payment exception");
    } catch (const PaymentException& exception) {
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

std::uint64_t CountTaskLogsByBizKey(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::string& task_type,
    const std::string& biz_key
) {
    return QueryUint64(
        config,
        project_root,
        "SELECT COUNT(*) FROM task_log WHERE task_type = '" + task_type + "' AND biz_key = '" +
            biz_key + "'"
    );
}

std::uint64_t CountRejectedCallbackLogsByOrder(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t order_id
) {
    return QueryUint64(
        config,
        project_root,
        "SELECT COUNT(*) FROM payment_callback_log WHERE order_id = " + std::to_string(order_id) +
            " AND callback_status = 'REJECTED'"
    );
}

void UpdateAuctionWindow(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t auction_id,
    const int start_offset_seconds,
    const int end_offset_seconds
) {
    ExecuteSql(
        config,
        project_root,
        "UPDATE auction SET start_time = DATE_ADD(CURRENT_TIMESTAMP(3), INTERVAL " +
            std::to_string(start_offset_seconds) +
            " SECOND), end_time = DATE_ADD(CURRENT_TIMESTAMP(3), INTERVAL " +
            std::to_string(end_offset_seconds) + " SECOND) WHERE auction_id = " +
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
            .title = "Ops test item " + unique_suffix,
            .description = "used for S11 ops tests",
            .category_id = 1,
            .start_price = 100.0,
            .cover_image_url = "",
        }
    );
    const auto image = item_service.AddItemImage(
        seller_header,
        created.item_id,
        {
            .image_url = "/images/ops/" + unique_suffix + ".jpg",
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
            .reason = "approved for ops tests",
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
    UpdateAuctionWindow(config, project_root, created.auction_id, -5, 300);
    const auto started = auction_scheduler.RunStartCycle();
    assert(Contains(started.affected_auction_ids, created.auction_id));
    return created.auction_id;
}

struct OpsScenarioContext {
    std::uint64_t auction_id{0};
    std::uint64_t notification_id{0};
    RegisteredUser seller;
    RegisteredUser first_bidder;
    RegisteredUser second_bidder;
};

OpsScenarioContext CreateOutbidFailureScenario(
    auction::modules::auth::AuthService& auth_service,
    auction::modules::item::ItemService& item_service,
    auction::modules::audit::ItemAuditService& item_audit_service,
    auction::modules::auction::AuctionService& auction_service,
    auction::modules::bid::BidService& bid_service,
    auction::jobs::AuctionScheduler& auction_scheduler,
    auction::ws::InMemoryAuctionEventGateway& event_gateway,
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::string& admin_header,
    const std::string& unique_suffix
) {
    OpsScenarioContext context;
    context.seller = RegisterAndLoginUser(auth_service, unique_suffix, "ops_seller", 10);
    context.first_bidder = RegisterAndLoginUser(auth_service, unique_suffix, "ops_bidder_a", 20);
    context.second_bidder = RegisterAndLoginUser(auth_service, unique_suffix, "ops_bidder_b", 30);

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

    const auto first_bid = bid_service.PlaceBid(
        context.first_bidder.header,
        context.auction_id,
        {
            .request_id = "ops-bid-" + unique_suffix + "-1",
            .bid_amount = 120.0,
        }
    );
    assert(first_bid.bid_status == "WINNING");

    event_gateway.SetFailDirect(true);
    const auto second_bid = bid_service.PlaceBid(
        context.second_bidder.header,
        context.auction_id,
        {
            .request_id = "ops-bid-" + unique_suffix + "-2",
            .bid_amount = 140.0,
        }
    );
    event_gateway.SetFailDirect(false);
    assert(second_bid.bid_status == "WINNING");

    context.notification_id = QueryNotificationIdByAuctionAndUser(
        config,
        project_root,
        context.auction_id,
        context.first_bidder.user_id
    );
    assert(context.notification_id > 0);
    assert(
        QueryNotificationPushStatus(config, project_root, context.notification_id) == "FAILED"
    );

    return context;
}

const auction::modules::ops::SystemExceptionRecord* FindExceptionRecord(
    const std::vector<auction::modules::ops::SystemExceptionRecord>& records,
    const std::string& source_type,
    const std::string& source_id
) {
    const auto iterator = std::find_if(
        records.begin(),
        records.end(),
        [&](const auction::modules::ops::SystemExceptionRecord& record) {
            return record.source_type == source_type && record.source_id == source_id;
        }
    );
    return iterator == records.end() ? nullptr : &(*iterator);
}

const auction::modules::ops::SystemExceptionRecord* FindExceptionByBizKey(
    const std::vector<auction::modules::ops::SystemExceptionRecord>& records,
    const std::string& source_type,
    const std::string& biz_key
) {
    const auto iterator = std::find_if(
        records.begin(),
        records.end(),
        [&](const auction::modules::ops::SystemExceptionRecord& record) {
            return record.source_type == source_type && record.biz_key == biz_key;
        }
    );
    return iterator == records.end() ? nullptr : &(*iterator);
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
            return record.operation_name == operation_name && record.biz_key == biz_key;
        }
    );
}

bool HasTaskLog(
    const std::vector<auction::modules::ops::TaskLogRecord>& records,
    const std::string& task_type,
    const std::string& biz_key
) {
    return std::any_of(
        records.begin(),
        records.end(),
        [&](const auction::modules::ops::TaskLogRecord& record) {
            return record.task_type == task_type && record.biz_key == biz_key;
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

    const auto scenario = CreateOutbidFailureScenario(
        auth_service,
        item_service,
        item_audit_service,
        auction_service,
        bid_service,
        auction_scheduler,
        event_gateway,
        config,
        project_root,
        admin_header,
        unique_suffix
    );

    const auto exceptions_before_retry = ops_service.ListSystemExceptions(
        admin_header,
        {
            .limit = 100,
        }
    );
    const auto notification_exception = FindExceptionRecord(
        exceptions_before_retry.records,
        "NOTIFICATION",
        std::to_string(scenario.notification_id)
    );
    assert(notification_exception != nullptr);
    assert(notification_exception->retryable);

    const auto notification_task_log_before = CountTaskLogsByBizKey(
        config,
        project_root,
        "NOTIFICATION_PUSH",
        std::to_string(scenario.notification_id)
    );

    ExpectAuthError(
        ErrorCode::kAuthPermissionDenied,
        [&]() {
            (void)ops_service.RetryFailedNotifications(
                support_header,
                {
                    .notification_id = scenario.notification_id,
                    .limit = 20,
                }
            );
        }
    );

    const auto retry_result = ops_service.RetryFailedNotifications(
        admin_header,
        {
            .notification_id = scenario.notification_id,
            .limit = 20,
        }
    );
    assert(retry_result.scanned == 1);
    assert(retry_result.succeeded == 1);
    assert(retry_result.failed == 0);
    assert(retry_result.skipped == 0);
    assert(Contains(retry_result.affected_notification_ids, scenario.notification_id));
    assert(QueryNotificationPushStatus(config, project_root, scenario.notification_id) == "SENT");
    assert(
        CountTaskLogsByBizKey(
            config,
            project_root,
            "NOTIFICATION_PUSH",
            std::to_string(scenario.notification_id)
        ) >=
        notification_task_log_before + 1
    );

    const auto events_after_retry = event_gateway.SnapshotPublishedEvents();
    assert(std::any_of(
        events_after_retry.begin(),
        events_after_retry.end(),
        [&](const auction::ws::PublishedAuctionEvent& event) {
            return event.user_id.has_value() && *event.user_id == scenario.first_bidder.user_id &&
                   event.message.event == "OUTBID";
        }
    ));

    const auto degradation_mark = ops_service.MarkException(
        support_header,
        {
            .exception_type = "REDIS_DEGRADED",
            .biz_key = "bid_cache_" + unique_suffix,
            .detail = "fallback to mysql reads",
        }
    );
    assert(degradation_mark.exception_type == "REDIS_DEGRADED");

    UpdateAuctionWindow(config, project_root, scenario.auction_id, -600, -5);
    const auto finished = auction_scheduler.RunFinishCycle();
    assert(Contains(finished.affected_auction_ids, scenario.auction_id));

    const auto compensation_result = ops_service.RunCompensation(
        admin_header,
        {
            .compensation_type = "ORDER_SETTLEMENT",
            .biz_key = "",
            .limit = 20,
        }
    );
    assert(compensation_result.succeeded >= 1);

    const auto order_id = QueryOrderIdByAuction(config, project_root, scenario.auction_id);
    assert(order_id > 0);

    const auto initiated_payment = payment_service.InitiatePayment(
        scenario.second_bidder.header,
        order_id,
        {
            .pay_channel = "MOCK_ALIPAY",
        }
    );

    ExpectPaymentError(
        ErrorCode::kPaymentSignatureInvalid,
        [&]() {
            (void)payment_service.HandlePaymentCallback({
                .payment_no = initiated_payment.payment_no,
                .order_no = initiated_payment.order_no,
                .merchant_no = initiated_payment.merchant_no,
                .transaction_no = "TXN_OPS_REJECTED_" + unique_suffix,
                .pay_status = "SUCCESS",
                .pay_amount = initiated_payment.pay_amount,
                .signature = "invalid-signature",
            });
        }
    );
    assert(CountRejectedCallbackLogsByOrder(config, project_root, order_id) >= 1);

    const auto exception_records = ops_service.ListSystemExceptions(
        admin_header,
        {
            .limit = 100,
        }
    );
    assert(
        FindExceptionRecord(
            exception_records.records,
            "NOTIFICATION",
            std::to_string(scenario.notification_id)
        ) == nullptr
    );
    assert(
        FindExceptionByBizKey(
            exception_records.records,
            "MANUAL_MARK",
            "bid_cache_" + unique_suffix
        ) != nullptr
    );
    assert(
        FindExceptionByBizKey(
            exception_records.records,
            "PAYMENT_CALLBACK",
            std::to_string(order_id)
        ) != nullptr
    );

    const auto operation_logs = ops_service.ListOperationLogs(
        support_header,
        {
            .module_name = std::string("ops"),
            .limit = 100,
        }
    );
    assert(
        HasOperationLog(
            operation_logs.records,
            "mark_exception:REDIS_DEGRADED",
            "bid_cache_" + unique_suffix
        )
    );
    assert(
        HasOperationLog(operation_logs.records, "retry_failed_notifications", std::to_string(scenario.notification_id))
    );

    const auto task_logs = ops_service.ListTaskLogs(
        support_header,
        {
            .task_type = std::string("NOTIFICATION_PUSH"),
            .limit = 100,
        }
    );
    assert(
        HasTaskLog(task_logs.records, "NOTIFICATION_PUSH", std::to_string(scenario.notification_id))
    );

    ExpectAuthError(
        ErrorCode::kAuthPermissionDenied,
        [&]() {
            (void)ops_service.ListOperationLogs(
                scenario.first_bidder.header,
                {
                    .limit = 20,
                }
            );
        }
    );

    ExpectOpsError(
        ErrorCode::kOpsCompensationTypeInvalid,
        [&]() {
            (void)ops_service.RunCompensation(
                admin_header,
                {
                    .compensation_type = "UNKNOWN_COMPENSATION",
                    .biz_key = "",
                    .limit = 20,
                }
            );
        }
    );

    return 0;
}
