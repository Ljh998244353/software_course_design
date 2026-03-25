#include "HttpServer.h"
#include "ConfigManager.h"
#include "Logger.h"
#include <drogon/HttpAppFramework.h>

namespace auction {

class HttpServer::Impl {
public:
    void start() {
        auto& app = drogon::app();

        app.registerHandler("/api/health",
            [](const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
                (void)req;
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setBody(R"({"status":"ok"})");
                callback(resp);
            }, {drogon::Get});

        app.run();
        Logger::info("HTTP Server started");
    }
};

HttpServer::HttpServer() : impl_(std::make_unique<Impl>()) {}
HttpServer::~HttpServer() = default;

void HttpServer::start() { impl_->start(); }
void HttpServer::stop() { drogon::app().quit(); }

} // namespace auction
