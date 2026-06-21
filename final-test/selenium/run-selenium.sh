#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
RUN_ID="${FINAL_TEST_RUN_ID:-finaltest-$(date +%Y%m%d-%H%M%S)}"
BACKEND_HOST="${SELENIUM_BACKEND_HOST:-127.0.0.1}"
BACKEND_PORT="${SELENIUM_BACKEND_PORT:-18080}"
FRONTEND_HOST="${SELENIUM_FRONTEND_HOST:-127.0.0.1}"
FRONTEND_PORT="${SELENIUM_FRONTEND_PORT:-3100}"
API_BASE_URL="http://${BACKEND_HOST}:${BACKEND_PORT}"
FRONTEND_BASE_URL="http://${FRONTEND_HOST}:${FRONTEND_PORT}"
SELENIUM_DIR="${ROOT_DIR}/final-test/selenium"
ARTIFACT_DIR="${SELENIUM_DIR}/target/selenium-artifacts"
SERVER_LOG="${ARTIFACT_DIR}/logs/backend.log"
FRONTEND_LOG="${ARTIFACT_DIR}/logs/frontend.log"
backend_pid=""
frontend_pid=""

source "${ROOT_DIR}/scripts/db/mysql_test_env.sh"

cleanup() {
    if [[ -n "${frontend_pid}" ]]; then
        kill "${frontend_pid}" >/dev/null 2>&1 || true
        wait "${frontend_pid}" >/dev/null 2>&1 || true
    fi
    if [[ -n "${backend_pid}" ]]; then
        kill "${backend_pid}" >/dev/null 2>&1 || true
        wait "${backend_pid}" >/dev/null 2>&1 || true
    fi
    mysql_test_env::shutdown
}

archive_results() {
    local report_dir="${SELENIUM_DIR}/junit-report/${RUN_ID}"
    local screenshot_dir="${SELENIUM_DIR}/screenshots/${RUN_ID}"
    local console_dir="${SELENIUM_DIR}/console/${RUN_ID}"
    local log_dir="${SELENIUM_DIR}/logs/${RUN_ID}"
    mkdir -p "${report_dir}" "${screenshot_dir}" "${console_dir}" "${log_dir}"

    if [[ -d "${SELENIUM_DIR}/target/surefire-reports" ]]; then
        cp -R "${SELENIUM_DIR}/target/surefire-reports/." "${report_dir}/"
    fi
    if [[ -d "${ARTIFACT_DIR}/screenshots" ]]; then
        cp -R "${ARTIFACT_DIR}/screenshots/." "${screenshot_dir}/"
    fi
    if [[ -d "${ARTIFACT_DIR}/console" ]]; then
        cp -R "${ARTIFACT_DIR}/console/." "${console_dir}/"
    fi
    if [[ -d "${ARTIFACT_DIR}/logs" ]]; then
        cp -R "${ARTIFACT_DIR}/logs/." "${log_dir}/"
    fi
}

on_exit() {
    archive_results || true
    cleanup || true
}
trap on_exit EXIT

mkdir -p "${ARTIFACT_DIR}/logs"

echo "构建后端测试二进制..."
cmake --build "${ROOT_DIR}/build"

echo "准备 Selenium 临时 MySQL..."
connection_info="$("${ROOT_DIR}/scripts/db/setup_local_mysql.sh")"
mysql_test_env::load "${connection_info}"

test_config_path="$(
    AUCTION_TEST_SERVER_HOST="${BACKEND_HOST}" \
    AUCTION_TEST_SERVER_PORT="${BACKEND_PORT}" \
    "${ROOT_DIR}/scripts/db/write_test_config.sh"
)"

echo "启动后端: ${API_BASE_URL}"
: >"${SERVER_LOG}"
AUCTION_APP_CONFIG="${test_config_path}" "${ROOT_DIR}/build/bin/auction_app" >"${SERVER_LOG}" 2>&1 &
backend_pid="$!"

