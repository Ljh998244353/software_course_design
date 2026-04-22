#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace auction::modules::review {

inline constexpr std::string_view kReviewTypeBuyerToSeller = "BUYER_TO_SELLER";
inline constexpr std::string_view kReviewTypeSellerToBuyer = "SELLER_TO_BUYER";

inline constexpr int kReviewRatingMin = 1;
inline constexpr int kReviewRatingMax = 5;
inline constexpr std::size_t kReviewContentMaxLength = 500;

inline bool IsSupportedReviewType(const std::string_view review_type) {
    return review_type == kReviewTypeBuyerToSeller || review_type == kReviewTypeSellerToBuyer;
}

struct ReviewRecord {
    std::uint64_t review_id{0};
    std::uint64_t order_id{0};
    std::uint64_t reviewer_id{0};
    std::uint64_t reviewee_id{0};
    std::string review_type;
    int rating{0};
    std::string content;
    std::string created_at;
};

struct SubmitReviewRequest {
    int rating{0};
    std::string content;
};

struct SubmitReviewResult {
    ReviewRecord review;
    std::string order_status_after;
    bool order_marked_reviewed{false};
};

struct OrderReviewListResult {
    std::uint64_t order_id{0};
    std::string order_status;
    std::vector<ReviewRecord> reviews;
};

struct ReviewSummary {
    std::uint64_t user_id{0};
    int total_reviews{0};
    double average_rating{0.0};
    int positive_reviews{0};
    int neutral_reviews{0};
    int negative_reviews{0};
};

}  // namespace auction::modules::review
