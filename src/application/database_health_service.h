#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

#include "common/config/app_config.h"
#include "common/http/api_response.h"

namespace auction::application {

struct DatabaseHealthReport {
    std::string database;
    std::string charset;
    std::string endpoint;
    std::string server_version;
    std::uint64_t table_count{0};
    std::uint64_t required_table_count{0};
    std::uint64_t expected_table_count{0};
    std::uint64_t admin_user_count{0};
    std::uint64_t category_count{0};
    bool schema_ready{false};
    bool seed_ready{false};
};

DatabaseHealthReport CollectDatabaseHealth(
    const common::config::AppConfig& config,
    const std::filesystem::path& project_root
);

common::http::ApiResponse BuildDatabaseCheckResponse(
    const common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::filesystem::path& config_path
);

}  // namespace auction::application
