#include "application/health_service.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace auction::application {

namespace {

std::string ToIsoTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm{};
    localtime_r(&time, &local_tm);

    std::ostringstream output;
    output << std::put_time(&local_tm, "%Y-%m-%dT%H:%M:%S%z");
    return output.str();
}

}  // namespace

common::http::ApiResponse BuildHealthResponse(
    const common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::filesystem::path& config_path
) {
    Json::Value payload(Json::objectValue);
    payload["service"] = "auction_app";
    payload["status"] = "ok";
    payload["mode"] = AUCTION_HAS_DROGON ? "http" : "bootstrap";
    payload["drogonEnabled"] = static_cast<bool>(AUCTION_HAS_DROGON);
    payload["server"]["host"] = config.server.host;
    payload["server"]["port"] = config.server.port;
    payload["mysql"]["database"] = config.mysql.database;
    payload["mysql"]["charset"] = config.mysql.charset;
    payload["mysql"]["endpoint"] = config.mysql.socket.empty()
        ? config.mysql.host + ":" + std::to_string(config.mysql.port)
        : config.mysql.ResolveSocketPath(project_root).string();
    payload["configPath"] = config_path.string();
    payload["projectRoot"] = project_root.string();
    payload["uploadBasePath"] = config.ResolveUploadBasePath(project_root).string();
    payload["logPath"] = config.ResolveLogPath(project_root).string();
    payload["serverTime"] = ToIsoTimestamp();
    return common::http::ApiResponse::Success(payload);
}

}  // namespace auction::application
