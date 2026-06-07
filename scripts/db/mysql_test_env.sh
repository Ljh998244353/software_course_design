#!/usr/bin/env bash
set -euo pipefail

mysql_test_env::load() {
    local connection_info="${1:-}"
    if [[ -z "${connection_info}" ]]; then
        echo "mysql connection info is required" >&2
        return 1
    fi

    local key
    local value
    while IFS='=' read -r key value; do
        [[ -n "${key}" ]] || continue
        case "${key}" in
            mode) export AUCTION_TEST_MYSQL_MODE="${value}" ;;
            host) export AUCTION_TEST_MYSQL_HOST="${value}" ;;
            port) export AUCTION_TEST_MYSQL_PORT="${value}" ;;
            socket) export AUCTION_TEST_MYSQL_SOCKET="${value}" ;;
            pid_file) export AUCTION_TEST_MYSQL_PID_FILE="${value}" ;;
            root_dir) export AUCTION_TEST_MYSQL_ROOT_DIR="${value}" ;;
            database) export AUCTION_TEST_DB_NAME="${value}" ;;
            username) export AUCTION_TEST_MYSQL_USER="${value}" ;;
            password) export AUCTION_TEST_MYSQL_PASSWORD="${value}" ;;
            app_username) export AUCTION_TEST_MYSQL_APP_USER="${value}" ;;
            app_password) export AUCTION_TEST_MYSQL_APP_PASSWORD="${value}" ;;
            admin_username) export AUCTION_TEST_MYSQL_USER="${value}" ;;
            admin_password) export AUCTION_TEST_MYSQL_PASSWORD="${value}" ;;
        esac
    done <<< "${connection_info}"

    if [[ -z "${AUCTION_TEST_MYSQL_MODE:-}" ]]; then
        echo "mysql connection mode is missing" >&2
        return 1
    fi
}

mysql_test_env::mysql_args() {
    local user="${AUCTION_TEST_MYSQL_USER:-root}"
    if [[ "${AUCTION_TEST_MYSQL_MODE:-}" == "socket" ]]; then
        printf -- "--socket=%s -u%s" "${AUCTION_TEST_MYSQL_SOCKET}" "${user}"
        return
    fi

    printf -- "-h%s -P%s -u%s" \
        "${AUCTION_TEST_MYSQL_HOST:-127.0.0.1}" \
        "${AUCTION_TEST_MYSQL_PORT:-3406}" \
        "${user}"
}

mysql_test_env::admin_ping() {
    # shellcheck disable=SC2086
    mysqladmin $(mysql_test_env::mysql_args) ping >/dev/null
}

mysql_test_env::shutdown() {
    if [[ -z "${AUCTION_TEST_MYSQL_MODE:-}" ]]; then
        return 0
    fi

    # shellcheck disable=SC2086
    mysqladmin $(mysql_test_env::mysql_args) shutdown >/dev/null 2>&1 || true

    if [[ -n "${AUCTION_TEST_MYSQL_PID_FILE:-}" && -f "${AUCTION_TEST_MYSQL_PID_FILE}" ]]; then
        local pid
        pid="$(<"${AUCTION_TEST_MYSQL_PID_FILE}")"
        if [[ "${pid}" =~ ^[0-9]+$ ]]; then
            for _ in {1..20}; do
                if ! kill -0 "${pid}" >/dev/null 2>&1; then
                    break
                fi
                sleep 0.2
            done
        fi
    fi
}
