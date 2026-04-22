#include <algorithm>
#include <cassert>
#include <chrono>
#include <filesystem>
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
#include "modules/order/order_service.h"
#include "ws/auction_event_gateway.h"

namespace {

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
    auto connection = CreateConnection(config, project_root);
    return connection.QueryCount(
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

std::uint64_t CountNotificationsByOrder(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t order_id
) {
    return QueryUint64(
        config,
        project_root,
        "SELECT COUNT(*) FROM notification WHERE biz_type = 'ORDER' AND biz_id = " +
            std::to_string(order_id)
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

void UpdateAuctionEndToPast(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t auction_id
) {
    auto connection = CreateConnection(config, project_root);
    connection.Execute(
        "UPDATE auction SET end_time = DATE_SUB(CURRENT_TIMESTAMP(3), INTERVAL 5 SECOND) "
        "WHERE auction_id = " +
        std::to_string(auction_id)
    );
}

void UpdateOrderDeadline(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t order_id,
    const int offset_seconds
) {
    auto connection = CreateConnection(config, project_root);
    connection.Execute(
        "UPDATE order_info SET pay_deadline_at = DATE_ADD(CURRENT_TIMESTAMP(3), INTERVAL " +
        std::to_string(offset_seconds) + " SECOND) WHERE order_id = " +
        std::to_string(order_id)
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
            .title = "Order test item " + unique_suffix,
            .description = "used for S08 order tests",
            .category_id = 1,
            .start_price = 120.0,
            .cover_image_url = "",
        }
    );
    const auto image = item_service.AddItemImage(
        seller_header,
        created.item_id,
        {
            .image_url = "/images/order/" + unique_suffix + ".jpg",
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
            .reason = "approved for order tests",
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
    result.username = username_prefix + "_" + unique_suffix;
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
            .start_price = 120.0,
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
    auction::jobs::AuctionScheduler auction_scheduler(auction_service);
    auction::jobs::OrderScheduler order_scheduler(order_service);

    const auto unique_suffix = BuildUniqueSuffix();
    const auto admin_login = auth_service.Login({
        .principal = "admin",
        .password = "Admin@123",
    });
    const auto admin_header = "Bearer " + admin_login.token;

    const auto seller = RegisterAndLoginUser(
        auth_service,
        unique_suffix,
        "order_seller",
        1
    );
    const auto buyer = RegisterAndLoginUser(
        auth_service,
        unique_suffix,
        "order_buyer",
        2
    );

    const auto auction_id = CreateRunningAuction(
        item_service,
        item_audit_service,
        auction_service,
        auction_scheduler,
        config,
        project_root,
        seller.header,
        admin_header,
        unique_suffix
    );

    const auto bid_result = bid_service.PlaceBid(
        buyer.header,
        auction_id,
        {
            .request_id = "order-bid-" + unique_suffix,
            .bid_amount = 150.0,
        }
    );
    assert(bid_result.bid_status == "WINNING");

    UpdateAuctionEndToPast(config, project_root, auction_id);
    const auto finished = auction_scheduler.RunFinishCycle();
    assert(Contains(finished.affected_auction_ids, auction_id));
    assert(QueryAuctionStatus(config, project_root, auction_id) == "SETTLING");
    assert(CountOrdersByAuction(config, project_root, auction_id) == 0);

    const auto settlement_task_before =
        CountTaskLogByType(config, project_root, "ORDER_SETTLEMENT");
    const auto settlement_result = order_scheduler.RunSettlementCycle();
    assert(settlement_result.succeeded >= 1);

    const auto order_id = QueryOrderIdByAuction(config, project_root, auction_id);
    assert(order_id > 0);
    assert(Contains(settlement_result.affected_order_ids, order_id));
    assert(CountOrdersByAuction(config, project_root, auction_id) == 1);
    assert(QueryOrderStatus(config, project_root, order_id) == "PENDING_PAYMENT");
    assert(QueryAuctionStatus(config, project_root, auction_id) == "SETTLING");
    assert(QueryItemStatusByAuction(config, project_root, auction_id) == "IN_AUCTION");
    assert(CountNotificationsByOrder(config, project_root, order_id) == 2);
    assert(CountTaskLogByType(config, project_root, "ORDER_SETTLEMENT") >= settlement_task_before + 1);

    const auto settlement_result_again = order_scheduler.RunSettlementCycle();
    (void)settlement_result_again;
    assert(CountOrdersByAuction(config, project_root, auction_id) == 1);

    const auto timeout_task_before =
        CountTaskLogByType(config, project_root, "ORDER_TIMEOUT_CLOSE");
    UpdateOrderDeadline(config, project_root, order_id, -5);
    const auto timeout_result = order_scheduler.RunTimeoutCloseCycle();
    assert(timeout_result.succeeded >= 1);
    assert(Contains(timeout_result.affected_order_ids, order_id));
    assert(QueryOrderStatus(config, project_root, order_id) == "CLOSED");
    assert(QueryAuctionStatus(config, project_root, auction_id) == "UNSOLD");
    assert(QueryItemStatusByAuction(config, project_root, auction_id) == "UNSOLD");
    assert(CountNotificationsByOrder(config, project_root, order_id) == 4);
    assert(
        CountTaskLogByType(config, project_root, "ORDER_TIMEOUT_CLOSE") >=
        timeout_task_before + 1
    );

    return 0;
}
