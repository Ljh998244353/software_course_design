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
    src/access/http/demo_http.cpp
    src/access/http/health_http.cpp
    src/application/database_health_service.cpp
    src/application/demo_dashboard_service.cpp
    src/application/health_service.cpp
    src/common/config/config_loader.cpp
    src/common/db/mysql_connection.cpp
    src/common/errors/error_code.cpp
    src/common/logging/logger.cpp
    src/common/runtime/application_bootstrap.cpp
    src/jobs/auction_scheduler.cpp
    src/jobs/order_scheduler.cpp
    src/jobs/statistics_scheduler.cpp
    src/middleware/auth_middleware.cpp
    src/modules/audit/item_audit_service.cpp
    src/modules/auction/auction_service.cpp
    src/modules/auth/auth_service.cpp
    src/modules/auth/auth_session_store.cpp
    src/modules/bid/bid_cache_store.cpp
    src/modules/bid/bid_service.cpp
    src/modules/item/item_service.cpp
    src/modules/notification/notification_service.cpp
    src/modules/ops/ops_service.cpp
    src/modules/order/order_service.cpp
    src/modules/auth/password_hasher.cpp
    src/modules/payment/payment_signature.cpp
    src/modules/payment/payment_service.cpp
    src/modules/review/review_service.cpp
    src/modules/statistics/statistics_service.cpp
    src/modules/auth/token_codec.cpp
    src/repository/auction_repository.cpp
    src/repository/auth_user_repository.cpp
    src/repository/bid_repository.cpp
    src/repository/database_meta_repository.cpp
    src/repository/item_repository.cpp
    src/repository/notification_repository.cpp
    src/repository/ops_repository.cpp
    src/repository/order_repository.cpp
    src/repository/payment_repository.cpp
    src/repository/review_repository.cpp
    src/repository/statistics_repository.cpp
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

add_executable(auction_integration_tests tests/integration/full_flow_tests.cpp)
target_link_libraries(auction_integration_tests PRIVATE auction_core)

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

add_executable(auction_review_tests tests/review/review_flow_tests.cpp)
target_link_libraries(auction_review_tests PRIVATE auction_core)

add_executable(auction_statistics_tests tests/statistics/statistics_flow_tests.cpp)
target_link_libraries(auction_statistics_tests PRIVATE auction_core)

add_executable(auction_ops_tests tests/ops/ops_flow_tests.cpp)
target_link_libraries(auction_ops_tests PRIVATE auction_core)

add_executable(auction_high_risk_tests tests/stress/high_risk_flow_tests.cpp)
target_link_libraries(auction_high_risk_tests PRIVATE auction_core)

add_executable(auction_security_negative_tests tests/security/security_negative_tests.cpp)
target_link_libraries(auction_security_negative_tests PRIVATE auction_core)

add_executable(auction_demo_dashboard_tests tests/demo/demo_dashboard_tests.cpp)
target_link_libraries(auction_demo_dashboard_tests PRIVATE auction_core)

if(BUILD_TESTING)
    add_test(NAME auction_smoke_tests COMMAND auction_smoke_tests)
    add_test(NAME auction_check_config COMMAND auction_app --check-config)
    add_test(NAME auction_database_smoke COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_db.sh)
    add_test(NAME auction_integration_flow COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_integration.sh)
    add_test(NAME auction_auth_flow COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_auth.sh)
    add_test(NAME auction_item_flow COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_item.sh)
    add_test(NAME auction_auction_flow COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_auction.sh)
    add_test(NAME auction_bid_flow COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_bid.sh)
    add_test(NAME auction_order_flow COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_order.sh)
    add_test(NAME auction_payment_flow COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_payment.sh)
    add_test(NAME auction_review_flow COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_review.sh)
    add_test(NAME auction_statistics_flow COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_statistics.sh)
    add_test(NAME auction_ops_flow COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_ops.sh)
    add_test(NAME auction_high_risk_flow COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_high_risk.sh)
    add_test(NAME auction_security_negative COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_security.sh)
    add_test(NAME auction_demo_dashboard COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_demo_frontend.sh)

    set_tests_properties(
        auction_smoke_tests auction_check_config
        PROPERTIES
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    set_tests_properties(
        auction_database_smoke
        auction_integration_flow
        auction_auth_flow
        auction_item_flow
        auction_auction_flow
        auction_bid_flow
        auction_order_flow
        auction_payment_flow
        auction_review_flow
        auction_statistics_flow
        auction_ops_flow
        auction_high_risk_flow
        auction_security_negative
        auction_demo_dashboard
        PROPERTIES
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            RESOURCE_LOCK auction_mysql_test_runtime
    )

    set_tests_properties(auction_smoke_tests PROPERTIES LABELS "suite_unit;suite_smoke")
    set_tests_properties(auction_check_config PROPERTIES LABELS "suite_unit;suite_smoke;suite_config")
    set_tests_properties(
        auction_database_smoke
        PROPERTIES LABELS "suite_db;suite_integration;requires_mysql"
    )
    set_tests_properties(
        auction_integration_flow
        PROPERTIES LABELS "suite_integration;suite_contract;requires_mysql"
    )
    set_tests_properties(
        auction_auth_flow
        PROPERTIES LABELS "suite_module;module_auth;requires_mysql"
    )
    set_tests_properties(
        auction_item_flow
        PROPERTIES LABELS "suite_module;module_item;requires_mysql"
    )
    set_tests_properties(
        auction_auction_flow
        PROPERTIES LABELS "suite_module;module_auction;requires_mysql"
    )
    set_tests_properties(
        auction_bid_flow
        PROPERTIES LABELS "suite_module;module_bid;requires_mysql"
    )
    set_tests_properties(
        auction_order_flow
        PROPERTIES LABELS "suite_module;module_order;requires_mysql"
    )
    set_tests_properties(
        auction_payment_flow
        PROPERTIES LABELS "suite_module;module_payment;requires_mysql"
    )
    set_tests_properties(
        auction_review_flow
        PROPERTIES LABELS "suite_module;module_review;requires_mysql"
    )
    set_tests_properties(
        auction_statistics_flow
        PROPERTIES LABELS "suite_module;module_statistics;requires_mysql"
    )
    set_tests_properties(
        auction_ops_flow
        PROPERTIES LABELS "suite_module;module_ops;requires_mysql"
    )
    set_tests_properties(
        auction_high_risk_flow
        PROPERTIES LABELS "suite_stress;suite_risk;module_bid;module_auction;module_order;module_payment;module_notification;requires_mysql"
    )
    set_tests_properties(
        auction_security_negative
        PROPERTIES LABELS "suite_security;suite_risk;module_auth;module_bid;module_auction;module_payment;requires_mysql"
    )
    set_tests_properties(
        auction_demo_dashboard
        PROPERTIES LABELS "suite_demo;module_frontend;requires_mysql"
    )
endif()
