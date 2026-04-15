#pragma once

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace auction::ws {

struct AuctionEventMessage {
    std::string event;
    std::uint64_t auction_id{0};
    double current_price{0.0};
    std::string highest_bidder_masked;
    std::string end_time;
    std::string server_time;
};

struct PublishedAuctionEvent {
    std::optional<std::uint64_t> user_id;
    std::optional<std::uint64_t> auction_channel_id;
    AuctionEventMessage message;
};

class AuctionEventGateway {
public:
    virtual ~AuctionEventGateway() = default;

    virtual void BroadcastToAuction(
        std::uint64_t auction_id,
        const AuctionEventMessage& message
    ) = 0;
    virtual void PushToUser(std::uint64_t user_id, const AuctionEventMessage& message) = 0;
};

class InMemoryAuctionEventGateway final : public AuctionEventGateway {
public:
    void BroadcastToAuction(
        std::uint64_t auction_id,
        const AuctionEventMessage& message
    ) override;
    void PushToUser(std::uint64_t user_id, const AuctionEventMessage& message) override;

    void SetFailBroadcast(bool fail_broadcast);
    void SetFailDirect(bool fail_direct);
    [[nodiscard]] std::vector<PublishedAuctionEvent> SnapshotPublishedEvents() const;
    void Clear();

private:
    mutable std::mutex mutex_;
    bool fail_broadcast_{false};
    bool fail_direct_{false};
    std::vector<PublishedAuctionEvent> published_events_;
};

}  // namespace auction::ws
