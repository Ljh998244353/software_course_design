#pragma once

#include <string>
#include <memory>

namespace spdlog {
    class logger;
}

namespace auction {

class Logger {
public:
    static void init();

    static void info(const char* fmt, ...);
    static void error(const char* fmt, ...);
    static void warn(const char* fmt, ...);
    static void debug(const char* fmt, ...);
};

#define AUCTION_LOG_INFO(...) auction::Logger::info(__VA_ARGS__)
#define AUCTION_LOG_ERROR(...) auction::Logger::error(__VA_ARGS__)
#define AUCTION_LOG_WARN(...) auction::Logger::warn(__VA_ARGS__)
#define AUCTION_LOG_DEBUG(...) auction::Logger::debug(__VA_ARGS__)

} // namespace auction
