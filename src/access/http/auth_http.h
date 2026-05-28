#pragma once

#include "middleware/auth_middleware.h"
#include "modules/auth/auth_service.h"

namespace auction::access::http {

void RegisterAuthHttpRoutes(
    modules::auth::AuthService& auth_service,
    middleware::AuthMiddleware& auth_middleware
);

}  // namespace auction::access::http
