#pragma once

#include <string>
#include <optional>
#include <cstdint>

namespace auction {

enum class AuctionItemStatus {
    WAITING,
    ON_AUCTION,
    SOLD,
    UNSOLD
};

struct AuctionItem {
    int64_t id;
    int64_t auction_id;
    int64_t item_id;
    double current_price;
    int64_t highest_bidder_id;
    int bid_count;
    int extension_count;
    AuctionItemStatus status;
    int version;
};

class AuctionService {
public:
    static AuctionService& instance();

    bool createAuction(const std::string& name,
                      const std::string& description,
                      int64_t start_time,
                      int64_t end_time,
                      int anti_snipe_N = 60,
                      int anti_snipe_M = 120,
                      int max_extensions = 10);

    bool addItemToAuction(int64_t auction_id, int64_t item_id, double current_price);

    bool startAuction(int64_t auction_id);
    bool endAuction(int64_t auction_id);

    std::optional<AuctionItem> getAuctionItem(int64_t auction_item_id);

private:
    AuctionService() = default;

    static AuctionItemStatus stringToStatus(const std::string& status);
};

} // namespace auction
