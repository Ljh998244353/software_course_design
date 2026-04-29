#include "access/http/auction_admin_http.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "common/http/http_utils.h"
#include "modules/auction/auction_types.h"

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

std::optional<std::uint64_t> ParseOptionalPositiveId(
    const std::optional<std::string>& raw_value,
    const std::string& field_name
) {
    if (!raw_value.has_value() || raw_value->empty()) {
        return std::nullopt;
    }
    return ParsePositiveId(*raw_value, field_name);
}

std::optional<std::string> NormalizeOptionalString(std::optional<std::string> raw_value) {
    if (!raw_value.has_value() || raw_value->empty()) {
        return std::nullopt;
    }
    return raw_value;
}

std::uint64_t ReadRequiredUInt64(const Json::Value& json, const std::string_view field_name) {
    const std::string field(field_name);
    if (!json.isMember(field) || json[field].isNull() || !json[field].isIntegral()) {
        throw std::invalid_argument(field + " must be an integer");
    }
    if (!json[field].isUInt64() && json[field].asInt64() <= 0) {
        throw std::invalid_argument(field + " must be positive");
    }

    const auto value = json[field].isUInt64()
                           ? json[field].asUInt64()
                           : static_cast<Json::UInt64>(json[field].asInt64());
    if (value == 0) {
        throw std::invalid_argument(field + " must be positive");
    }
    return static_cast<std::uint64_t>(value);
}

double ReadRequiredDouble(const Json::Value& json, const std::string_view field_name) {
    const std::string field(field_name);
    if (!json.isMember(field) || json[field].isNull() || !json[field].isNumeric()) {
        throw std::invalid_argument(field + " must be a number");
    }
    return json[field].asDouble();
}

std::optional<double> ReadOptionalDouble(
    const Json::Value& json,
    const std::string_view field_name
) {
    const std::string field(field_name);
    if (!json.isMember(field) || json[field].isNull()) {
        return std::nullopt;
    }
    if (!json[field].isNumeric()) {
        throw std::invalid_argument(field + " must be a number");
    }
    return json[field].asDouble();
}

std::optional<int> ReadOptionalNonNegativeInt(
    const Json::Value& json,
    const std::string_view field_name
) {
    const std::string field(field_name);
    if (!json.isMember(field) || json[field].isNull()) {
        return std::nullopt;
    }
    if (!json[field].isInt()) {
        throw std::invalid_argument(field + " must be an integer");
    }
    const auto value = json[field].asInt();
    if (value < 0) {
        throw std::invalid_argument(field + " must be non-negative");
    }
    return value;
}

Json::Value ToOptionalUInt64Json(const std::optional<std::uint64_t>& value) {
    if (!value.has_value()) {
        return Json::Value(Json::nullValue);
    }
    return Json::Value(static_cast<Json::UInt64>(*value));
}

Json::Value ToAuctionRecordJson(const modules::auction::AuctionRecord& auction) {
    Json::Value json(Json::objectValue);
    json["auctionId"] = static_cast<Json::UInt64>(auction.auction_id);
    json["itemId"] = static_cast<Json::UInt64>(auction.item_id);
    json["sellerId"] = static_cast<Json::UInt64>(auction.seller_id);
    json["startTime"] = auction.start_time;
    json["endTime"] = auction.end_time;
    json["startPrice"] = auction.start_price;
    json["currentPrice"] = auction.current_price;
    json["bidStep"] = auction.bid_step;
    json["highestBidderId"] = ToOptionalUInt64Json(auction.highest_bidder_id);
    json["status"] = auction.status;
    json["antiSnipingWindowSeconds"] = auction.anti_sniping_window_seconds;
    json["extendSeconds"] = auction.extend_seconds;
    json["version"] = static_cast<Json::UInt64>(auction.version);
    json["createdAt"] = auction.created_at;
    json["updatedAt"] = auction.updated_at;
    return json;
}

Json::Value ToAuctionItemSnapshotJson(const modules::auction::AuctionItemSnapshot& item) {
    Json::Value json(Json::objectValue);
    json["itemId"] = static_cast<Json::UInt64>(item.item_id);
    json["sellerId"] = static_cast<Json::UInt64>(item.seller_id);
    json["title"] = item.title;
    json["coverImageUrl"] = item.cover_image_url;
    json["itemStatus"] = item.item_status;
    return json;
}

