#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

usage() {
    cat <<'EOF'
用法:
  scripts/test.sh [all|smoke|db|module|integration|contract|risk|stress|security|frontend|auth|item|auction|bid|order|payment|review|statistics|ops|list] [额外 ctest 参数]

示例:
  scripts/test.sh
  scripts/test.sh smoke
  scripts/test.sh module -V
  scripts/test.sh risk
  scripts/test.sh frontend
  scripts/test.sh bid
EOF
}

list_suites() {
    usage
    if [[ -d "${BUILD_DIR}" ]]; then
        echo
        ctest --test-dir "${BUILD_DIR}" -N
    fi
}

run_ctest() {
    local suite_label="${1:-}"
    shift || true

    "${ROOT_DIR}/scripts/bootstrap.sh"

    local args=(--test-dir "${BUILD_DIR}" --output-on-failure)
    if [[ -n "${suite_label}" ]]; then
        args+=(-L "${suite_label}")
    fi
    args+=("$@")

    ctest "${args[@]}"
}

main() {
    local suite="${1:-all}"
    if [[ $# -gt 0 ]]; then
        shift
    fi

    case "${suite}" in
        all)
            run_ctest "" "$@"
            ;;
        smoke)
            run_ctest "suite_smoke" "$@"
            ;;
        db)
            run_ctest "suite_db" "$@"
            ;;
        module)
            run_ctest "suite_module" "$@"
            ;;
        integration)
            run_ctest "suite_integration" "$@"
            ;;
        contract)
            run_ctest "suite_contract" "$@"
            ;;
        risk)
            run_ctest "suite_risk" "$@"
            ;;
        stress)
            run_ctest "suite_stress" "$@"
            ;;
        security)
            run_ctest "suite_security" "$@"
            ;;
        frontend)
            run_ctest "suite_demo" "$@"
            ;;
        auth)
            run_ctest "module_auth" "$@"
            ;;
        item)
            run_ctest "module_item" "$@"
            ;;
        auction)
            run_ctest "module_auction" "$@"
            ;;
        bid)
            run_ctest "module_bid" "$@"
            ;;
        order)
            run_ctest "module_order" "$@"
            ;;
        payment)
            run_ctest "module_payment" "$@"
            ;;
        review)
            run_ctest "module_review" "$@"
            ;;
        statistics)
            run_ctest "module_statistics" "$@"
            ;;
        ops)
            run_ctest "module_ops" "$@"
            ;;
        list)
            list_suites
            ;;
        -h|--help|help)
            usage
            ;;
        *)
            echo "unknown suite: ${suite}" >&2
            usage >&2
            exit 1
            ;;
    esac
}

main "$@"
