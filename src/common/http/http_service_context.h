#pragma once

#include <filesystem>

#include "common/config/app_config.h"
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
#include "modules/review/review_service.h"
#include "modules/statistics/statistics_service.h"
#include "ws/auction_event_gateway.h"

namespace auction::common::http {

class HttpServiceContext {
public:
    HttpServiceContext(
        const config::AppConfig& config,
        std::filesystem::path project_root
    );

    [[nodiscard]] modules::auth::AuthService& auth_service();
    [[nodiscard]] middleware::AuthMiddleware& auth_middleware();
    [[nodiscard]] modules::notification::NotificationService& notification_service();
    [[nodiscard]] modules::item::ItemService& item_service();
    [[nodiscard]] modules::audit::ItemAuditService& item_audit_service();
    [[nodiscard]] modules::auction::AuctionService& auction_service();
    [[nodiscard]] modules::bid::BidService& bid_service();
    [[nodiscard]] modules::order::OrderService& order_service();
    [[nodiscard]] modules::payment::PaymentService& payment_service();
    [[nodiscard]] modules::review::ReviewService& review_service();
    [[nodiscard]] modules::statistics::StatisticsService& statistics_service();
    [[nodiscard]] modules::ops::OpsService& ops_service();

private:
    config::AppConfig config_;
    std::filesystem::path project_root_;
    modules::auth::InMemoryAuthSessionStore auth_session_store_;
    ws::InMemoryAuctionEventGateway event_gateway_;
    modules::bid::InMemoryBidCacheStore bid_cache_store_;
    modules::auth::AuthService auth_service_;
    middleware::AuthMiddleware auth_middleware_;
    modules::notification::NotificationService notification_service_;
    modules::item::ItemService item_service_;
    modules::audit::ItemAuditService item_audit_service_;
    modules::auction::AuctionService auction_service_;
    modules::bid::BidService bid_service_;
    modules::order::OrderService order_service_;
    modules::payment::PaymentService payment_service_;
    modules::review::ReviewService review_service_;
    modules::statistics::StatisticsService statistics_service_;
    modules::ops::OpsService ops_service_;
};

}  // namespace auction::common::http
