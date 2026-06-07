#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
MYSQL_ROOT_DIR="${AUCTION_TEST_MYSQL_ROOT_DIR:-${ROOT_DIR}/build/test_mysql/run-$$}"
DATADIR="${MYSQL_ROOT_DIR}/data"
SOCKET_FILE="${MYSQL_ROOT_DIR}/mysql-runtime.sock"
PID_FILE="${MYSQL_ROOT_DIR}/mysql-runtime.pid"
LOG_FILE="${MYSQL_ROOT_DIR}/error.log"
LOCK_FILE="${MYSQL_ROOT_DIR}/setup.lock"
DB_NAME="${AUCTION_DB_NAME:-auction_system}"
DB_USER="${AUCTION_DB_USER:-auction_user}"
DB_PASSWORD="${AUCTION_DB_PASSWORD:-change_me}"
TCP_HOST="${AUCTION_TEST_MYSQL_HOST:-127.0.0.1}"
TCP_PORT="${AUCTION_TEST_MYSQL_PORT:-$((3406 + ($$ % 1000)))}"

mkdir -p "${MYSQL_ROOT_DIR}"

exec 9>"${LOCK_FILE}"
flock 9

# Close fd 9 before starting mysqld --daemonize so the daemonized process
# does not inherit and hold the flock, which would block subsequent invocations.
exec 9>&-

if [[ ! -d "${DATADIR}/mysql" ]]; then
    mysqld \
        --initialize-insecure \
        --datadir="${DATADIR}" \
        --basedir=/usr \
        --log-error="${LOG_FILE}"
fi

print_connection_info() {
    local mode="$1"
    cat <<EOF
mode=${mode}
host=${TCP_HOST}
port=${TCP_PORT}
socket=${SOCKET_FILE}
pid_file=${PID_FILE}
root_dir=${MYSQL_ROOT_DIR}
database=${DB_NAME}
app_username=${DB_USER}
app_password=${DB_PASSWORD}
admin_username=root
admin_password=
EOF
}

socket_start_failed_due_to_bind_permission() {
    [[ -f "${LOG_FILE}" ]] || return 1
    grep -Eq "Bind on unix socket: Operation not permitted|Can't start server : Bind on unix socket" "${LOG_FILE}"
}

start_socket_mode() {
    rm -f \
        "${SOCKET_FILE}" \
        "${SOCKET_FILE}.lock" \
        "${PID_FILE}" \
        "${MYSQL_ROOT_DIR}/mysql.sock.lock"

    mysqld \
        --datadir="${DATADIR}" \
        --basedir=/usr \
        --socket="${SOCKET_FILE}" \
        --pid-file="${PID_FILE}" \
        --log-error="${LOG_FILE}" \
        --skip-networking \
        --mysqlx=0 \
        --daemonize
}

start_tcp_mode() {
    rm -f "${PID_FILE}"

    mysqld \
        --datadir="${DATADIR}" \
        --basedir=/usr \
        --port="${TCP_PORT}" \
        --bind-address="${TCP_HOST}" \
        --pid-file="${PID_FILE}" \
        --log-error="${LOG_FILE}" \
        --mysqlx=0 \
        --daemonize
}

wait_socket_mode() {
    for _ in {1..10}; do
        if mysqladmin --socket="${SOCKET_FILE}" -uroot ping >/dev/null 2>&1; then
            return 0
        fi
        sleep 1
    done
    return 1
}

wait_tcp_mode() {
    for _ in {1..10}; do
        if mysqladmin -h"${TCP_HOST}" -P"${TCP_PORT}" -uroot ping >/dev/null 2>&1; then
            return 0
        fi
        sleep 1
    done
    return 1
}

if ! mysqladmin --socket="${SOCKET_FILE}" -uroot ping >/dev/null 2>&1; then
    : > "${LOG_FILE}"
    if start_socket_mode && wait_socket_mode; then
        CONNECTION_MODE="socket"
    elif socket_start_failed_due_to_bind_permission; then
        : > "${LOG_FILE}"
        start_tcp_mode
        wait_tcp_mode
        CONNECTION_MODE="tcp"
    else
        exit 1
    fi
else
    CONNECTION_MODE="socket"
fi

if [[ "${CONNECTION_MODE}" == "socket" ]]; then
    mysqladmin --socket="${SOCKET_FILE}" -uroot ping >/dev/null
else
    mysqladmin -h"${TCP_HOST}" -P"${TCP_PORT}" -uroot ping >/dev/null
fi

# shellcheck disable=SC2086
mysql $(if [[ "${CONNECTION_MODE}" == "socket" ]]; then printf -- "--socket=%s -uroot" "${SOCKET_FILE}"; else printf -- "-h%s -P%s -uroot" "${TCP_HOST}" "${TCP_PORT}"; fi) <<SQL
CREATE DATABASE IF NOT EXISTS \`${DB_NAME}\`
  CHARACTER SET utf8mb4
  COLLATE utf8mb4_0900_ai_ci;
CREATE USER IF NOT EXISTS '${DB_USER}'@'localhost' IDENTIFIED BY '${DB_PASSWORD}';
CREATE USER IF NOT EXISTS '${DB_USER}'@'127.0.0.1' IDENTIFIED BY '${DB_PASSWORD}';
ALTER USER '${DB_USER}'@'localhost' IDENTIFIED BY '${DB_PASSWORD}';
ALTER USER '${DB_USER}'@'127.0.0.1' IDENTIFIED BY '${DB_PASSWORD}';
GRANT ALL PRIVILEGES ON \`${DB_NAME}\`.* TO '${DB_USER}'@'localhost';
GRANT ALL PRIVILEGES ON \`${DB_NAME}\`.* TO '${DB_USER}'@'127.0.0.1';
FLUSH PRIVILEGES;
SQL

# shellcheck disable=SC2086
mysql $(if [[ "${CONNECTION_MODE}" == "socket" ]]; then printf -- "--socket=%s -uroot" "${SOCKET_FILE}"; else printf -- "-h%s -P%s -uroot" "${TCP_HOST}" "${TCP_PORT}"; fi) "${DB_NAME}" < "${ROOT_DIR}/sql/schema.sql"
# shellcheck disable=SC2086
mysql $(if [[ "${CONNECTION_MODE}" == "socket" ]]; then printf -- "--socket=%s -uroot" "${SOCKET_FILE}"; else printf -- "-h%s -P%s -uroot" "${TCP_HOST}" "${TCP_PORT}"; fi) "${DB_NAME}" < "${ROOT_DIR}/sql/seed.sql"

print_connection_info "${CONNECTION_MODE}"
