#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

"${ROOT_DIR}/scripts/deploy/init_demo_env.sh"
"${ROOT_DIR}/scripts/test.sh" frontend
"${ROOT_DIR}/scripts/test.sh" risk
ctest --test-dir "${ROOT_DIR}/build" --output-on-failure

echo "release verification passed"
