#include <algorithm>
#include <barrier>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cctype>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "common/config/config_loader.h"
#include "common/db/mysql_connection.h"
#include "common/errors/error_code.h"
#include "jobs/auction_scheduler.h"
#include "jobs/order_scheduler.h"
#include "middleware/auth_middleware.h"
#include "modules/audit/item_audit_service.h"
#include "modules/auction/auction_service.h"
#include "modules/auth/auth_service.h"
#include "modules/auth/auth_session_store.h"
#include "modules/bid/bid_cache_store.h"
#include "modules/bid/bid_exception.h"
#include "modules/bid/bid_service.h"
#include "modules/item/item_service.h"
#include "modules/notification/notification_service.h"
#include "modules/order/order_service.h"
#include "modules/payment/payment_exception.h"
#include "modules/payment/payment_service.h"
#include "modules/payment/payment_signature.h"
#include "ws/auction_event_gateway.h"

namespace {

using auction::common::errors::ErrorCode;
using auction::modules::bid::BidException;
using auction::modules::payment::PaymentException;

constexpr double kAmountEpsilon = 0.0001;

struct RegisteredUser {
    std::uint64_t user_id{0};
    std::string username;
    std::string header;
};

struct ServiceBundle {
    auction::modules::auth::AuthService& auth_service;
    auction::modules::item::ItemService& item_service;
    auction::modules::audit::ItemAuditService& item_audit_service;
    auction::modules::auction::AuctionService& auction_service;
    auction::modules::bid::BidService& bid_service;
    auction::modules::notification::NotificationService& notification_service;
    auction::modules::order::OrderService& order_service;
    auction::modules::payment::PaymentService& payment_service;
    auction::jobs::AuctionScheduler& auction_scheduler;
    auction::jobs::OrderScheduler& order_scheduler;
    auction::modules::bid::InMemoryBidCacheStore& cache_store;
    auction::ws::InMemoryAuctionEventGateway& event_gateway;
    const auction::common::config::AppConfig& config;
    const std::filesystem::path& project_root;
};

struct ConcurrentBidOutcome {
    bool success{false};
    ErrorCode error_code{ErrorCode::kOk};
};

struct PaymentCallbackOutcome {
    bool success{false};
    bool idempotent{false};
    ErrorCode error_code{ErrorCode::kOk};
};

struct SettledOrderContext {
    std::uint64_t auction_id{0};
    std::uint64_t order_id{0};
    std::string order_no;
    RegisteredUser seller;
    RegisteredUser buyer;
};

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

std::string CompactToken(const std::string& value) {
    return std::to_string(std::hash<std::string>{}(value));
}

std::string BuildPhoneNumber(const std::string& suffix, const int index) {
    std::string digits;
    digits.reserve(suffix.size() + 24);
    for (const char character : suffix) {
        if (std::isdigit(static_cast<unsigned char>(character)) != 0) {
            digits.push_back(character);
        }
    }
    digits += CompactToken(suffix + ":" + std::to_string(index));
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

bool AmountEquals(const double left, const double right) {
    return std::fabs(left - right) < kAmountEpsilon;
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

double QueryDouble(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::string& sql
) {
    return std::stod(QueryString(config, project_root, sql));
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

std::uint64_t CountBidRecords(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t auction_id
) {
    return QueryUint64(
        config,
        project_root,
        "SELECT COUNT(*) FROM bid_record WHERE auction_id = " + std::to_string(auction_id)
    );
}

std::uint64_t CountWinningBidRecords(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t auction_id
) {
    return QueryUint64(
        config,
        project_root,
        "SELECT COUNT(*) FROM bid_record WHERE auction_id = " + std::to_string(auction_id) +
            " AND bid_status = 'WINNING'"
    );
}

std::uint64_t CountOrdersByAuction(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t auction_id
) {
    return QueryUint64(
        config,
        project_root,
        "SELECT COUNT(*) FROM order_info WHERE auction_id = " + std::to_string(auction_id)
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

std::string QueryOrderNoByOrderId(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t order_id
) {
    return QueryString(
        config,
        project_root,
        "SELECT order_no FROM order_info WHERE order_id = " + std::to_string(order_id)
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

std::uint64_t CountPaymentRecordsByOrder(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t order_id
) {
    return QueryUint64(
        config,
        project_root,
        "SELECT COUNT(*) FROM payment_record WHERE order_id = " + std::to_string(order_id)
    );
}

std::uint64_t CountCallbackLogsByOrder(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t order_id
) {
    return QueryUint64(
        config,
        project_root,
        "SELECT COUNT(*) FROM payment_callback_log WHERE order_id = " + std::to_string(order_id)
    );
}

std::uint64_t CountProcessedCallbackLogsByOrder(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t order_id
) {
    return QueryUint64(
        config,
        project_root,
        "SELECT COUNT(*) FROM payment_callback_log WHERE order_id = " + std::to_string(order_id) +
            " AND callback_status = 'PROCESSED'"
    );
}

std::uint64_t CountFailedNotifications(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t user_id,
    const std::uint64_t auction_id
) {
    return QueryUint64(
        config,
        project_root,
        "SELECT COUNT(*) FROM notification WHERE user_id = " + std::to_string(user_id) +
            " AND biz_type = 'AUCTION' AND biz_id = " + std::to_string(auction_id) +
            " AND push_status = 'FAILED'"
    );
}

std::uint64_t QueryFailedNotificationId(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t user_id,
    const std::uint64_t auction_id
) {
    return QueryUint64(
        config,
        project_root,
        "SELECT COALESCE(MAX(notification_id), 0) FROM notification WHERE user_id = " +
            std::to_string(user_id) + " AND biz_type = 'AUCTION' AND biz_id = " +
            std::to_string(auction_id) + " AND push_status = 'FAILED'"
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

RegisteredUser RegisterAndLoginUser(
    auction::modules::auth::AuthService& auth_service,
    const std::string& unique_suffix,
    const std::string& username_prefix,
    const int index
) {
    RegisteredUser result;
    const auto compact_suffix = CompactToken(unique_suffix + ":" + std::to_string(index));
    const auto max_prefix_length = static_cast<std::size_t>(50 - 1 - compact_suffix.size());
    const auto compact_prefix =
        username_prefix.size() > max_prefix_length ? username_prefix.substr(0, max_prefix_length)
                                                   : username_prefix;
    const auto password = "Password@1";
    result.username = compact_prefix + "_" + compact_suffix;
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

std::uint64_t CreateApprovedItem(
    ServiceBundle& services,
    const std::string& seller_header,
    const std::string& admin_header,
    const std::string& unique_suffix,
    const std::string& title_prefix
) {
    const auto created = services.item_service.CreateDraftItem(
        seller_header,
        {
            .title = title_prefix + " " + unique_suffix,
            .description = "used for S14 high-risk tests",
            .category_id = 1,
            .start_price = 100.0,
            .cover_image_url = "",
        }
    );
    const auto image = services.item_service.AddItemImage(
        seller_header,
        created.item_id,
        {
            .image_url = "/images/s14/" + unique_suffix + "/" + title_prefix + ".jpg",
            .sort_no = std::nullopt,
            .is_cover = true,
        }
    );
    assert(image.image_id > 0);
    const auto submitted = services.item_service.SubmitForAudit(seller_header, created.item_id);
    assert(submitted.item_status == "PENDING_AUDIT");
    const auto approved = services.item_audit_service.AuditItem(
        admin_header,
        created.item_id,
        {
            .audit_result = "APPROVED",
            .reason = "approved for S14 high-risk tests",
        }
    );
    assert(approved.new_status == "READY_FOR_AUCTION");
    return created.item_id;
}

std::uint64_t CreateRunningAuction(
    ServiceBundle& services,
    const std::string& seller_header,
    const std::string& admin_header,
    const std::string& unique_suffix,
    const std::string& title_prefix,
    const int end_offset_seconds
) {
    const auto item_id = CreateApprovedItem(
        services,
        seller_header,
        admin_header,
        unique_suffix,
        title_prefix
    );
    const auto created = services.auction_service.CreateAuction(
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
    UpdateAuctionWindow(
        services.config,
        services.project_root,
        created.auction_id,
        -5,
        end_offset_seconds
    );
    const auto started = services.auction_scheduler.RunStartCycle();
    assert(Contains(started.affected_auction_ids, created.auction_id));
    return created.auction_id;
}

SettledOrderContext CreateSettledOrder(
    ServiceBundle& services,
    const std::string& admin_header,
    const std::string& unique_suffix,
    const int start_index
) {
    SettledOrderContext context;
    context.seller = RegisterAndLoginUser(
        services.auth_service,
        unique_suffix,
        "risk_payment_seller",
        start_index
    );
    context.buyer = RegisterAndLoginUser(
        services.auth_service,
        unique_suffix,
        "risk_payment_buyer",
        start_index + 1
    );

    context.auction_id = CreateRunningAuction(
        services,
        context.seller.header,
        admin_header,
        unique_suffix,
        "risk-payment-item",
        300
    );
    const auto bid = services.bid_service.PlaceBid(
        context.buyer.header,
        context.auction_id,
        {
            .request_id = "risk-payment-bid-" + unique_suffix,
            .bid_amount = 150.0,
        }
    );
    assert(bid.bid_status == "WINNING");

    UpdateAuctionEndToPast(services.config, services.project_root, context.auction_id);
    const auto finished = services.auction_scheduler.RunFinishCycle();
    assert(Contains(finished.affected_auction_ids, context.auction_id));
    const auto settled = services.order_scheduler.RunSettlementCycle();
    assert(settled.succeeded >= 1);

    context.order_id =
        QueryOrderIdByAuction(services.config, services.project_root, context.auction_id);
    assert(context.order_id > 0);
    context.order_no =
        QueryOrderNoByOrderId(services.config, services.project_root, context.order_id);
    return context;
}

void RunConcurrentBidConsistency(
    ServiceBundle& services,
    const std::string& seller_header,
    const std::string& admin_header,
    const std::vector<RegisteredUser>& bidders,
    const std::string& unique_suffix
) {
    const auto auction_id = CreateRunningAuction(
        services,
        seller_header,
        admin_header,
        unique_suffix + "_concurrent",
        "risk-concurrent-item",
        300
    );

    std::barrier sync_point(static_cast<std::ptrdiff_t>(bidders.size() + 1));
    std::vector<ConcurrentBidOutcome> outcomes(bidders.size());
    std::vector<std::thread> threads;
    threads.reserve(bidders.size());

    for (std::size_t index = 0; index < bidders.size(); ++index) {
        threads.emplace_back([&, index] {
            sync_point.arrive_and_wait();
            try {
                const auto ignored = services.bid_service.PlaceBid(
                    bidders[index].header,
                    auction_id,
                    {
                        .request_id = "risk-concurrent-bid-" + std::to_string(index),
                        .bid_amount = 100.0,
                    }
                );
                (void)ignored;
                outcomes[index].success = true;
            } catch (const BidException& exception) {
                outcomes[index].error_code = exception.code();
            }
        });
    }

    sync_point.arrive_and_wait();
    for (auto& thread : threads) {
        thread.join();
    }

    const auto success_count = std::count_if(outcomes.begin(), outcomes.end(), [](const auto& outcome) {
        return outcome.success;
    });
    assert(success_count == 1);
    for (const auto& outcome : outcomes) {
        if (outcome.success) {
            continue;
        }
        assert(
            outcome.error_code == ErrorCode::kBidConflict ||
            outcome.error_code == ErrorCode::kBidAmountTooLow
        );
    }
    assert(CountBidRecords(services.config, services.project_root, auction_id) == 1);
    assert(CountWinningBidRecords(services.config, services.project_root, auction_id) == 1);
    assert(AmountEquals(
        QueryDouble(
            services.config,
            services.project_root,
            "SELECT CAST(current_price AS CHAR) FROM auction WHERE auction_id = " +
                std::to_string(auction_id)
        ),
        100.0
    ));
}

void RunCacheDegradationScenario(
    ServiceBundle& services,
    const std::string& seller_header,
    const std::string& admin_header,
    const RegisteredUser& buyer_1,
    const RegisteredUser& buyer_2,
    const std::string& unique_suffix
) {
    const auto auction_id = CreateRunningAuction(
        services,
        seller_header,
        admin_header,
        unique_suffix + "_cache",
        "risk-cache-item",
        300
    );

    services.cache_store.SetFailUpsert(true);
    const auto first_bid = services.bid_service.PlaceBid(
        buyer_1.header,
        auction_id,
        {
            .request_id = "risk-cache-bid-1",
            .bid_amount = 100.0,
        }
    );
    services.cache_store.SetFailUpsert(false);
    assert(first_bid.bid_status == "WINNING");
    assert(CountBidRecords(services.config, services.project_root, auction_id) == 1);
    assert(services.cache_store.FindAuctionSnapshot(auction_id) == std::nullopt);
    assert(AmountEquals(
        QueryDouble(
            services.config,
            services.project_root,
            "SELECT CAST(current_price AS CHAR) FROM auction WHERE auction_id = " +
                std::to_string(auction_id)
        ),
        100.0
    ));

    const auto second_bid = services.bid_service.PlaceBid(
        buyer_2.header,
        auction_id,
        {
            .request_id = "risk-cache-bid-2",
            .bid_amount = 110.0,
        }
    );
    assert(second_bid.current_price == 110.0);
    const auto cache_snapshot = services.cache_store.FindAuctionSnapshot(auction_id);
    assert(cache_snapshot.has_value());
    assert(AmountEquals(cache_snapshot->current_price, 110.0));
}

void RunNotificationFailureAndRetryScenario(
    ServiceBundle& services,
    const std::string& seller_header,
    const std::string& admin_header,
    const RegisteredUser& buyer_1,
    const RegisteredUser& buyer_2,
    const std::string& unique_suffix
) {
    const auto auction_id = CreateRunningAuction(
        services,
        seller_header,
        admin_header,
        unique_suffix + "_notify",
        "risk-notify-item",
        300
    );
    const auto first_bid = services.bid_service.PlaceBid(
        buyer_1.header,
        auction_id,
        {
            .request_id = "risk-notify-bid-1",
            .bid_amount = 100.0,
        }
    );
    assert(first_bid.bid_status == "WINNING");

    services.event_gateway.SetFailDirect(true);
    const auto second_bid = services.bid_service.PlaceBid(
        buyer_2.header,
        auction_id,
        {
            .request_id = "risk-notify-bid-2",
            .bid_amount = 110.0,
        }
    );
    services.event_gateway.SetFailDirect(false);
    assert(second_bid.current_price == 110.0);
    assert(CountFailedNotifications(services.config, services.project_root, buyer_1.user_id, auction_id) == 1);

    const auto notification_id =
        QueryFailedNotificationId(services.config, services.project_root, buyer_1.user_id, auction_id);
    assert(notification_id > 0);
    const auto retry_result = services.notification_service.RetryFailedNotifications({
        .notification_id = notification_id,
        .limit = 20,
    });
    assert(retry_result.succeeded == 1);
    assert(QueryNotificationPushStatus(services.config, services.project_root, notification_id) == "SENT");
}

std::uint64_t RunAuctionCloseAndSettlementRace(
    ServiceBundle& services,
    const std::string& seller_header,
    const std::string& admin_header,
    const RegisteredUser& winner,
    const RegisteredUser& late_bidder,
    const std::string& unique_suffix
) {
    const auto auction_id = CreateRunningAuction(
        services,
        seller_header,
        admin_header,
        unique_suffix + "_finish",
        "risk-finish-item",
        300
    );
    const auto winning_bid = services.bid_service.PlaceBid(
        winner.header,
        auction_id,
        {
            .request_id = "risk-finish-bid-1",
            .bid_amount = 150.0,
        }
    );
    assert(winning_bid.bid_status == "WINNING");
    UpdateAuctionEndToPast(services.config, services.project_root, auction_id);

    std::barrier finish_sync(3);
    auction::modules::auction::AuctionScheduleResult finish_result;
    ConcurrentBidOutcome late_outcome;
    std::thread finish_thread([&] {
        finish_sync.arrive_and_wait();
        finish_result = services.auction_scheduler.RunFinishCycle();
    });
    std::thread late_bid_thread([&] {
        finish_sync.arrive_and_wait();
        try {
            const auto ignored = services.bid_service.PlaceBid(
                late_bidder.header,
                auction_id,
                {
                    .request_id = "risk-finish-late-bid",
                    .bid_amount = 160.0,
                }
            );
            (void)ignored;
            late_outcome.success = true;
        } catch (const BidException& exception) {
            late_outcome.error_code = exception.code();
        }
    });
    finish_sync.arrive_and_wait();
    finish_thread.join();
    late_bid_thread.join();

    assert(!late_outcome.success);
    assert(late_outcome.error_code == ErrorCode::kBidAuctionClosed);
    assert(finish_result.failed == 0);
    assert(QueryAuctionStatus(services.config, services.project_root, auction_id) == "SETTLING");
    assert(CountBidRecords(services.config, services.project_root, auction_id) == 1);
    assert(CountWinningBidRecords(services.config, services.project_root, auction_id) == 1);

    constexpr int kSettlementWorkers = 4;
    std::barrier settlement_sync(kSettlementWorkers + 1);
    std::vector<auction::modules::order::OrderScheduleResult> settlement_results(kSettlementWorkers);
    std::vector<std::thread> settlement_threads;
    settlement_threads.reserve(kSettlementWorkers);
    for (int index = 0; index < kSettlementWorkers; ++index) {
        settlement_threads.emplace_back([&, index] {
            settlement_sync.arrive_and_wait();
            settlement_results[index] = services.order_scheduler.RunSettlementCycle();
        });
    }
    settlement_sync.arrive_and_wait();
    for (auto& thread : settlement_threads) {
        thread.join();
    }

    assert(CountOrdersByAuction(services.config, services.project_root, auction_id) == 1);
    const auto order_id = QueryOrderIdByAuction(services.config, services.project_root, auction_id);
    assert(order_id > 0);
    assert(QueryOrderStatus(services.config, services.project_root, order_id) == "PENDING_PAYMENT");
    assert(std::any_of(
        settlement_results.begin(),
        settlement_results.end(),
        [](const auto& result) { return result.succeeded >= 1; }
    ));
    return order_id;
}

void RunConcurrentPaymentCallbackScenario(
    ServiceBundle& services,
    const std::string& admin_header,
    const std::string& unique_suffix
) {
    const auto context = CreateSettledOrder(
        services,
        admin_header,
        unique_suffix + "_payment",
        200
    );
    const auto initiated = services.payment_service.InitiatePayment(
        context.buyer.header,
        context.order_id,
        {
            .pay_channel = "MOCK_ALIPAY",
        }
    );
    assert(initiated.pay_status == "WAITING_CALLBACK");
    assert(CountPaymentRecordsByOrder(services.config, services.project_root, context.order_id) == 1);

    auction::modules::payment::PaymentCallbackRequest success_callback{
        .payment_no = initiated.payment_no,
        .order_no = initiated.order_no,
        .merchant_no = initiated.merchant_no,
        .transaction_no = "TXN_RISK_" + unique_suffix,
        .pay_status = "SUCCESS",
        .pay_amount = initiated.pay_amount,
        .signature = "",
    };
    success_callback.signature = auction::modules::payment::SignMockPaymentCallback(
        services.config.auth.token_secret,
        success_callback
    );

    constexpr int kCallbackWorkers = 6;
    std::barrier callback_sync(kCallbackWorkers + 1);
    std::vector<PaymentCallbackOutcome> outcomes(kCallbackWorkers);
    std::vector<std::thread> callback_threads;
    callback_threads.reserve(kCallbackWorkers);
    for (int index = 0; index < kCallbackWorkers; ++index) {
        callback_threads.emplace_back([&, index] {
            callback_sync.arrive_and_wait();
            try {
                const auto result =
                    services.payment_service.HandlePaymentCallback(success_callback);
                outcomes[index].success = true;
                outcomes[index].idempotent = result.idempotent;
            } catch (const PaymentException& exception) {
                outcomes[index].error_code = exception.code();
            }
        });
    }
    callback_sync.arrive_and_wait();
    for (auto& thread : callback_threads) {
        thread.join();
    }

    const auto success_count = std::count_if(outcomes.begin(), outcomes.end(), [](const auto& outcome) {
        return outcome.success;
    });
    const auto non_idempotent_count =
        std::count_if(outcomes.begin(), outcomes.end(), [](const auto& outcome) {
            return outcome.success && !outcome.idempotent;
        });
    assert(success_count == kCallbackWorkers);
    assert(non_idempotent_count == 1);
    assert(QueryOrderStatus(services.config, services.project_root, context.order_id) == "PAID");
    assert(QueryAuctionStatus(services.config, services.project_root, context.auction_id) == "SOLD");
    assert(QueryItemStatusByAuction(services.config, services.project_root, context.auction_id) == "SOLD");
    assert(CountPaymentRecordsByOrder(services.config, services.project_root, context.order_id) == 1);
    assert(CountCallbackLogsByOrder(services.config, services.project_root, context.order_id) ==
           kCallbackWorkers);
    assert(CountProcessedCallbackLogsByOrder(services.config, services.project_root, context.order_id) ==
           kCallbackWorkers);

    auto failed_after_success = success_callback;
    failed_after_success.pay_status = "FAILED";
    failed_after_success.signature = auction::modules::payment::SignMockPaymentCallback(
        services.config.auth.token_secret,
        failed_after_success
    );
    const auto failed_result = services.payment_service.HandlePaymentCallback(failed_after_success);
    assert(failed_result.idempotent);
    assert(QueryOrderStatus(services.config, services.project_root, context.order_id) == "PAID");
    assert(CountCallbackLogsByOrder(services.config, services.project_root, context.order_id) ==
           kCallbackWorkers + 1);
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
    auction::jobs::AuctionScheduler auction_scheduler(auction_service);
    auction::jobs::OrderScheduler order_scheduler(order_service);
    ServiceBundle services{
        .auth_service = auth_service,
        .item_service = item_service,
        .item_audit_service = item_audit_service,
        .auction_service = auction_service,
        .bid_service = bid_service,
        .notification_service = notification_service,
        .order_service = order_service,
        .payment_service = payment_service,
        .auction_scheduler = auction_scheduler,
        .order_scheduler = order_scheduler,
        .cache_store = cache_store,
        .event_gateway = event_gateway,
        .config = config,
        .project_root = project_root,
    };

    const auto admin_login = auth_service.Login({
        .principal = "admin",
        .password = "Admin@123",
    });
    const auto admin_header = "Bearer " + admin_login.token;
    const auto unique_suffix = BuildUniqueSuffix();
    const auto seller = RegisterAndLoginUser(auth_service, unique_suffix, "risk_seller", 1);

    std::vector<RegisteredUser> bidders;
    bidders.reserve(12);
    for (int index = 0; index < 12; ++index) {
        bidders.push_back(RegisterAndLoginUser(
            auth_service,
            unique_suffix,
            "risk_bidder",
            index + 10
        ));
    }

    RunConcurrentBidConsistency(services, seller.header, admin_header, bidders, unique_suffix);
    RunCacheDegradationScenario(
        services,
        seller.header,
        admin_header,
        bidders.at(0),
        bidders.at(1),
        unique_suffix
    );
    RunNotificationFailureAndRetryScenario(
        services,
        seller.header,
        admin_header,
        bidders.at(2),
        bidders.at(3),
        unique_suffix
    );
    const auto race_order_id = RunAuctionCloseAndSettlementRace(
        services,
        seller.header,
        admin_header,
        bidders.at(4),
        bidders.at(5),
        unique_suffix
    );
    assert(race_order_id > 0);
    RunConcurrentPaymentCallbackScenario(services, admin_header, unique_suffix);

    return 0;
}
