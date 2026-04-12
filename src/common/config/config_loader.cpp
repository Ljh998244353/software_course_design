#include "common/config/config_loader.h"

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <json/json.h>

namespace auction::common::config {

namespace {

std::filesystem::path ResolvePath(
    const std::filesystem::path& project_root,
    const std::string& configured_path
) {
    const std::filesystem::path path(configured_path);
    if (path.is_absolute()) {
        return path;
    }
    return project_root / path;
}

std::string ReadString(
    const Json::Value& root,
    const char* section,
    const char* key,
    const std::string& default_value
) {
    if (!root.isMember(section) || !root[section].isMember(key)) {
        return default_value;
    }
    return root[section][key].asString();
}

std::uint16_t ReadPort(
    const Json::Value& root,
    const char* section,
    const char* key,
    std::uint16_t default_value
) {
    if (!root.isMember(section) || !root[section].isMember(key)) {
        return default_value;
    }
    return static_cast<std::uint16_t>(root[section][key].asUInt());
}

int ReadInt(
    const Json::Value& root,
    const char* section,
    const char* key,
    int default_value
) {
    if (!root.isMember(section) || !root[section].isMember(key)) {
        return default_value;
    }
    return root[section][key].asInt();
}

unsigned int ReadUInt(
    const Json::Value& root,
    const char* section,
    const char* key,
    unsigned int default_value
) {
    if (!root.isMember(section) || !root[section].isMember(key)) {
        return default_value;
    }
    return root[section][key].asUInt();
}

}  // namespace

std::filesystem::path MysqlConfig::ResolveSocketPath(
    const std::filesystem::path& project_root
) const {
    if (socket.empty()) {
        return {};
    }
    return ResolvePath(project_root, socket);
}

std::filesystem::path AppConfig::ResolveLogPath(const std::filesystem::path& project_root) const {
    return ResolvePath(project_root, logging.path);
}

std::filesystem::path AppConfig::ResolveUploadBasePath(
    const std::filesystem::path& project_root
) const {
    return ResolvePath(project_root, storage.base_path);
}

std::filesystem::path ConfigLoader::ResolveConfigPath(
    const std::filesystem::path& project_root,
    const std::optional<std::filesystem::path>& explicit_path
) {
    if (explicit_path.has_value()) {
        return explicit_path->is_absolute() ? *explicit_path : project_root / *explicit_path;
    }

    if (const char* env_value = std::getenv("AUCTION_APP_CONFIG"); env_value != nullptr) {
        return std::filesystem::path(env_value);
    }

    const auto local_path = project_root / "config/app.local.json";
    if (std::filesystem::exists(local_path)) {
        return local_path;
    }

    return project_root / "config/app.example.json";
}

AppConfig ConfigLoader::LoadFromFile(const std::filesystem::path& config_path) {
    std::ifstream input(config_path);
    if (!input.is_open()) {
        throw std::runtime_error("failed to open config file: " + config_path.string());
    }

    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;
    Json::Value root;
    std::string errors;
    if (!Json::parseFromStream(builder, input, &root, &errors)) {
        throw std::runtime_error("failed to parse config file: " + errors);
    }

    AppConfig config;
    config.server.host = ReadString(root, "server", "host", config.server.host);
    config.server.port = ReadPort(root, "server", "port", config.server.port);

    config.mysql.host = ReadString(root, "mysql", "host", config.mysql.host);
    config.mysql.port = ReadPort(root, "mysql", "port", config.mysql.port);
    config.mysql.database = ReadString(root, "mysql", "database", config.mysql.database);
    config.mysql.username = ReadString(root, "mysql", "username", config.mysql.username);
    config.mysql.password = ReadString(root, "mysql", "password", config.mysql.password);
    config.mysql.socket = ReadString(root, "mysql", "socket", config.mysql.socket);
    config.mysql.charset = ReadString(root, "mysql", "charset", config.mysql.charset);
    config.mysql.connect_timeout_seconds = ReadUInt(
        root,
        "mysql",
        "connect_timeout_seconds",
        config.mysql.connect_timeout_seconds
    );

    config.redis.host = ReadString(root, "redis", "host", config.redis.host);
    config.redis.port = ReadPort(root, "redis", "port", config.redis.port);
    config.redis.db = ReadInt(root, "redis", "db", config.redis.db);

    config.storage.type = ReadString(root, "storage", "type", config.storage.type);
    config.storage.base_path = ReadString(root, "storage", "base_path", config.storage.base_path);

    config.auth.token_expire_minutes =
        ReadInt(root, "auth", "token_expire_minutes", config.auth.token_expire_minutes);
    config.auth.token_secret =
        ReadString(root, "auth", "token_secret", config.auth.token_secret);
    config.auth.password_hash_rounds = ReadUInt(
        root,
        "auth",
        "password_hash_rounds",
        config.auth.password_hash_rounds
    );

    config.logging.level = ReadString(root, "logging", "level", config.logging.level);
    config.logging.path = ReadString(root, "logging", "path", config.logging.path);

    return config;
}

}  // namespace auction::common::config
