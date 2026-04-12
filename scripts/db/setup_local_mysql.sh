#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
MYSQL_ROOT_DIR="${ROOT_DIR}/build/test_mysql"
DATADIR="${MYSQL_ROOT_DIR}/data"
SOCKET_FILE="${MYSQL_ROOT_DIR}/mysql.sock"
PID_FILE="${MYSQL_ROOT_DIR}/mysql.pid"
LOG_FILE="${MYSQL_ROOT_DIR}/error.log"
DB_NAME="${AUCTION_DB_NAME:-auction_system}"
DB_USER="${AUCTION_DB_USER:-auction_user}"
DB_PASSWORD="${AUCTION_DB_PASSWORD:-change_me}"

mkdir -p "${MYSQL_ROOT_DIR}"

if [[ ! -d "${DATADIR}/mysql" ]]; then
    mysqld \
        --initialize-insecure \
        --datadir="${DATADIR}" \
        --basedir=/usr \
        --log-error="${LOG_FILE}"
fi

if ! mysqladmin --socket="${SOCKET_FILE}" -uroot ping >/dev/null 2>&1; then
    mysqld \
        --datadir="${DATADIR}" \
        --basedir=/usr \
        --socket="${SOCKET_FILE}" \
        --pid-file="${PID_FILE}" \
        --log-error="${LOG_FILE}" \
        --skip-networking \
        --mysqlx=0 \
        --daemonize
fi

for _ in {1..10}; do
    if mysqladmin --socket="${SOCKET_FILE}" -uroot ping >/dev/null 2>&1; then
        break
    fi
    sleep 1
done

mysqladmin --socket="${SOCKET_FILE}" -uroot ping >/dev/null

mysql --socket="${SOCKET_FILE}" -uroot <<SQL
CREATE DATABASE IF NOT EXISTS \`${DB_NAME}\`
  CHARACTER SET utf8mb4
  COLLATE utf8mb4_0900_ai_ci;
CREATE USER IF NOT EXISTS '${DB_USER}'@'localhost' IDENTIFIED BY '${DB_PASSWORD}';
ALTER USER '${DB_USER}'@'localhost' IDENTIFIED BY '${DB_PASSWORD}';
GRANT ALL PRIVILEGES ON \`${DB_NAME}\`.* TO '${DB_USER}'@'localhost';
FLUSH PRIVILEGES;
SQL

mysql --socket="${SOCKET_FILE}" -uroot "${DB_NAME}" < "${ROOT_DIR}/sql/schema.sql"
mysql --socket="${SOCKET_FILE}" -uroot "${DB_NAME}" < "${ROOT_DIR}/sql/seed.sql"

printf '%s\n' "${SOCKET_FILE}"