for _ in {1..30}; do
    if ! kill -0 "${backend_pid}" >/dev/null 2>&1; then
        echo "后端提前退出" >&2
        tail -n 160 "${SERVER_LOG}" >&2 || true
        exit 1
    fi
    if curl -fsS "${API_BASE_URL}/healthz" >/dev/null 2>&1; then
        break
    fi
    sleep 1
done
curl -fsS "${API_BASE_URL}/healthz" >/dev/null

echo "构建并启动前端: ${FRONTEND_BASE_URL}"
(
    cd "${ROOT_DIR}/frontend"
    NEXT_PUBLIC_API_BASE_URL="${API_BASE_URL}" npm run build
)
: >"${FRONTEND_LOG}"
(
    cd "${ROOT_DIR}/frontend"
    NEXT_PUBLIC_API_BASE_URL="${API_BASE_URL}" npm run start -- -H "${FRONTEND_HOST}" -p "${FRONTEND_PORT}"
) >"${FRONTEND_LOG}" 2>&1 &
frontend_pid="$!"

for _ in {1..40}; do
    if ! kill -0 "${frontend_pid}" >/dev/null 2>&1; then
        echo "前端提前退出" >&2
        tail -n 160 "${FRONTEND_LOG}" >&2 || true
        exit 1
    fi
    if curl -fsS "${FRONTEND_BASE_URL}/auth/login" >/dev/null 2>&1; then
        break
    fi
    sleep 1
done
curl -fsS "${FRONTEND_BASE_URL}/auth/login" >/dev/null

chrome_binary="$(
    find "${HOME}/.cache/selenium/chrome/linux64" -path "*/chrome" -type f 2>/dev/null | sort -V | tail -n 1 || true
)"
chromedriver_binary="$(
    find "${HOME}/.cache/selenium/chromedriver/linux64" -path "*/chromedriver" -type f 2>/dev/null | sort -V | tail -n 1 || true
)"
if [[ -z "${chrome_binary}" || -z "${chromedriver_binary}" ]]; then
    echo "未找到 Selenium Chrome 或 chromedriver 缓存" >&2
    exit 1
fi

echo "运行 JUnit + Selenium WebDriver..."
(
    cd "${SELENIUM_DIR}"
    SELENIUM_MYSQL_MODE="${AUCTION_TEST_MYSQL_MODE}" \
    SELENIUM_MYSQL_SOCKET="${AUCTION_TEST_MYSQL_SOCKET:-}" \
    SELENIUM_MYSQL_HOST="${AUCTION_TEST_MYSQL_HOST:-127.0.0.1}" \
    SELENIUM_MYSQL_PORT="${AUCTION_TEST_MYSQL_PORT:-3406}" \
    SELENIUM_MYSQL_DATABASE="${AUCTION_TEST_DB_NAME:-auction_system}" \
    SELENIUM_MYSQL_USER="${AUCTION_TEST_MYSQL_APP_USER:-auction_user}" \
    SELENIUM_MYSQL_PASSWORD="${AUCTION_TEST_MYSQL_APP_PASSWORD:-change_me}" \
    mvn -B test \
        -Dauction.run.id="${RUN_ID}" \
        -Dauction.api.url="${API_BASE_URL}" \
        -Dauction.frontend.url="${FRONTEND_BASE_URL}" \
        -Dauction.chrome.binary="${chrome_binary}" \
        -Dwebdriver.chrome.driver="${chromedriver_binary}" \
        -Dauction.artifact.dir="${ARTIFACT_DIR}"
)

echo "Selenium 测试完成"
echo "JUnit 报告: ${SELENIUM_DIR}/junit-report/${RUN_ID}"
echo "截图: ${SELENIUM_DIR}/screenshots/${RUN_ID}"
echo "浏览器 Console: ${SELENIUM_DIR}/console/${RUN_ID}"
echo "服务日志: ${SELENIUM_DIR}/logs/${RUN_ID}"
