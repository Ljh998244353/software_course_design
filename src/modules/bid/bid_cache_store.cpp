#include "modules/bid/bid_cache_store.h"

#include <stdexcept>

namespace auction::modules::bid {

void InMemoryBidCacheStore::UpsertAuctionSnapshot(const BidCacheSnapshot& snapshot) {
    std::lock_guard lock(mutex_);
    if (fail_upsert_) {
        throw std::runtime_error("bid cache store is unavailable");
    }
    snapshots_[snapshot.auction_id] = snapshot;
}

std::optional<BidCacheSnapshot> InMemoryBidCacheStore::FindAuctionSnapshot(
    const std::uint64_t auction_id
) {
    std::lock_guard lock(mutex_);
    const auto iterator = snapshots_.find(auction_id);
    if (iterator == snapshots_.end()) {
        return std::nullopt;
    }
    return iterator->second;
}

void InMemoryBidCacheStore::RemoveAuctionSnapshot(const std::uint64_t auction_id) {
    std::lock_guard lock(mutex_);
    snapshots_.erase(auction_id);
}

void InMemoryBidCacheStore::SetFailUpsert(const bool fail_upsert) {
    std::lock_guard lock(mutex_);
    fail_upsert_ = fail_upsert;
}

}  // namespace auction::modules::bid
