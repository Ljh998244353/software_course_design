#include "BidEngine.h"
#include "ConfigManager.h"
#include "Logger.h"
#include <sstream>

namespace auction {

BidEngine::BidEngine()
    : redis_(RedisPool::instance()) {
}

BidEngine& BidEngine::instance() {
    static BidEngine instance;
    return instance;
}

void BidEngine::start() {
    running_.store(true);
    Logger::info("BidEngine started");
}

void BidEngine::stop() {
    running_.store(false);
    Logger::info("BidEngine stopped");
}

BidResult BidEngine::submitBid(int64_t auction_item_id, int64_t bidder_id,
                               double amount, const std::string& idempotent_token) {
    (void)idempotent_token;
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

    return processBidAtomic(auction_item_id, bidder_id, amount, now);
}

BidResult BidEngine::processBidAtomic(int64_t auction_item_id, int64_t bidder_id,
                                      double amount, int64_t timestamp) {
    BidResult result{};
    result.success = false;

    auto& redis = RedisPool::instance();
    auto& config = ConfigManager::instance();

    std::ostringstream key;
    key << "auction:item:" << auction_item_id;

    std::vector<std::string> keys = {key.str()};
    std::vector<std::string> args = {
        std::to_string(amount),
        std::to_string(bidder_id),
        std::to_string(timestamp),
        std::to_string(config.auction().anti_snipe_N),
        std::to_string(config.auction().anti_snipe_M)
    };

    std::string lua_script = R"(
local current_price = tonumber(redis.call('HGET', KEYS[1], 'price'))
local bid_step = tonumber(redis.call('HGET', KEYS[1], 'step'))
local end_time = tonumber(redis.call('HGET', KEYS[1], 'end_time'))
local current_bidder = redis.call('HGET', KEYS[1], 'bidder')
local anti_snipe_N = tonumber(ARGV[4])
local anti_snipe_M = tonumber(ARGV[5])

if current_price == nil or bid_step == nil or end_time == nil then
    return {'err', 'AUCTION_NOT_FOUND'}
end

if tonumber(ARGV[1]) < current_price + bid_step then
    return {'err', 'BID_TOO_LOW', current_price}
end

if tonumber(ARGV[3]) > end_time then
    return {'err', 'AUCTION_ENDED'}
end

local remaining = end_time - tonumber(ARGV[3])
if remaining < anti_snipe_N then
    local new_end_time = end_time + anti_snipe_M
    redis.call('HSET', KEYS[1], 'price', ARGV[1])
    redis.call('HSET', KEYS[1], 'bidder', ARGV[2])
    redis.call('HSET', KEYS[1], 'end_time', new_end_time)
    return {'ok', 'BID_ACCEPTED_EXTENDED', ARGV[1], new_end_time}
end

redis.call('HSET', KEYS[1], 'price', ARGV[1])
redis.call('HSET', KEYS[1], 'bidder', ARGV[2])

return {'ok', 'BID_ACCEPTED', ARGV[1]}
)";

    try {
        std::string eval_result = redis.eval(lua_script, keys, args);

        if (eval_result.find("err") != std::string::npos) {
            if (eval_result.find("BID_TOO_LOW") != std::string::npos) {
                result.error_code = "BID_TOO_LOW";
            } else if (eval_result.find("AUCTION_ENDED") != std::string::npos) {
                result.error_code = "AUCTION_ENDED";
            } else if (eval_result.find("AUCTION_NOT_FOUND") != std::string::npos) {
                result.error_code = "AUCTION_NOT_FOUND";
            }
            return result;
        }

        result.success = true;
        result.current_price = amount;
        result.highest_bidder_id = bidder_id;
        result.is_extended = (eval_result.find("EXTENDED") != std::string::npos);

        redis.zadd("auction:history:" + std::to_string(auction_item_id),
                   static_cast<double>(timestamp), std::to_string(amount));

    } catch (const std::exception& e) {
        Logger::error("Bid processing error: %s", e.what());
        result.error_code = "INTERNAL_ERROR";
    }

    return result;
}

void BidEngine::onAuctionEnded(int64_t auction_item_id) {
    Logger::info("Auction ended: %lld", (long long)auction_item_id);

    std::lock_guard<std::shared_mutex> lock(auctions_mutex_);
    active_auctions_.erase(auction_item_id);
}

bool BidEngine::loadAuctionState(int64_t auction_item_id) {
    (void)auction_item_id;
    return true;
}

void BidEngine::saveAuctionState(int64_t auction_item_id) {
    (void)auction_item_id;
}

} // namespace auction
