#include <drogon/drogon.h>
#include "ConfigManager.h"
#include "Logger.h"
#include "HttpServer.h"
#include "WebSocketServer.h"
#include "BidEngine.h"

using namespace auction;

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    Logger::init();
    AUCTION_LOG_INFO("Auction Server starting...");

    auto& config = ConfigManager::instance();
    if (!config.load("config/server.yaml")) {
        AUCTION_LOG_ERROR("Failed to load configuration");
        return 1;
    }

    BidEngine::instance().start();

    HttpServer httpServer;
    httpServer.start();

    WebSocketServer wsServer;
    wsServer.start();

    AUCTION_LOG_INFO("Auction Server started");

    trantor::EventLoop loop;
    loop.loop();

    return 0;
}
