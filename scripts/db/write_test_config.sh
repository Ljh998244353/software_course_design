#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TEST_CONFIG_DIR="${ROOT_DIR}/build/test_config"
TEST_CONFIG_PATH="${TEST_CONFIG_DIR}/app.mysql.test.json"
TEST_DB_NAME="${AUCTION_TEST_DB_NAME:-auction_system}"
TEST_SERVER_HOST="${AUCTION_TEST_SERVER_HOST:-0.0.0.0}"
TEST_SERVER_PORT="${AUCTION_TEST_SERVER_PORT:-8080}"
TEST_MYSQL_MODE="${AUCTION_TEST_MYSQL_MODE:-socket}"
TEST_MYSQL_HOST="${AUCTION_TEST_MYSQL_HOST:-127.0.0.1}"
TEST_MYSQL_PORT="${AUCTION_TEST_MYSQL_PORT:-3406}"
TEST_MYSQL_SOCKET="${AUCTION_TEST_MYSQL_SOCKET:-}"

mkdir -p "${TEST_CONFIG_DIR}"

cat > "${TEST_CONFIG_PATH}" <<JSON
{
  "server": {
    "host": "${TEST_SERVER_HOST}",
    "port": ${TEST_SERVER_PORT}
  },
  "mysql": {
    "host": "${TEST_MYSQL_HOST}",
    "port": ${TEST_MYSQL_PORT},
    "database": "${TEST_DB_NAME}",
    "username": "auction_user",
    "password": "change_me",
    "socket": "$(if [[ "${TEST_MYSQL_MODE}" == "socket" ]]; then printf '%s' "${TEST_MYSQL_SOCKET}"; fi)",
    "charset": "utf8mb4",
    "connect_timeout_seconds": 3
  },
  "redis": {
    "host": "127.0.0.1",
    "port": 6379,
    "db": 0
  },
  "storage": {
    "type": "local",
    "base_path": "./data/uploads"
  },
  "auth": {
    "token_expire_minutes": 120,
    "token_secret": "change_me_auth_secret",
    "password_hash_rounds": 100000
  },
  "logging": {
    "level": "info",
    "path": "./logs/app.log"
  }
}
JSON

printf '%s\n' "${TEST_CONFIG_PATH}"
