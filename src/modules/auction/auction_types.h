#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace auction::modules::auction {

inline constexpr std::string_view kAuctionStatusPendingStart = "PENDING_START";
inline constexpr std::string_view kAuctionStatusRunning = "RUNNING";
inline constexpr std::string_view kAuctionStatusSettling = "SETTLING";
inline constexpr std::string_view kAuctionStatusSold = "SOLD";
inline constexpr std::string_view kAuctionStatusUnsold = "UNSOLD";
inline constexpr std::string_view kAuctionStatusCancelled = "CANCELLED";

inline constexpr std::string_view kTaskTypeAuctionStart = "AUCTION_START";
inline constexpr std::string_view kTaskTypeAuctionFinish = "AUCTION_FINISH";

inline constexpr std::string_view kTaskStatusSuccess = "SUCCESS";
inline constexpr std::string_view kTaskStatusFailed = "FAILED";
inline constexpr std::string_view kTaskStatusSkipped = "SKIPPED";

inline bool IsKnownAuctionStatus(const std::string_view status) {
    return status == kAuctionStatusPendingStart || status == kAuctionStatusRunning ||
           status == kAuctionStatusSettling || status == kAuctionStatusSold ||
           status == kAuctionStatusUnsold || status == kAuctionStatusCancelled;
}

inline bool IsTerminalAuctionStatus(const std::string_view status) {
    return status == kAuctionStatusSold || status == kAuctionStatusUnsold ||
           status == kAuctionStatusCancelled;
}

struct AuctionRecord {
    std::uint64_t auction_id{0};
    std::uint64_t item_id{0};
    std::uint64_t seller_id{0};
    std::string start_time;
    std::string end_time;
    double start_price{0.0};
    double current_price{0.0};
    double bid_step{0.0};
    std::optional<std::uint64_t> highest_bidder_id;
    std::string status;
    int anti_sniping_window_seconds{0};
    int extend_seconds{0};
    std::uint64_t version{0};
    std::string created_at;
    std::string updated_at;
};

struct AuctionItemSnapshot {
    std::uint64_t item_id{0};
    std::uint64_t seller_id{0};
    std::string title;
    std::string cover_image_url;
    std::string item_status;
};

struct CreateAuctionRequest {
    std::uint64_t item_id{0};
    std::string start_time;
    std::string end_time;
    double start_price{0.0};
    double bid_step{0.0};
    int anti_sniping_window_seconds{0};
    int extend_seconds{0};
};

struct UpdateAuctionRequest {
    std::optional<std::string> start_time;
    std::optional<std::string> end_time;
    std::optional<double> start_price;
    std::optional<double> bid_step;
    std::optional<int> anti_sniping_window_seconds;
    std::optional<int> extend_seconds;
};

struct CancelAuctionRequest {
    std::string reason;
};

struct CreateAuctionResult {
    std::uint64_t auction_id{0};
    std::uint64_t item_id{0};
    std::string status;
    double current_price{0.0};
    std::string created_at;
};

struct UpdateAuctionResult {
    std::uint64_t auction_id{0};
    std::string status;
    std::string updated_at;
};

struct CancelAuctionResult {
    std::uint64_t auction_id{0};
    std::string old_status;
    std::string new_status;
    std::string cancelled_at;
};

struct AdminAuctionQuery {
    std::optional<std::string> status;
    std::optional<std::uint64_t> item_id;
    std::optional<std::uint64_t> seller_id;
    int page_no{1};
    int page_size{20};
};

struct PublicAuctionQuery {
    std::optional<std::string> keyword;
    std::optional<std::string> status;
    int page_no{1};
    int page_size{20};
};

struct AdminAuctionSummary {
    std::uint64_t auction_id{0};
    std::uint64_t item_id{0};
    std::uint64_t seller_id{0};
    std::string title;
    std::string cover_image_url;
    std::string status;
    double start_price{0.0};
    double current_price{0.0};
    double bid_step{0.0};
    std::optional<std::uint64_t> highest_bidder_id;
    std::string start_time;
    std::string end_time;
    std::string updated_at;
};

struct PublicAuctionSummary {
    std::uint64_t auction_id{0};
    std::uint64_t item_id{0};
    std::string title;
    std::string cover_image_url;
    std::string status;
    double current_price{0.0};
    std::string start_time;
    std::string end_time;
};

struct AdminAuctionDetail {
    AuctionRecord auction;
    AuctionItemSnapshot item;
    std::string seller_username;
    std::string highest_bidder_username;
};

struct PublicAuctionDetail {
    std::uint64_t auction_id{0};
    std::uint64_t item_id{0};
    std::string title;
    std::string cover_image_url;
    std::string status;
    double start_price{0.0};
    double current_price{0.0};
    double bid_step{0.0};
    std::string start_time;
    std::string end_time;
    int anti_sniping_window_seconds{0};
    int extend_seconds{0};
    std::string highest_bidder_masked;
};

struct AuctionScheduleResult {
    int scanned{0};
    int succeeded{0};
    int failed{0};
    int skipped{0};
    std::vector<std::uint64_t> affected_auction_ids;
};

}  // namespace auction::modules::auction