Json::Value ToCreateAuctionResultJson(const modules::auction::CreateAuctionResult& result) {
    Json::Value json(Json::objectValue);
    json["auctionId"] = static_cast<Json::UInt64>(result.auction_id);
    json["itemId"] = static_cast<Json::UInt64>(result.item_id);
    json["status"] = result.status;
    json["currentPrice"] = result.current_price;
    json["createdAt"] = result.created_at;
    return json;
}

Json::Value ToUpdateAuctionResultJson(const modules::auction::UpdateAuctionResult& result) {
    Json::Value json(Json::objectValue);
    json["auctionId"] = static_cast<Json::UInt64>(result.auction_id);
    json["status"] = result.status;
    json["updatedAt"] = result.updated_at;
    return json;
}

Json::Value ToCancelAuctionResultJson(const modules::auction::CancelAuctionResult& result) {
    Json::Value json(Json::objectValue);
    json["auctionId"] = static_cast<Json::UInt64>(result.auction_id);
    json["oldStatus"] = result.old_status;
    json["newStatus"] = result.new_status;
    json["cancelledAt"] = result.cancelled_at;
    return json;
}

Json::Value ToAdminAuctionSummaryJson(const modules::auction::AdminAuctionSummary& auction) {
    Json::Value json(Json::objectValue);
    json["auctionId"] = static_cast<Json::UInt64>(auction.auction_id);
    json["itemId"] = static_cast<Json::UInt64>(auction.item_id);
    json["sellerId"] = static_cast<Json::UInt64>(auction.seller_id);
    json["title"] = auction.title;
    json["coverImageUrl"] = auction.cover_image_url;
    json["status"] = auction.status;
    json["startPrice"] = auction.start_price;
    json["currentPrice"] = auction.current_price;
    json["bidStep"] = auction.bid_step;
    json["highestBidderId"] = ToOptionalUInt64Json(auction.highest_bidder_id);
    json["startTime"] = auction.start_time;
    json["endTime"] = auction.end_time;
    json["updatedAt"] = auction.updated_at;
    return json;
}

Json::Value ToAdminAuctionListJson(
    const std::vector<modules::auction::AdminAuctionSummary>& auctions,
    const modules::auction::AdminAuctionQuery& query
) {
    Json::Value data(Json::objectValue);
    Json::Value list(Json::arrayValue);
    for (const auto& auction : auctions) {
        list.append(ToAdminAuctionSummaryJson(auction));
    }
    data["list"] = std::move(list);
    data["total"] = static_cast<Json::UInt64>(auctions.size());
    data["pageNo"] = query.page_no;
    data["pageSize"] = query.page_size;
    return data;
}

Json::Value ToAdminAuctionDetailJson(const modules::auction::AdminAuctionDetail& detail) {
    Json::Value json(Json::objectValue);
    json["auction"] = ToAuctionRecordJson(detail.auction);
    json["item"] = ToAuctionItemSnapshotJson(detail.item);
    json["sellerUsername"] = detail.seller_username;
    json["highestBidderUsername"] = detail.highest_bidder_username;
    return json;
}

modules::auction::CreateAuctionRequest BuildCreateAuctionRequest(const Json::Value& body) {
    return modules::auction::CreateAuctionRequest{
        .item_id = ReadRequiredUInt64(body, "itemId"),
        .start_time = common::http::ReadRequiredString(body, "startTime"),
        .end_time = common::http::ReadRequiredString(body, "endTime"),
        .start_price = ReadRequiredDouble(body, "startPrice"),
        .bid_step = ReadRequiredDouble(body, "bidStep"),
        .anti_sniping_window_seconds =
            ReadOptionalNonNegativeInt(body, "antiSnipingWindowSeconds").value_or(0),
        .extend_seconds = ReadOptionalNonNegativeInt(body, "extendSeconds").value_or(0),
    };
}

