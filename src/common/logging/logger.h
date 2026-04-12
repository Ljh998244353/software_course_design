#pragma once

#include <filesystem>
#include <mutex>
#include <optional>
#include <string>

namespace auction::common::logging {

enum class LogLevel {
    kDebug,
    kInfo,
    kWarn,
    kError,
};

class Logger {
public:
    static Logger& Instance();

    void Init(const std::filesystem::path& log_path, const std::string& level_name);
    void Log(LogLevel level, const std::string& message);
    void Debug(const std::string& message);
    void Info(const std::string& message);
    void Warn(const std::string& message);
    void Error(const std::string& message);

private:
    Logger() = default;

    [[nodiscard]] bool ShouldLog(LogLevel level) const;
    [[nodiscard]] std::string MakeLine(LogLevel level, const std::string& message) const;
    [[nodiscard]] static LogLevel ParseLevel(const std::string& level_name);
    [[nodiscard]] static std::string ToUpperName(LogLevel level);

    LogLevel level_{LogLevel::kInfo};
    std::optional<std::filesystem::path> log_path_;
    mutable std::mutex mutex_;
};

}  // namespace auction::common::logging

