#include "access/http/auction_http.h"

#include <stdexcept>

#include "common/errors/error_code.h"
#include "common/http/api_response.h"
#include "common/logging/logger.h"
#include "modules/auction/auction_exception.h"

#if AUCTION_HAS_DROGON
#include <drogon/drogon.h>
#endif

namespace auction::access::http {

#if AUCTION_HAS_DROGON

namespace {

void AddCorsHeaders(const drogon::HttpResponsePtr& response) {
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
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

int SafeParseInt(const std::string& value, const std::string& param_name) {
    try {
        std::size_t parsed = 0;
        const auto result = std::stoi(value, &parsed);
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

int ClampPageNo(int value) {
    return value < 1 ? 1 : value;
}

int ClampPageSize(int value) {
    if (value < 1) return 1;
    if (value > 100) return 100;
    return value;
}

Json::Value PublicAuctionSummaryToJson(const modules::auction::PublicAuctionSummary& s) {
    Json::Value json(Json::objectValue);
    json["auction_id"] = static_cast<Json::UInt64>(s.auction_id);
    json["item_id"] = static_cast<Json::UInt64>(s.item_id);
    json["title"] = s.title;
    json["category_name"] = s.category_name;
    json["cover_image_url"] = s.cover_image_url;
    json["status"] = s.status;
    json["start_price"] = s.start_price;
    json["current_price"] = s.current_price;
    json["bid_step"] = s.bid_step;
    json["seller_username"] = s.seller_username;
    json["seller_rating"] = s.seller_rating;
    json["seller_deals"] = s.seller_deals;
    json["watcher_count"] = s.watcher_count;
    json["trade_mode"] = s.trade_mode;
    json["location"] = s.location;
    json["tags_json"] = s.tags_json;
    json["description"] = s.description;
    json["start_time"] = s.start_time;
    json["end_time"] = s.end_time;
    return json;
}

Json::Value PublicAuctionDetailToJson(const modules::auction::PublicAuctionDetail& d) {
    Json::Value json(Json::objectValue);
    json["auction_id"] = static_cast<Json::UInt64>(d.auction_id);
    json["item_id"] = static_cast<Json::UInt64>(d.item_id);
    json["title"] = d.title;
    json["category_name"] = d.category_name;
    json["cover_image_url"] = d.cover_image_url;
    json["status"] = d.status;
    json["start_price"] = d.start_price;
    json["current_price"] = d.current_price;
    json["bid_step"] = d.bid_step;
    json["seller_username"] = d.seller_username;
    json["seller_rating"] = d.seller_rating;
    json["seller_deals"] = d.seller_deals;
    json["watcher_count"] = d.watcher_count;
    json["trade_mode"] = d.trade_mode;
    json["location"] = d.location;
    json["tags_json"] = d.tags_json;
    json["description"] = d.description;
    json["start_time"] = d.start_time;
    json["end_time"] = d.end_time;
    json["anti_sniping_window_seconds"] = d.anti_sniping_window_seconds;
    json["extend_seconds"] = d.extend_seconds;
    json["highest_bidder_masked"] = d.highest_bidder_masked;
    return json;
}

}  // namespace

#endif

void RegisterAuctionHttpRoutes(
    modules::auction::AuctionService& auction_service
) {
#if AUCTION_HAS_DROGON

    // GET /api/auctions - public auction list
    RegisterCorsPreflight("/api/auctions");
    drogon::app().registerHandler(
        "/api/auctions",
        [&auction_service](const drogon::HttpRequestPtr& request,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            try {
                modules::auction::PublicAuctionQuery query;
                const auto& keyword = request->getParameter("keyword");
                const auto& status = request->getParameter("status");
                const auto& category = request->getParameter("category");
                const auto& price_min = request->getParameter("price_min");
                const auto& price_max = request->getParameter("price_max");
                const auto& seller_rating = request->getParameter("seller_rating");
                const auto& seller_has_deals = request->getParameter("seller_has_deals");
                const auto& trade_mode = request->getParameter("trade_mode");
                const auto& page_no = request->getParameter("page_no");
                const auto& page_size = request->getParameter("page_size");

                if (!keyword.empty()) query.keyword = keyword;
                if (!status.empty()) query.status = status;
                if (!category.empty()) query.category = category;
                if (!price_min.empty()) query.price_min = std::stod(price_min);
                if (!price_max.empty()) query.price_max = std::stod(price_max);
                if (!seller_rating.empty()) query.seller_rating_min = std::stod(seller_rating);
                if (!seller_has_deals.empty()) {
                    query.seller_has_deals =
                        seller_has_deals == "1" || seller_has_deals == "true";
                }
                if (!trade_mode.empty()) query.trade_mode = trade_mode;
                if (!page_no.empty()) query.page_no = ClampPageNo(SafeParseInt(page_no, "page_no"));
                if (!page_size.empty()) query.page_size = ClampPageSize(SafeParseInt(page_size, "page_size"));

                const auto auctions = auction_service.ListPublicAuctions(query);

                Json::Value arr(Json::arrayValue);
                for (const auto& a : auctions) {
                    arr.append(PublicAuctionSummaryToJson(a));
                }

                callback(MakeOk(std::move(arr)));
            } catch (const modules::auction::AuctionException& e) {
                callback(MakeError(e.code(), e.what()));
            } catch (const std::invalid_argument& e) {
                callback(MakeError(common::errors::ErrorCode::kInvalidArgument, e.what()));
            } catch (...) {
                common::logging::Logger::Instance().Error(
                    "unhandled exception in GET /api/auctions"
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

    // GET /api/auctions/{id} - public auction detail
    RegisterCorsPreflight("/api/auctions/{id}");
    drogon::app().registerHandler(
        "/api/auctions/{id}",
        [&auction_service](const drogon::HttpRequestPtr& request,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                           const std::string& id_str) {
            static_cast<void>(request);
            try {
                if (id_str.empty()) {
                    callback(MakeError(
                        common::errors::ErrorCode::kInvalidArgument,
                        "auction id is required"
                    ));
                    return;
                }

                const auto auction_id = SafeParseUint64(id_str, "auction id");
                const auto detail = auction_service.GetPublicAuctionDetail(auction_id);

                callback(MakeOk(PublicAuctionDetailToJson(detail)));
            } catch (const modules::auction::AuctionException& e) {
                const auto http_status =
                    (e.code() == common::errors::ErrorCode::kAuctionNotFound)
                        ? drogon::k404NotFound
                        : drogon::k400BadRequest;
                callback(MakeError(e.code(), e.what(), http_status));
            } catch (const std::invalid_argument& e) {
                callback(MakeError(common::errors::ErrorCode::kInvalidArgument, e.what()));
            } catch (...) {
                common::logging::Logger::Instance().Error(
                    "unhandled exception in GET /api/auctions/{id}"
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

#else
    static_cast<void>(auction_service);
#endif
}

}  // namespace auction::access::http
