#include <algorithm>
#include <barrier>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

#include "common/config/config_loader.h"
#include "common/db/mysql_connection.h"
#include "common/errors/error_code.h"
#include "jobs/auction_scheduler.h"
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
#include "repository/bid_repository.h"
#include "ws/auction_event_gateway.h"

namespace {

using auction::common::errors::ErrorCode;
using auction::modules::bid::BidException;

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
    std::string digits = suffix + std::to_string(index);
    if (digits.size() < 9) {
        digits.insert(digits.begin(), 9 - digits.size(), '0');
    }
    digits = digits.substr(digits.size() - 9);
    return "13" + digits;
}

template <typename Fn>
void ExpectBidError(const ErrorCode expected_code, Fn&& function) {
    try {
        function();
        assert(false && "expected bid exception");
    } catch (const BidException& exception) {
        assert(exception.code() == expected_code);
    }
}

auction::common::db::MysqlConnection CreateConnection(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root
) {
    return auction::common::db::MysqlConnection(config.mysql, project_root);
}

std::uint64_t CountBidRecords(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t auction_id
) {
    auto connection = CreateConnection(config, project_root);
    return connection.QueryCount(
        "SELECT COUNT(*) FROM bid_record WHERE auction_id = " + std::to_string(auction_id)
    );
}

std::uint64_t CountWinningBidRecords(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t auction_id
) {
    auto connection = CreateConnection(config, project_root);
    return connection.QueryCount(
        "SELECT COUNT(*) FROM bid_record WHERE auction_id = " + std::to_string(auction_id) +
        " AND bid_status = 'WINNING'"
    );
}

std::uint64_t CountFailedNotifications(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t user_id,
    const std::uint64_t auction_id
) {
    auto connection = CreateConnection(config, project_root);
    return connection.QueryCount(
        "SELECT COUNT(*) FROM notification WHERE user_id = " + std::to_string(user_id) +
        " AND biz_id = " + std::to_string(auction_id) + " AND push_status = 'FAILED'"
    );
}

std::uint64_t CountTaskLogByType(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::string& task_type
) {
    auto connection = CreateConnection(config, project_root);
    return connection.QueryCount(
        "SELECT COUNT(*) FROM task_log WHERE task_type = '" + task_type + "'"
    );
}

std::string QueryBidStatus(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t bid_id
) {
    auto connection = CreateConnection(config, project_root);
    return connection.QueryString(
        "SELECT bid_status FROM bid_record WHERE bid_id = " + std::to_string(bid_id)
    );
}

void UpdateAuctionWindow(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t auction_id,
    const int start_offset_seconds,
    const int end_offset_seconds
) {
    auto connection = CreateConnection(config, project_root);
    connection.Execute(
        "UPDATE auction SET start_time = DATE_ADD(CURRENT_TIMESTAMP(3), INTERVAL " +
        std::to_string(start_offset_seconds) + " SECOND), end_time = DATE_ADD(CURRENT_TIMESTAMP(3), INTERVAL " +
        std::to_string(end_offset_seconds) + " SECOND) WHERE auction_id = " +
        std::to_string(auction_id)
    );
}

std::uint64_t CreateApprovedItem(
    auction::modules::item::ItemService& item_service,
    auction::modules::audit::ItemAuditService& item_audit_service,
    const std::string& seller_header,
    const std::string& admin_header,
    const std::string& unique_suffix,
    const std::string& title_prefix
) {
    const auto created = item_service.CreateDraftItem(
        seller_header,
        {
            .title = title_prefix + " " + unique_suffix,
            .description = "used for S07 tests",
            .category_id = 1,
            .start_price = 100.0,
            .cover_image_url = "",
        }
    );
    const auto image = item_service.AddItemImage(
        seller_header,
        created.item_id,
        {
            .image_url = "/images/" + unique_suffix + "/" + title_prefix + ".jpg",
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
            .reason = "approved for bid tests",
        }
    );
    assert(approved.new_status == "READY_FOR_AUCTION");
    return created.item_id;
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
    const std::string& unique_suffix,
    const std::string& title_prefix,
    const int end_offset_seconds,
    const int anti_sniping_window_seconds,
    const int extend_seconds
) {
    const auto item_id = CreateApprovedItem(
        item_service,
        item_audit_service,
        seller_header,
        admin_header,
        unique_suffix,
        title_prefix
    );
    const auto created = auction_service.CreateAuction(
        admin_header,
        {
            .item_id = item_id,
            .start_time = FormatNowOffsetSeconds(600),
            .end_time = FormatNowOffsetSeconds(1200),
            .start_price = 100.0,
            .bid_step = 10.0,
            .anti_sniping_window_seconds = anti_sniping_window_seconds,
            .extend_seconds = extend_seconds,
        }
    );
    UpdateAuctionWindow(config, project_root, created.auction_id, -5, end_offset_seconds);
    const auto schedule_result = auction_scheduler.RunStartCycle();
    assert(std::find(
               schedule_result.affected_auction_ids.begin(),
               schedule_result.affected_auction_ids.end(),
               created.auction_id
           ) != schedule_result.affected_auction_ids.end());
    return created.auction_id;
}

