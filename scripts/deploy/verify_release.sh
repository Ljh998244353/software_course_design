#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
DEMO_CONFIG_PATH="${ROOT_DIR}/build/demo_config/app.demo.json"
DEMO_SERVER_HOST="${AUCTION_DEMO_SERVER_HOST:-127.0.0.1}"
DEMO_SERVER_PORT="${AUCTION_DEMO_SERVER_PORT:-18080}"
SERVER_LOG="${ROOT_DIR}/build/demo_config/verify_release_server.log"
SERVER_PID=""

cleanup() {
    if [[ -n "${SERVER_PID}" ]]; then
        kill "${SERVER_PID}" >/dev/null 2>&1 || true
        wait "${SERVER_PID}" >/dev/null 2>&1 || true
    fi
}

wait_for_demo_server() {
    for _ in {1..20}; do
        if ! kill -0 "${SERVER_PID}" >/dev/null 2>&1; then
            echo "demo server exited before readiness" >&2
            if [[ -f "${SERVER_LOG}" ]]; then
                tail -n 120 "${SERVER_LOG}" >&2
            fi
            return 1
        fi
        if curl -fsS "http://${DEMO_SERVER_HOST}:${DEMO_SERVER_PORT}/healthz" >/dev/null 2>&1; then
            return 0
        fi
        sleep 1
    done

    echo "demo server did not become ready" >&2
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

verify_browser_entries() {
    mkdir -p "${ROOT_DIR}/build/demo_config"
    : >"${SERVER_LOG}"

    AUCTION_APP_CONFIG="${DEMO_CONFIG_PATH}" "${ROOT_DIR}/build/bin/auction_app" \
        >"${SERVER_LOG}" 2>&1 &
    SERVER_PID=$!

    wait_for_demo_server

    expect_http_200 \
        "http://${DEMO_SERVER_HOST}:${DEMO_SERVER_PORT}/healthz" \
        "${ROOT_DIR}/build/demo_config/verify_healthz.json"
    expect_http_200 \
        "http://${DEMO_SERVER_HOST}:${DEMO_SERVER_PORT}/demo" \
        "${ROOT_DIR}/build/demo_config/verify_demo.html"
    expect_http_200 \
        "http://${DEMO_SERVER_HOST}:${DEMO_SERVER_PORT}/app" \
        "${ROOT_DIR}/build/demo_config/verify_app.html"
    expect_http_200 \
        "http://${DEMO_SERVER_HOST}:${DEMO_SERVER_PORT}/assets/demo/app.css" \
        "${ROOT_DIR}/build/demo_config/verify_demo.css"
    expect_http_200 \
        "http://${DEMO_SERVER_HOST}:${DEMO_SERVER_PORT}/assets/demo/app.js" \
        "${ROOT_DIR}/build/demo_config/verify_demo.js"
    expect_http_200 \
        "http://${DEMO_SERVER_HOST}:${DEMO_SERVER_PORT}/assets/app/app.css" \
        "${ROOT_DIR}/build/demo_config/verify_app.css"
    expect_http_200 \
        "http://${DEMO_SERVER_HOST}:${DEMO_SERVER_PORT}/assets/app/app.js" \
        "${ROOT_DIR}/build/demo_config/verify_app.js"
    expect_http_200 \
        "http://${DEMO_SERVER_HOST}:${DEMO_SERVER_PORT}/api/demo/dashboard" \
        "${ROOT_DIR}/build/demo_config/verify_dashboard.json"

    grep -q "在线拍卖平台演示台" "${ROOT_DIR}/build/demo_config/verify_demo.html"
    grep -q "在线拍卖平台业务工作台" "${ROOT_DIR}/build/demo_config/verify_app.html"
    grep -q '"view"' "${ROOT_DIR}/build/demo_config/verify_dashboard.json"
    grep -q 'demo_dashboard' "${ROOT_DIR}/build/demo_config/verify_dashboard.json"

    cleanup
    SERVER_PID=""
}

trap cleanup EXIT

"${ROOT_DIR}/scripts/deploy/init_demo_env.sh"
verify_browser_entries
"${ROOT_DIR}/scripts/test.sh" frontend
"${ROOT_DIR}/scripts/test.sh" ui
"${ROOT_DIR}/scripts/test.sh" http
"${ROOT_DIR}/scripts/test.sh" risk
ctest --test-dir "${ROOT_DIR}/build" --output-on-failure

echo "release verification passed"
