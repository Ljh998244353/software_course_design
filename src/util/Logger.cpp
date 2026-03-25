#include "Logger.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <cstdarg>

namespace auction {

std::shared_ptr<spdlog::logger> g_logger;

void Logger::init() {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::info);
    console_sink->set_pattern("[%Y-%m-%d %H:%M:%S] [%^%l%$] [%n] %v");

    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        "logs/auction.log", 1024 * 1024 * 10, 5);
    file_sink->set_level(spdlog::level::debug);
    file_sink->set_pattern("[%Y-%m-%d %H:%M:%S] [%l] [%n] %v");

    spdlog::sinks_init_list sinks = {console_sink, file_sink};

    g_logger = std::make_shared<spdlog::logger>("auction", sinks);
    g_logger->set_level(spdlog::level::debug);
    g_logger->flush_on(spdlog::level::info);

    spdlog::register_logger(g_logger);
}

void Logger::info(const char* fmt, ...) {
    if (!g_logger) return;
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    g_logger->info(buffer);
}

void Logger::error(const char* fmt, ...) {
    if (!g_logger) return;
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    g_logger->error(buffer);
}

void Logger::warn(const char* fmt, ...) {
    if (!g_logger) return;
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    g_logger->warn(buffer);
}

void Logger::debug(const char* fmt, ...) {
    if (!g_logger) return;
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    g_logger->debug(buffer);
}

} // namespace auction
