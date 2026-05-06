#include "access/http/business_http.h"

#include <memory>

#include "access/http/api_error_http.h"
#include "access/http/app_http.h"
#include "access/http/auth_http.h"
#include "access/http/audit_http.h"
#include "access/http/auction_admin_http.h"
#include "access/http/auction_public_http.h"
#include "access/http/bid_http.h"
#include "access/http/item_http.h"
#include "access/http/notification_http.h"
#include "access/http/order_http.h"
#include "access/http/payment_http.h"
#include "access/http/review_http.h"
#include "access/http/system_http.h"
#include "common/http/http_service_context.h"

namespace auction::access::http {

void RegisterBusinessHttpRoutes(
    const common::config::AppConfig& config,
    const std::filesystem::path& project_root
) {
    auto services = std::make_shared<common::http::HttpServiceContext>(config, project_root);
    RegisterAuthHttpRoutes(services);
    RegisterAuditHttpRoutes(services);
    RegisterAuctionAdminHttpRoutes(services);
    RegisterAuctionPublicHttpRoutes(services);
    RegisterBidHttpRoutes(services);
    RegisterItemHttpRoutes(services);
    RegisterNotificationHttpRoutes(services);
    RegisterOrderHttpRoutes(services);
    RegisterPaymentHttpRoutes(services);
    RegisterReviewHttpRoutes(services);
    RegisterSystemHttpRoutes(services);
    RegisterAppHttpRoutes(project_root);
    RegisterApiErrorHttpHandlers();
}

}  // namespace auction::access::http
