cmake_minimum_required(VERSION 3.28)

project(auction_system VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(AUCTION_ENABLE_DROGON "Enable Drogon integration when Drogon is available." ON)

include(CTest)
include(GNUInstallDirs)

find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED IMPORTED_TARGET jsoncpp)

find_package(Drogon CONFIG QUIET)
if(AUCTION_ENABLE_DROGON AND Drogon_FOUND)
    set(AUCTION_HAS_DROGON 1)
    message(STATUS "Drogon found. HTTP server target will be enabled.")
else()
    set(AUCTION_HAS_DROGON 0)
    message(STATUS "Drogon not found. Building bootstrap mode only.")
endif()

set(AUCTION_CORE_SOURCES
    src/access/http/health_http.cpp
    src/application/health_service.cpp
    src/common/config/config_loader.cpp
    src/common/errors/error_code.cpp
    src/common/logging/logger.cpp
    src/common/runtime/application_bootstrap.cpp
)

add_library(auction_core STATIC ${AUCTION_CORE_SOURCES})
target_include_directories(auction_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(auction_core PUBLIC PkgConfig::JSONCPP)
target_compile_definitions(
    auction_core
    PUBLIC
        AUCTION_HAS_DROGON=${AUCTION_HAS_DROGON}
        AUCTION_PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}"
)

if(AUCTION_HAS_DROGON)
    target_link_libraries(auction_core PUBLIC Drogon::Drogon)
endif()

add_executable(auction_app src/main.cpp)
target_link_libraries(auction_app PRIVATE auction_core)

add_executable(auction_smoke_tests tests/unit/smoke_tests.cpp)
target_link_libraries(auction_smoke_tests PRIVATE auction_core)

if(BUILD_TESTING)
    add_test(NAME auction_smoke_tests COMMAND auction_smoke_tests)
    add_test(NAME auction_check_config COMMAND auction_app --check-config)
    set_tests_properties(
        auction_smoke_tests auction_check_config
        PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

