#include "AuctionService.h"
#include "MySQLPool.h"
#include "BidEngine.h"
#include "Logger.h"

namespace auction {

AuctionService& AuctionService::instance() {
    static AuctionService instance;
    return instance;
}

bool AuctionService::createAuction(const std::string& name,
                                  const std::string& description,
                                  int64_t start_time,
                                  int64_t end_time,
                                  int anti_snipe_N,
                                  int anti_snipe_M,
                                  int max_extensions) {
    auto& pool = MySQLPool::instance();

    std::string sql = R"(
        INSERT INTO auctions (name, description, start_time, end_time,
                            anti_snipe_N, anti_snipe_M, max_extensions, status)
        VALUES (?, ?, FROM_UNIXTIME(?), FROM_UNIXTIME(?), ?, ?, ?, 'planned')
    )";

    int result = pool.executeUpdate(sql, {
        name,
        description,
        std::to_string(start_time),
        std::to_string(end_time),
        std::to_string(anti_snipe_N),
        std::to_string(anti_snipe_M),
        std::to_string(max_extensions)
    });

    return result > 0;
}

bool AuctionService::addItemToAuction(int64_t auction_id, int64_t item_id, double current_price) {
    auto& pool = MySQLPool::instance();

    std::string sql = R"(
        INSERT INTO auction_items (auction_id, item_id, current_price, status)
        VALUES (?, ?, ?, 'waiting')
    )";

    int result = pool.executeUpdate(sql, {
        std::to_string(auction_id),
        std::to_string(item_id),
        std::to_string(current_price)
    });

    return result > 0;
}

bool AuctionService::startAuction(int64_t auction_id) {
    auto& pool = MySQLPool::instance();

    std::string sql = "UPDATE auctions SET status = 'running' WHERE id = ? AND status = 'planned'";

    int result = pool.executeUpdate(sql, {std::to_string(auction_id)});

    if (result > 0) {
        AUCTION_LOG_INFO("Auction {} started", auction_id);
        return true;
    }
    return false;
}

bool AuctionService::endAuction(int64_t auction_id) {
    auto& pool = MySQLPool::instance();

    std::string sql = "UPDATE auctions SET status = 'ended' WHERE id = ? AND status = 'running'";

    int result = pool.executeUpdate(sql, {std::to_string(auction_id)});

    if (result > 0) {
        AUCTION_LOG_INFO("Auction {} ended", auction_id);
        BidEngine::instance().onAuctionEnded(auction_id);
        return true;
    }
    return false;
}

std::optional<AuctionItem> AuctionService::getAuctionItem(int64_t auction_item_id) {
    auto& pool = MySQLPool::instance();

    std::string sql = "SELECT * FROM auction_items WHERE id = ?";

    auto result = pool.executeQuery(sql, {std::to_string(auction_item_id)});

    if (!result || !result->next()) {
        return std::nullopt;
    }

    AuctionItem item;
    item.id = result->getInt64("id");
    item.auction_id = result->getInt64("auction_id");
    item.item_id = result->getInt64("item_id");
    item.current_price = result->getDouble("current_price");
    item.highest_bidder_id = result->getInt64("highest_bidder_id");
    item.bid_count = result->getInt("bid_count");
    item.extension_count = result->getInt("extension_count");
    item.status = stringToStatus(result->getString("status"));
    return item;
}

AuctionItemStatus AuctionService::stringToStatus(const std::string& status) {
    if (status == "waiting") return AuctionItemStatus::WAITING;
    if (status == "on_auction") return AuctionItemStatus::ON_AUCTION;
    if (status == "sold") return AuctionItemStatus::SOLD;
    if (status == "unsold") return AuctionItemStatus::UNSOLD;
    return AuctionItemStatus::WAITING;
}

} // namespace auction
