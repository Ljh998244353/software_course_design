#include "common/http/http_utils.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

#include "common/errors/error_code.h"
#include "modules/auction/auction_exception.h"
#include "modules/auth/auth_exception.h"
#include "modules/bid/bid_exception.h"
#include "modules/item/item_exception.h"
#include "modules/ops/ops_exception.h"
#include "modules/order/order_exception.h"
#include "modules/payment/payment_exception.h"
#include "modules/review/review_exception.h"
#include "modules/statistics/statistics_exception.h"

#if AUCTION_HAS_DROGON
#include <drogon/drogon.h>
#endif

namespace auction::common::http {

namespace {

template <typename ExceptionType>
std::optional<ApiResponse> TryBuildDomainFailure(const std::exception& exception) {
    if (const auto* typed = dynamic_cast<const ExceptionType*>(&exception); typed != nullptr) {
        return ApiResponse::Failure(typed->code(), typed->what());
    }
    return std::nullopt;
}

bool ContainsMysqlText(const std::string_view text) {
    std::string normalized(text);
    std::transform(
        normalized.begin(),
        normalized.end(),
        normalized.begin(),
        [](const unsigned char character) { return static_cast<char>(std::tolower(character)); }
    );
    return normalized.find("mysql") != std::string::npos;
}

}  // namespace

#if AUCTION_HAS_DROGON

ApiResponse BuildFailureResponse(const std::exception& exception) {
    if (const auto domain_response = TryBuildDomainFailure<modules::auth::AuthException>(exception);
        domain_response.has_value()) {
        return *domain_response;
    }
    if (const auto domain_response = TryBuildDomainFailure<modules::item::ItemException>(exception);
        domain_response.has_value()) {
        return *domain_response;
    }
    if (const auto domain_response =
            TryBuildDomainFailure<modules::auction::AuctionException>(exception);
        domain_response.has_value()) {
        return *domain_response;
    }
    if (const auto domain_response = TryBuildDomainFailure<modules::bid::BidException>(exception);
        domain_response.has_value()) {
        return *domain_response;
    }
    if (const auto domain_response = TryBuildDomainFailure<modules::order::OrderException>(exception);
        domain_response.has_value()) {
        return *domain_response;
    }
    if (const auto domain_response =
            TryBuildDomainFailure<modules::payment::PaymentException>(exception);
        domain_response.has_value()) {
        return *domain_response;
    }
    if (const auto domain_response =
            TryBuildDomainFailure<modules::review::ReviewException>(exception);
        domain_response.has_value()) {
        return *domain_response;
    }
    if (const auto domain_response =
            TryBuildDomainFailure<modules::statistics::StatisticsException>(exception);
        domain_response.has_value()) {
        return *domain_response;
    }
    if (const auto domain_response = TryBuildDomainFailure<modules::ops::OpsException>(exception);
        domain_response.has_value()) {
        return *domain_response;
    }

    if (dynamic_cast<const std::invalid_argument*>(&exception) != nullptr ||
        dynamic_cast<const std::out_of_range*>(&exception) != nullptr) {
        return ApiResponse::Failure(errors::ErrorCode::kInvalidArgument, exception.what());
    }

    if (ContainsMysqlText(exception.what())) {
        return ApiResponse::Failure(errors::ErrorCode::kDatabaseQueryFailed, exception.what());
    }

    return ApiResponse::Failure(errors::ErrorCode::kInternalError, exception.what());
}

drogon::HttpStatusCode MapErrorCodeToHttpStatus(const errors::ErrorCode code) {
    using errors::ErrorCode;
    switch (code) {
        case ErrorCode::kOk:
            return drogon::k200OK;
        case ErrorCode::kInvalidArgument:
            return drogon::k400BadRequest;
        case ErrorCode::kResourceNotFound:
        case ErrorCode::kItemNotFound:
        case ErrorCode::kAuctionNotFound:
        case ErrorCode::kAuctionItemNotFound:
        case ErrorCode::kBidAuctionNotFound:
        case ErrorCode::kOrderNotFound:
        case ErrorCode::kPaymentNotFound:
            return drogon::k404NotFound;
        case ErrorCode::kPermissionDenied:
        case ErrorCode::kAuthPermissionDenied:
        case ErrorCode::kItemOwnerMismatch:
        case ErrorCode::kOrderOwnerMismatch:
        case ErrorCode::kReviewPermissionDenied:
            return drogon::k403Forbidden;
        case ErrorCode::kAuthTokenMissing:
        case ErrorCode::kAuthTokenExpired:
        case ErrorCode::kAuthSessionInvalid:
        case ErrorCode::kAuthUserFrozen:
        case ErrorCode::kAuthUserDisabled:
            return drogon::k401Unauthorized;
        case ErrorCode::kAuthIdentityAlreadyExists:
        case ErrorCode::kAuctionDuplicateActiveByItem:
        case ErrorCode::kBidIdempotencyConflict:
        case ErrorCode::kBidConflict:
        case ErrorCode::kOrderSettlementConflict:
        case ErrorCode::kReviewDuplicateSubmitted:
            return drogon::k409Conflict;
        case ErrorCode::kDatabaseUnavailable:
            return drogon::k503ServiceUnavailable;
        case ErrorCode::kInternalError:
        case ErrorCode::kConfigError:
        case ErrorCode::kDependencyMissing:
        case ErrorCode::kDatabaseQueryFailed:
            return drogon::k500InternalServerError;
        default:
            return drogon::k400BadRequest;
    }
}

drogon::HttpResponsePtr BuildJsonResponse(
    const ApiResponse& response,
    const std::optional<drogon::HttpStatusCode> forced_status
) {
    auto http_response = drogon::HttpResponse::newHttpJsonResponse(response.ToJsonValue());
    http_response->setStatusCode(
        forced_status.value_or(MapErrorCodeToHttpStatus(response.code))
    );
    http_response->addHeader("Cache-Control", "no-store");
    return http_response;
}

ApiResponse BuildApiRouteNotFoundResponse() {
    Json::Value data(Json::objectValue);
    data["hint"] = "complete business route is not registered for this path";
    return ApiResponse::Failure(
        errors::ErrorCode::kResourceNotFound,
        "route not found",
        std::move(data)
    );
}

Json::Value RequireJsonBody(const drogon::HttpRequestPtr& request) {
    const auto& json_object = request->getJsonObject();
    if (json_object == nullptr || !json_object->isObject()) {
        const auto error = request->getJsonError();
        throw std::invalid_argument(
            error.empty() ? "request body must be a valid JSON object" : error
        );
    }
    return *json_object;
}

std::string ReadRequiredString(const Json::Value& json, const std::string_view field_name) {
    const std::string field(field_name);
    if (!json.isMember(field) || json[field].isNull() || !json[field].isString()) {
        throw std::invalid_argument(field + " must be a string");
    }
    return json[field].asString();
}

std::optional<std::string> ReadOptionalString(
    const Json::Value& json,
    const std::string_view field_name
) {
    const std::string field(field_name);
    if (!json.isMember(field) || json[field].isNull()) {
        return std::nullopt;
    }
    if (!json[field].isString()) {
        throw std::invalid_argument(field + " must be a string");
    }
    return json[field].asString();
}

modules::auth::AuthContext RequireAuthContext(
    const drogon::HttpRequestPtr& request,
    middleware::AuthMiddleware& auth_middleware
) {
    const auto key = std::string(kAuthContextAttributeKey);
    auto attributes = request->getAttributes();
    if (attributes->find(key)) {
        return attributes->get<modules::auth::AuthContext>(key);
    }

    const auto context =
        auth_middleware.RequireAuthenticated(request->getHeader("authorization"));
    attributes->insert(key, context);
    return context;
}

modules::auth::AuthContext RequireAnyRoleContext(
    const drogon::HttpRequestPtr& request,
    middleware::AuthMiddleware& auth_middleware,
    const std::initializer_list<std::string_view> allowed_roles
) {
    const auto key = std::string(kAuthContextAttributeKey);
    auto attributes = request->getAttributes();
    if (attributes->find(key)) {
        const auto context = attributes->get<modules::auth::AuthContext>(key);
        const auto checked_context =
            auth_middleware.RequireAnyRole(request->getHeader("authorization"), allowed_roles);
        (void)checked_context;
        return context;
    }

    const auto context =
        auth_middleware.RequireAnyRole(request->getHeader("authorization"), allowed_roles);
    attributes->insert(key, context);
    return context;
}

#endif

}  // namespace auction::common::http
