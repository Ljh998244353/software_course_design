#include "common/logging/logger.h"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace auction::common::logging {

namespace {

std::string CurrentTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm{};
    localtime_r(&time, &local_tm);

    std::ostringstream output;
    output << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
    return output.str();
}

}  // namespace

Logger& Logger::Instance() {
    static Logger instance;
    return instance;
}

void Logger::Init(const std::filesystem::path& log_path, const std::string& level_name) {
    std::scoped_lock lock(mutex_);
    level_ = ParseLevel(level_name);
    log_path_ = log_path;

    if (log_path.has_parent_path()) {
        std::filesystem::create_directories(log_path.parent_path());
    }

    std::ofstream output(log_path, std::ios::app);
    if (!output.is_open()) {
        throw std::runtime_error("failed to open log file: " + log_path.string());
    }
}

void Logger::Log(const LogLevel level, const std::string& message) {
    if (!ShouldLog(level)) {
        return;
    }

    const std::string line = MakeLine(level, message);
    std::scoped_lock lock(mutex_);
    std::cout << line << '\n';

    if (log_path_.has_value()) {
        std::ofstream output(*log_path_, std::ios::app);
        if (output.is_open()) {
            output << line << '\n';
        }
    }
}

void Logger::Debug(const std::string& message) {
    Log(LogLevel::kDebug, message);
}

void Logger::Info(const std::string& message) {
    Log(LogLevel::kInfo, message);
}

void Logger::Warn(const std::string& message) {
    Log(LogLevel::kWarn, message);
}

void Logger::Error(const std::string& message) {
    Log(LogLevel::kError, message);
}

bool Logger::ShouldLog(const LogLevel level) const {
    return static_cast<int>(level) >= static_cast<int>(level_);
}

std::string Logger::MakeLine(const LogLevel level, const std::string& message) const {
    std::ostringstream output;
    output << '[' << CurrentTimestamp() << "] [" << ToUpperName(level) << "] " << message;
    return output.str();
}

LogLevel Logger::ParseLevel(const std::string& level_name) {
    if (level_name == "debug") {
        return LogLevel::kDebug;
    }
    if (level_name == "warn") {
        return LogLevel::kWarn;
    }
    if (level_name == "error") {
        return LogLevel::kError;
    }
    return LogLevel::kInfo;
}

std::string Logger::ToUpperName(const LogLevel level) {
    switch (level) {
        case LogLevel::kDebug:
            return "DEBUG";
        case LogLevel::kInfo:
            return "INFO";
        case LogLevel::kWarn:
            return "WARN";
        case LogLevel::kError:
            return "ERROR";
    }
    return "INFO";
}

}  // namespace auction::common::logging

