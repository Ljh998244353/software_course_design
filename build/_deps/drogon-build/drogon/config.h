#pragma once

#define USE_POSTGRESQL 0
#define LIBPQ_SUPPORTS_BATCH_MODE 0
#define USE_MYSQL 0
#define USE_SQLITE3 0
#define HAS_STD_FILESYSTEM_PATH 1
/* #undef OpenSSL_FOUND */
/* #undef Boost_FOUND */

#define COMPILATION_FLAGS "-g -std=c++20"
#define COMPILER_COMMAND "clang++-20"
#define COMPILER_ID "Clang"

#define INCLUDING_DIRS " -I/usr/include/jsoncpp -I/usr/local/include"
