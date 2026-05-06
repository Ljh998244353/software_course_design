#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
DEMO_CONFIG_PATH="${ROOT_DIR}/build/demo_config/app.demo.json"
DEMO_SERVER_HOST="${AUCTION_DEMO_SERVER_HOST:-127.0.0.1}"
DEMO_SERVER_PORT="${AUCTION_DEMO_SERVER_PORT:-18080}"

if [[ ! -f "${DEMO_CONFIG_PATH}" ]]; then
    "${ROOT_DIR}/scripts/deploy/init_demo_env.sh"
fi

cat <<EOF
starting auction demo server
config: ${DEMO_CONFIG_PATH}
health: http://${DEMO_SERVER_HOST}:${DEMO_SERVER_PORT}/healthz
demo:   http://${DEMO_SERVER_HOST}:${DEMO_SERVER_PORT}/demo
app:    http://${DEMO_SERVER_HOST}:${DEMO_SERVER_PORT}/app
EOF

AUCTION_APP_CONFIG="${DEMO_CONFIG_PATH}" "${ROOT_DIR}/build/bin/auction_app"
