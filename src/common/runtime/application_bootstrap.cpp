#include "common/runtime/application_bootstrap.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>

#include "access/http/auction_http.h"
#include "access/http/auth_http.h"
#include "access/http/bid_http.h"
#include "access/http/health_http.h"
#include "application/database_health_service.h"
#include "application/health_service.h"
#include "common/config/config_loader.h"
#include "common/logging/logger.h"
#include "middleware/auth_middleware.h"
#include "modules/auction/auction_service.h"
#include "modules/auth/auth_service.h"
#include "modules/auth/auth_session_store.h"
#include "modules/bid/bid_cache_store.h"
#include "modules/bid/bid_service.h"
#include "modules/notification/notification_service.h"
#include "ws/auction_event_gateway.h"

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

    // NOTE: session_store, auth_service, auth_middleware are stack-local here.
    // They outlive all Drogon handler lambdas because drogon::app().run() blocks
    // until the server shuts down, and these objects are destroyed after run() returns.
    modules::auth::InMemoryAuthSessionStore session_store;
    modules::auth::AuthService auth_service(app_config, kProjectRoot, session_store);
    middleware::AuthMiddleware auth_middleware(auth_service);
    access::http::RegisterAuthHttpRoutes(auth_service, auth_middleware);

    modules::auction::AuctionService auction_service(app_config, kProjectRoot, auth_middleware);
    access::http::RegisterAuctionHttpRoutes(auction_service);

    ws::InMemoryAuctionEventGateway event_gateway;
    modules::notification::NotificationService notification_service(app_config, kProjectRoot, event_gateway);
    modules::bid::InMemoryBidCacheStore bid_cache_store;
    modules::bid::BidService bid_service(app_config, kProjectRoot, auth_middleware, notification_service, bid_cache_store);
    access::http::RegisterBidHttpRoutes(bid_service);

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
