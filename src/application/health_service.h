#pragma once

#include <filesystem>

#include "common/config/app_config.h"
#include "common/http/api_response.h"

namespace auction::application {

common::http::ApiResponse BuildHealthResponse(
    const common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::filesystem::path& config_path
);

}  // namespace auction::application