struct RegisteredUser {
    std::uint64_t user_id{0};
    std::string username;
    std::string password;
    std::string header;
};

RegisteredUser RegisterAndLoginUser(
    auction::modules::auth::AuthService& auth_service,
    const std::string& unique_suffix,
    const std::string& username_prefix,
    const int index
) {
    RegisteredUser result;
    result.username = username_prefix + "_" + unique_suffix;
    result.password = "Password@1";
    const auto registered = auth_service.RegisterUser({
        .username = result.username,
        .password = result.password,
        .phone = BuildPhoneNumber(unique_suffix, index),
        .email = result.username + "@auction.local",
        .nickname = username_prefix,
    });
    result.user_id = registered.user_id;
    const auto login = auth_service.Login({
        .principal = result.username,
        .password = result.password,
    });
    result.header = "Bearer " + login.token;
    return result;
}

struct ConcurrentBidOutcome {
    bool success{false};
    ErrorCode error_code{ErrorCode::kOk};
};

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
    auction::jobs::AuctionScheduler auction_scheduler(auction_service);
    auction::ws::InMemoryAuctionEventGateway event_gateway;
    auction::modules::bid::InMemoryBidCacheStore cache_store;
    auction::modules::notification::NotificationService notification_service(
        config,
        project_root,
        event_gateway
    );
    auction::modules::bid::BidService bid_service(
        config,
        project_root,
        auth_middleware,
        notification_service,
        cache_store
    );

    const auto unique_suffix = BuildUniqueSuffix();
    const auto seller = RegisterAndLoginUser(auth_service, unique_suffix, "bid_seller", 1);
    const auto buyer_1 = RegisterAndLoginUser(auth_service, unique_suffix, "bid_buyer_a", 2);
    const auto buyer_2 = RegisterAndLoginUser(auth_service, unique_suffix, "bid_buyer_b", 3);
    const auto buyer_3 = RegisterAndLoginUser(auth_service, unique_suffix, "bid_buyer_c", 4);
    const auto admin_login = auth_service.Login({
        .principal = "admin",
        .password = "Admin@123",
    });
    const auto admin_header = "Bearer " + admin_login.token;

    const auto basic_auction_id = CreateRunningAuction(
        item_service,
        item_audit_service,
        auction_service,
        auction_scheduler,
        config,
        project_root,
        seller.header,
        admin_header,
        unique_suffix + "_basic",
        "basic-bid-item",
        300,
        0,
        0
    );

    const auto first_bid = bid_service.PlaceBid(
        buyer_1.header,
        basic_auction_id,
        {
            .request_id = "basic-bid-1",
            .bid_amount = 100.0,
        }
    );
    assert(first_bid.bid_id > 0);
    assert(first_bid.bid_status == "WINNING");
    assert(first_bid.current_price == 100.0);
    assert(!first_bid.extended);

    const auto history = bid_service.ListAuctionBidHistory(
        basic_auction_id,
        {
            .page_no = 1,
            .page_size = 20,
        }
    );
    assert(history.records.size() == 1);
    assert(history.records.front().bid_id == first_bid.bid_id);
    assert(history.records.front().bidder_masked.find("***") != std::string::npos);

    const auto my_status = bid_service.GetMyBidStatus(buyer_1.header, basic_auction_id);
    assert(my_status.my_highest_bid == 100.0);
    assert(my_status.is_current_highest);
    assert(my_status.current_price == 100.0);

    const auto cache_snapshot = cache_store.FindAuctionSnapshot(basic_auction_id);
    assert(cache_snapshot.has_value());
    assert(cache_snapshot->current_price == 100.0);
    assert(cache_snapshot->minimum_next_bid == 110.0);

    const auto published_events = event_gateway.SnapshotPublishedEvents();
    assert(std::any_of(published_events.begin(), published_events.end(), [&](const auto& event) {
        return event.auction_channel_id.has_value() &&
               *event.auction_channel_id == basic_auction_id &&
               event.message.event == "PRICE_UPDATED";
    }));

    const auto retry_bid = bid_service.PlaceBid(
        buyer_1.header,
        basic_auction_id,
        {
            .request_id = "basic-bid-1",
            .bid_amount = 100.0,
        }
    );
    assert(retry_bid.bid_id == first_bid.bid_id);
    assert(CountBidRecords(config, project_root, basic_auction_id) == 1);

    ExpectBidError(ErrorCode::kBidAmountTooLow, [&] {
        const auto ignored = bid_service.PlaceBid(
            buyer_2.header,
            basic_auction_id,
            {
                .request_id = "basic-bid-too-low",
                .bid_amount = 105.0,
            }
        );
        (void)ignored;
    });

    const auto anti_sniping_auction_id = CreateRunningAuction(
        item_service,
        item_audit_service,
        auction_service,
        auction_scheduler,
        config,
        project_root,
        seller.header,
        admin_header,
        unique_suffix + "_extend",
        "extend-bid-item",
        10,
        30,
        60
    );
    auto anti_connection = CreateConnection(config, project_root);
    auction::repository::BidRepository anti_repository(anti_connection);
    const auto anti_before =
        anti_repository.FindAuctionSnapshotById(anti_sniping_auction_id).value();
    const auto extended_bid = bid_service.PlaceBid(
        buyer_2.header,
        anti_sniping_auction_id,
        {
            .request_id = "extend-bid-1",
            .bid_amount = 100.0,
        }
    );
    const auto anti_after =
        anti_repository.FindAuctionSnapshotById(anti_sniping_auction_id).value();
    assert(extended_bid.extended);
    assert(anti_before.end_time != anti_after.end_time);
    assert(anti_after.current_price == 100.0);

    const auto notification_failure_auction_id = CreateRunningAuction(
        item_service,
        item_audit_service,
        auction_service,
        auction_scheduler,
        config,
        project_root,
        seller.header,
        admin_header,
        unique_suffix + "_notify",
        "notify-bid-item",
        300,
        0,
        0
    );
    const auto failure_first = bid_service.PlaceBid(
        buyer_1.header,
        notification_failure_auction_id,
        {
            .request_id = "notify-bid-1",
            .bid_amount = 100.0,
        }
    );
    const auto task_log_before = CountTaskLogByType(
        config,
        project_root,
        "NOTIFICATION_PUSH"
    );
    event_gateway.SetFailDirect(true);
    const auto failure_second = bid_service.PlaceBid(
        buyer_2.header,
        notification_failure_auction_id,
        {
            .request_id = "notify-bid-2",
            .bid_amount = 110.0,
        }
    );
    event_gateway.SetFailDirect(false);
    assert(failure_second.bid_id > failure_first.bid_id);
    assert(failure_second.current_price == 110.0);
    assert(QueryBidStatus(config, project_root, failure_first.bid_id) == "OUTBID");
    assert(CountWinningBidRecords(config, project_root, notification_failure_auction_id) == 1);
    assert(
        CountFailedNotifications(
            config,
            project_root,
            buyer_1.user_id,
            notification_failure_auction_id
        ) == 1
    );
    assert(
        CountTaskLogByType(config, project_root, "NOTIFICATION_PUSH") == task_log_before + 1
    );

    const auto concurrent_auction_id = CreateRunningAuction(
        item_service,
        item_audit_service,
        auction_service,
        auction_scheduler,
        config,
        project_root,
        seller.header,
        admin_header,
        unique_suffix + "_concurrent",
        "concurrent-bid-item",
        300,
        0,
        0
    );

    std::barrier sync_point(3);
    ConcurrentBidOutcome outcome_1;
    ConcurrentBidOutcome outcome_2;
    auto concurrent_worker = [&](const std::string& header, const std::string& request_id, ConcurrentBidOutcome& outcome) {
        sync_point.arrive_and_wait();
        try {
            const auto ignored = bid_service.PlaceBid(
                header,
                concurrent_auction_id,
                {
                    .request_id = request_id,
                    .bid_amount = 100.0,
                }
            );
            (void)ignored;
            outcome.success = true;
        } catch (const BidException& exception) {
            outcome.error_code = exception.code();
        }
    };

    std::thread thread_1(concurrent_worker, buyer_2.header, "concurrent-bid-1", std::ref(outcome_1));
    std::thread thread_2(concurrent_worker, buyer_3.header, "concurrent-bid-2", std::ref(outcome_2));
    sync_point.arrive_and_wait();
    thread_1.join();
    thread_2.join();

    assert(outcome_1.success != outcome_2.success);
    const auto failed_code =
        outcome_1.success ? outcome_2.error_code : outcome_1.error_code;
    assert(
        failed_code == ErrorCode::kBidConflict ||
        failed_code == ErrorCode::kBidAmountTooLow
    );
    assert(CountWinningBidRecords(config, project_root, concurrent_auction_id) == 1);
    assert(CountBidRecords(config, project_root, concurrent_auction_id) == 1);

    return 0;
}
