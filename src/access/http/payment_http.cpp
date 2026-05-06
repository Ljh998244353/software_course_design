#include "access/http/payment_http.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

#include "common/http/http_utils.h"
#include "modules/payment/payment_types.h"

#if AUCTION_HAS_DROGON
#include <drogon/drogon.h>
#endif

namespace auction::access::http {

namespace {

#if AUCTION_HAS_DROGON

std::uint64_t ParsePositiveId(const std::string& raw_value, const std::string& field_name) {
    std::size_t parsed_length = 0;
    const auto value = std::stoull(raw_value, &parsed_length);
    if (parsed_length != raw_value.size() || value == 0) {
        throw std::invalid_argument(field_name + " must be a positive integer");
    }
    return value;
}

double ReadRequiredDouble(const Json::Value& json, const std::string_view field_name) {
    const std::string field(field_name);
    if (!json.isMember(field) || json[field].isNull() || !json[field].isNumeric()) {
        throw std::invalid_argument(field + " must be a number");
    }
    return json[field].asDouble();
}

modules::payment::InitiatePaymentRequest BuildInitiatePaymentRequest(const Json::Value& body) {
    return modules::payment::InitiatePaymentRequest{
        .pay_channel = common::http::ReadRequiredString(body, "payChannel"),
    };
}

modules::payment::PaymentCallbackRequest BuildPaymentCallbackRequest(const Json::Value& body) {
    return modules::payment::PaymentCallbackRequest{
        .payment_no = common::http::ReadRequiredString(body, "paymentNo"),
        .order_no = common::http::ReadRequiredString(body, "orderNo"),
        .merchant_no = common::http::ReadRequiredString(body, "merchantNo"),
        .transaction_no = common::http::ReadRequiredString(body, "transactionNo"),
        .pay_status = common::http::ReadRequiredString(body, "payStatus"),
        .pay_amount = ReadRequiredDouble(body, "payAmount"),
        .signature = common::http::ReadRequiredString(body, "signature"),
    };
}

Json::Value ToInitiatePaymentResultJson(
    const modules::payment::InitiatePaymentResult& result
) {
    Json::Value json(Json::objectValue);
    json["paymentId"] = static_cast<Json::UInt64>(result.payment_id);
    json["orderId"] = static_cast<Json::UInt64>(result.order_id);
    json["orderNo"] = result.order_no;
    json["paymentNo"] = result.payment_no;
    json["payChannel"] = result.pay_channel;
    json["payAmount"] = result.pay_amount;
    json["payStatus"] = result.pay_status;
    json["merchantNo"] = result.merchant_no;
    json["payUrl"] = result.pay_url;
    json["expireAt"] = result.expire_at;
    json["reusedExisting"] = result.reused_existing;
    return json;
}

Json::Value ToPaymentCallbackResultJson(
    const modules::payment::PaymentCallbackResult& result
) {
    Json::Value json(Json::objectValue);
    json["paymentId"] = static_cast<Json::UInt64>(result.payment_id);
    json["orderId"] = static_cast<Json::UInt64>(result.order_id);
    json["orderStatus"] = result.order_status;
    json["payStatus"] = result.pay_status;
    json["idempotent"] = result.idempotent;
    json["processedAt"] = result.processed_at;
    return json;
}

#endif

}  // namespace

void RegisterPaymentHttpRoutes(const std::shared_ptr<common::http::HttpServiceContext> services) {
#if AUCTION_HAS_DROGON
    drogon::app().registerHandler(
        "/api/orders/{1}/pay",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                   const std::string& raw_order_id) {
            callback(common::http::ExecuteApi([&]() {
                const auto body = common::http::RequireJsonBody(request);
                const auto result = services->payment_service().InitiatePayment(
                    request->getHeader("authorization"),
                    ParsePositiveId(raw_order_id, "order id"),
                    BuildInitiatePaymentRequest(body)
                );
                return common::http::ApiResponse::Success(ToInitiatePaymentResultJson(result));
            }));
        },
        {drogon::Post}
    );

    drogon::app().registerHandler(
        "/api/payments/callback",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            callback(common::http::ExecuteApi([&]() {
                const auto body = common::http::RequireJsonBody(request);
                const auto result = services->payment_service().HandlePaymentCallback(
                    BuildPaymentCallbackRequest(body)
                );
                return common::http::ApiResponse::Success(ToPaymentCallbackResultJson(result));
            }));
        },
        {drogon::Post}
    );
#else
    static_cast<void>(services);
#endif
}

}  // namespace auction::access::http
