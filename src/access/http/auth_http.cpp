#include "access/http/auth_http.h"

#include "common/errors/error_code.h"
#include "common/http/api_response.h"
#include "modules/auth/auth_exception.h"

#if AUCTION_HAS_DROGON
#include <drogon/drogon.h>
#endif

namespace auction::access::http {

#if AUCTION_HAS_DROGON

namespace {

void AddCorsHeaders(const drogon::HttpResponsePtr& response) {
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
    response->addHeader("Access-Control-Max-Age", "86400");
}

drogon::HttpResponsePtr MakeOk(Json::Value data) {
    auto response = drogon::HttpResponse::newHttpJsonResponse(
        common::http::ApiResponse::Success(std::move(data)).ToJsonValue()
    );
    response->setStatusCode(drogon::k200OK);
    AddCorsHeaders(response);
    return response;
}

drogon::HttpResponsePtr MakeError(
    common::errors::ErrorCode code,
    const std::string& message,
    drogon::HttpStatusCode http_status = drogon::k400BadRequest
) {
    auto response = drogon::HttpResponse::newHttpJsonResponse(
        common::http::ApiResponse::Failure(code, message).ToJsonValue()
    );
    response->setStatusCode(http_status);
    AddCorsHeaders(response);
    return response;
}

void RegisterCorsPreflight(const std::string& path) {
    drogon::app().registerHandler(
        path,
        [](const drogon::HttpRequestPtr&,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            auto response = drogon::HttpResponse::newHttpResponse();
            response->setStatusCode(drogon::k204NoContent);
            AddCorsHeaders(response);
            callback(response);
        },
        {drogon::Options}
    );
}

Json::Value UserProfileToJson(const modules::auth::UserProfile& user) {
    Json::Value json(Json::objectValue);
    json["user_id"] = static_cast<Json::UInt64>(user.user_id);
    json["username"] = user.username;
    json["nickname"] = user.nickname;
    json["role_code"] = user.role_code;
    json["status"] = user.status;
    return json;
}

}  // namespace

#endif

void RegisterAuthHttpRoutes(
    modules::auth::AuthService& auth_service,
    middleware::AuthMiddleware& auth_middleware
) {
#if AUCTION_HAS_DROGON

    RegisterCorsPreflight("/api/auth/register");
    RegisterCorsPreflight("/api/auth/login");
    RegisterCorsPreflight("/api/auth/logout");
    RegisterCorsPreflight("/api/auth/me");

    // POST /api/auth/register
    drogon::app().registerHandler(
        "/api/auth/register",
        [&auth_service](const drogon::HttpRequestPtr& request,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            const auto body = request->getJsonObject();
            if (!body) {
                callback(MakeError(
                    common::errors::ErrorCode::kInvalidArgument,
                    "request body must be valid JSON"
                ));
                return;
            }

            try {
                modules::auth::RegisterUserRequest req;
                req.username = body->get("username", "").asString();
                req.password = body->get("password", "").asString();
                req.phone = body->get("phone", "").asString();
                req.email = body->get("email", "").asString();
                req.nickname = body->get("nickname", req.username).asString();

                const auto result = auth_service.RegisterUser(req);

                Json::Value data(Json::objectValue);
                data["user_id"] = static_cast<Json::UInt64>(result.user_id);
                data["username"] = result.username;
                data["role_code"] = result.role_code;
                data["status"] = result.status;
                callback(MakeOk(std::move(data)));
            } catch (const modules::auth::AuthException& e) {
                callback(MakeError(e.code(), e.what()));
            } catch (const std::invalid_argument& e) {
                callback(MakeError(common::errors::ErrorCode::kInvalidArgument, e.what()));
            } catch (...) {
                callback(MakeError(
                    common::errors::ErrorCode::kInternalError,
                    "internal server error",
                    drogon::k500InternalServerError
                ));
            }
        },
        {drogon::Post}
    );

    // POST /api/auth/login
    drogon::app().registerHandler(
        "/api/auth/login",
        [&auth_service](const drogon::HttpRequestPtr& request,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            const auto body = request->getJsonObject();
            if (!body) {
                callback(MakeError(
                    common::errors::ErrorCode::kInvalidArgument,
                    "request body must be valid JSON"
                ));
                return;
            }

            try {
                modules::auth::LoginRequest req;
                req.principal = body->get("username", body->get("principal", "")).asString();
                req.password = body->get("password", "").asString();

                const auto result = auth_service.Login(req);

                Json::Value data(Json::objectValue);
                data["token"] = result.token;
                data["expire_at"] = result.expire_at;
                data["user_info"] = UserProfileToJson(result.user_info);
                callback(MakeOk(std::move(data)));
            } catch (const modules::auth::AuthException& e) {
                const auto http_status =
                    (e.code() == common::errors::ErrorCode::kAuthCredentialInvalid ||
                     e.code() == common::errors::ErrorCode::kAuthUserFrozen ||
                     e.code() == common::errors::ErrorCode::kAuthUserDisabled)
                        ? drogon::k401Unauthorized
                        : drogon::k400BadRequest;
                callback(MakeError(e.code(), e.what(), http_status));
            } catch (const std::invalid_argument& e) {
                callback(MakeError(common::errors::ErrorCode::kInvalidArgument, e.what()));
            } catch (...) {
                callback(MakeError(
                    common::errors::ErrorCode::kInternalError,
                    "internal server error",
                    drogon::k500InternalServerError
                ));
            }
        },
        {drogon::Post}
    );

    // POST /api/auth/logout
    drogon::app().registerHandler(
        "/api/auth/logout",
        [&auth_service, &auth_middleware](const drogon::HttpRequestPtr& request,
                                          std::function<void(const drogon::HttpResponsePtr&)>&&
                                              callback) {
            try {
                const auto auth_header = request->getHeader("authorization");
                const auto context = auth_middleware.RequireAuthenticated(auth_header);
                auth_service.Logout(context.token);

                Json::Value data(Json::objectValue);
                data["message"] = "logged out";
                callback(MakeOk(std::move(data)));
            } catch (const modules::auth::AuthException& e) {
                const auto http_status =
                    (e.code() == common::errors::ErrorCode::kAuthTokenMissing ||
                     e.code() == common::errors::ErrorCode::kAuthTokenExpired ||
                     e.code() == common::errors::ErrorCode::kAuthSessionInvalid)
                        ? drogon::k401Unauthorized
                        : drogon::k400BadRequest;
                callback(MakeError(e.code(), e.what(), http_status));
            } catch (...) {
                callback(MakeError(
                    common::errors::ErrorCode::kInternalError,
                    "internal server error",
                    drogon::k500InternalServerError
                ));
            }
        },
        {drogon::Post}
    );

    // GET /api/auth/me
    drogon::app().registerHandler(
        "/api/auth/me",
        [&auth_middleware](const drogon::HttpRequestPtr& request,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            try {
                const auto auth_header = request->getHeader("authorization");
                const auto context = auth_middleware.RequireAuthenticated(auth_header);

                Json::Value data(Json::objectValue);
                data["user_id"] = static_cast<Json::UInt64>(context.user_id);
                data["username"] = context.username;
                data["nickname"] = context.nickname;
                data["role_code"] = context.role_code;
                data["status"] = context.status;
                callback(MakeOk(std::move(data)));
            } catch (const modules::auth::AuthException& e) {
                const auto http_status =
                    (e.code() == common::errors::ErrorCode::kAuthTokenMissing ||
                     e.code() == common::errors::ErrorCode::kAuthTokenExpired ||
                     e.code() == common::errors::ErrorCode::kAuthSessionInvalid)
                        ? drogon::k401Unauthorized
                        : drogon::k400BadRequest;
                callback(MakeError(e.code(), e.what(), http_status));
            } catch (...) {
                callback(MakeError(
                    common::errors::ErrorCode::kInternalError,
                    "internal server error",
                    drogon::k500InternalServerError
                ));
            }
        },
        {drogon::Get}
    );

#else
    static_cast<void>(auth_service);
    static_cast<void>(auth_middleware);
#endif
}

}  // namespace auction::access::http
