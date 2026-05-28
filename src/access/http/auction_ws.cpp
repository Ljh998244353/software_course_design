#include "access/http/auction_ws.h"

#if AUCTION_HAS_DROGON

#include <drogon/utils/Utilities.h>
#include <json/json.h>

#include "common/logging/logger.h"
#include "modules/auth/token_codec.h"

namespace auction::access::http {

namespace {

std::string SerializeMessage(const ws::AuctionEventMessage& message) {
    Json::Value root;
    root["event"] = message.event;
    root["auction_id"] = static_cast<Json::UInt64>(message.auction_id);
    root["current_price"] = message.current_price;
    root["highest_bidder_masked"] = message.highest_bidder_masked;
    root["end_time"] = message.end_time;
    root["server_time"] = message.server_time;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, root);
}

std::string BuildPongMessage() {
    Json::Value pong;
    pong["event"] = "PONG";

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, pong);
}

std::string BuildWelcomeMessage(std::uint64_t auction_id) {
    Json::Value welcome;
    welcome["event"] = "CONNECTED";
    welcome["auction_id"] = static_cast<Json::UInt64>(auction_id);

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, welcome);
}

}  // namespace

std::shared_ptr<DrogonAuctionEventGateway> AuctionWebSocketController::gateway_;
std::mutex AuctionWebSocketController::gateway_mutex_;
std::string AuctionWebSocketController::token_secret_;

void DrogonAuctionEventGateway::BroadcastToAuction(
    const std::uint64_t auction_id,
    const ws::AuctionEventMessage& message
) {
    const auto payload = SerializeMessage(message);
    std::vector<drogon::WebSocketConnectionPtr> targets;

    {
        std::lock_guard lock(mutex_);
        auto it = auction_subscribers_.find(auction_id);
        if (it == auction_subscribers_.end()) {
            return;
        }
        targets.assign(it->second.begin(), it->second.end());
    }

    for (const auto& conn : targets) {
        if (conn && conn->connected()) {
            conn->send(payload);
        }
    }
}

void DrogonAuctionEventGateway::PushToUser(
    const std::uint64_t user_id,
    const ws::AuctionEventMessage& message
) {
    const auto payload = SerializeMessage(message);
    std::vector<drogon::WebSocketConnectionPtr> targets;

    {
        std::lock_guard lock(mutex_);
        auto it = user_connections_.find(user_id);
        if (it == user_connections_.end()) {
            return;
        }
        targets.assign(it->second.begin(), it->second.end());
    }

    for (const auto& conn : targets) {
        if (conn && conn->connected()) {
            conn->send(payload);
        }
    }
}

void DrogonAuctionEventGateway::AddSubscriber(
    const std::uint64_t auction_id,
    const drogon::WebSocketConnectionPtr& conn
) {
    std::lock_guard lock(mutex_);
    auction_subscribers_[auction_id].insert(conn);
}

void DrogonAuctionEventGateway::SetUserForConnection(
    const drogon::WebSocketConnectionPtr& conn,
    const std::uint64_t user_id
) {
    std::lock_guard lock(mutex_);
    auto old_it = connection_to_user_.find(conn);
    if (old_it != connection_to_user_.end() && old_it->second != user_id) {
        auto user_it = user_connections_.find(old_it->second);
        if (user_it != user_connections_.end()) {
            user_it->second.erase(conn);
            if (user_it->second.empty()) {
                user_connections_.erase(user_it);
            }
        }
    }
    connection_to_user_[conn] = user_id;
    user_connections_[user_id].insert(conn);
}

void DrogonAuctionEventGateway::Disconnect(
    const std::uint64_t auction_id,
    const drogon::WebSocketConnectionPtr& conn
) {
    std::lock_guard lock(mutex_);
    auto sub_it = auction_subscribers_.find(auction_id);
    if (sub_it != auction_subscribers_.end()) {
        sub_it->second.erase(conn);
        if (sub_it->second.empty()) {
            auction_subscribers_.erase(sub_it);
        }
    }
    auto user_it = connection_to_user_.find(conn);
    if (user_it != connection_to_user_.end()) {
        auto uid = user_it->second;
        auto conns_it = user_connections_.find(uid);
        if (conns_it != user_connections_.end()) {
            conns_it->second.erase(conn);
            if (conns_it->second.empty()) {
                user_connections_.erase(conns_it);
            }
        }
        connection_to_user_.erase(user_it);
    }
}

