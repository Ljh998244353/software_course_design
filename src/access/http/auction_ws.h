#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "ws/auction_event_gateway.h"

#if AUCTION_HAS_DROGON
#include <drogon/WebSocketController.h>
#include <drogon/WebSocketConnection.h>

namespace auction::access::http {

class DrogonAuctionEventGateway final : public ws::AuctionEventGateway {
public:
    void BroadcastToAuction(
        std::uint64_t auction_id,
        const ws::AuctionEventMessage& message
    ) override;
    void PushToUser(
        std::uint64_t user_id,
        const ws::AuctionEventMessage& message
    ) override;

    void AddSubscriber(std::uint64_t auction_id, const drogon::WebSocketConnectionPtr& conn);
    void SetUserForConnection(const drogon::WebSocketConnectionPtr& conn, std::uint64_t user_id);
    void Disconnect(std::uint64_t auction_id, const drogon::WebSocketConnectionPtr& conn);

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::uint64_t, std::unordered_set<drogon::WebSocketConnectionPtr>> auction_subscribers_;
    std::unordered_map<drogon::WebSocketConnectionPtr, std::uint64_t> connection_to_user_;
    std::unordered_map<std::uint64_t, std::unordered_set<drogon::WebSocketConnectionPtr>> user_connections_;
};

class AuctionWebSocketController
    : public drogon::WebSocketController<AuctionWebSocketController, false> {
public:
    void handleNewMessage(
        const drogon::WebSocketConnectionPtr& wsConnPtr,
        std::string&& message,
        const drogon::WebSocketMessageType& type
    ) override;
    void handleConnectionClosed(
        const drogon::WebSocketConnectionPtr& wsConnPtr
    ) override;
    void handleNewConnection(
        const drogon::HttpRequestPtr& req,
        const drogon::WebSocketConnectionPtr& wsConnPtr
    ) override;

    WS_PATH_LIST_BEGIN
    WS_PATH_ADD("/ws/auction/{id}", drogon::Get);
    WS_PATH_LIST_END

    static void SetGateway(std::shared_ptr<DrogonAuctionEventGateway> gateway);
    static void SetTokenSecret(std::string secret);

private:
    static std::shared_ptr<DrogonAuctionEventGateway> gateway_;
    static std::mutex gateway_mutex_;
    static std::string token_secret_;
};

}  // namespace auction::access::http

#endif
