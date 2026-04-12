#include <exception>
#include <iostream>
#include <string_view>

#include "common/runtime/application_bootstrap.h"

namespace {

auction::common::runtime::BootstrapOptions ParseArgs(int argc, char* argv[]) {
    auction::common::runtime::BootstrapOptions options;
    for (int index = 1; index < argc; ++index) {
        const std::string_view arg(argv[index]);
        if (arg == "--check-config") {
            options.check_config_only = true;
        }
    }
    return options;
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        auction::common::runtime::ApplicationBootstrap bootstrap;
        return bootstrap.Run(ParseArgs(argc, argv));
    } catch (const std::exception& exception) {
        std::cerr << "auction_app bootstrap failed: " << exception.what() << '\n';
        return 1;
    }
}