void AuctionWebSocketController::SetGateway(
    std::shared_ptr<DrogonAuctionEventGateway> gateway
) {
    std::lock_guard lock(gateway_mutex_);
    gateway_ = std::move(gateway);
}

void AuctionWebSocketController::SetTokenSecret(std::string secret) {
    std::lock_guard lock(gateway_mutex_);
    token_secret_ = std::move(secret);
}

void AuctionWebSocketController::handleNewConnection(
    const drogon::HttpRequestPtr& req,
    const drogon::WebSocketConnectionPtr& wsConnPtr
) {
    const auto auction_id_str = req->getParameter("id");
    if (auction_id_str.empty()) {
        wsConnPtr->shutdown();
        return;
    }

    std::uint64_t auction_id = 0;
    try {
        auction_id = std::stoull(auction_id_str);
    } catch (...) {
        wsConnPtr->shutdown();
        return;
    }

    if (auction_id == 0) {
        wsConnPtr->shutdown();
        return;
    }

    wsConnPtr->setContext(std::make_shared<std::uint64_t>(auction_id));

    {
        std::lock_guard lock(gateway_mutex_);
        if (gateway_) {
            gateway_->AddSubscriber(auction_id, wsConnPtr);
        }
    }

    common::logging::Logger::Instance().Info(
        "WebSocket connected for auction " + std::to_string(auction_id)
    );

    wsConnPtr->send(BuildWelcomeMessage(auction_id));
}

void AuctionWebSocketController::handleNewMessage(
    const drogon::WebSocketConnectionPtr& wsConnPtr,
    std::string&& message,
    const drogon::WebSocketMessageType& type
) {
    if (type != drogon::WebSocketMessageType::Text) {
        return;
    }

    Json::Value root;
    Json::CharReaderBuilder reader_builder;
    std::string errors;
    std::istringstream stream(message);

    if (!Json::parseFromStream(reader_builder, stream, &root, &errors)) {
        return;
    }

    const auto action = root.get("action", "").asString();

    if (action == "ping") {
        wsConnPtr->send(BuildPongMessage());
        return;
    }

    if (action == "identify") {
        const auto token = root.get("token", "").asString();
        if (token.empty()) {
            return;
        }

        std::string secret;
        {
            std::lock_guard lock(gateway_mutex_);
            secret = token_secret_;
        }

        if (secret.empty()) {
            common::logging::Logger::Instance().Warn(
                "WebSocket identify rejected: token secret not configured"
            );
            return;
        }

        try {
            modules::auth::TokenCodec codec(secret, 0);
            const auto payload = codec.ParseAndVerify(token);
            if (payload.user_id == 0) {
                return;
            }

            std::lock_guard lock(gateway_mutex_);
            if (gateway_) {
                gateway_->SetUserForConnection(wsConnPtr, payload.user_id);
            }

            common::logging::Logger::Instance().Info(
                "WebSocket identified as user " + std::to_string(payload.user_id)
            );
        } catch (const std::exception& exception) {
            common::logging::Logger::Instance().Warn(
                "WebSocket identify rejected: " + std::string(exception.what())
            );
        }
        return;
    }
}

void AuctionWebSocketController::handleConnectionClosed(
    const drogon::WebSocketConnectionPtr& wsConnPtr
) {
    const auto ctx = wsConnPtr->getContext<std::uint64_t>();
    if (!ctx) {
        return;
    }

    std::lock_guard lock(gateway_mutex_);
    if (gateway_) {
        gateway_->Disconnect(*ctx, wsConnPtr);
    }

    common::logging::Logger::Instance().Info(
        "WebSocket disconnected for auction " + std::to_string(*ctx)
    );
}

}  // namespace auction::access::http

#endif
