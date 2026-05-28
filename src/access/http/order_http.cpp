#include "access/http/order_http.h"

#include <stdexcept>

#include "common/errors/error_code.h"
#include "common/http/api_response.h"
#include "common/logging/logger.h"
#include "modules/auth/auth_exception.h"
#include "modules/order/order_exception.h"
#include "modules/payment/payment_exception.h"

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

void RegisterCorsPreflight(const std::string& path, const std::string& methods) {
    drogon::app().registerHandler(
        path,
        [methods](const drogon::HttpRequestPtr&,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            auto response = drogon::HttpResponse::newHttpResponse();
            response->setStatusCode(drogon::k204NoContent);
            response->addHeader("Access-Control-Allow-Origin", "*");
            response->addHeader("Access-Control-Allow-Methods", methods);
            response->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
            response->addHeader("Access-Control-Max-Age", "86400");
            callback(response);
        },
        {drogon::Options}
    );
}

std::uint64_t SafeParseUint64(const std::string& value, const std::string& param_name) {
    try {
        std::size_t parsed = 0;
        const auto result = static_cast<std::uint64_t>(std::stoull(value, &parsed));
        if (parsed != value.size()) {
            throw std::invalid_argument(param_name + " is not a valid number");
        }
        return result;
    } catch (const std::out_of_range&) {
        throw std::invalid_argument(param_name + " out of range");
    } catch (const std::invalid_argument&) {
        throw std::invalid_argument(param_name + " is not a valid number");
    }
}

Json::Value OrderDetailToJson(const repository::OrderDetailAggregate& agg) {
    Json::Value json(Json::objectValue);
    json["order_id"] = static_cast<Json::UInt64>(agg.order.order_id);
    json["order_no"] = agg.order.order_no;
    json["auction_id"] = static_cast<Json::UInt64>(agg.auction_id);
    json["item_title"] = agg.item_title;
    json["buyer_id"] = static_cast<Json::UInt64>(agg.order.buyer_id);
    json["seller_id"] = static_cast<Json::UInt64>(agg.order.seller_id);
    json["final_amount"] = agg.order.final_amount;
    json["order_status"] = agg.order.order_status;
    json["pay_deadline_at"] = agg.order.pay_deadline_at;
    json["paid_at"] = agg.order.paid_at;
    json["created_at"] = agg.order.created_at;
    return json;
}

Json::Value InitiatePaymentResultToJson(const modules::payment::InitiatePaymentResult& r) {
    Json::Value json(Json::objectValue);
    json["payment_id"] = static_cast<Json::UInt64>(r.payment_id);
    json["order_id"] = static_cast<Json::UInt64>(r.order_id);
    json["order_no"] = r.order_no;
    json["payment_no"] = r.payment_no;
    json["pay_channel"] = r.pay_channel;
    json["pay_amount"] = r.pay_amount;
    json["pay_status"] = r.pay_status;
    json["pay_url"] = r.pay_url;
    json["expire_at"] = r.expire_at;
    json["reused_existing"] = r.reused_existing;
    return json;
}

void MapOrderHttpStatus(
    const common::errors::ErrorCode code,
    drogon::HttpStatusCode& out_status
) {
    switch (code) {
        case common::errors::ErrorCode::kOrderNotFound:
        case common::errors::ErrorCode::kPaymentNotFound:
            out_status = drogon::k404NotFound;
            break;
        case common::errors::ErrorCode::kOrderOwnerMismatch:
        case common::errors::ErrorCode::kOrderStatusInvalid:
        case common::errors::ErrorCode::kOrderPaymentDeadlineExpired:
        case common::errors::ErrorCode::kOrderSettlementConflict:
        case common::errors::ErrorCode::kPaymentStatusInvalid:
        case common::errors::ErrorCode::kPaymentChannelInvalid:
            out_status = drogon::k409Conflict;
            break;
        case common::errors::ErrorCode::kAuthTokenMissing:
        case common::errors::ErrorCode::kAuthTokenExpired:
        case common::errors::ErrorCode::kAuthSessionInvalid:
            out_status = drogon::k401Unauthorized;
            break;
        case common::errors::ErrorCode::kAuthUserFrozen:
        case common::errors::ErrorCode::kAuthUserDisabled:
        case common::errors::ErrorCode::kAuthPermissionDenied:
            out_status = drogon::k403Forbidden;
            break;
        default:
            out_status = drogon::k400BadRequest;
            break;
    }
}

}  // namespace

