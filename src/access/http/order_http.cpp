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

Json::Value LatestPaymentToJson(const modules::order::OrderPaymentSummary& payment) {
    Json::Value json(Json::objectValue);
    json["paymentId"] = static_cast<Json::UInt64>(payment.payment_id);
    json["paymentNo"] = payment.payment_no;
    json["orderId"] = static_cast<Json::UInt64>(payment.order_id);
    json["payChannel"] = payment.pay_channel;
    json["transactionNo"] = payment.transaction_no;
    json["payAmount"] = payment.pay_amount;
    json["payStatus"] = payment.pay_status;
    json["paidAt"] = payment.paid_at;
    json["createdAt"] = payment.created_at;
    json["updatedAt"] = payment.updated_at;
    return json;
}

Json::Value UserOrderEntryToJson(const modules::order::UserOrderEntry& entry) {
    Json::Value json(Json::objectValue);
    json["orderId"] = static_cast<Json::UInt64>(entry.order.order_id);
    json["orderNo"] = entry.order.order_no;
    json["auctionId"] = static_cast<Json::UInt64>(entry.order.auction_id);
    json["buyerId"] = static_cast<Json::UInt64>(entry.order.buyer_id);
    json["sellerId"] = static_cast<Json::UInt64>(entry.order.seller_id);
    json["finalAmount"] = entry.order.final_amount;
    json["orderStatus"] = entry.order.order_status;
    json["payDeadlineAt"] = entry.order.pay_deadline_at;
    json["paidAt"] = entry.order.paid_at;
    json["shippedAt"] = entry.order.shipped_at;
    json["completedAt"] = entry.order.completed_at;
    json["closedAt"] = entry.order.closed_at;
    json["createdAt"] = entry.order.created_at;
    json["updatedAt"] = entry.order.updated_at;
    json["itemId"] = static_cast<Json::UInt64>(entry.item_id);
    json["itemTitle"] = entry.item_title;
    json["coverImageUrl"] = entry.cover_image_url;
    json["auctionStatus"] = entry.auction_status;
    json["buyerUsername"] = entry.buyer_username;
    json["sellerUsername"] = entry.seller_username;
    if (entry.latest_payment.has_value()) {
        json["latestPayment"] = LatestPaymentToJson(*entry.latest_payment);
    } else {
        json["latestPayment"] = Json::nullValue;
    }
    return json;
}

Json::Value UserOrderDetailToJson(const modules::order::UserOrderEntry& entry) {
    auto json = UserOrderEntryToJson(entry);
    json["order_id"] = static_cast<Json::UInt64>(entry.order.order_id);
    json["order_no"] = entry.order.order_no;
    json["auction_id"] = static_cast<Json::UInt64>(entry.order.auction_id);
    json["item_title"] = entry.item_title;
    json["buyer_id"] = static_cast<Json::UInt64>(entry.order.buyer_id);
    json["seller_id"] = static_cast<Json::UInt64>(entry.order.seller_id);
    json["final_amount"] = entry.order.final_amount;
    json["order_status"] = entry.order.order_status;
    json["pay_deadline_at"] = entry.order.pay_deadline_at;
    json["paid_at"] = entry.order.paid_at;
    json["created_at"] = entry.order.created_at;
    if (entry.latest_payment.has_value()) {
        json["payStatus"] = entry.latest_payment->pay_status;
    }
    return json;
}

Json::Value UserOrderListToJson(const modules::order::UserOrderListResult& result) {
    Json::Value json(Json::objectValue);
    Json::Value records(Json::arrayValue);
    for (const auto& entry : result.records) {
        records.append(UserOrderEntryToJson(entry));
    }
    json["records"] = std::move(records);
    json["pageNo"] = result.page_no;
    json["pageSize"] = result.page_size;
    return json;
}

Json::Value InitiatePaymentResultToJson(const modules::payment::InitiatePaymentResult& r) {
    Json::Value json(Json::objectValue);
    json["payment_id"] = static_cast<Json::UInt64>(r.payment_id);
    json["paymentId"] = static_cast<Json::UInt64>(r.payment_id);
    json["order_id"] = static_cast<Json::UInt64>(r.order_id);
    json["orderId"] = static_cast<Json::UInt64>(r.order_id);
    json["order_no"] = r.order_no;
    json["orderNo"] = r.order_no;
    json["payment_no"] = r.payment_no;
    json["paymentNo"] = r.payment_no;
    json["pay_channel"] = r.pay_channel;
    json["payChannel"] = r.pay_channel;
    json["pay_amount"] = r.pay_amount;
    json["payAmount"] = r.pay_amount;
    json["pay_status"] = r.pay_status;
    json["payStatus"] = r.pay_status;
    json["merchantNo"] = r.merchant_no;
    json["pay_url"] = r.pay_url;
    json["payUrl"] = r.pay_url;
    json["expire_at"] = r.expire_at;
    json["expireAt"] = r.expire_at;
    json["reused_existing"] = r.reused_existing;
    json["reusedExisting"] = r.reused_existing;
    return json;
}

