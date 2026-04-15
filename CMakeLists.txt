cmake_minimum_required(VERSION 3.28)

project(auction_system VERSION 0.1.0 LANGUAGES CXX)

if(POLICY CMP0153)
    cmake_policy(SET CMP0153 OLD)
endif()

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

# Ubuntu 24.04's packaged Drogon still ships a FindMySQL.cmake that only looks
# for mysqlclient_r/mariadbclient. Preseed the cache with the actual
# libmysqlclient paths so find_package(Drogon) can succeed with system packages.
if(NOT MYSQL_INCLUDE_DIRS OR MYSQL_INCLUDE_DIRS MATCHES "-NOTFOUND$")
    find_path(AUCTION_DROGON_MYSQL_INCLUDE_DIR NAMES mysql.h PATH_SUFFIXES mysql)
    if(AUCTION_DROGON_MYSQL_INCLUDE_DIR)
        set(
            MYSQL_INCLUDE_DIRS
            "${AUCTION_DROGON_MYSQL_INCLUDE_DIR}"
            CACHE PATH "MySQL include directory for Drogon dependency discovery" FORCE
        )
    endif()
endif()

if(NOT MYSQL_LIBRARIES OR MYSQL_LIBRARIES MATCHES "-NOTFOUND$")
    find_library(AUCTION_DROGON_MYSQL_LIBRARY NAMES mysqlclient mariadbclient mysqlclient_r)
    if(AUCTION_DROGON_MYSQL_LIBRARY)
        set(
            MYSQL_LIBRARIES
            "${AUCTION_DROGON_MYSQL_LIBRARY}"
            CACHE FILEPATH "MySQL client library for Drogon dependency discovery" FORCE
        )
    endif()
endif()

# Prefer the system yaml-cpp package so Drogon and the application resolve the
# same SONAME, even when Linuxbrew is installed in the user's PATH.
if(
    (
        NOT DEFINED yaml-cpp_DIR
        OR NOT yaml-cpp_DIR
        OR yaml-cpp_DIR MATCHES "/linuxbrew/"
        OR NOT EXISTS "${yaml-cpp_DIR}/yaml-cpp-config.cmake"
    )
    AND EXISTS "/usr/lib/x86_64-linux-gnu/cmake/yaml-cpp/yaml-cpp-config.cmake"
)
    set(
        yaml-cpp_DIR
        "/usr/lib/x86_64-linux-gnu/cmake/yaml-cpp"
        CACHE PATH "Preferred yaml-cpp config directory for Drogon" FORCE
    )
endif()

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
    src/jobs/auction_scheduler.cpp
    src/jobs/order_scheduler.cpp
    src/middleware/auth_middleware.cpp
    src/modules/audit/item_audit_service.cpp
    src/modules/auction/auction_service.cpp
    src/modules/auth/auth_service.cpp
    src/modules/auth/auth_session_store.cpp
    src/modules/bid/bid_cache_store.cpp
    src/modules/bid/bid_service.cpp
    src/modules/item/item_service.cpp
    src/modules/notification/notification_service.cpp
    src/modules/order/order_service.cpp
    src/modules/auth/password_hasher.cpp
    src/modules/payment/payment_signature.cpp
    src/modules/payment/payment_service.cpp
    src/modules/auth/token_codec.cpp
    src/repository/auction_repository.cpp
    src/repository/auth_user_repository.cpp
    src/repository/bid_repository.cpp
    src/repository/database_meta_repository.cpp
    src/repository/item_repository.cpp
    src/repository/notification_repository.cpp
    src/repository/order_repository.cpp
    src/repository/payment_repository.cpp
    src/ws/auction_event_gateway.cpp
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

add_executable(auction_item_tests tests/item/item_flow_tests.cpp)
target_link_libraries(auction_item_tests PRIVATE auction_core)

add_executable(auction_auction_tests tests/auction/auction_flow_tests.cpp)
target_link_libraries(auction_auction_tests PRIVATE auction_core)

add_executable(auction_bid_tests tests/bid/bid_flow_tests.cpp)
target_link_libraries(auction_bid_tests PRIVATE auction_core)

add_executable(auction_order_tests tests/order/order_flow_tests.cpp)
target_link_libraries(auction_order_tests PRIVATE auction_core)

add_executable(auction_payment_tests tests/payment/payment_flow_tests.cpp)
target_link_libraries(auction_payment_tests PRIVATE auction_core)

if(BUILD_TESTING)
    add_test(NAME auction_smoke_tests COMMAND auction_smoke_tests)
    add_test(NAME auction_check_config COMMAND auction_app --check-config)
    add_test(NAME auction_database_smoke COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_db.sh)
    add_test(NAME auction_auth_flow COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_auth.sh)
    add_test(NAME auction_item_flow COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_item.sh)
    add_test(NAME auction_auction_flow COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_auction.sh)
    add_test(NAME auction_bid_flow COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_bid.sh)
    add_test(NAME auction_order_flow COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_order.sh)
    add_test(NAME auction_payment_flow COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_payment.sh)
    set_tests_properties(
        auction_smoke_tests auction_check_config auction_database_smoke auction_auth_flow auction_item_flow auction_auction_flow auction_bid_flow auction_order_flow auction_payment_flow
        PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()
