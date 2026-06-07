#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
source "${ROOT_DIR}/scripts/db/mysql_test_env.sh"
TEST_SERVER_HOST="${AUCTION_RELEASE_SERVER_HOST:-127.0.0.1}"
TEST_SERVER_PORT="${AUCTION_RELEASE_SERVER_PORT:-18080}"
SERVER_LOG="${ROOT_DIR}/build/release_verify_server.log"
SERVER_PID=""

cleanup() {
    if [[ -n "${SERVER_PID}" ]]; then
        kill "${SERVER_PID}" >/dev/null 2>&1 || true
        wait "${SERVER_PID}" >/dev/null 2>&1 || true
    fi
    mysql_test_env::shutdown
}

wait_for_server() {
    for _ in {1..20}; do
        if ! kill -0 "${SERVER_PID}" >/dev/null 2>&1; then
            echo "server exited before readiness" >&2
            if [[ -f "${SERVER_LOG}" ]]; then
                tail -n 120 "${SERVER_LOG}" >&2
            fi
            return 1
        fi
        if curl -fsS "http://${TEST_SERVER_HOST}:${TEST_SERVER_PORT}/healthz" >/dev/null 2>&1; then
            return 0
        fi
        sleep 1
    done

    echo "server did not become ready" >&2
    if [[ -f "${SERVER_LOG}" ]]; then
        tail -n 120 "${SERVER_LOG}" >&2
    fi
    return 1
}

expect_http_200() {
    local url="$1"
    local output_path="$2"
    local status
    status="$(curl -sS -o "${output_path}" -w "%{http_code}" "${url}")"
    if [[ "${status}" != "200" ]]; then
        echo "unexpected HTTP ${status}: ${url}" >&2
        return 1
    fi
}

verify_server_entries() {
    : >"${SERVER_LOG}"
    local connection_info
    connection_info="$("${ROOT_DIR}/scripts/db/setup_local_mysql.sh")"
    mysql_test_env::load "${connection_info}"

    local test_config_path
    test_config_path="$(
        AUCTION_TEST_SERVER_HOST="${TEST_SERVER_HOST}" \
        AUCTION_TEST_SERVER_PORT="${TEST_SERVER_PORT}" \
        "${ROOT_DIR}/scripts/db/write_test_config.sh"
    )"

    AUCTION_APP_CONFIG="${test_config_path}" "${ROOT_DIR}/build/bin/auction_app" \
        >"${SERVER_LOG}" 2>&1 &
    SERVER_PID=$!

    wait_for_server

    expect_http_200 \
        "http://${TEST_SERVER_HOST}:${TEST_SERVER_PORT}/healthz" \
        "${ROOT_DIR}/build/verify_healthz.json"
    expect_http_200 \
        "http://${TEST_SERVER_HOST}:${TEST_SERVER_PORT}/api/auctions" \
        "${ROOT_DIR}/build/verify_auctions.json"
    grep -q '"code"' "${ROOT_DIR}/build/verify_auctions.json"

    cleanup
    SERVER_PID=""
}

trap cleanup EXIT

cd "${ROOT_DIR}/frontend"
npm run release-check
cd "${ROOT_DIR}"

verify_server_entries
"${ROOT_DIR}/scripts/test.sh" frontend
"${ROOT_DIR}/scripts/test.sh" http
"${ROOT_DIR}/scripts/test.sh" risk
ctest --test-dir "${ROOT_DIR}/build" --output-on-failure

echo "release verification passed"
