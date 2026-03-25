#pragma once

#include <string>
#include <memory>

namespace auction {

class HttpServer {
public:
    HttpServer();
    ~HttpServer();

    void start();
    void stop();

private:
    void setupRoutes();

    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace auction
