#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace auction::common::config {

struct ServerConfig {
    std::string host{"0.0.0.0"};
    std::uint16_t port{8080};
};

struct MysqlConfig {
    std::string host{"127.0.0.1"};
    std::uint16_t port{3306};
    std::string database{"auction_system"};
    std::string username{"auction_user"};
    std::string password{"change_me"};
};

struct RedisConfig {
    std::string host{"127.0.0.1"};
    std::uint16_t port{6379};
    int db{0};
};

struct StorageConfig {
    std::string type{"local"};
    std::string base_path{"./data/uploads"};
};

struct AuthConfig {
    int token_expire_minutes{120};
};

struct LoggingConfig {
    std::string level{"info"};
    std::string path{"./logs/app.log"};
};

struct AppConfig {
    ServerConfig server{};
    MysqlConfig mysql{};
    RedisConfig redis{};
    StorageConfig storage{};
    AuthConfig auth{};
    LoggingConfig logging{};

    [[nodiscard]] std::filesystem::path ResolveLogPath(
        const std::filesystem::path& project_root
    ) const;

    [[nodiscard]] std::filesystem::path ResolveUploadBasePath(
        const std::filesystem::path& project_root
    ) const;
};

}  // namespace auction::common::config

