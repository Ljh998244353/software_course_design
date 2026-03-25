#pragma once

#include <string>
#include <yaml-cpp/yaml.h>

namespace auction {

struct ServerConfig {
    std::string host;
    int port;
    int worker_threads;
};

struct DatabaseConfig {
    std::string host;
    int port;
    std::string username;
    std::string password;
    std::string database;
    int pool_size;
};

struct RedisConfig {
    std::string host;
    int port;
    int pool_size;
};

struct JwtConfig {
    std::string secret;
    int expiry_hours;
};

struct AuctionConfig {
    int anti_snipe_N;
    int anti_snipe_M;
    int max_extensions;
    int bid_timeout_seconds;
};

class ConfigManager {
public:
    static ConfigManager& instance();

    bool load(const std::string& path);

    const ServerConfig& server() const { return server_; }
    const DatabaseConfig& database() const { return database_; }
    const RedisConfig& redis() const { return redis_; }
    const JwtConfig& jwt() const { return jwt_; }
    const AuctionConfig& auction() const { return auction_; }

private:
    ConfigManager() = default;

    ServerConfig server_;
    DatabaseConfig database_;
    RedisConfig redis_;
    JwtConfig jwt_;
    AuctionConfig auction_;
};

} // namespace auction
