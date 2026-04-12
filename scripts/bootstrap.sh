#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

mkdir -p "${ROOT_DIR}/build" "${ROOT_DIR}/logs" "${ROOT_DIR}/data/uploads"

if [[ ! -f "${ROOT_DIR}/config/app.local.json" ]]; then
    echo "config/app.local.json not found, fallback to config/app.example.json"
fi

cmake -S "${ROOT_DIR}" -B "${ROOT_DIR}/build"
cmake --build "${ROOT_DIR}/build"

echo "bootstrap completed"

