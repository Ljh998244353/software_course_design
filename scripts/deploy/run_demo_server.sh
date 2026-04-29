#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
DEMO_CONFIG_PATH="${ROOT_DIR}/build/demo_config/app.demo.json"

if [[ ! -f "${DEMO_CONFIG_PATH}" ]]; then
    "${ROOT_DIR}/scripts/deploy/init_demo_env.sh"
fi

AUCTION_APP_CONFIG="${DEMO_CONFIG_PATH}" "${ROOT_DIR}/build/bin/auction_app"
