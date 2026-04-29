#pragma once

#include <filesystem>

#include "common/config/app_config.h"
#include "common/http/api_response.h"

namespace auction::application {

common::http::ApiResponse BuildDemoDashboardResponse(
    const common::config::AppConfig& config,
    const std::filesystem::path& project_root
);

}  // namespace auction::application
