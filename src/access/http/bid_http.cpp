#include "access/http/bid_http.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

#include "common/http/http_utils.h"
#include "modules/bid/bid_types.h"

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

Json::Value ToPlaceBidResultJson(const modules::bid::PlaceBidResult& result) {
    Json::Value json(Json::objectValue);
    json["bidId"] = static_cast<Json::UInt64>(result.bid_id);
    json["auctionId"] = static_cast<Json::UInt64>(result.auction_id);
    json["bidAmount"] = result.bid_amount;
    json["bidStatus"] = result.bid_status;
    json["currentPrice"] = result.current_price;
    json["currentHighestPrice"] = result.current_price;
    json["highestBidderMasked"] = result.highest_bidder_masked;
    json["endTime"] = result.end_time;
    json["extended"] = result.extended;
    json["serverTime"] = result.server_time;
    return json;
}

Json::Value ToMyBidStatusJson(const modules::bid::MyBidStatusResult& result) {
    Json::Value json(Json::objectValue);
    json["auctionId"] = static_cast<Json::UInt64>(result.auction_id);
    json["userId"] = static_cast<Json::UInt64>(result.user_id);
    json["myHighestBid"] = result.my_highest_bid;
    json["isCurrentHighest"] = result.is_current_highest;
    json["lastBidTime"] = result.last_bid_time;
    json["currentPrice"] = result.current_price;
    json["endTime"] = result.end_time;
    return json;
}

modules::bid::PlaceBidRequest BuildPlaceBidRequest(const Json::Value& body) {
    return modules::bid::PlaceBidRequest{
        .request_id = common::http::ReadRequiredString(body, "requestId"),
        .bid_amount = ReadRequiredDouble(body, "bidAmount"),
    };
}

#endif

}  // namespace

void RegisterBidHttpRoutes(const std::shared_ptr<common::http::HttpServiceContext> services) {
#if AUCTION_HAS_DROGON
    drogon::app().registerHandler(
        "/api/auctions/{1}/bids",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                   const std::string& raw_auction_id) {
            callback(common::http::ExecuteApi([&]() {
                const auto body = common::http::RequireJsonBody(request);
                const auto result = services->bid_service().PlaceBid(
                    request->getHeader("authorization"),
                    ParsePositiveId(raw_auction_id, "auction id"),
                    BuildPlaceBidRequest(body)
                );
                return common::http::ApiResponse::Success(ToPlaceBidResultJson(result));
            }));
        },
        {drogon::Post}
    );

    drogon::app().registerHandler(
        "/api/auctions/{1}/my-bid",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                   const std::string& raw_auction_id) {
            callback(common::http::ExecuteApi([&]() {
                const auto result = services->bid_service().GetMyBidStatus(
                    request->getHeader("authorization"),
                    ParsePositiveId(raw_auction_id, "auction id")
                );
                return common::http::ApiResponse::Success(ToMyBidStatusJson(result));
            }));
        },
        {drogon::Get}
    );
#else
    static_cast<void>(services);
#endif
}

}  // namespace auction::access::http