modules::auction::UpdateAuctionRequest BuildUpdateAuctionRequest(const Json::Value& body) {
    return modules::auction::UpdateAuctionRequest{
        .start_time = common::http::ReadOptionalString(body, "startTime"),
        .end_time = common::http::ReadOptionalString(body, "endTime"),
        .start_price = ReadOptionalDouble(body, "startPrice"),
        .bid_step = ReadOptionalDouble(body, "bidStep"),
        .anti_sniping_window_seconds =
            ReadOptionalNonNegativeInt(body, "antiSnipingWindowSeconds"),
        .extend_seconds = ReadOptionalNonNegativeInt(body, "extendSeconds"),
    };
}

modules::auction::CancelAuctionRequest BuildCancelAuctionRequest(const Json::Value& body) {
    return modules::auction::CancelAuctionRequest{
        .reason = common::http::ReadOptionalString(body, "reason").value_or(""),
    };
}

modules::auction::AdminAuctionQuery BuildAdminAuctionQuery(
    const drogon::HttpRequestPtr& request
) {
    modules::auction::AdminAuctionQuery query;
    query.status = NormalizeOptionalString(request->getOptionalParameter<std::string>("status"));
    query.item_id = ParseOptionalPositiveId(
        request->getOptionalParameter<std::string>("itemId"),
        "item id"
    );
    query.seller_id = ParseOptionalPositiveId(
        request->getOptionalParameter<std::string>("sellerId"),
        "seller id"
    );

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

void RegisterAuctionAdminHttpRoutes(
    const std::shared_ptr<common::http::HttpServiceContext> services
) {
#if AUCTION_HAS_DROGON
    drogon::app().registerHandler(
        "/api/admin/auctions",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            callback(common::http::ExecuteApi([&]() {
                const auto body = common::http::RequireJsonBody(request);
                const auto result = services->auction_service().CreateAuction(
                    request->getHeader("authorization"),
                    BuildCreateAuctionRequest(body)
                );
                return common::http::ApiResponse::Success(ToCreateAuctionResultJson(result));
            }));
        },
        {drogon::Post}
    );

    drogon::app().registerHandler(
        "/api/admin/auctions",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            callback(common::http::ExecuteApi([&]() {
                const auto query = BuildAdminAuctionQuery(request);
                const auto auctions = services->auction_service().ListAdminAuctions(
                    request->getHeader("authorization"),
                    query
                );
                return common::http::ApiResponse::Success(
                    ToAdminAuctionListJson(auctions, query)
                );
            }));
        },
        {drogon::Get}
    );

    drogon::app().registerHandler(
        "/api/admin/auctions/{1}/cancel",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                   const std::string& raw_auction_id) {
            callback(common::http::ExecuteApi([&]() {
                const auto body = common::http::RequireJsonBody(request);
                const auto result = services->auction_service().CancelPendingAuction(
                    request->getHeader("authorization"),
                    ParsePositiveId(raw_auction_id, "auction id"),
                    BuildCancelAuctionRequest(body)
                );
                return common::http::ApiResponse::Success(ToCancelAuctionResultJson(result));
            }));
        },
        {drogon::Post}
    );

    drogon::app().registerHandler(
        "/api/admin/auctions/{1}",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                   const std::string& raw_auction_id) {
            callback(common::http::ExecuteApi([&]() {
                const auto body = common::http::RequireJsonBody(request);
                const auto result = services->auction_service().UpdatePendingAuction(
                    request->getHeader("authorization"),
                    ParsePositiveId(raw_auction_id, "auction id"),
                    BuildUpdateAuctionRequest(body)
                );
                return common::http::ApiResponse::Success(ToUpdateAuctionResultJson(result));
            }));
        },
        {drogon::Put}
    );

    drogon::app().registerHandler(
        "/api/admin/auctions/{1}",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                   const std::string& raw_auction_id) {
            callback(common::http::ExecuteApi([&]() {
                const auto detail = services->auction_service().GetAdminAuctionDetail(
                    request->getHeader("authorization"),
                    ParsePositiveId(raw_auction_id, "auction id")
                );
                return common::http::ApiResponse::Success(ToAdminAuctionDetailJson(detail));
            }));
        },
        {drogon::Get}
    );
#else
    static_cast<void>(services);
#endif
}

}  // namespace auction::access::http
