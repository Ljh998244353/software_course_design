#include "ConfigManager.h"
#include <fstream>

namespace auction {

ConfigManager& ConfigManager::instance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::load(const std::string& path) {
    try {
        YAML::Node config = YAML::LoadFile(path);

        server_.host = config["server"]["host"].as<std::string>("0.0.0.0");
        server_.port = config["server"]["port"].as<int>(8080);
        server_.worker_threads = config["server"]["worker_threads"].as<int>(4);

        database_.host = config["database"]["host"].as<std::string>("localhost");
        database_.port = config["database"]["port"].as<int>(3306);
        database_.username = config["database"]["username"].as<std::string>("root");
        database_.password = config["database"]["password"].as<std::string>("");
        database_.database = config["database"]["database"].as<std::string>("auction_db");
        database_.pool_size = config["database"]["pool_size"].as<int>(16);

        redis_.host = config["redis"]["host"].as<std::string>("localhost");
        redis_.port = config["redis"]["port"].as<int>(6379);
        redis_.pool_size = config["redis"]["pool_size"].as<int>(32);

        jwt_.secret = config["jwt"]["secret"].as<std::string>("secret");
        jwt_.expiry_hours = config["jwt"]["expiry_hours"].as<int>(24);

        auction_.anti_snipe_N = config["auction"]["anti_snipe_N"].as<int>(60);
        auction_.anti_snipe_M = config["auction"]["anti_snipe_M"].as<int>(120);
        auction_.max_extensions = config["auction"]["max_extensions"].as<int>(10);
        auction_.bid_timeout_seconds = config["auction"]["bid_timeout_seconds"].as<int>(30);

        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

} // namespace auction
