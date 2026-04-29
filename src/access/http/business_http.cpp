#include "access/http/business_http.h"

#include <memory>

#include "access/http/api_error_http.h"
#include "access/http/app_http.h"
#include "access/http/auth_http.h"
#include "access/http/item_http.h"
#include "access/http/system_http.h"
#include "common/http/http_service_context.h"

namespace auction::access::http {

void RegisterBusinessHttpRoutes(
    const common::config::AppConfig& config,
    const std::filesystem::path& project_root
) {
    auto services = std::make_shared<common::http::HttpServiceContext>(config, project_root);
    RegisterAuthHttpRoutes(services);
    RegisterItemHttpRoutes(services);
    RegisterSystemHttpRoutes(services);
    RegisterAppHttpRoutes(project_root);
    RegisterApiErrorHttpHandlers();
}

}  // namespace auction::access::http
