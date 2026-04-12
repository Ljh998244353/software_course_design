#include "access/http/health_http.h"

#include "application/health_service.h"

#if AUCTION_HAS_DROGON
#include <drogon/drogon.h>
#endif

namespace auction::access::http {

void RegisterHealthHttpRoutes(
    const common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::filesystem::path& config_path
) {
#if AUCTION_HAS_DROGON
    drogon::app().registerHandler(
        "/healthz",
        [config, project_root, config_path](const drogon::HttpRequestPtr&,
                                            std::function<void(const drogon::HttpResponsePtr&)>&&
                                                callback) {
            const auto payload =
                application::BuildHealthResponse(config, project_root, config_path).ToJsonValue();
            auto response = drogon::HttpResponse::newHttpJsonResponse(payload);
            response->setStatusCode(drogon::k200OK);
            callback(response);
        }
    );
#else
    static_cast<void>(config);
    static_cast<void>(project_root);
    static_cast<void>(config_path);
#endif
}

}  // namespace auction::access::http

