#include "ws/auction_event_gateway.h"

#include <stdexcept>

namespace auction::ws {

void InMemoryAuctionEventGateway::BroadcastToAuction(
    const std::uint64_t auction_id,
    const AuctionEventMessage& message
) {
    std::lock_guard lock(mutex_);
    if (fail_broadcast_) {
        throw std::runtime_error("auction broadcast channel is unavailable");
    }
    published_events_.push_back(PublishedAuctionEvent{
        .user_id = std::nullopt,
        .auction_channel_id = auction_id,
        .message = message,
    });
}

void InMemoryAuctionEventGateway::PushToUser(
    const std::uint64_t user_id,
    const AuctionEventMessage& message
) {
    std::lock_guard lock(mutex_);
    if (fail_direct_) {
        throw std::runtime_error("auction direct push channel is unavailable");
    }
    published_events_.push_back(PublishedAuctionEvent{
        .user_id = user_id,
        .auction_channel_id = std::nullopt,
        .message = message,
    });
}

void InMemoryAuctionEventGateway::SetFailBroadcast(const bool fail_broadcast) {
    std::lock_guard lock(mutex_);
    fail_broadcast_ = fail_broadcast;
}

void InMemoryAuctionEventGateway::SetFailDirect(const bool fail_direct) {
    std::lock_guard lock(mutex_);
    fail_direct_ = fail_direct;
}

std::vector<PublishedAuctionEvent> InMemoryAuctionEventGateway::SnapshotPublishedEvents() const {
    std::lock_guard lock(mutex_);
    return published_events_;
}

void InMemoryAuctionEventGateway::Clear() {
    std::lock_guard lock(mutex_);
    published_events_.clear();
}

}  // namespace auction::ws