Json::Value OrderTransitionToJson(const modules::order::OrderTransitionResult& result) {
    Json::Value json(Json::objectValue);
    json["order_id"] = static_cast<Json::UInt64>(result.order_id);
    json["old_status"] = result.old_status;
    json["new_status"] = result.new_status;
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
            out_status = drogon::k403Forbidden;
            break;
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

    // GET /api/orders/mine - current user's buyer/seller order list (requires auth)
    RegisterCorsPreflight("/api/orders/mine", "GET, OPTIONS");
    drogon::app().registerHandler(
        "/api/orders/mine",
        [&order_service](const drogon::HttpRequestPtr& request,
                         std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            try {
                modules::order::UserOrderQuery query;
                query.role = request->getParameter("role");
                query.order_status = request->getParameter("status");

                const auto page_no = request->getParameter("pageNo");
                if (!page_no.empty()) {
                    query.page_no = static_cast<int>(SafeParseUint64(page_no, "pageNo"));
                }

                const auto page_size = request->getParameter("pageSize");
                if (!page_size.empty()) {
                    query.page_size = static_cast<int>(SafeParseUint64(page_size, "pageSize"));
                }

                const auto auth_header = request->getHeader("authorization");
                const auto result = order_service.ListMyOrders(auth_header, query);

                callback(MakeOk(UserOrderListToJson(result)));
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
                    "unhandled exception in GET /api/orders/mine"
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

                const auto result = order_service.GetMyOrder(auth_header, order_id);

                callback(MakeOk(UserOrderDetailToJson(result)));
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
                req.pay_channel = body->get(
                    "pay_channel",
                    body->get("payChannel", "MOCK_ALIPAY")
                ).asString();

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

    // GET /api/orders/{id}/payment - latest payment snapshot (requires auth)
    RegisterCorsPreflight("/api/orders/{id}/payment", "GET, OPTIONS");
    drogon::app().registerHandler(
        "/api/orders/{id}/payment",
        [&order_service](const drogon::HttpRequestPtr& request,
                         std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                         const std::string& id_str) {
            try {
                const auto order_id = SafeParseUint64(id_str, "order id");
                const auto auth_header = request->getHeader("authorization");
                const auto result = order_service.GetMyOrder(auth_header, order_id);
                callback(MakeOk(UserOrderDetailToJson(result)));
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
                    "unhandled exception in GET /api/orders/{id}/payment"
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

    // POST /api/orders/{id}/ship - seller ships a paid order (requires auth)
    RegisterCorsPreflight("/api/orders/{id}/ship", "POST, OPTIONS");
    drogon::app().registerHandler(
        "/api/orders/{id}/ship",
        [&order_service](const drogon::HttpRequestPtr& request,
                         std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                         const std::string& id_str) {
            try {
                const auto order_id = SafeParseUint64(id_str, "order id");
                const auto auth_header = request->getHeader("authorization");
                const auto result = order_service.ShipOrder(auth_header, order_id);
                callback(MakeOk(OrderTransitionToJson(result)));
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
                    "unhandled exception in POST /api/orders/{id}/ship"
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

    // POST /api/orders/{id}/confirm-receipt - buyer confirms receipt (requires auth)
    RegisterCorsPreflight("/api/orders/{id}/confirm-receipt", "POST, OPTIONS");
    drogon::app().registerHandler(
        "/api/orders/{id}/confirm-receipt",
        [&order_service](const drogon::HttpRequestPtr& request,
                         std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                         const std::string& id_str) {
            try {
                const auto order_id = SafeParseUint64(id_str, "order id");
                const auto auth_header = request->getHeader("authorization");
                const auto result = order_service.ConfirmReceipt(auth_header, order_id);
                callback(MakeOk(OrderTransitionToJson(result)));
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
                    "unhandled exception in POST /api/orders/{id}/confirm-receipt"
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
