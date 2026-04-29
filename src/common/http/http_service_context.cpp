#include "common/http/http_service_context.h"

namespace auction::common::http {

HttpServiceContext::HttpServiceContext(
    const config::AppConfig& config,
    std::filesystem::path project_root
) : config_(config),
    project_root_(std::move(project_root)),
    auth_service_(config_, project_root_, auth_session_store_),
    auth_middleware_(auth_service_),
    notification_service_(config_, project_root_, event_gateway_),
    item_service_(config_, project_root_, auth_middleware_),
    item_audit_service_(config_, project_root_, auth_middleware_),
    auction_service_(config_, project_root_, auth_middleware_),
    bid_service_(
        config_,
        project_root_,
        auth_middleware_,
        notification_service_,
        bid_cache_store_
    ),
    order_service_(config_, project_root_, auth_middleware_, notification_service_),
    payment_service_(config_, project_root_, auth_middleware_, notification_service_),
    review_service_(config_, project_root_, auth_middleware_, notification_service_),
    statistics_service_(config_, project_root_, auth_middleware_),
    ops_service_(
        config_,
        project_root_,
        auth_middleware_,
        notification_service_,
        order_service_,
        statistics_service_
    ) {}

modules::auth::AuthService& HttpServiceContext::auth_service() {
    return auth_service_;
}

middleware::AuthMiddleware& HttpServiceContext::auth_middleware() {
    return auth_middleware_;
}

modules::notification::NotificationService& HttpServiceContext::notification_service() {
    return notification_service_;
}

modules::item::ItemService& HttpServiceContext::item_service() {
    return item_service_;
}

modules::audit::ItemAuditService& HttpServiceContext::item_audit_service() {
    return item_audit_service_;
}

modules::auction::AuctionService& HttpServiceContext::auction_service() {
    return auction_service_;
}

modules::bid::BidService& HttpServiceContext::bid_service() {
    return bid_service_;
}

modules::order::OrderService& HttpServiceContext::order_service() {
    return order_service_;
}

modules::payment::PaymentService& HttpServiceContext::payment_service() {
    return payment_service_;
}

modules::review::ReviewService& HttpServiceContext::review_service() {
    return review_service_;
}

modules::statistics::StatisticsService& HttpServiceContext::statistics_service() {
    return statistics_service_;
}

modules::ops::OpsService& HttpServiceContext::ops_service() {
    return ops_service_;
}

}  // namespace auction::common::http
