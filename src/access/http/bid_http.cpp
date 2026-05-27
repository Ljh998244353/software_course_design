#include "access/http/bid_http.h"

#include <stdexcept>

#include "common/errors/error_code.h"
#include "common/http/api_response.h"
#include "common/logging/logger.h"
#include "modules/auth/auth_exception.h"
#include "modules/bid/bid_exception.h"

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

Json::Value BidHistoryEntryToJson(const modules::bid::BidHistoryEntry& e) {
    Json::Value json(Json::objectValue);
    json["bid_id"] = static_cast<Json::UInt64>(e.bid_id);
    json["bid_amount"] = e.bid_amount;
    json["bid_status"] = e.bid_status;
    json["bid_time"] = e.bid_time;
    json["bidder_masked"] = e.bidder_masked;
    return json;
}

Json::Value PlaceBidResultToJson(const modules::bid::PlaceBidResult& r) {
    Json::Value json(Json::objectValue);
    json["bid_id"] = static_cast<Json::UInt64>(r.bid_id);
    json["auction_id"] = static_cast<Json::UInt64>(r.auction_id);
    json["bid_amount"] = r.bid_amount;
    json["bid_status"] = r.bid_status;
    json["current_price"] = r.current_price;
    json["highest_bidder_masked"] = r.highest_bidder_masked;
    json["end_time"] = r.end_time;
    json["extended"] = r.extended;
    json["server_time"] = r.server_time;
    return json;
}

void MapBidHttpStatus(
    const common::errors::ErrorCode code,
    drogon::HttpStatusCode& out_status
) {
    switch (code) {
        case common::errors::ErrorCode::kBidAuctionNotFound:
            out_status = drogon::k404NotFound;
            break;
        case common::errors::ErrorCode::kBidAuctionNotStarted:
        case common::errors::ErrorCode::kBidAuctionClosed:
        case common::errors::ErrorCode::kBidAuctionStatusInvalid:
            out_status = drogon::k409Conflict;
            break;
        case common::errors::ErrorCode::kBidConflict:
            out_status = drogon::k409Conflict;
            break;
        case common::errors::ErrorCode::kBidAmountTooLow:
            out_status = drogon::k409Conflict;
            break;
        case common::errors::ErrorCode::kBidIdempotencyConflict:
            out_status = drogon::k409Conflict;
            break;
        case common::errors::ErrorCode::kBidRateLimited:
            out_status = static_cast<drogon::HttpStatusCode>(429);
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

void RegisterBidHttpRoutes(
    modules::bid::BidService& bid_service
) {
#if AUCTION_HAS_DROGON

    // GET /api/auctions/{id}/bids - public bid history
    RegisterCorsPreflight("/api/auctions/{id}/bids");
    drogon::app().registerHandler(
        "/api/auctions/{id}/bids",
        [&bid_service](const drogon::HttpRequestPtr& request,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                       const std::string& id_str) {
            try {
                if (id_str.empty()) {
                    callback(MakeError(
                        common::errors::ErrorCode::kInvalidArgument,
                        "auction id is required"
                    ));
                    return;
                }

                const auto auction_id = SafeParseUint64(id_str, "auction id");

                modules::bid::BidHistoryQuery query;
                const auto& page_no = request->getParameter("page_no");
                const auto& page_size = request->getParameter("page_size");
                if (!page_no.empty()) query.page_no = ClampPageNo(SafeParseInt(page_no, "page_no"));
                if (!page_size.empty()) query.page_size = ClampPageSize(SafeParseInt(page_size, "page_size"));

                const auto result = bid_service.ListAuctionBidHistory(auction_id, query);

                Json::Value data(Json::objectValue);
                Json::Value arr(Json::arrayValue);
                for (const auto& entry : result.records) {
                    arr.append(BidHistoryEntryToJson(entry));
                }
                data["records"] = std::move(arr);
                data["page_no"] = result.page_no;
                data["page_size"] = result.page_size;

                callback(MakeOk(std::move(data)));
            } catch (const modules::bid::BidException& e) {
                drogon::HttpStatusCode http_status = drogon::k400BadRequest;
                MapBidHttpStatus(e.code(), http_status);
                callback(MakeError(e.code(), e.what(), http_status));
            } catch (const std::invalid_argument& e) {
                callback(MakeError(common::errors::ErrorCode::kInvalidArgument, e.what()));
            } catch (...) {
                common::logging::Logger::Instance().Error(
                    "unhandled exception in GET /api/auctions/{id}/bids"
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

    // POST /api/auctions/{id}/bids - place bid (requires auth)
    drogon::app().registerHandler(
        "/api/auctions/{id}/bids",
        [&bid_service](const drogon::HttpRequestPtr& request,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                       const std::string& id_str) {
            try {
                if (id_str.empty()) {
                    callback(MakeError(
                        common::errors::ErrorCode::kInvalidArgument,
                        "auction id is required"
                    ));
                    return;
                }

                const auto auction_id = SafeParseUint64(id_str, "auction id");
                const auto auth_header = request->getHeader("authorization");

                const auto body = request->getJsonObject();
                if (!body) {
                    callback(MakeError(
                        common::errors::ErrorCode::kInvalidArgument,
                        "request body must be valid JSON"
                    ));
                    return;
                }

                modules::bid::PlaceBidRequest req;
                req.request_id = body->get("request_id", "").asString();
                req.bid_amount = body->get("bid_amount", 0.0).asDouble();

                const auto result = bid_service.PlaceBid(auth_header, auction_id, req);

                callback(MakeOk(PlaceBidResultToJson(result)));
            } catch (const modules::bid::BidException& e) {
                drogon::HttpStatusCode http_status = drogon::k400BadRequest;
                MapBidHttpStatus(e.code(), http_status);
                callback(MakeError(e.code(), e.what(), http_status));
            } catch (const modules::auth::AuthException& e) {
                drogon::HttpStatusCode http_status = drogon::k401Unauthorized;
                MapBidHttpStatus(e.code(), http_status);
                callback(MakeError(e.code(), e.what(), http_status));
            } catch (const std::invalid_argument& e) {
                callback(MakeError(common::errors::ErrorCode::kInvalidArgument, e.what()));
            } catch (...) {
                common::logging::Logger::Instance().Error(
                    "unhandled exception in POST /api/auctions/{id}/bids"
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
    static_cast<void>(bid_service);
#endif
}

}  // namespace auction::access::http
