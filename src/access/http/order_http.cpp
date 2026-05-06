#include "access/http/order_http.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

#include "common/http/http_utils.h"
#include "modules/order/order_types.h"

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

int ParsePositiveInt(const std::string& raw_value, const std::string& field_name) {
    std::size_t parsed_length = 0;
    const auto value = std::stoll(raw_value, &parsed_length);
    if (parsed_length != raw_value.size() || value <= 0) {
        throw std::invalid_argument(field_name + " must be a positive integer");
    }
    return static_cast<int>(value);
}

Json::Value ToOrderRecordJson(const modules::order::OrderRecord& order) {
    Json::Value json(Json::objectValue);
    json["orderId"] = static_cast<Json::UInt64>(order.order_id);
    json["orderNo"] = order.order_no;
    json["auctionId"] = static_cast<Json::UInt64>(order.auction_id);
    json["buyerId"] = static_cast<Json::UInt64>(order.buyer_id);
    json["sellerId"] = static_cast<Json::UInt64>(order.seller_id);
    json["finalAmount"] = order.final_amount;
    json["orderStatus"] = order.order_status;
    json["payDeadlineAt"] = order.pay_deadline_at;
    json["paidAt"] = order.paid_at;
    json["shippedAt"] = order.shipped_at;
    json["completedAt"] = order.completed_at;
    json["closedAt"] = order.closed_at;
    json["createdAt"] = order.created_at;
    json["updatedAt"] = order.updated_at;
    return json;
}

Json::Value ToPaymentSummaryJson(const modules::order::OrderPaymentSummary& payment) {
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

Json::Value ToLatestPaymentJson(
    const std::optional<modules::order::OrderPaymentSummary>& latest_payment
) {
    if (!latest_payment.has_value()) {
        return Json::Value(Json::nullValue);
    }
    return ToPaymentSummaryJson(*latest_payment);
}

Json::Value ToUserOrderEntryJson(const modules::order::UserOrderEntry& entry) {
    Json::Value json(Json::objectValue);
    json["order"] = ToOrderRecordJson(entry.order);
    json["orderId"] = static_cast<Json::UInt64>(entry.order.order_id);
    json["orderNo"] = entry.order.order_no;
    json["auctionId"] = static_cast<Json::UInt64>(entry.order.auction_id);
    json["itemId"] = static_cast<Json::UInt64>(entry.item_id);
    json["itemTitle"] = entry.item_title;
    json["coverImageUrl"] = entry.cover_image_url;
    json["auctionStatus"] = entry.auction_status;
    json["buyerId"] = static_cast<Json::UInt64>(entry.order.buyer_id);
    json["sellerId"] = static_cast<Json::UInt64>(entry.order.seller_id);
    json["buyerUsername"] = entry.buyer_username;
    json["sellerUsername"] = entry.seller_username;
    json["finalAmount"] = entry.order.final_amount;
    json["orderStatus"] = entry.order.order_status;
    json["payDeadlineAt"] = entry.order.pay_deadline_at;
    json["latestPayment"] = ToLatestPaymentJson(entry.latest_payment);
    return json;
}

Json::Value ToUserOrderListJson(const modules::order::UserOrderListResult& result) {
    Json::Value data(Json::objectValue);
    Json::Value list(Json::arrayValue);
    for (const auto& entry : result.records) {
        list.append(ToUserOrderEntryJson(entry));
    }
    data["list"] = std::move(list);
    data["total"] = static_cast<Json::UInt64>(result.records.size());
    data["pageNo"] = result.page_no;
    data["pageSize"] = result.page_size;
    return data;
}

Json::Value ToUserOrderDetailJson(const modules::order::UserOrderEntry& entry) {
    return ToUserOrderEntryJson(entry);
}

Json::Value ToPaymentStatusJson(const modules::order::UserOrderEntry& entry) {
    Json::Value json(Json::objectValue);
    json["orderId"] = static_cast<Json::UInt64>(entry.order.order_id);
    json["orderNo"] = entry.order.order_no;
    json["orderStatus"] = entry.order.order_status;
    json["payDeadlineAt"] = entry.order.pay_deadline_at;
    json["latestPayment"] = ToLatestPaymentJson(entry.latest_payment);
    return json;
}

Json::Value ToOrderTransitionResultJson(const modules::order::OrderTransitionResult& result) {
    Json::Value json(Json::objectValue);
    json["orderId"] = static_cast<Json::UInt64>(result.order_id);
    json["oldStatus"] = result.old_status;
    json["newStatus"] = result.new_status;
    return json;
}

modules::order::UserOrderQuery BuildUserOrderQuery(const drogon::HttpRequestPtr& request) {
    modules::order::UserOrderQuery query;
    query.role = request->getOptionalParameter<std::string>("role").value_or("");
    query.order_status = request->getOptionalParameter<std::string>("status").value_or("");

    const auto page_no = request->getOptionalParameter<std::string>("pageNo");
    if (page_no.has_value() && !page_no->empty()) {
        query.page_no = ParsePositiveInt(*page_no, "pageNo");
    }
    const auto page_size = request->getOptionalParameter<std::string>("pageSize");
    if (page_size.has_value() && !page_size->empty()) {
        query.page_size = ParsePositiveInt(*page_size, "pageSize");
    }
    return query;
}

#endif

}  // namespace

