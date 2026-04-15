#pragma once

#include <cstdint>
#include <mutex>
#include <optional>
#include <unordered_map>

#include "modules/bid/bid_types.h"

namespace auction::modules::bid {

class BidCacheStore {
public:
    virtual ~BidCacheStore() = default;

    virtual void UpsertAuctionSnapshot(const BidCacheSnapshot& snapshot) = 0;
    [[nodiscard]] virtual std::optional<BidCacheSnapshot> FindAuctionSnapshot(
        std::uint64_t auction_id
    ) = 0;
    virtual void RemoveAuctionSnapshot(std::uint64_t auction_id) = 0;
};

class InMemoryBidCacheStore final : public BidCacheStore {
public:
    void UpsertAuctionSnapshot(const BidCacheSnapshot& snapshot) override;
    [[nodiscard]] std::optional<BidCacheSnapshot> FindAuctionSnapshot(
        std::uint64_t auction_id
    ) override;
    void RemoveAuctionSnapshot(std::uint64_t auction_id) override;

    void SetFailUpsert(bool fail_upsert);

private:
    std::mutex mutex_;
    bool fail_upsert_{false};
    std::unordered_map<std::uint64_t, BidCacheSnapshot> snapshots_;
};

}  // namespace auction::modules::bid
