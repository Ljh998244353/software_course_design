#pragma once

#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>

#include <json/json.h>

#include "common/http/api_response.h"
#include "middleware/auth_middleware.h"
#include "modules/auth/auth_types.h"

#if AUCTION_HAS_DROGON
#include <drogon/drogon.h>
#endif

namespace auction::common::http {

inline constexpr std::string_view kAuthContextAttributeKey = "auction.auth_context";

#if AUCTION_HAS_DROGON
[[nodiscard]] ApiResponse BuildFailureResponse(const std::exception& exception);
[[nodiscard]] drogon::HttpStatusCode MapErrorCodeToHttpStatus(errors::ErrorCode code);
[[nodiscard]] drogon::HttpResponsePtr BuildJsonResponse(
    const ApiResponse& response,
    std::optional<drogon::HttpStatusCode> forced_status = std::nullopt
);
[[nodiscard]] ApiResponse BuildApiRouteNotFoundResponse();
[[nodiscard]] Json::Value RequireJsonBody(const drogon::HttpRequestPtr& request);
[[nodiscard]] std::string ReadRequiredString(const Json::Value& json, std::string_view field_name);
[[nodiscard]] std::optional<std::string> ReadOptionalString(
    const Json::Value& json,
    std::string_view field_name
);
[[nodiscard]] modules::auth::AuthContext RequireAuthContext(
    const drogon::HttpRequestPtr& request,
    middleware::AuthMiddleware& auth_middleware
);
[[nodiscard]] modules::auth::AuthContext RequireAnyRoleContext(
    const drogon::HttpRequestPtr& request,
    middleware::AuthMiddleware& auth_middleware,
    std::initializer_list<std::string_view> allowed_roles
);

template <typename Handler>
drogon::HttpResponsePtr ExecuteApi(Handler&& handler) {
    try {
        return BuildJsonResponse(handler());
    } catch (const std::exception& exception) {
        return BuildJsonResponse(BuildFailureResponse(exception));
    }
}
#endif

}  // namespace auction::common::http
