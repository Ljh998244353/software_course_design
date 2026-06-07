#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FRONTEND_DIR="${ROOT_DIR}/frontend"

"${ROOT_DIR}/scripts/bootstrap.sh"

cd "${FRONTEND_DIR}"
npm run release-check
