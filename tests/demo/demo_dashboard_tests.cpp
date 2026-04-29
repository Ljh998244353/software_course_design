#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include "application/demo_dashboard_service.h"
#include "common/config/config_loader.h"

namespace {

const std::filesystem::path kProjectRoot(AUCTION_PROJECT_ROOT);

std::uint64_t ReadCount(const Json::Value& value, const char* field_name) {
    return std::stoull(value[field_name].asString());
}

}  // namespace

int main() {
    const auto config_path = auction::common::config::ConfigLoader::ResolveConfigPath(kProjectRoot);
    const auto config = auction::common::config::ConfigLoader::LoadFromFile(config_path);
    const auto response = auction::application::BuildDemoDashboardResponse(config, kProjectRoot);
    const auto payload = response.ToJsonValue();

    if (payload["code"].asInt() != 0) {
        std::cerr << payload["message"].asString() << '\n';
    }

    assert(payload["code"].asInt() == 0);
    assert(payload["data"]["view"].asString() == "demo_dashboard");
    assert(ReadCount(payload["data"]["summary"], "demoUserCount") >= 5);
    assert(ReadCount(payload["data"]["summary"], "demoItemCount") >= 4);
    assert(ReadCount(payload["data"]["summary"], "demoAuctionCount") >= 4);
    assert(ReadCount(payload["data"]["summary"], "demoBidCount") >= 4);
    assert(ReadCount(payload["data"]["summary"], "demoOrderCount") >= 1);
    assert(payload["data"]["accounts"].isArray());
    assert(payload["data"]["accounts"].size() >= 5);
    assert(payload["data"]["auctions"].isArray());
    assert(payload["data"]["auctions"].size() >= 4);
    assert(payload["data"]["orders"].isArray());
    assert(payload["data"]["orders"].size() >= 1);
    assert(payload["data"]["payments"].isArray());
    assert(payload["data"]["payments"].size() >= 1);
    assert(payload["data"]["notifications"].isArray());
    assert(payload["data"]["riskScenarios"].isArray());
    assert(payload["data"]["riskScenarios"].size() >= 5);

    return 0;
}
