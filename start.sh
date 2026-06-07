#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FRONTEND_DIR="${ROOT_DIR}/frontend"
BUILD_DIR="${ROOT_DIR}/build"

BACKEND_CONFIG="${ROOT_DIR}/config/app.local.json"
BACKEND_HOST="127.0.0.1"
BACKEND_PORT="18080"
FRONTEND_HOST="127.0.0.1"
FRONTEND_PORT="3000"
BACKEND_LOG="${BUILD_DIR}/start_backend.log"
FRONTEND_LOG="${BUILD_DIR}/start_frontend.log"
MYSQL_RUNTIME_INFO="${BUILD_DIR}/start_mysql_runtime.info"
PREPARED_RUNTIME_CONFIG=""
SELECTED_C_COMPILER=""
SELECTED_CXX_COMPILER=""

START_BACKEND=true
START_FRONTEND=true
RUN_BUILD=true
INSTALL_FRONTEND_DEPS=false
OPEN_BROWSER=false

BACKEND_PID=""
FRONTEND_PID=""
MYSQL_RUNTIME_STARTED=false

RED=$'\033[0;31m'
GREEN=$'\033[0;32m'
YELLOW=$'\033[1;33m'
BLUE=$'\033[0;34m'
CYAN=$'\033[0;36m'
NC=$'\033[0m'

info() { printf '%s\n' "${GREEN}[OK]${NC} $*"; }
warn() { printf '%s\n' "${YELLOW}[WARN]${NC} $*"; }
fail() { printf '%s\n' "${RED}[ERR]${NC} $*" >&2; }
step() { printf '%s\n' "${BLUE}[RUN]${NC} $*"; }
note() { printf '%s\n' "${CYAN}[..]${NC} $*"; }

show_help() {
    cat <<EOF
校园拍卖平台一键启动脚本

用法:
  ./start.sh [选项]

常用选项:
  --config <path>          后端配置文件，默认优先 config/app.local.json
  --backend-only           仅启动后端
  --frontend-only          仅启动前端
  --backend-port <port>    后端监听端口，默认 18080
  --frontend-port <port>   前端监听端口，默认 3000
  --host <host>            前后端监听 host，默认 127.0.0.1
  --backend-host <host>    后端监听 host
  --frontend-host <host>   前端监听 host
  --no-build               不自动 cmake 构建后端
  --install                前端缺少 node_modules 时自动 npm install
  --open                   启动后尝试打开前端页面
  --help, -h               显示帮助

示例:
  ./start.sh
  ./start.sh --config config/app.local.json
  ./start.sh --backend-only --backend-port 18081
  ./start.sh --frontend-only --backend-port 18080 --frontend-port 3001

说明:
  启动后端前会执行 --check-config 和 --check-db。
  如果数据库不可用，脚本会先尝试自动拉起仓库用户态本地 MySQL；
  若当前环境禁止 mysqld 监听 socket/TCP 或 fallback 仍失败，则会直接退出并提示错误日志位置。

日志:
  后端: ${BACKEND_LOG}
  前端: ${FRONTEND_LOG}
EOF
}

parse_args() {
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --config)
                [[ -n "${2:-}" ]] || { fail "--config 需要路径"; exit 1; }
                BACKEND_CONFIG="$2"
                shift 2
                ;;
            --backend-only)
                START_FRONTEND=false
                shift
                ;;
            --frontend-only)
                START_BACKEND=false
                shift
                ;;
            --backend-port)
                [[ -n "${2:-}" ]] || { fail "--backend-port 需要端口"; exit 1; }
                BACKEND_PORT="$2"
                shift 2
                ;;
            --frontend-port)
                [[ -n "${2:-}" ]] || { fail "--frontend-port 需要端口"; exit 1; }
                FRONTEND_PORT="$2"
                shift 2
                ;;
            --host)
                [[ -n "${2:-}" ]] || { fail "--host 需要 host"; exit 1; }
                BACKEND_HOST="$2"
                FRONTEND_HOST="$2"
                shift 2
                ;;
            --backend-host)
                [[ -n "${2:-}" ]] || { fail "--backend-host 需要 host"; exit 1; }
                BACKEND_HOST="$2"
                shift 2
                ;;
            --frontend-host)
                [[ -n "${2:-}" ]] || { fail "--frontend-host 需要 host"; exit 1; }
                FRONTEND_HOST="$2"
                shift 2
                ;;
            --no-build)
                RUN_BUILD=false
                shift
                ;;
            --install)
                INSTALL_FRONTEND_DEPS=true
                shift
                ;;
            --open)
                OPEN_BROWSER=true
                shift
                ;;
            --help|-h)
                show_help
                exit 0
                ;;
            *)
                fail "未知选项: $1"
                show_help
                exit 1
                ;;
        esac
    done
}

