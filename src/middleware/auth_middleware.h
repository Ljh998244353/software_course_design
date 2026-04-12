#pragma once

#include <initializer_list>
#include <string>
#include <string_view>

#include "modules/auth/auth_service.h"

namespace auction::middleware {

class AuthMiddleware {
public:
    explicit AuthMiddleware(modules::auth::AuthService& auth_service);

    [[nodiscard]] modules::auth::AuthContext RequireAuthenticated(
        std::string_view authorization_header
    );
    [[nodiscard]] modules::auth::AuthContext RequireAnyRole(
        std::string_view authorization_header,
        std::initializer_list<std::string_view> allowed_roles
    );

private:
    [[nodiscard]] static std::string ExtractBearerToken(std::string_view authorization_header);

    modules::auth::AuthService* auth_service_;
};

}  // namespace auction::middleware
