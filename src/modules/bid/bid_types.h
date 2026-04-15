#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace auction::modules::bid {

inline constexpr std::string_view kBidStatusWinning = "WINNING";
inline constexpr std::string_view kBidStatusOutbid = "OUTBID";

inline bool IsKnownBidStatus(const std::string_view bid_status) {
    return bid_status == kBidStatusWinning || bid_status == kBidStatusOutbid;
}

struct AuctionBidSnapshot {
    std::uint64_t auction_id{0};
    std::uint64_t item_id{0};
    std::uint64_t seller_id{0};
    std::string status;
    std::string start_time;
    std::string end_time;
    double start_price{0.0};
    double current_price{0.0};
    double bid_step{0.0};
    std::optional<std::uint64_t> highest_bidder_id;
    std::string highest_bidder_username;
    int anti_sniping_window_seconds{0};
    int extend_seconds{0};
    std::uint64_t version{0};
};

struct BidRecord {
    std::uint64_t bid_id{0};
    std::uint64_t auction_id{0};
    std::uint64_t bidder_id{0};
    std::string bidder_username;
    std::string request_id;
    double bid_amount{0.0};
    std::string bid_status;
    std::string bid_time;
    std::string created_at;
};

struct PlaceBidRequest {
    std::string request_id;
    double bid_amount{0.0};
};

struct PlaceBidResult {
    std::uint64_t bid_id{0};
    std::uint64_t auction_id{0};
    double bid_amount{0.0};
    std::string bid_status;
    double current_price{0.0};
    std::string highest_bidder_masked;
    std::string end_time;
    bool extended{false};
    std::string server_time;
};

struct BidHistoryQuery {
    int page_no{1};
    int page_size{20};
};

struct BidHistoryEntry {
    std::uint64_t bid_id{0};
    double bid_amount{0.0};
    std::string bid_status;
    std::string bid_time;
    std::string bidder_masked;
};

struct BidHistoryResult {
    std::vector<BidHistoryEntry> records;
    int page_no{1};
    int page_size{20};
};

struct MyBidStatusResult {
    std::uint64_t auction_id{0};
    std::uint64_t user_id{0};
    double my_highest_bid{0.0};
    bool is_current_highest{false};
    std::string last_bid_time;
    double current_price{0.0};
    std::string end_time;
};

struct BidCacheSnapshot {
    std::uint64_t auction_id{0};
    double current_price{0.0};
    double minimum_next_bid{0.0};
    std::string highest_bidder_masked;
    std::string end_time;
    std::string server_time;
};

}  // namespace auction::modules::bid
