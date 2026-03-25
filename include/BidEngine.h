#pragma once

#include <memory>
#include <atomic>
#include <unordered_map>
#include <shared_mutex>
#include <chrono>
#include "RedisPool.h"

namespace auction {

struct BidResult {
    bool success;
    std::string error_code;
    double current_price;
    int64_t highest_bidder_id;
    int bid_count;
    int64_t next_end_time;
    bool is_extended;
    int extension_count;
};

struct AuctionItemState {
    int64_t id;
    double current_price;
    double bid_step;
    int64_t highest_bidder_id;
    int64_t end_time;
    int bid_count;
    int extension_count;
    std::atomic<bool> processing{false};
};

class BidEngine {
public:
    static BidEngine& instance();

    void start();
    void stop();

    BidResult submitBid(int64_t auction_item_id, int64_t bidder_id,
                        double amount, const std::string& idempotent_token);

    void onAuctionEnded(int64_t auction_item_id);

private:
    BidEngine();

    BidResult processBidAtomic(int64_t auction_item_id, int64_t bidder_id,
                                double amount, int64_t timestamp);

    bool loadAuctionState(int64_t auction_item_id);
    void saveAuctionState(int64_t auction_item_id);

    RedisPool& redis_;

    std::unordered_map<int64_t, std::shared_ptr<AuctionItemState>> active_auctions_;
    std::shared_mutex auctions_mutex_;
    std::atomic<bool> running_{false};
};

} // namespace auction