#endif

void RegisterOrderHttpRoutes(
    modules::order::OrderService& order_service,
    modules::payment::PaymentService& payment_service
) {
#if AUCTION_HAS_DROGON

    // GET /api/orders/{id} - order detail (requires auth)
    RegisterCorsPreflight("/api/orders/{id}", "GET, OPTIONS");
    drogon::app().registerHandler(
        "/api/orders/{id}",
        [&order_service](const drogon::HttpRequestPtr& request,
                         std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                         const std::string& id_str) {
            try {
                if (id_str.empty()) {
                    callback(MakeError(
                        common::errors::ErrorCode::kInvalidArgument,
                        "order id is required"
                    ));
                    return;
                }

                const auto order_id = SafeParseUint64(id_str, "order id");
                const auto auth_header = request->getHeader("authorization");

                const auto result = order_service.GetOrderDetail(auth_header, order_id);

                callback(MakeOk(OrderDetailToJson(result)));
            } catch (const modules::order::OrderException& e) {
                drogon::HttpStatusCode http_status = drogon::k400BadRequest;
                MapOrderHttpStatus(e.code(), http_status);
                callback(MakeError(e.code(), e.what(), http_status));
            } catch (const modules::auth::AuthException& e) {
                drogon::HttpStatusCode http_status = drogon::k401Unauthorized;
                MapOrderHttpStatus(e.code(), http_status);
                callback(MakeError(e.code(), e.what(), http_status));
            } catch (const std::invalid_argument& e) {
                callback(MakeError(common::errors::ErrorCode::kInvalidArgument, e.what()));
            } catch (...) {
                common::logging::Logger::Instance().Error(
                    "unhandled exception in GET /api/orders/{id}"
                );
                callback(MakeError(
                    common::errors::ErrorCode::kInternalError,
                    "internal server error",
                    drogon::k500InternalServerError
                ));
            }
        },
        {drogon::Get}
    );

    // POST /api/orders/{id}/pay - initiate payment (requires auth)
    RegisterCorsPreflight("/api/orders/{id}/pay", "POST, OPTIONS");
    drogon::app().registerHandler(
        "/api/orders/{id}/pay",
        [&payment_service](const drogon::HttpRequestPtr& request,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                           const std::string& id_str) {
            try {
                if (id_str.empty()) {
                    callback(MakeError(
                        common::errors::ErrorCode::kInvalidArgument,
                        "order id is required"
                    ));
                    return;
                }

                const auto order_id = SafeParseUint64(id_str, "order id");
                const auto auth_header = request->getHeader("authorization");

                const auto body = request->getJsonObject();
                if (!body) {
                    callback(MakeError(
                        common::errors::ErrorCode::kInvalidArgument,
                        "request body must be valid JSON"
                    ));
                    return;
                }

                modules::payment::InitiatePaymentRequest req;
                req.pay_channel = body->get("pay_channel", "MOCK_ALIPAY").asString();

                const auto result = payment_service.InitiatePayment(auth_header, order_id, req);

                callback(MakeOk(InitiatePaymentResultToJson(result)));
            } catch (const modules::payment::PaymentException& e) {
                drogon::HttpStatusCode http_status = drogon::k400BadRequest;
                MapOrderHttpStatus(e.code(), http_status);
                callback(MakeError(e.code(), e.what(), http_status));
            } catch (const modules::order::OrderException& e) {
                drogon::HttpStatusCode http_status = drogon::k400BadRequest;
                MapOrderHttpStatus(e.code(), http_status);
                callback(MakeError(e.code(), e.what(), http_status));
            } catch (const modules::auth::AuthException& e) {
                drogon::HttpStatusCode http_status = drogon::k401Unauthorized;
                MapOrderHttpStatus(e.code(), http_status);
                callback(MakeError(e.code(), e.what(), http_status));
            } catch (const std::invalid_argument& e) {
                callback(MakeError(common::errors::ErrorCode::kInvalidArgument, e.what()));
            } catch (...) {
                common::logging::Logger::Instance().Error(
                    "unhandled exception in POST /api/orders/{id}/pay"
                );
                callback(MakeError(
                    common::errors::ErrorCode::kInternalError,
                    "internal server error",
                    drogon::k500InternalServerError
                ));
            }
        },
        {drogon::Post}
    );

#else
    static_cast<void>(order_service);
    static_cast<void>(payment_service);
#endif
}

}  // namespace auction::access::http
