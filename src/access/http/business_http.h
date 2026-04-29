#pragma once

#include <filesystem>

#include "common/config/app_config.h"

namespace auction::access::http {

void RegisterBusinessHttpRoutes(
    const common::config::AppConfig& config,
    const std::filesystem::path& project_root
);

}  // namespace auction::access::http
