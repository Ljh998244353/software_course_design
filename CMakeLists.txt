cmake_minimum_required(VERSION 3.20)
project(auction_system VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

option(ENABLE_TESTING "Enable unit tests" OFF)
option(ENABLE_DROGON "Enable Drogon framework" ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g)
endif()

include(FetchContent)
include_directories(${CMAKE_SOURCE_DIR}/include)

# Drogon framework
if(ENABLE_DROGON)
    FetchContent_Declare(
        drogon
        GIT_REPOSITORY https://github.com/an-tao/drogon.git
        GIT_TAG v1.9.2
    )
    FetchContent_MakeAvailable(drogon)
endif()

# JWT library
FetchContent_Declare(
    jwt_cpp
    GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp.git
    GIT_TAG v0.7.0
)
FetchContent_MakeAvailable(jwt_cpp)

# Redis client (hiredis)
FetchContent_Declare(
    hiredis
    GIT_REPOSITORY https://github.com/redis/hiredis.git
    GIT_TAG v1.2.0
)
FetchContent_MakeAvailable(hiredis)

# YAML parser - use system package
find_package(YAML-CPP REQUIRED)

# fmt for spdlog
find_package(fmt REQUIRED)

# MySQL connector - use system mariadb
find_library(MYSQL_LIBRARY NAMES mysqlclient)

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

set(SOURCES
    src/main.cpp
    src/config/ConfigManager.cpp
    src/network/HttpServer.cpp
    src/network/WebSocketServer.cpp
    src/engine/BidEngine.cpp
    src/engine/TimingWheel.cpp
    src/engine/AntiSnipe.cpp
    src/service/UserService.cpp
    src/service/ItemService.cpp
    src/service/AuctionService.cpp
    src/service/OrderService.cpp
    src/service/PaymentService.cpp
    src/db/MySQLPool.cpp
    src/db/RedisPool.cpp
    src/cache/DoubleBuffer.cpp
    src/model/User.cpp
    src/model/Item.cpp
    src/model/Bid.cpp
    src/model/Order.cpp
    src/util/Logger.cpp
    src/util/JwtHelper.cpp
)

add_executable(auction_server ${SOURCES})

target_link_libraries(auction_server
    drogon
    jwt-cpp::jwt-cpp
    hiredis
    yaml-cpp
    fmt::fmt
    ${MYSQL_LIBRARY}
    Threads::Threads
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Unit tests
if(ENABLE_TESTING)
    enable_testing()
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    FetchContent_MakeAvailable(googletest)

    add_executable(bid_engine_test tests/unit/BidEngineTest.cpp)
    target_link_libraries(bid_engine_test gtest_main drogon)
    add_test(NAME BidEngineTest COMMAND bid_engine_test)

    add_executable(timing_wheel_test tests/unit/TimingWheelTest.cpp)
    target_link_libraries(timing_wheel_test gtest_main)
    add_test(NAME TimingWheelTest COMMAND timing_wheel_test)
endif()
