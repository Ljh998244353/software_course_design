#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "modules/bid/bid_types.h"
#include "repository/base_repository.h"

namespace auction::repository {

struct CreateBidRecordParams {
    std::uint64_t auction_id{0};
    std::uint64_t bidder_id{0};
    std::string request_id;
    std::string bid_amount;
    std::string bid_status;
};

class BidRepository : public BaseRepository {
public:
    using BaseRepository::BaseRepository;
    using BaseRepository::EscapeString;

    [[nodiscard]] std::optional<modules::bid::AuctionBidSnapshot> FindAuctionSnapshotById(
        std::uint64_t auction_id
    ) const;
    [[nodiscard]] std::optional<modules::bid::AuctionBidSnapshot> FindAuctionSnapshotByIdForUpdate(
        std::uint64_t auction_id
    ) const;
    [[nodiscard]] bool IsAuctionExpired(std::uint64_t auction_id) const;
    [[nodiscard]] bool IsWithinAntiSnipingWindow(
        std::uint64_t auction_id,
        int window_seconds
    ) const;

    [[nodiscard]] std::optional<modules::bid::BidRecord> FindBidByRequestId(
        std::uint64_t auction_id,
        std::uint64_t bidder_id,
        std::string_view request_id
    ) const;
    [[nodiscard]] std::optional<modules::bid::BidRecord> FindCurrentWinningBid(
        std::uint64_t auction_id
    ) const;
    [[nodiscard]] std::optional<modules::bid::BidRecord> FindLatestBidByBidder(
        std::uint64_t auction_id,
        std::uint64_t bidder_id
    ) const;
    [[nodiscard]] modules::bid::BidRecord CreateBidRecord(const CreateBidRecordParams& params) const;

    void MarkWinningBidOutbid(std::uint64_t auction_id) const;
    void UpdateAuctionPrice(
        std::uint64_t auction_id,
        const std::string& current_price,
        std::uint64_t highest_bidder_id
    ) const;
    void UpdateAuctionPriceAndExtendEndTime(
        std::uint64_t auction_id,
        const std::string& current_price,
        std::uint64_t highest_bidder_id,
        int extend_seconds
    ) const;

    [[nodiscard]] std::vector<modules::bid::BidHistoryEntry> ListAuctionBidHistory(
        std::uint64_t auction_id,
        const modules::bid::BidHistoryQuery& query
    ) const;
};

}  // namespace auction::repository
