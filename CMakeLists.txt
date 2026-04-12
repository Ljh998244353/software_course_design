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
pkg_check_modules(MYSQLCLIENT REQUIRED IMPORTED_TARGET mysqlclient)
find_package(OpenSSL REQUIRED)
find_library(CRYPT_LIBRARY crypt REQUIRED)

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
    src/application/database_health_service.cpp
    src/application/health_service.cpp
    src/common/config/config_loader.cpp
    src/common/db/mysql_connection.cpp
    src/common/errors/error_code.cpp
    src/common/logging/logger.cpp
    src/common/runtime/application_bootstrap.cpp
    src/middleware/auth_middleware.cpp
    src/modules/auth/auth_service.cpp
    src/modules/auth/auth_session_store.cpp
    src/modules/auth/password_hasher.cpp
    src/modules/auth/token_codec.cpp
    src/repository/auth_user_repository.cpp
    src/repository/database_meta_repository.cpp
)

add_library(auction_core STATIC ${AUCTION_CORE_SOURCES})
target_include_directories(auction_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(
    auction_core
    PUBLIC
        PkgConfig::JSONCPP
        PkgConfig::MYSQLCLIENT
        OpenSSL::Crypto
        ${CRYPT_LIBRARY}
)
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

add_executable(auction_database_smoke_tests tests/integration/database_smoke_tests.cpp)
target_link_libraries(auction_database_smoke_tests PRIVATE auction_core)

add_executable(auction_auth_tests tests/auth/auth_flow_tests.cpp)
target_link_libraries(auction_auth_tests PRIVATE auction_core)

if(BUILD_TESTING)
    add_test(NAME auction_smoke_tests COMMAND auction_smoke_tests)
    add_test(NAME auction_check_config COMMAND auction_app --check-config)
    add_test(NAME auction_database_smoke COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_db.sh)
    add_test(NAME auction_auth_flow COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_auth.sh)
    set_tests_properties(
        auction_smoke_tests auction_check_config auction_database_smoke auction_auth_flow
        PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()
