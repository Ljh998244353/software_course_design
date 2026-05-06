#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
DEMO_DB_NAME="${AUCTION_DEMO_DB_NAME:-auction_demo}"
DEMO_DB_USER="${AUCTION_DEMO_DB_USER:-auction_user}"
DEMO_DB_PASSWORD="${AUCTION_DEMO_DB_PASSWORD:-change_me}"
DEMO_SERVER_HOST="${AUCTION_DEMO_SERVER_HOST:-127.0.0.1}"
DEMO_SERVER_PORT="${AUCTION_DEMO_SERVER_PORT:-18080}"
DEMO_CONFIG_DIR="${ROOT_DIR}/build/demo_config"
DEMO_CONFIG_PATH="${DEMO_CONFIG_DIR}/app.demo.json"
DEMO_UPLOAD_DIR="${ROOT_DIR}/data/uploads/demo"

require_command() {
    local command_name="$1"
    if ! command -v "${command_name}" >/dev/null 2>&1; then
        echo "missing required command: ${command_name}" >&2
        exit 1
    fi
}

write_demo_config() {
    local socket_file="$1"

    mkdir -p "${DEMO_CONFIG_DIR}"
    cat >"${DEMO_CONFIG_PATH}" <<JSON
{
  "server": {
    "host": "${DEMO_SERVER_HOST}",
    "port": ${DEMO_SERVER_PORT}
  },
  "mysql": {
    "host": "localhost",
    "port": 3306,
    "database": "${DEMO_DB_NAME}",
    "username": "${DEMO_DB_USER}",
    "password": "${DEMO_DB_PASSWORD}",
    "socket": "${socket_file}",
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
    "path": "./logs/app.demo.log"
  }
}
JSON
}

prepare_demo_uploads() {
    mkdir -p "${DEMO_UPLOAD_DIR}"
    printf 'demo image placeholder: keyboard\n' >"${DEMO_UPLOAD_DIR}/keyboard.jpg"
    printf 'demo image placeholder: camera\n' >"${DEMO_UPLOAD_DIR}/camera.jpg"
    printf 'demo image placeholder: book\n' >"${DEMO_UPLOAD_DIR}/book.jpg"
    printf 'demo image placeholder: badge\n' >"${DEMO_UPLOAD_DIR}/badge.jpg"
}

main() {
    require_command mysql
    require_command mysqladmin
    require_command mysqld

    "${ROOT_DIR}/scripts/bootstrap.sh"

    local socket_file
    socket_file="$(
        AUCTION_DB_NAME="${DEMO_DB_NAME}" \
        AUCTION_DB_USER="${DEMO_DB_USER}" \
        AUCTION_DB_PASSWORD="${DEMO_DB_PASSWORD}" \
        "${ROOT_DIR}/scripts/db/setup_local_mysql.sh"
    )"

    write_demo_config "${socket_file}"
    prepare_demo_uploads

    mysql --socket="${socket_file}" -uroot "${DEMO_DB_NAME}" <"${ROOT_DIR}/sql/demo_data.sql"

    AUCTION_APP_CONFIG="${DEMO_CONFIG_PATH}" "${ROOT_DIR}/build/bin/auction_app" --check-config
    AUCTION_APP_CONFIG="${DEMO_CONFIG_PATH}" "${ROOT_DIR}/build/bin/auction_app" --check-db

    cat <<EOF
demo environment is ready
config: ${DEMO_CONFIG_PATH}
database: ${DEMO_DB_NAME}
server command: AUCTION_APP_CONFIG="${DEMO_CONFIG_PATH}" ${ROOT_DIR}/build/bin/auction_app
browser:
  http://${DEMO_SERVER_HOST}:${DEMO_SERVER_PORT}/demo
  http://${DEMO_SERVER_HOST}:${DEMO_SERVER_PORT}/app
api checks:
  http://${DEMO_SERVER_HOST}:${DEMO_SERVER_PORT}/healthz
  http://${DEMO_SERVER_HOST}:${DEMO_SERVER_PORT}/api/demo/dashboard

demo accounts:
  admin / Admin@123
  support / Support@123
  seller_demo / Seller@123
  buyer_demo / Buyer@123
  buyer_demo_2 / BuyerB@123
EOF
}

main "$@"
