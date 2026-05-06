#include "access/http/auction_public_http.h"

#include <cstdint>
#include <ctime>
#include <functional>
#include <iomanip>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "common/http/http_utils.h"
#include "modules/auction/auction_types.h"
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

int ParsePositiveInt(const std::string& raw_value, const std::string& field_name) {
    std::size_t parsed_length = 0;
    const auto value = std::stoll(raw_value, &parsed_length);
    if (parsed_length != raw_value.size() || value <= 0) {
        throw std::invalid_argument(field_name + " must be a positive integer");
    }
    return static_cast<int>(value);
}

std::optional<std::string> NormalizeOptionalString(std::optional<std::string> raw_value) {
    if (!raw_value.has_value() || raw_value->empty()) {
        return std::nullopt;
    }
    return raw_value;
}

std::string CurrentTimestampText() {
    const auto now = std::time(nullptr);
    std::tm local_tm{};
    localtime_r(&now, &local_tm);

    std::ostringstream output;
    output << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
    return output.str();
}

bool IsAcceptingBids(const std::string& status) {
    return status == modules::auction::kAuctionStatusRunning;
}

Json::Value ToPublicAuctionSummaryJson(const modules::auction::PublicAuctionSummary& auction) {
    Json::Value json(Json::objectValue);
    json["auctionId"] = static_cast<Json::UInt64>(auction.auction_id);
    json["itemId"] = static_cast<Json::UInt64>(auction.item_id);
    json["title"] = auction.title;
    json["coverImageUrl"] = auction.cover_image_url;
    json["status"] = auction.status;
    json["currentPrice"] = auction.current_price;
    json["startTime"] = auction.start_time;
    json["endTime"] = auction.end_time;
    json["acceptingBids"] = IsAcceptingBids(auction.status);
    return json;
}

Json::Value ToPublicAuctionListJson(
    const std::vector<modules::auction::PublicAuctionSummary>& auctions,
    const modules::auction::PublicAuctionQuery& query
) {
    Json::Value data(Json::objectValue);
    Json::Value list(Json::arrayValue);
    for (const auto& auction : auctions) {
        list.append(ToPublicAuctionSummaryJson(auction));
    }
    data["list"] = std::move(list);
    data["total"] = static_cast<Json::UInt64>(auctions.size());
    data["pageNo"] = query.page_no;
    data["pageSize"] = query.page_size;
    return data;
}

Json::Value ToPublicAuctionDetailJson(const modules::auction::PublicAuctionDetail& detail) {
    Json::Value json(Json::objectValue);
    json["auctionId"] = static_cast<Json::UInt64>(detail.auction_id);
    json["itemId"] = static_cast<Json::UInt64>(detail.item_id);
    json["title"] = detail.title;
    json["coverImageUrl"] = detail.cover_image_url;
    json["status"] = detail.status;
    json["startPrice"] = detail.start_price;
    json["currentPrice"] = detail.current_price;
    json["bidStep"] = detail.bid_step;
    json["startTime"] = detail.start_time;
    json["endTime"] = detail.end_time;
    json["antiSnipingWindowSeconds"] = detail.anti_sniping_window_seconds;
    json["extendSeconds"] = detail.extend_seconds;
    json["highestBidderMasked"] = detail.highest_bidder_masked;
    json["acceptingBids"] = IsAcceptingBids(detail.status);
    return json;
}

Json::Value ToCurrentPriceJson(const modules::auction::PublicAuctionDetail& detail) {
    Json::Value json(Json::objectValue);
    json["auctionId"] = static_cast<Json::UInt64>(detail.auction_id);
    json["status"] = detail.status;
    json["currentPrice"] = detail.current_price;
    json["currentHighestPrice"] = detail.current_price;
    json["minimumNextBid"] = detail.highest_bidder_masked.empty()
                                  ? detail.current_price
                                  : detail.current_price + detail.bid_step;
    json["highestBidderMasked"] = detail.highest_bidder_masked;
    json["endTime"] = detail.end_time;
    json["serverTime"] = CurrentTimestampText();
    json["acceptingBids"] = IsAcceptingBids(detail.status);
    return json;
}

