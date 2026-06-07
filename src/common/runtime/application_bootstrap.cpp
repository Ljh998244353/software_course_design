#include "common/runtime/application_bootstrap.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>

#include "access/http/auction_admin_http.h"
#include "access/http/auction_http.h"
#include "access/http/auction_public_http.h"
#include "access/http/auction_ws.h"
#include "access/http/api_error_http.h"
#include "access/http/auth_http.h"
#include "access/http/bid_http.h"
#include "access/http/health_http.h"
#include "access/http/item_http.h"
#include "access/http/admin_http.h"
#include "access/http/notification_http.h"
#include "access/http/ops_http.h"
#include "access/http/order_http.h"
#include "access/http/payment_http.h"
#include "access/http/review_http.h"
#include "access/http/statistics_http.h"
#include "access/http/system_http.h"
#include "application/database_health_service.h"
#include "application/health_service.h"
#include "common/config/config_loader.h"
#include "common/http/http_service_context.h"
#include "common/logging/logger.h"

#if AUCTION_HAS_DROGON
#include <drogon/drogon.h>
#endif

namespace auction::common::runtime {

namespace {

const std::filesystem::path kProjectRoot(AUCTION_PROJECT_ROOT);

void EnsureRuntimeDirectories(
    const config::AppConfig& config,
    const std::filesystem::path& project_root
) {
    std::filesystem::create_directories(project_root / "build");
    std::filesystem::create_directories(config.ResolveLogPath(project_root).parent_path());
    std::filesystem::create_directories(config.ResolveUploadBasePath(project_root));
}

}  // namespace

int ApplicationBootstrap::Run(const BootstrapOptions& options) const {
    const auto config_path = config::ConfigLoader::ResolveConfigPath(kProjectRoot);
    const auto app_config = config::ConfigLoader::LoadFromFile(config_path);

    EnsureRuntimeDirectories(app_config, kProjectRoot);
    logging::Logger::Instance().Init(
        app_config.ResolveLogPath(kProjectRoot),
        app_config.logging.level
    );

    logging::Logger::Instance().Info("auction_app bootstrap started");
    logging::Logger::Instance().Info("config path: " + config_path.string());

    const auto health_response =
        application::BuildHealthResponse(app_config, kProjectRoot, config_path);

    if (options.check_config_only) {
        logging::Logger::Instance().Info("check-config mode completed");
        std::cout << health_response.ToJsonString() << '\n';
        return 0;
    }

    if (options.check_database_only) {
        const auto database_response =
            application::BuildDatabaseCheckResponse(app_config, kProjectRoot, config_path);
        logging::Logger::Instance().Info("check-db mode completed");
        std::cout << database_response.ToJsonString() << '\n';
        return static_cast<int>(database_response.code) == 0 ? 0 : 1;
    }

#if AUCTION_HAS_DROGON
    access::http::RegisterHealthHttpRoutes(app_config, kProjectRoot, config_path);
    auto ws_gateway = std::make_shared<access::http::DrogonAuctionEventGateway>();
    auto services = std::make_shared<common::http::HttpServiceContext>(
        app_config,
        kProjectRoot,
        ws_gateway
    );

    access::http::RegisterAuthHttpRoutes(
        services->auth_service(),
        services->auth_middleware()
    );
    access::http::RegisterAuctionHttpRoutes(services->auction_service());
    access::http::RegisterAuctionPublicHttpRoutes(services);
    access::http::RegisterBidHttpRoutes(services->bid_service());
    access::http::RegisterItemHttpRoutes(services->item_service());
    access::http::RegisterOrderHttpRoutes(
        services->order_service(),
        services->payment_service()
    );
    access::http::RegisterAdminHttpRoutes(
        services->item_audit_service(),
        services->statistics_service()
    );
    access::http::RegisterAuctionAdminHttpRoutes(services);
    access::http::RegisterNotificationHttpRoutes(services);
    access::http::RegisterOpsHttpRoutes(services);
    access::http::RegisterPaymentHttpRoutes(services);
    access::http::RegisterReviewHttpRoutes(services);
    access::http::RegisterStatisticsHttpRoutes(services);
    access::http::RegisterSystemHttpRoutes(services);
    access::http::RegisterApiErrorHttpHandlers();

    access::http::AuctionWebSocketController::SetGateway(ws_gateway);
    access::http::AuctionWebSocketController::SetTokenSecret(app_config.auth.token_secret);

    drogon::app().addListener(app_config.server.host, app_config.server.port);
    drogon::app().setThreadNum(1);
    logging::Logger::Instance().Info(
        "starting Drogon server on " + app_config.server.host + ":" +
        std::to_string(app_config.server.port)
    );
    drogon::app().run();
    return 0;
#else
    logging::Logger::Instance().Warn(
        "Drogon is not available. Bootstrap verification completed in fallback mode."
    );
    std::cout << health_response.ToJsonString() << '\n';
    return 0;
#endif
}

}  // namespace auction::common::runtime