void RegisterOrderHttpRoutes(const std::shared_ptr<common::http::HttpServiceContext> services) {
#if AUCTION_HAS_DROGON
    drogon::app().registerHandler(
        "/api/orders/mine",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            callback(common::http::ExecuteApi([&]() {
                const auto result = services->order_service().ListMyOrders(
                    request->getHeader("authorization"),
                    BuildUserOrderQuery(request)
                );
                return common::http::ApiResponse::Success(ToUserOrderListJson(result));
            }));
        },
        {drogon::Get}
    );

    drogon::app().registerHandler(
        "/api/orders/{1}/payment",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                   const std::string& raw_order_id) {
            callback(common::http::ExecuteApi([&]() {
                const auto detail = services->order_service().GetMyOrder(
                    request->getHeader("authorization"),
                    ParsePositiveId(raw_order_id, "order id")
                );
                return common::http::ApiResponse::Success(ToPaymentStatusJson(detail));
            }));
        },
        {drogon::Get}
    );

    drogon::app().registerHandler(
        "/api/orders/{1}/ship",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                   const std::string& raw_order_id) {
            callback(common::http::ExecuteApi([&]() {
                const auto result = services->order_service().ShipOrder(
                    request->getHeader("authorization"),
                    ParsePositiveId(raw_order_id, "order id")
                );
                return common::http::ApiResponse::Success(ToOrderTransitionResultJson(result));
            }));
        },
        {drogon::Post}
    );

    drogon::app().registerHandler(
        "/api/orders/{1}/confirm-receipt",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                   const std::string& raw_order_id) {
            callback(common::http::ExecuteApi([&]() {
                const auto result = services->order_service().ConfirmReceipt(
                    request->getHeader("authorization"),
                    ParsePositiveId(raw_order_id, "order id")
                );
                return common::http::ApiResponse::Success(ToOrderTransitionResultJson(result));
            }));
        },
        {drogon::Post}
    );

    drogon::app().registerHandler(
        "/api/orders/{1}",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                   const std::string& raw_order_id) {
            callback(common::http::ExecuteApi([&]() {
                const auto detail = services->order_service().GetMyOrder(
                    request->getHeader("authorization"),
                    ParsePositiveId(raw_order_id, "order id")
                );
                return common::http::ApiResponse::Success(ToUserOrderDetailJson(detail));
            }));
        },
        {drogon::Get}
    );
#else
    static_cast<void>(services);
#endif
}

}  // namespace auction::access::http
