#include "access/http/demo_http.h"

#include "application/demo_dashboard_service.h"

#if AUCTION_HAS_DROGON
#include <drogon/drogon.h>
#endif

namespace auction::access::http {

namespace {

#if AUCTION_HAS_DROGON

drogon::HttpResponsePtr BuildFileResponse(
    const std::filesystem::path& file_path,
    const drogon::ContentType content_type
) {
    if (!std::filesystem::exists(file_path)) {
        return drogon::HttpResponse::newNotFoundResponse();
    }
    return drogon::HttpResponse::newFileResponse(file_path.string(), "", content_type);
}

void RegisterFileRoute(
    const std::string& route_path,
    const std::filesystem::path& file_path,
    const drogon::ContentType content_type
) {
    drogon::app().registerHandler(
        route_path,
        [file_path, content_type](const drogon::HttpRequestPtr&,
                                  std::function<void(const drogon::HttpResponsePtr&)>&&
                                      callback) {
            callback(BuildFileResponse(file_path, content_type));
        }
    );
}

#endif

}  // namespace

void RegisterDemoHttpRoutes(
    const common::config::AppConfig& config,
    const std::filesystem::path& project_root
) {
#if AUCTION_HAS_DROGON
    const auto frontend_root = project_root / "assets" / "demo";
    const auto index_path = frontend_root / "index.html";
    const auto css_path = frontend_root / "app.css";
    const auto js_path = frontend_root / "app.js";

    RegisterFileRoute("/", index_path, drogon::CT_TEXT_HTML);
    RegisterFileRoute("/demo", index_path, drogon::CT_TEXT_HTML);
    RegisterFileRoute("/demo/", index_path, drogon::CT_TEXT_HTML);
    RegisterFileRoute("/assets/demo/app.css", css_path, drogon::CT_TEXT_CSS);
    RegisterFileRoute("/assets/demo/app.js", js_path, drogon::CT_TEXT_JAVASCRIPT);

    drogon::app().registerHandler(
        "/api/demo/dashboard",
        [config, project_root](const drogon::HttpRequestPtr&,
                               std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            const auto api_response =
                application::BuildDemoDashboardResponse(config, project_root).ToJsonValue();
            auto response = drogon::HttpResponse::newHttpJsonResponse(api_response);
            response->setStatusCode(
                api_response["code"].asInt() == 0 ? drogon::k200OK : drogon::k503ServiceUnavailable
            );
            callback(response);
        }
    );
#else
    static_cast<void>(config);
    static_cast<void>(project_root);
#endif
}

}  // namespace auction::access::http
