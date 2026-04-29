#include "access/http/app_http.h"

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

void RegisterAppHttpRoutes(const std::filesystem::path& project_root) {
#if AUCTION_HAS_DROGON
    const auto frontend_root = project_root / "assets" / "app";

    RegisterFileRoute("/app", frontend_root / "index.html", drogon::CT_TEXT_HTML);
    RegisterFileRoute("/app/", frontend_root / "index.html", drogon::CT_TEXT_HTML);
    RegisterFileRoute("/assets/app/app.css", frontend_root / "app.css", drogon::CT_TEXT_CSS);
    RegisterFileRoute(
        "/assets/app/app.js",
        frontend_root / "app.js",
        drogon::CT_TEXT_JAVASCRIPT
    );
#else
    static_cast<void>(project_root);
#endif
}

}  // namespace auction::access::http
