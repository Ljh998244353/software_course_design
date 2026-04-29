#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEMO_DB_NAME="${AUCTION_DEMO_DB_NAME:-auction_demo}"
DEMO_DB_USER="${AUCTION_DEMO_DB_USER:-auction_user}"
DEMO_DB_PASSWORD="${AUCTION_DEMO_DB_PASSWORD:-change_me}"

"${ROOT_DIR}/scripts/bootstrap.sh"

SOCKET_FILE="$(
    AUCTION_DB_NAME="${DEMO_DB_NAME}" \
    AUCTION_DB_USER="${DEMO_DB_USER}" \
    AUCTION_DB_PASSWORD="${DEMO_DB_PASSWORD}" \
    "${ROOT_DIR}/scripts/db/setup_local_mysql.sh"
)"

mysql --socket="${SOCKET_FILE}" -uroot "${DEMO_DB_NAME}" <"${ROOT_DIR}/sql/demo_data.sql"

TEST_CONFIG_PATH="$(
    AUCTION_TEST_DB_NAME="${DEMO_DB_NAME}" \
    AUCTION_TEST_SERVER_HOST="127.0.0.1" \
    AUCTION_TEST_SERVER_PORT="18080" \
    "${ROOT_DIR}/scripts/db/write_test_config.sh" "${SOCKET_FILE}"
)"

AUCTION_APP_CONFIG="${TEST_CONFIG_PATH}" "${ROOT_DIR}/build/bin/auction_demo_dashboard_tests"
