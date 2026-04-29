#include "access/http/auth_http.h"

#include <cstdint>
#include <stdexcept>
#include <string>

#include "common/http/http_utils.h"

#if AUCTION_HAS_DROGON
#include <drogon/drogon.h>
#endif

namespace auction::access::http {

namespace {

#if AUCTION_HAS_DROGON

Json::Value ToUserInfoJson(const modules::auth::UserProfile& profile) {
    Json::Value json(Json::objectValue);
    json["userId"] = static_cast<Json::UInt64>(profile.user_id);
    json["username"] = profile.username;
    json["nickname"] = profile.nickname;
    json["roleCode"] = profile.role_code;
    json["status"] = profile.status;
    return json;
}

Json::Value ToRegisterResultJson(const modules::auth::RegisterUserResult& result) {
    Json::Value json(Json::objectValue);
    json["userId"] = static_cast<Json::UInt64>(result.user_id);
    json["username"] = result.username;
    json["roleCode"] = result.role_code;
    json["status"] = result.status;
    return json;
}

Json::Value ToAuthContextJson(const modules::auth::AuthContext& context) {
    Json::Value json(Json::objectValue);
    json["userId"] = static_cast<Json::UInt64>(context.user_id);
    json["username"] = context.username;
    json["nickname"] = context.nickname;
    json["roleCode"] = context.role_code;
    json["status"] = context.status;
    json["tokenExpireAt"] = modules::auth::ToIsoTimestamp(context.expire_at_epoch_seconds);
    return json;
}

Json::Value ToChangeUserStatusJson(const modules::auth::ChangeUserStatusResult& result) {
    Json::Value json(Json::objectValue);
    json["userId"] = static_cast<Json::UInt64>(result.user_id);
    json["oldStatus"] = result.old_status;
    json["newStatus"] = result.new_status;
    json["updatedAt"] = result.updated_at;
    return json;
}

std::string ReadLoginPrincipal(const Json::Value& body) {
    if (const auto principal = common::http::ReadOptionalString(body, "principal");
        principal.has_value() && !principal->empty()) {
        return *principal;
    }
    return common::http::ReadRequiredString(body, "username");
}

std::uint64_t ParseUserId(const std::string& raw_user_id) {
    std::size_t parsed_length = 0;
    const auto user_id = std::stoull(raw_user_id, &parsed_length);
    if (parsed_length != raw_user_id.size() || user_id == 0) {
        throw std::invalid_argument("user id must be a positive integer");
    }
    return user_id;
}

#endif

}  // namespace

void RegisterAuthHttpRoutes(const std::shared_ptr<common::http::HttpServiceContext> services) {
#if AUCTION_HAS_DROGON
    drogon::app().registerHandler(
        "/api/auth/register",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            callback(common::http::ExecuteApi([&]() {
                const auto body = common::http::RequireJsonBody(request);
                const auto result = services->auth_service().RegisterUser(
                    modules::auth::RegisterUserRequest{
                        .username = common::http::ReadRequiredString(body, "username"),
                        .password = common::http::ReadRequiredString(body, "password"),
                        .phone = common::http::ReadOptionalString(body, "phone").value_or(""),
                        .email = common::http::ReadOptionalString(body, "email").value_or(""),
                        .nickname =
                            common::http::ReadOptionalString(body, "nickname").value_or(""),
                    }
                );
                return common::http::ApiResponse::Success(ToRegisterResultJson(result));
            }));
        },
        {drogon::Post}
    );

    drogon::app().registerHandler(
        "/api/auth/login",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            callback(common::http::ExecuteApi([&]() {
                const auto body = common::http::RequireJsonBody(request);
                const auto result = services->auth_service().Login(modules::auth::LoginRequest{
                    .principal = ReadLoginPrincipal(body),
                    .password = common::http::ReadRequiredString(body, "password"),
                });

                Json::Value data(Json::objectValue);
                data["token"] = result.token;
                data["expireAt"] = result.expire_at;
                data["userInfo"] = ToUserInfoJson(result.user_info);
                return common::http::ApiResponse::Success(std::move(data));
            }));
        },
        {drogon::Post}
    );

    drogon::app().registerHandler(
        "/api/auth/logout",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            callback(common::http::ExecuteApi([&]() {
                const auto context =
                    common::http::RequireAuthContext(request, services->auth_middleware());
                services->auth_service().Logout(context.token);

                Json::Value data(Json::objectValue);
                data["success"] = true;
                return common::http::ApiResponse::Success(std::move(data));
            }));
        },
        {drogon::Post}
    );

    drogon::app().registerHandler(
        "/api/auth/me",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            callback(common::http::ExecuteApi([&]() {
                const auto context =
                    common::http::RequireAuthContext(request, services->auth_middleware());
                return common::http::ApiResponse::Success(ToAuthContextJson(context));
            }));
        },
        {drogon::Get}
    );

    drogon::app().registerHandler(
        "/api/admin/users/{1}/status",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                   const std::string& raw_user_id) {
            callback(common::http::ExecuteApi([&]() {
                const auto context = common::http::RequireAnyRoleContext(
                    request,
                    services->auth_middleware(),
                    {modules::auth::kRoleAdmin}
                );
                const auto body = common::http::RequireJsonBody(request);
                const auto result = services->auth_service().ChangeUserStatus(
                    context.token,
                    ParseUserId(raw_user_id),
                    common::http::ReadRequiredString(body, "status"),
                    common::http::ReadOptionalString(body, "reason").value_or("")
                );
                return common::http::ApiResponse::Success(ToChangeUserStatusJson(result));
            }));
        },
        {drogon::Patch}
    );
#else
    static_cast<void>(services);
#endif
}

}  // namespace auction::access::http
