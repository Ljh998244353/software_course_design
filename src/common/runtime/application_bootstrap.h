#pragma once

namespace auction::common::runtime {

struct BootstrapOptions {
    bool check_config_only{false};
};

class ApplicationBootstrap {
public:
    int Run(const BootstrapOptions& options) const;
};

}  // namespace auction::common::runtime

