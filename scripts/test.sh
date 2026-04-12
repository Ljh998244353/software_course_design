#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

"${ROOT_DIR}/scripts/bootstrap.sh"
ctest --test-dir "${ROOT_DIR}/build" --output-on-failure

