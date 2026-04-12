#pragma once

#include <filesystem>

#include "common/config/app_config.h"

namespace auction::access::http {

void RegisterHealthHttpRoutes(
    const common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::filesystem::path& config_path
);

}  // namespace auction::access::http

