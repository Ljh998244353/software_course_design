#include "access/http/review_http.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

#include "common/http/http_utils.h"
#include "modules/review/review_types.h"

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

int ReadRequiredInt(const Json::Value& json, const std::string_view field_name) {
    const std::string field(field_name);
    if (!json.isMember(field) || json[field].isNull() || !json[field].isInt()) {
        throw std::invalid_argument(field + " must be an integer");
    }
    return json[field].asInt();
}

std::uint64_t ReadRequiredUInt64(const Json::Value& json, const std::string_view field_name) {
    const std::string field(field_name);
    if (!json.isMember(field) || json[field].isNull() || !json[field].isUInt64()) {
        throw std::invalid_argument(field + " must be a positive integer");
    }
    const auto value = json[field].asUInt64();
    if (value == 0) {
        throw std::invalid_argument(field + " must be a positive integer");
    }
    return value;
}

modules::review::SubmitReviewRequest BuildSubmitReviewRequest(const Json::Value& body) {
    return modules::review::SubmitReviewRequest{
        .rating = ReadRequiredInt(body, "rating"),
        .content = common::http::ReadOptionalString(body, "content").value_or(""),
    };
}

Json::Value ToReviewRecordJson(const modules::review::ReviewRecord& review) {
    Json::Value json(Json::objectValue);
    json["reviewId"] = static_cast<Json::UInt64>(review.review_id);
    json["orderId"] = static_cast<Json::UInt64>(review.order_id);
    json["reviewerId"] = static_cast<Json::UInt64>(review.reviewer_id);
    json["revieweeId"] = static_cast<Json::UInt64>(review.reviewee_id);
    json["reviewType"] = review.review_type;
    json["rating"] = review.rating;
    json["content"] = review.content;
    json["createdAt"] = review.created_at;
    return json;
}

Json::Value ToSubmitReviewResultJson(const modules::review::SubmitReviewResult& result) {
    Json::Value json = ToReviewRecordJson(result.review);
    json["orderStatusAfter"] = result.order_status_after;
    json["orderMarkedReviewed"] = result.order_marked_reviewed;
    return json;
}

Json::Value ToOrderReviewListJson(const modules::review::OrderReviewListResult& result) {
    Json::Value data(Json::objectValue);
    data["orderId"] = static_cast<Json::UInt64>(result.order_id);
    data["orderStatus"] = result.order_status;
    Json::Value reviews(Json::arrayValue);
    for (const auto& review : result.reviews) {
        reviews.append(ToReviewRecordJson(review));
    }
    data["reviews"] = std::move(reviews);
    data["total"] = static_cast<Json::UInt64>(result.reviews.size());
    return data;
}

Json::Value ToReviewSummaryJson(const modules::review::ReviewSummary& summary) {
    Json::Value json(Json::objectValue);
    json["userId"] = static_cast<Json::UInt64>(summary.user_id);
    json["totalReviews"] = summary.total_reviews;
    json["averageRating"] = summary.average_rating;
    json["positiveReviews"] = summary.positive_reviews;
    json["neutralReviews"] = summary.neutral_reviews;
    json["negativeReviews"] = summary.negative_reviews;
    return json;
}

#endif

}  // namespace

void RegisterReviewHttpRoutes(const std::shared_ptr<common::http::HttpServiceContext> services) {
#if AUCTION_HAS_DROGON
    drogon::app().registerHandler(
        "/api/reviews",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            callback(common::http::ExecuteApi([&]() {
                const auto body = common::http::RequireJsonBody(request);
                const auto order_id = ReadRequiredUInt64(body, "orderId");
                const auto result = services->review_service().SubmitReview(
                    request->getHeader("authorization"),
                    order_id,
                    BuildSubmitReviewRequest(body)
                );
                return common::http::ApiResponse::Success(ToSubmitReviewResultJson(result));
            }));
        },
        {drogon::Post}
    );

    drogon::app().registerHandler(
        "/api/orders/{1}/reviews",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                   const std::string& raw_order_id) {
            callback(common::http::ExecuteApi([&]() {
                const auto order_id = ParsePositiveId(raw_order_id, "order id");
                (void)services->order_service().GetMyOrder(
                    request->getHeader("authorization"),
                    order_id
                );
                const auto result = services->review_service().ListOrderReviews(order_id);
                return common::http::ApiResponse::Success(ToOrderReviewListJson(result));
            }));
        },
        {drogon::Get}
    );

    drogon::app().registerHandler(
        "/api/users/{1}/reviews/summary",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                   const std::string& raw_user_id) {
            callback(common::http::ExecuteApi([&]() {
                (void)common::http::RequireAuthContext(request, services->auth_middleware());
                const auto result = services->review_service().GetUserReviewSummary(
                    ParsePositiveId(raw_user_id, "user id")
                );
                return common::http::ApiResponse::Success(ToReviewSummaryJson(result));
            }));
        },
        {drogon::Get}
    );
#else
    static_cast<void>(services);
#endif
}

}  // namespace auction::access::http