require_command() {
    local command_name="$1"
    if ! command -v "${command_name}" >/dev/null 2>&1; then
        fail "缺少命令: ${command_name}"
        exit 1
    fi
}

ensure_parent_dir() {
    local target_path="$1"
    mkdir -p "$(dirname "${target_path}")"
}

find_first_linux_command() {
    local command_name="$1"
    local path=""

    while IFS= read -r path; do
        [[ -n "${path}" ]] || continue
        if [[ "${path}" != /mnt/[a-zA-Z]/* && "${path}" != *.exe ]]; then
            printf '%s\n' "${path}"
            return 0
        fi
    done < <(type -aP "${command_name}" 2>/dev/null | awk '!seen[$0]++')

    return 1
}

is_windows_compiler_path() {
    local compiler_path="$1"
    [[ "${compiler_path}" == /mnt/[a-zA-Z]/* || "${compiler_path}" == *.exe ]]
}

read_cmake_cache_value() {
    local key="$1"
    local cache_path="${BUILD_DIR}/CMakeCache.txt"
    [[ -f "${cache_path}" ]] || return 1
    sed -n "s#^${key}:[^=]*=##p" "${cache_path}" | head -n 1
}

select_build_compilers() {
    if [[ -n "${SELECTED_C_COMPILER}" && -n "${SELECTED_CXX_COMPILER}" ]]; then
        return 0
    fi

    local linux_clang=""
    local linux_clangxx=""
    local linux_gcc=""
    local linux_gxx=""

    linux_clang="$(find_first_linux_command clang || true)"
    linux_clangxx="$(find_first_linux_command clang++ || true)"
    if [[ -n "${linux_clang}" && -n "${linux_clangxx}" ]]; then
        SELECTED_C_COMPILER="${linux_clang}"
        SELECTED_CXX_COMPILER="${linux_clangxx}"
    else
        linux_gcc="$(find_first_linux_command gcc || true)"
        linux_gxx="$(find_first_linux_command g++ || true)"
        if [[ -n "${linux_gcc}" && -n "${linux_gxx}" ]]; then
            SELECTED_C_COMPILER="${linux_gcc}"
            SELECTED_CXX_COMPILER="${linux_gxx}"
        else
            fail "未找到可用的 Linux C/C++ 编译器（clang/clang++ 或 gcc/g++）"
            fail "请确认当前 WSL 环境已安装编译工具链，且 PATH 中优先使用 Linux 路径而不是 /mnt/c 下的 Windows 编译器"
            exit 1
        fi
    fi

    note "后端编译器: C=${SELECTED_C_COMPILER} CXX=${SELECTED_CXX_COMPILER}"
}

cmake_cache_needs_reset() {
    local cache_path="${BUILD_DIR}/CMakeCache.txt"
    [[ -f "${cache_path}" ]] || return 1

    local cached_c_compiler=""
    local cached_cxx_compiler=""
    cached_c_compiler="$(read_cmake_cache_value CMAKE_C_COMPILER || true)"
    cached_cxx_compiler="$(read_cmake_cache_value CMAKE_CXX_COMPILER || true)"

    if [[ -z "${cached_c_compiler}" || -z "${cached_cxx_compiler}" ]]; then
        return 0
    fi

    if is_windows_compiler_path "${cached_c_compiler}" || is_windows_compiler_path "${cached_cxx_compiler}"; then
        return 0
    fi

    if [[ "${cached_c_compiler}" != "${SELECTED_C_COMPILER}" || "${cached_cxx_compiler}" != "${SELECTED_CXX_COMPILER}" ]]; then
        return 0
    fi

    return 1
}

reset_cmake_cache() {
    warn "检测到 build 目录缓存的编译器与当前 WSL/Linux 工具链不一致，正在重置 CMake 配置缓存"
    rm -f "${BUILD_DIR}/CMakeCache.txt"
    rm -rf "${BUILD_DIR}/CMakeFiles"
}

load_mysql_runtime_info() {
    local info_path="$1"
    [[ -f "${info_path}" ]] || return 1

    local key
    local value
    while IFS='=' read -r key value; do
        [[ -n "${key}" ]] || continue
        case "${key}" in
            mode) export START_MYSQL_MODE="${value}" ;;
            host) export START_MYSQL_HOST="${value}" ;;
            port) export START_MYSQL_PORT="${value}" ;;
            socket) export START_MYSQL_SOCKET="${value}" ;;
            pid_file) export START_MYSQL_PID_FILE="${value}" ;;
            root_dir) export START_MYSQL_ROOT_DIR="${value}" ;;
            database) export START_MYSQL_DB_NAME="${value}" ;;
            username) export START_MYSQL_USER="${value}" ;;
            password) export START_MYSQL_PASSWORD="${value}" ;;
            app_username) export START_MYSQL_APP_USER="${value}" ;;
            app_password) export START_MYSQL_APP_PASSWORD="${value}" ;;
            admin_username) export START_MYSQL_USER="${value}" ;;
            admin_password) export START_MYSQL_PASSWORD="${value}" ;;
        esac
    done <"${info_path}"

    [[ -n "${START_MYSQL_MODE:-}" ]]
}

validate_port() {
    local name="$1"
    local port="$2"
    if ! [[ "${port}" =~ ^[0-9]+$ ]] || (( port < 1 || port > 65535 )); then
        fail "${name} 端口非法: ${port}"
        exit 1
    fi
}

write_runtime_config() {
    local config_path="$1"
    local output_path="${BUILD_DIR}/start_app.json"

    mkdir -p "${BUILD_DIR}"
    cp "${config_path}" "${output_path}"
    perl -0pi -e 's/"server"\s*:\s*\{\s*"host"\s*:\s*"[^"]*"\s*,\s*"port"\s*:\s*\d+\s*\}/"server": {\n    "host": "'"${BACKEND_HOST//\//\\/}"'",\n    "port": '"${BACKEND_PORT}"'\n  }/s' "${output_path}"
    if [[ -n "${START_MYSQL_MODE:-}" ]]; then
        local mysql_socket="${START_MYSQL_SOCKET:-}"
        perl -0pi -e 's/"mysql"\s*:\s*\{\s*"host"\s*:\s*"[^"]*"\s*,\s*"port"\s*:\s*\d+\s*,\s*"database"\s*:\s*"[^"]*"\s*,\s*"username"\s*:\s*"[^"]*"\s*,\s*"password"\s*:\s*"[^"]*"\s*,\s*"socket"\s*:\s*"[^"]*"\s*,\s*"charset"\s*:\s*"[^"]*"\s*,\s*"connect_timeout_seconds"\s*:\s*\d+\s*\}/"mysql": {\n    "host": "'"${START_MYSQL_HOST:-127.0.0.1}"'",\n    "port": '"${START_MYSQL_PORT:-3306}"',\n    "database": "'"${START_MYSQL_DB_NAME:-auction_system}"'",\n    "username": "'"${START_MYSQL_APP_USER:-auction_user}"'",\n    "password": "'"${START_MYSQL_APP_PASSWORD:-change_me}"'",\n    "socket": "'"${mysql_socket//\//\\/}"'",\n    "charset": "utf8mb4",\n    "connect_timeout_seconds": 3\n  }/s' "${output_path}"
    fi
    printf '%s\n' "${output_path}"
}

last_mysql_error_log() {
    if [[ -n "${START_MYSQL_ROOT_DIR:-}" && -f "${START_MYSQL_ROOT_DIR}/error.log" ]]; then
        printf '%s\n' "${START_MYSQL_ROOT_DIR}/error.log"
        return 0
    fi

    local latest_log
    latest_log="$(find "${BUILD_DIR}/test_mysql" -maxdepth 2 -name 'error.log' -print 2>/dev/null | xargs -r ls -t 2>/dev/null | head -n 1)"
    [[ -n "${latest_log}" ]] || return 1
    printf '%s\n' "${latest_log}"
}

wait_for_http() {
    local name="$1"
    local url="$2"
    local pid="$3"
    local log_path="$4"
    local timeout_seconds="$5"

    for _ in $(seq 1 "${timeout_seconds}"); do
        if [[ -n "${pid}" ]] && ! kill -0 "${pid}" >/dev/null 2>&1; then
            fail "${name} 进程提前退出"
            [[ -f "${log_path}" ]] && tail -n 80 "${log_path}" >&2
            exit 1
        fi
        if curl -fsS "${url}" >/dev/null 2>&1; then
            info "${name} 已就绪: ${url}"
            return 0
        fi
        sleep 1
    done

    fail "${name} 启动超时: ${url}"
    [[ -f "${log_path}" ]] && tail -n 80 "${log_path}" >&2
    exit 1
}

check_backend_config() {
    if [[ "${BACKEND_CONFIG}" != /* ]]; then
        BACKEND_CONFIG="${ROOT_DIR}/${BACKEND_CONFIG}"
    fi

    if [[ ! -f "${BACKEND_CONFIG}" && "$(basename "${BACKEND_CONFIG}")" == "app.local.json" ]]; then
        local example_config="${ROOT_DIR}/config/app.example.json"
        if [[ -f "${example_config}" ]]; then
            ensure_parent_dir "${BACKEND_CONFIG}"
            cp "${example_config}" "${BACKEND_CONFIG}"
            warn "未找到本地配置，已自动生成模板: ${BACKEND_CONFIG}"
            warn "请先根据本机 MySQL/Redis 实际情况修改后再重新执行启动脚本。"
            exit 1
        fi
    fi

    if [[ ! -f "${BACKEND_CONFIG}" ]]; then
        fail "后端配置文件不存在: ${BACKEND_CONFIG}"
        fail "可复制 config/app.example.json 为 config/app.local.json 后修改 MySQL 配置"
        exit 1
    fi

    if [[ "${BACKEND_CONFIG}" == "${ROOT_DIR}/config/app.example.json" ]]; then
        fail "当前只有示例配置: ${BACKEND_CONFIG}"
        fail "示例配置仅用于模板展示，直接启动会导致登录等真实接口连库失败。"
        fail "请先准备 config/app.local.json，或显式传入 --config <你的配置文件>"
        exit 1
    fi
}

preflight_backend() {
    local runtime_config="$1"

    step "检查后端配置"
    AUCTION_APP_CONFIG="${runtime_config}" "${BUILD_DIR}/bin/auction_app" --check-config >/dev/null
    info "后端配置检查通过"

    step "检查数据库连通性"
    if ! AUCTION_APP_CONFIG="${runtime_config}" "${BUILD_DIR}/bin/auction_app" --check-db >/dev/null; then
        warn "当前配置数据库不可用，尝试自动拉起仓库本地 MySQL 运行时"
        step "初始化本地 MySQL 运行时"
        if ! "${ROOT_DIR}/scripts/db/setup_local_mysql.sh" >"${MYSQL_RUNTIME_INFO}"; then
            load_mysql_runtime_info "${MYSQL_RUNTIME_INFO}" || true
            local mysql_error_log=""
            mysql_error_log="$(last_mysql_error_log || true)"
            fail "自动拉起仓库本地 MySQL 失败"
            if [[ -n "${mysql_error_log}" ]]; then
                fail "最近 MySQL 错误日志: ${mysql_error_log}"
                if grep -Eq "Bind on unix socket: Operation not permitted|Can't start server : Bind on unix socket|Can't create IP socket: Operation not permitted|Failed to create a socket for IPv4" "${mysql_error_log}"; then
                    fail "当前运行环境禁止 mysqld 监听 Unix socket/TCP 端口；请在本机正常 shell 中运行，或先手工启动可访问的 MySQL 后再执行 start.sh"
                fi
            fi
            exit 1
        fi
        if ! load_mysql_runtime_info "${MYSQL_RUNTIME_INFO}"; then
            fail "本地 MySQL 运行时已启动，但连接信息文件解析失败: ${MYSQL_RUNTIME_INFO}"
            exit 1
        fi
        MYSQL_RUNTIME_STARTED=true
        runtime_config="$(write_runtime_config "${BACKEND_CONFIG}")"

        if ! AUCTION_APP_CONFIG="${runtime_config}" "${BUILD_DIR}/bin/auction_app" --check-db >/dev/null; then
            fail "数据库检查失败，自动 fallback 到仓库本地 MySQL 后仍不可用"
            fail "请检查 ${runtime_config} 中的 MySQL 配置，或检查 ${MYSQL_RUNTIME_INFO} 对应的本地测试库状态"
            exit 1
        fi
    fi
    info "数据库连通性检查通过"
    PREPARED_RUNTIME_CONFIG="${runtime_config}"
}

build_backend() {
    require_command cmake
    select_build_compilers

    if cmake_cache_needs_reset; then
        reset_cmake_cache
    fi

    step "构建后端"
    CC="${SELECTED_C_COMPILER}" \
    CXX="${SELECTED_CXX_COMPILER}" \
    cmake \
        -S "${ROOT_DIR}" \
        -B "${BUILD_DIR}" \
        -DCMAKE_C_COMPILER="${SELECTED_C_COMPILER}" \
        -DCMAKE_CXX_COMPILER="${SELECTED_CXX_COMPILER}"
    cmake --build "${BUILD_DIR}"
}

check_backend() {
    check_backend_config
    if [[ "${RUN_BUILD}" == "true" || ! -x "${BUILD_DIR}/bin/auction_app" ]]; then
        build_backend
    fi
    if [[ ! -x "${BUILD_DIR}/bin/auction_app" ]]; then
        fail "后端二进制不存在: ${BUILD_DIR}/bin/auction_app"
        exit 1
    fi
}

check_frontend() {
    require_command npm
    if [[ ! -d "${FRONTEND_DIR}" ]]; then
        fail "前端目录不存在: ${FRONTEND_DIR}"
        exit 1
    fi
    if [[ ! -d "${FRONTEND_DIR}/node_modules" ]]; then
        if [[ "${INSTALL_FRONTEND_DEPS}" == "true" ]]; then
            step "安装前端依赖"
            (cd "${FRONTEND_DIR}" && npm install)
        else
            fail "前端依赖不存在: ${FRONTEND_DIR}/node_modules"
            fail "请先运行: cd frontend && npm install，或使用 ./start.sh --install"
            exit 1
        fi
    fi
}

start_backend() {
    local runtime_config
    runtime_config="$(write_runtime_config "${BACKEND_CONFIG}")"

    preflight_backend "${runtime_config}"
    runtime_config="${PREPARED_RUNTIME_CONFIG}"

    : >"${BACKEND_LOG}"
    step "启动后端: ${BACKEND_HOST}:${BACKEND_PORT}"
    AUCTION_APP_CONFIG="${runtime_config}" "${BUILD_DIR}/bin/auction_app" \
        >"${BACKEND_LOG}" 2>&1 &
    BACKEND_PID=$!

    wait_for_http \
        "后端" \
        "http://${BACKEND_HOST}:${BACKEND_PORT}/healthz" \
        "${BACKEND_PID}" \
        "${BACKEND_LOG}" \
        30
}

write_frontend_env() {
    local env_path="${FRONTEND_DIR}/.env.local"
    cat >"${env_path}" <<EOF
NEXT_PUBLIC_API_BASE_URL=http://${BACKEND_HOST}:${BACKEND_PORT}
NEXT_PUBLIC_WS_BASE_URL=ws://${BACKEND_HOST}:${BACKEND_PORT}
EOF
    info "已写入前端运行配置: ${env_path}"
}

start_frontend() {
    if [[ "${START_BACKEND}" == "true" ]]; then
        write_frontend_env
    fi

    : >"${FRONTEND_LOG}"
    step "启动前端: ${FRONTEND_HOST}:${FRONTEND_PORT}"
    (
        cd "${FRONTEND_DIR}"
        npm run dev -- --hostname "${FRONTEND_HOST}" --port "${FRONTEND_PORT}"
    ) >"${FRONTEND_LOG}" 2>&1 &
    FRONTEND_PID=$!

    wait_for_http \
        "前端" \
        "http://${FRONTEND_HOST}:${FRONTEND_PORT}" \
        "${FRONTEND_PID}" \
        "${FRONTEND_LOG}" \
        45
}

stop_process() {
    local name="$1"
    local pid="$2"
    [[ -n "${pid}" ]] || return 0
    if kill -0 "${pid}" >/dev/null 2>&1; then
        kill "${pid}" >/dev/null 2>&1 || true
        wait "${pid}" >/dev/null 2>&1 || true
        info "${name} 已停止"
    fi
}

cleanup() {
    local exit_code=$?
    trap - EXIT INT TERM
    if [[ -n "${FRONTEND_PID}${BACKEND_PID}" ]]; then
        printf '\n'
        step "停止服务"
        stop_process "前端" "${FRONTEND_PID}"
        stop_process "后端" "${BACKEND_PID}"
    fi
    if [[ "${MYSQL_RUNTIME_STARTED}" == "true" && -f "${MYSQL_RUNTIME_INFO}" ]]; then
        if load_mysql_runtime_info "${MYSQL_RUNTIME_INFO}"; then
            if [[ "${START_MYSQL_MODE:-}" == "socket" ]]; then
                mysqladmin --socket="${START_MYSQL_SOCKET}" -u"${START_MYSQL_USER:-root}" shutdown >/dev/null 2>&1 || true
            else
                mysqladmin -h"${START_MYSQL_HOST:-127.0.0.1}" -P"${START_MYSQL_PORT:-3306}" -u"${START_MYSQL_USER:-root}" shutdown >/dev/null 2>&1 || true
            fi
        fi
    fi
    exit "${exit_code}"
}

open_frontend() {
    [[ "${OPEN_BROWSER}" == "true" && "${START_FRONTEND}" == "true" ]] || return 0
    local url="http://${FRONTEND_HOST}:${FRONTEND_PORT}"
    if command -v xdg-open >/dev/null 2>&1; then
        xdg-open "${url}" >/dev/null 2>&1 || true
    elif command -v powershell.exe >/dev/null 2>&1; then
        powershell.exe -NoProfile -Command "Start-Process '${url}'" >/dev/null 2>&1 || true
    else
        warn "未找到可用的浏览器打开命令，请手动访问 ${url}"
    fi
}

print_banner() {
    cat <<EOF

============================================================
校园拍卖平台一键启动
============================================================
EOF
}

print_ready_info() {
    cat <<EOF

============================================================
启动完成
============================================================
EOF
    if [[ "${START_FRONTEND}" == "true" ]]; then
        printf '前端: http://%s:%s\n' "${FRONTEND_HOST}" "${FRONTEND_PORT}"
    fi
    if [[ "${START_BACKEND}" == "true" ]]; then
        printf '后端: http://%s:%s\n' "${BACKEND_HOST}" "${BACKEND_PORT}"
        printf '健康检查: http://%s:%s/healthz\n' "${BACKEND_HOST}" "${BACKEND_PORT}"
        printf '后端配置: %s\n' "${BACKEND_CONFIG}"
    fi
    printf '后端日志: %s\n' "${BACKEND_LOG}"
    printf '前端日志: %s\n' "${FRONTEND_LOG}"
    printf '\n默认管理员账号: admin / Admin@123\n'
    printf '按 Ctrl+C 停止服务。\n\n'
}

wait_for_processes() {
    if [[ "${START_BACKEND}" == "true" && "${START_FRONTEND}" == "true" ]]; then
        wait -n "${BACKEND_PID}" "${FRONTEND_PID}" || true
        warn "有服务进程退出，准备停止其余服务"
    elif [[ "${START_BACKEND}" == "true" ]]; then
        wait "${BACKEND_PID}" || true
    elif [[ "${START_FRONTEND}" == "true" ]]; then
        wait "${FRONTEND_PID}" || true
    fi
}

main() {
    parse_args "$@"
    validate_port "后端" "${BACKEND_PORT}"
    validate_port "前端" "${FRONTEND_PORT}"
    require_command curl
    mkdir -p "${BUILD_DIR}"

    print_banner
    trap cleanup EXIT INT TERM

    if [[ "${START_BACKEND}" == "true" ]]; then
        check_backend
        start_backend
    fi
    if [[ "${START_FRONTEND}" == "true" ]]; then
        check_frontend
        start_frontend
    fi

    print_ready_info
    open_frontend
    wait_for_processes
}

main "$@"
