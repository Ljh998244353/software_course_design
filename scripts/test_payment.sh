#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/db/mysql_test_env.sh"
CONNECTION_INFO="$("${ROOT_DIR}/scripts/db/setup_local_mysql.sh")"
mysql_test_env::load "${CONNECTION_INFO}"
trap 'mysql_test_env::shutdown' EXIT
TEST_CONFIG_PATH="$("${ROOT_DIR}/scripts/db/write_test_config.sh")"

if [[ ! -x "${ROOT_DIR}/build/bin/auction_payment_tests" ]]; then
    echo "build/bin/auction_payment_tests not found, run scripts/bootstrap.sh first" >&2
    exit 1
fi

AUCTION_APP_CONFIG="${TEST_CONFIG_PATH}" "${ROOT_DIR}/build/bin/auction_payment_tests"
