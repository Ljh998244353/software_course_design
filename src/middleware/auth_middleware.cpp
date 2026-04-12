#include "middleware/auth_middleware.h"

#include "common/errors/error_code.h"
#include "modules/auth/auth_exception.h"

namespace auction::middleware {

AuthMiddleware::AuthMiddleware(modules::auth::AuthService& auth_service)
    : auth_service_(&auth_service) {}

modules::auth::AuthContext AuthMiddleware::RequireAuthenticated(
    const std::string_view authorization_header
) {
    return auth_service_->ValidateToken(ExtractBearerToken(authorization_header));
}

modules::auth::AuthContext AuthMiddleware::RequireAnyRole(
    const std::string_view authorization_header,
    const std::initializer_list<std::string_view> allowed_roles
) {
    const auto context = RequireAuthenticated(authorization_header);
    auth_service_->RequireRole(context, allowed_roles);
    return context;
}

std::string AuthMiddleware::ExtractBearerToken(const std::string_view authorization_header) {
    constexpr std::string_view kPrefix = "Bearer ";
    if (!authorization_header.starts_with(kPrefix) ||
        authorization_header.size() <= kPrefix.size()) {
        throw modules::auth::AuthException(
            common::errors::ErrorCode::kAuthTokenMissing,
            "authorization header must use Bearer token"
        );
    }

    return std::string(authorization_header.substr(kPrefix.size()));
}

}  // namespace auction::middleware
