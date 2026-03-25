#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <memory>

namespace auction {

class WebSocketServer {
public:
    WebSocketServer();
    ~WebSocketServer();

    void start();
    void stop();

    void broadcast(const std::string& message);
    void sendToUser(int64_t user_id, const std::string& message);
    void subscribe(int64_t user_id, const std::vector<int64_t>& auction_item_ids);
    void unsubscribe(int64_t user_id, const std::vector<int64_t>& auction_item_ids);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace auction
