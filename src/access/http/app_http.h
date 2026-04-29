#pragma once

#include <filesystem>

namespace auction::access::http {

void RegisterAppHttpRoutes(const std::filesystem::path& project_root);

}  // namespace auction::access::http
