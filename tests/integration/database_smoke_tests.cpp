#include <cassert>
#include <filesystem>
#include <iostream>

#include "application/database_health_service.h"
#include "common/config/config_loader.h"

int main() {
    namespace fs = std::filesystem;

    const fs::path project_root(AUCTION_PROJECT_ROOT);
    const fs::path config_path = auction::common::config::ConfigLoader::ResolveConfigPath(project_root);
    const auto config = auction::common::config::ConfigLoader::LoadFromFile(config_path);

    const auto report = auction::application::CollectDatabaseHealth(config, project_root);

    assert(report.schema_ready);
    assert(report.seed_ready);
    assert(report.required_table_count == report.expected_table_count);
    assert(report.admin_user_count >= 1);
    assert(report.category_count >= 4);

    std::cout << "auction_database_smoke_tests passed\n";
    return 0;
}
