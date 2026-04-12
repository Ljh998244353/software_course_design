#pragma once

#include <filesystem>
#include <optional>

#include "common/config/app_config.h"

namespace auction::common::config {

class ConfigLoader {
public:
    static std::filesystem::path ResolveConfigPath(
        const std::filesystem::path& project_root,
        const std::optional<std::filesystem::path>& explicit_path = std::nullopt
    );

    static AppConfig LoadFromFile(const std::filesystem::path& config_path);
};

}  // namespace auction::common::config