Json::Value ToBidHistoryEntryJson(const modules::bid::BidHistoryEntry& entry) {
    Json::Value json(Json::objectValue);
    json["bidId"] = static_cast<Json::UInt64>(entry.bid_id);
    json["bidAmount"] = entry.bid_amount;
    json["bidStatus"] = entry.bid_status;
    json["bidTime"] = entry.bid_time;
    json["bidderMasked"] = entry.bidder_masked;
    return json;
}

Json::Value ToBidHistoryJson(const modules::bid::BidHistoryResult& result) {
    Json::Value data(Json::objectValue);
    Json::Value list(Json::arrayValue);
    for (const auto& entry : result.records) {
        list.append(ToBidHistoryEntryJson(entry));
    }
    data["list"] = std::move(list);
    data["total"] = static_cast<Json::UInt64>(result.records.size());
    data["pageNo"] = result.page_no;
    data["pageSize"] = result.page_size;
    return data;
}

modules::auction::PublicAuctionQuery BuildPublicAuctionQuery(
    const drogon::HttpRequestPtr& request
) {
    modules::auction::PublicAuctionQuery query;
    query.keyword = NormalizeOptionalString(request->getOptionalParameter<std::string>("keyword"));
    query.status = NormalizeOptionalString(request->getOptionalParameter<std::string>("status"));

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

modules::bid::BidHistoryQuery BuildBidHistoryQuery(const drogon::HttpRequestPtr& request) {
    modules::bid::BidHistoryQuery query;
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

void RegisterAuctionPublicHttpRoutes(
    const std::shared_ptr<common::http::HttpServiceContext> services
) {
#if AUCTION_HAS_DROGON
    drogon::app().registerHandler(
        "/api/auctions",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            callback(common::http::ExecuteApi([&]() {
                const auto query = BuildPublicAuctionQuery(request);
                const auto auctions = services->auction_service().ListPublicAuctions(query);
                return common::http::ApiResponse::Success(
                    ToPublicAuctionListJson(auctions, query)
                );
            }));
        },
        {drogon::Get}
    );

    drogon::app().registerHandler(
        "/api/auctions/{1}/price",
        [services](const drogon::HttpRequestPtr&,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                   const std::string& raw_auction_id) {
            callback(common::http::ExecuteApi([&]() {
                const auto detail = services->auction_service().GetPublicAuctionDetail(
                    ParsePositiveId(raw_auction_id, "auction id")
                );
                return common::http::ApiResponse::Success(ToCurrentPriceJson(detail));
            }));
        },
        {drogon::Get}
    );

    drogon::app().registerHandler(
        "/api/auctions/{1}/bids",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                   const std::string& raw_auction_id) {
            callback(common::http::ExecuteApi([&]() {
                const auto auction_id = ParsePositiveId(raw_auction_id, "auction id");
                const auto ignored = services->auction_service().GetPublicAuctionDetail(auction_id);
                (void)ignored;
                const auto history = services->bid_service().ListAuctionBidHistory(
                    auction_id,
                    BuildBidHistoryQuery(request)
                );
                return common::http::ApiResponse::Success(ToBidHistoryJson(history));
            }));
        },
        {drogon::Get}
    );

    drogon::app().registerHandler(
        "/api/auctions/{1}",
        [services](const drogon::HttpRequestPtr&,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                   const std::string& raw_auction_id) {
            callback(common::http::ExecuteApi([&]() {
                const auto detail = services->auction_service().GetPublicAuctionDetail(
                    ParsePositiveId(raw_auction_id, "auction id")
                );
                return common::http::ApiResponse::Success(ToPublicAuctionDetailJson(detail));
            }));
        },
        {drogon::Get}
    );
#else
    static_cast<void>(services);
#endif
}

}  // namespace auction::access::http
