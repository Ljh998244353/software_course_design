#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SOCKET_FILE="$("${ROOT_DIR}/scripts/db/setup_local_mysql.sh")"
TEST_CONFIG_PATH="$("${ROOT_DIR}/scripts/db/write_test_config.sh" "${SOCKET_FILE}")"

if [[ ! -x "${ROOT_DIR}/build/bin/auction_auction_tests" ]]; then
    echo "build/bin/auction_auction_tests not found, run scripts/bootstrap.sh first" >&2
    exit 1
fi

AUCTION_APP_CONFIG="${TEST_CONFIG_PATH}" "${ROOT_DIR}/build/bin/auction_auction_tests"
