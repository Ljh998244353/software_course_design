#include "access/http/system_http.h"

#include "common/http/http_utils.h"

#if AUCTION_HAS_DROGON
#include <drogon/drogon.h>
#endif

namespace auction::access::http {

void RegisterSystemHttpRoutes(const std::shared_ptr<common::http::HttpServiceContext> services) {
#if AUCTION_HAS_DROGON
    drogon::app().registerHandler(
        "/api/system/context",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            callback(common::http::ExecuteApi([&]() {
                const auto context =
                    common::http::RequireAuthContext(request, services->auth_middleware());

                Json::Value user(Json::objectValue);
                user["userId"] = static_cast<Json::UInt64>(context.user_id);
                user["username"] = context.username;
                user["nickname"] = context.nickname;
                user["roleCode"] = context.role_code;
                user["status"] = context.status;
                user["tokenId"] = context.token_id;

                Json::Value data(Json::objectValue);
                data["view"] = "business_http_baseline";
                data["authenticated"] = true;
                data["requestPath"] = request->path();
                data["tokenExpireAt"] =
                    modules::auth::ToIsoTimestamp(context.expire_at_epoch_seconds);
                data["user"] = std::move(user);
                return common::http::ApiResponse::Success(std::move(data));
            }));
        },
        {drogon::Get}
    );
#else
    static_cast<void>(services);
#endif
}

}  // namespace auction::access::http
