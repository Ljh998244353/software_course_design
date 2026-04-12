#include <cassert>
#include <filesystem>
#include <iostream>

#include "application/health_service.h"
#include "common/config/config_loader.h"
#include "common/errors/error_code.h"
#include "common/http/api_response.h"

int main() {
    namespace fs = std::filesystem;

    const fs::path project_root(AUCTION_PROJECT_ROOT);
    const fs::path config_path = auction::common::config::ConfigLoader::ResolveConfigPath(project_root);
    const auto config = auction::common::config::ConfigLoader::LoadFromFile(config_path);

    assert(config.server.port == 8080);
    assert(config.mysql.database == "auction_system");
    assert(config.ResolveUploadBasePath(project_root).string().find("data/uploads") != std::string::npos);

    const auto success = auction::common::http::ApiResponse::Success();
    assert(static_cast<int>(success.code) == 0);
    assert(success.message == "success");

    assert(
        auction::common::errors::ErrorCodeMessage(
            auction::common::errors::ErrorCode::kInternalError
        ) == "internal error"
    );

    const auto health = auction::application::BuildHealthResponse(config, project_root, config_path);
    assert(health.ToJsonValue()["data"]["service"].asString() == "auction_app");

    std::cout << "auction_smoke_tests passed\n";
    return 0;
}

