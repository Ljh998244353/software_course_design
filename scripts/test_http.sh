#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HTTP_FIXTURE_DIR="${ROOT_DIR}/tests/http"
TMP_DIR="${ROOT_DIR}/build/test_http"
SERVER_HOST="127.0.0.1"
SERVER_PORT="${AUCTION_HTTP_TEST_PORT:-18082}"

mkdir -p "${TMP_DIR}"

"${ROOT_DIR}/scripts/bootstrap.sh"

SOCKET_FILE="$("${ROOT_DIR}/scripts/db/setup_local_mysql.sh")"
TEST_CONFIG_PATH="$(
    AUCTION_TEST_SERVER_HOST="${SERVER_HOST}" \
    AUCTION_TEST_SERVER_PORT="${SERVER_PORT}" \
    "${ROOT_DIR}/scripts/db/write_test_config.sh" "${SOCKET_FILE}"
)"

SERVER_LOG="${TMP_DIR}/server.log"
LOGIN_BODY="${TMP_DIR}/login.json"
REGISTER_BODY="${TMP_DIR}/register.json"
REGISTER_LOGIN_BODY="${TMP_DIR}/register_login.json"
REGISTER_ME_BODY="${TMP_DIR}/register_me.json"
ITEM_CREATE_BODY="${TMP_DIR}/item_create.json"
ITEM_INVALID_BODY="${TMP_DIR}/item_invalid.json"
ITEM_UPDATE_BODY="${TMP_DIR}/item_update.json"
ITEM_ADD_IMAGE_BODY="${TMP_DIR}/item_add_image.json"
ITEM_ADD_SECOND_IMAGE_BODY="${TMP_DIR}/item_add_second_image.json"
ITEM_DELETE_IMAGE_BODY="${TMP_DIR}/item_delete_image.json"
ITEM_DETAIL_BODY="${TMP_DIR}/item_detail.json"
ITEM_MINE_BODY="${TMP_DIR}/item_mine.json"
ITEM_SUBMIT_BODY="${TMP_DIR}/item_submit.json"
ITEM_EDIT_AFTER_SUBMIT_BODY="${TMP_DIR}/item_edit_after_submit.json"
PENDING_ITEMS_BODY="${TMP_DIR}/pending_items.json"
ITEM_AUDIT_BODY="${TMP_DIR}/item_audit.json"
ITEM_DETAIL_AFTER_APPROVE_BODY="${TMP_DIR}/item_detail_after_approve.json"
AUCTION_CREATE_BODY="${TMP_DIR}/auction_create.json"
AUCTION_LIST_BODY="${TMP_DIR}/auction_list.json"
AUCTION_DETAIL_BODY="${TMP_DIR}/auction_detail.json"
AUCTION_UPDATE_BODY="${TMP_DIR}/auction_update.json"
AUCTION_CANCEL_BODY="${TMP_DIR}/auction_cancel.json"
REJECT_ITEM_CREATE_BODY="${TMP_DIR}/reject_item_create.json"
REJECT_ITEM_IMAGE_BODY="${TMP_DIR}/reject_item_image.json"
REJECT_ITEM_SUBMIT_BODY="${TMP_DIR}/reject_item_submit.json"
REJECT_AUDIT_BODY="${TMP_DIR}/reject_audit.json"
REJECT_ITEM_DETAIL_BODY="${TMP_DIR}/reject_item_detail.json"
AUDIT_LOGS_BODY="${TMP_DIR}/audit_logs.json"
REGISTER_LOGOUT_BODY="${TMP_DIR}/register_logout.json"
REGISTER_LOGOUT_ME_BODY="${TMP_DIR}/register_logout_me.json"
FROZEN_LOGIN_BODY="${TMP_DIR}/frozen_login.json"
SUPPORT_LOGIN_BODY="${TMP_DIR}/support_login.json"
SUPPORT_STATUS_BODY="${TMP_DIR}/support_status.json"
ADMIN_STATUS_BODY="${TMP_DIR}/admin_status.json"
CONTEXT_BODY="${TMP_DIR}/context.json"
UNAUTH_BODY="${TMP_DIR}/unauthorized.json"
NOT_FOUND_BODY="${TMP_DIR}/not_found.json"
APP_BODY="${TMP_DIR}/app.html"
APP_TRAILING_BODY="${TMP_DIR}/app_trailing.html"
CSS_ASSET_BODY="${TMP_DIR}/app.css"
JS_ASSET_BODY="${TMP_DIR}/app.js"

cleanup() {
    if [[ -n "${SERVER_PID:-}" ]]; then
        kill "${SERVER_PID}" >/dev/null 2>&1 || true
        wait "${SERVER_PID}" >/dev/null 2>&1 || true
    fi
}
trap cleanup EXIT

AUCTION_APP_CONFIG="${TEST_CONFIG_PATH}" "${ROOT_DIR}/build/bin/auction_app" >"${SERVER_LOG}" 2>&1 &
SERVER_PID=$!

for _ in {1..20}; do
    if curl -fsS "http://${SERVER_HOST}:${SERVER_PORT}/healthz" >/dev/null 2>&1; then
        break
    fi
    sleep 1
done

curl -fsS "http://${SERVER_HOST}:${SERVER_PORT}/healthz" >/dev/null

APP_STATUS="$(
    curl -sS -o "${APP_BODY}" -w "%{http_code}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/app"
)"
[[ "${APP_STATUS}" == "200" ]]
grep -q "<title>在线拍卖平台业务工作台</title>" "${APP_BODY}"
grep -q "data-view=\"gate\"" "${APP_BODY}"
grep -q "id=\"sellerItemList\"" "${APP_BODY}"
grep -q "id=\"adminPendingItemList\"" "${APP_BODY}"
grep -q "id=\"auctionForm\"" "${APP_BODY}"

APP_TRAILING_STATUS="$(
    curl -sS -o "${APP_TRAILING_BODY}" -w "%{http_code}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/app/"
)"
[[ "${APP_TRAILING_STATUS}" == "200" ]]
grep -q "<title>在线拍卖平台业务工作台</title>" "${APP_TRAILING_BODY}"

CSS_ASSET_STATUS="$(
    curl -sS -o "${CSS_ASSET_BODY}" -w "%{http_code}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/assets/app/app.css"
)"
[[ "${CSS_ASSET_STATUS}" == "200" ]]
grep -q ":root" "${CSS_ASSET_BODY}"

JS_ASSET_STATUS="$(
    curl -sS -o "${JS_ASSET_BODY}" -w "%{http_code}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/assets/app/app.js"
)"
[[ "${JS_ASSET_STATUS}" == "200" ]]
grep -q 'const API_BASE = "/api"' "${JS_ASSET_BODY}"
grep -q 'POST /api/auth/register' "${JS_ASSET_BODY}"
grep -q 'PATCH /api/admin/users/{id}/status' "${JS_ASSET_BODY}"
grep -q 'POST /api/items' "${JS_ASSET_BODY}"
grep -q 'GET /api/items/mine' "${JS_ASSET_BODY}"
grep -q 'POST /api/admin/items/{id}/audit' "${JS_ASSET_BODY}"
grep -q 'POST /api/admin/auctions' "${JS_ASSET_BODY}"
grep -q 'GET /api/admin/auctions/{id}' "${JS_ASSET_BODY}"
grep -q 'data-route' "${APP_BODY}"

LOGIN_STATUS="$(
    curl -sS -o "${LOGIN_BODY}" -w "%{http_code}" \
        -H "Content-Type: application/json" \
        -H "Accept: application/json" \
        --data @"${HTTP_FIXTURE_DIR}/auth_login_request.json" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/auth/login"
)"
[[ "${LOGIN_STATUS}" == "200" ]]
grep -q '"code"[[:space:]]*:[[:space:]]*0' "${LOGIN_BODY}"
grep -q '"roleCode"[[:space:]]*:[[:space:]]*"ADMIN"' "${LOGIN_BODY}"

TOKEN="$(
    tr -d '\n' <"${LOGIN_BODY}" |
        sed -n 's/.*"token"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p'
)"
if [[ -z "${TOKEN}" ]]; then
    echo "failed to extract token from login response" >&2
    exit 1
fi

REGISTER_USERNAME="http_user_$(date +%s%N)"
REGISTER_PASSWORD="HttpPass1"
REGISTER_EMAIL="${REGISTER_USERNAME}@auction.local"
REGISTER_PHONE="139$(date +%N | tr -dc '0-9' | tail -c 8)"

REGISTER_STATUS="$(
    curl -sS -o "${REGISTER_BODY}" -w "%{http_code}" \
        -H "Content-Type: application/json" \
        -H "Accept: application/json" \
        --data "{\"username\":\"${REGISTER_USERNAME}\",\"password\":\"${REGISTER_PASSWORD}\",\"phone\":\"${REGISTER_PHONE}\",\"email\":\"${REGISTER_EMAIL}\",\"nickname\":\"HTTP User\"}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/auth/register"
)"
[[ "${REGISTER_STATUS}" == "200" ]]
grep -q '"code"[[:space:]]*:[[:space:]]*0' "${REGISTER_BODY}"
grep -q '"roleCode"[[:space:]]*:[[:space:]]*"USER"' "${REGISTER_BODY}"
grep -q '"status"[[:space:]]*:[[:space:]]*"ACTIVE"' "${REGISTER_BODY}"

REGISTER_USER_ID="$(
    tr -d '\n' <"${REGISTER_BODY}" |
        sed -n 's/.*"userId"[[:space:]]*:[[:space:]]*\([0-9][0-9]*\).*/\1/p'
)"
if [[ -z "${REGISTER_USER_ID}" ]]; then
    echo "failed to extract registered user id" >&2
    exit 1
fi

REGISTER_LOGIN_STATUS="$(
    curl -sS -o "${REGISTER_LOGIN_BODY}" -w "%{http_code}" \
        -H "Content-Type: application/json" \
        -H "Accept: application/json" \
        --data "{\"username\":\"${REGISTER_USERNAME}\",\"password\":\"${REGISTER_PASSWORD}\"}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/auth/login"
)"
[[ "${REGISTER_LOGIN_STATUS}" == "200" ]]

REGISTER_TOKEN="$(
    tr -d '\n' <"${REGISTER_LOGIN_BODY}" |
        sed -n 's/.*"token"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p'
)"
if [[ -z "${REGISTER_TOKEN}" ]]; then
    echo "failed to extract registered user token" >&2
    exit 1
fi

REGISTER_ME_STATUS="$(
    curl -sS -o "${REGISTER_ME_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${REGISTER_TOKEN}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/auth/me"
)"
[[ "${REGISTER_ME_STATUS}" == "200" ]]
grep -q "\"username\"[[:space:]]*:[[:space:]]*\"${REGISTER_USERNAME}\"" "${REGISTER_ME_BODY}"

ITEM_TITLE="HTTP Seller Item ${REGISTER_USERNAME}"
ITEM_CREATE_STATUS="$(
    curl -sS -o "${ITEM_CREATE_BODY}" -w "%{http_code}" \
        -H "Content-Type: application/json" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${REGISTER_TOKEN}" \
        --data "{\"title\":\"${ITEM_TITLE}\",\"description\":\"HTTP item description\",\"categoryId\":1,\"startPrice\":99.50,\"coverImageUrl\":\"\"}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/items"
)"
[[ "${ITEM_CREATE_STATUS}" == "200" ]]
grep -q '"code"[[:space:]]*:[[:space:]]*0' "${ITEM_CREATE_BODY}"
grep -q '"itemStatus"[[:space:]]*:[[:space:]]*"DRAFT"' "${ITEM_CREATE_BODY}"

ITEM_ID="$(
    tr -d '\n' <"${ITEM_CREATE_BODY}" |
        sed -n 's/.*"itemId"[[:space:]]*:[[:space:]]*\([0-9][0-9]*\).*/\1/p'
)"
if [[ -z "${ITEM_ID}" ]]; then
    echo "failed to extract item id" >&2
    exit 1
fi

ITEM_INVALID_STATUS="$(
    curl -sS -o "${ITEM_INVALID_BODY}" -w "%{http_code}" \
        -H "Content-Type: application/json" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${REGISTER_TOKEN}" \
        --data '{"title":"","description":"invalid","categoryId":1,"startPrice":10.00}' \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/items"
)"
[[ "${ITEM_INVALID_STATUS}" == "400" ]]
grep -q '"code"[[:space:]]*:[[:space:]]*4001' "${ITEM_INVALID_BODY}"

ITEM_ADD_IMAGE_STATUS="$(
    curl -sS -o "${ITEM_ADD_IMAGE_BODY}" -w "%{http_code}" \
        -H "Content-Type: application/json" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${REGISTER_TOKEN}" \
        --data "{\"imageUrl\":\"/uploads/http/${REGISTER_USERNAME}/front.jpg\",\"isCover\":true}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/items/${ITEM_ID}/images"
)"
[[ "${ITEM_ADD_IMAGE_STATUS}" == "200" ]]
grep -q '"isCover"[[:space:]]*:[[:space:]]*true' "${ITEM_ADD_IMAGE_BODY}"

IMAGE_ID="$(
    tr -d '\n' <"${ITEM_ADD_IMAGE_BODY}" |
        sed -n 's/.*"imageId"[[:space:]]*:[[:space:]]*\([0-9][0-9]*\).*/\1/p'
)"
if [[ -z "${IMAGE_ID}" ]]; then
    echo "failed to extract image id" >&2
    exit 1
fi

ITEM_ADD_SECOND_IMAGE_STATUS="$(
    curl -sS -o "${ITEM_ADD_SECOND_IMAGE_BODY}" -w "%{http_code}" \
        -H "Content-Type: application/json" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${REGISTER_TOKEN}" \
        --data "{\"imageUrl\":\"/uploads/http/${REGISTER_USERNAME}/side.jpg\",\"sortNo\":2,\"isCover\":false}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/items/${ITEM_ID}/images"
)"
[[ "${ITEM_ADD_SECOND_IMAGE_STATUS}" == "200" ]]

ITEM_DELETE_IMAGE_STATUS="$(
    curl -sS -o "${ITEM_DELETE_IMAGE_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${REGISTER_TOKEN}" \
        -X DELETE \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/items/${ITEM_ID}/images/${IMAGE_ID}"
)"
[[ "${ITEM_DELETE_IMAGE_STATUS}" == "200" ]]
grep -q "\"imageId\"[[:space:]]*:[[:space:]]*${IMAGE_ID}" "${ITEM_DELETE_IMAGE_BODY}"

ITEM_UPDATE_STATUS="$(
    curl -sS -o "${ITEM_UPDATE_BODY}" -w "%{http_code}" \
        -H "Content-Type: application/json" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${REGISTER_TOKEN}" \
        -X PUT \
        --data "{\"title\":\"${ITEM_TITLE} Updated\",\"description\":\"HTTP item updated\",\"categoryId\":1,\"startPrice\":120.00,\"coverImageUrl\":\"\"}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/items/${ITEM_ID}"
)"
[[ "${ITEM_UPDATE_STATUS}" == "200" ]]
grep -q '"itemStatus"[[:space:]]*:[[:space:]]*"DRAFT"' "${ITEM_UPDATE_BODY}"

ITEM_DETAIL_STATUS="$(
    curl -sS -o "${ITEM_DETAIL_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${REGISTER_TOKEN}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/items/${ITEM_ID}"
)"
[[ "${ITEM_DETAIL_STATUS}" == "200" ]]
grep -q '"images"[[:space:]]*:' "${ITEM_DETAIL_BODY}"
grep -q 'HTTP Seller Item' "${ITEM_DETAIL_BODY}"

ITEM_MINE_STATUS="$(
    curl -sS -o "${ITEM_MINE_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${REGISTER_TOKEN}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/items/mine?itemStatus=DRAFT"
)"
[[ "${ITEM_MINE_STATUS}" == "200" ]]
grep -q "\"itemId\"[[:space:]]*:[[:space:]]*${ITEM_ID}" "${ITEM_MINE_BODY}"

ITEM_SUBMIT_STATUS="$(
    curl -sS -o "${ITEM_SUBMIT_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${REGISTER_TOKEN}" \
        -X POST \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/items/${ITEM_ID}/submit-audit"
)"
[[ "${ITEM_SUBMIT_STATUS}" == "200" ]]
grep -q '"itemStatus"[[:space:]]*:[[:space:]]*"PENDING_AUDIT"' "${ITEM_SUBMIT_BODY}"

ITEM_EDIT_AFTER_SUBMIT_STATUS="$(
    curl -sS -o "${ITEM_EDIT_AFTER_SUBMIT_BODY}" -w "%{http_code}" \
        -H "Content-Type: application/json" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${REGISTER_TOKEN}" \
        -X PUT \
        --data '{"title":"should fail"}' \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/items/${ITEM_ID}"
)"
[[ "${ITEM_EDIT_AFTER_SUBMIT_STATUS}" == "400" ]]
grep -q '"code"[[:space:]]*:[[:space:]]*4203' "${ITEM_EDIT_AFTER_SUBMIT_BODY}"

PENDING_ITEMS_STATUS="$(
    curl -sS -o "${PENDING_ITEMS_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${TOKEN}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/admin/items/pending"
)"
[[ "${PENDING_ITEMS_STATUS}" == "200" ]]
grep -q "\"itemId\"[[:space:]]*:[[:space:]]*${ITEM_ID}" "${PENDING_ITEMS_BODY}"
grep -q "\"sellerUsername\"[[:space:]]*:[[:space:]]*\"${REGISTER_USERNAME}\"" "${PENDING_ITEMS_BODY}"

ITEM_AUDIT_STATUS="$(
    curl -sS -o "${ITEM_AUDIT_BODY}" -w "%{http_code}" \
        -H "Content-Type: application/json" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${TOKEN}" \
        --data '{"auditStatus":"APPROVED","reason":"http approval"}' \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/admin/items/${ITEM_ID}/audit"
)"
[[ "${ITEM_AUDIT_STATUS}" == "200" ]]
grep -q '"auditStatus"[[:space:]]*:[[:space:]]*"APPROVED"' "${ITEM_AUDIT_BODY}"
grep -q '"newStatus"[[:space:]]*:[[:space:]]*"READY_FOR_AUCTION"' "${ITEM_AUDIT_BODY}"

ITEM_DETAIL_AFTER_APPROVE_STATUS="$(
    curl -sS -o "${ITEM_DETAIL_AFTER_APPROVE_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${REGISTER_TOKEN}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/items/${ITEM_ID}"
)"
[[ "${ITEM_DETAIL_AFTER_APPROVE_STATUS}" == "200" ]]
grep -q '"itemStatus"[[:space:]]*:[[:space:]]*"READY_FOR_AUCTION"' "${ITEM_DETAIL_AFTER_APPROVE_BODY}"
grep -q '"auditResult"[[:space:]]*:[[:space:]]*"APPROVED"' "${ITEM_DETAIL_AFTER_APPROVE_BODY}"

AUCTION_START_TIME="$(date -d '+10 minutes' '+%Y-%m-%d %H:%M:%S')"
AUCTION_END_TIME="$(date -d '+25 minutes' '+%Y-%m-%d %H:%M:%S')"
AUCTION_UPDATE_START_TIME="$(date -d '+15 minutes' '+%Y-%m-%d %H:%M:%S')"
AUCTION_UPDATE_END_TIME="$(date -d '+35 minutes' '+%Y-%m-%d %H:%M:%S')"

AUCTION_CREATE_STATUS="$(
    curl -sS -o "${AUCTION_CREATE_BODY}" -w "%{http_code}" \
        -H "Content-Type: application/json" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${TOKEN}" \
        --data "{\"itemId\":${ITEM_ID},\"startTime\":\"${AUCTION_START_TIME}\",\"endTime\":\"${AUCTION_END_TIME}\",\"startPrice\":120.00,\"bidStep\":10.00,\"antiSnipingWindowSeconds\":0,\"extendSeconds\":0}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/admin/auctions"
)"
[[ "${AUCTION_CREATE_STATUS}" == "200" ]]
grep -q '"status"[[:space:]]*:[[:space:]]*"PENDING_START"' "${AUCTION_CREATE_BODY}"

AUCTION_ID="$(
    tr -d '\n' <"${AUCTION_CREATE_BODY}" |
        sed -n 's/.*"auctionId"[[:space:]]*:[[:space:]]*\([0-9][0-9]*\).*/\1/p'
)"
if [[ -z "${AUCTION_ID}" ]]; then
    echo "failed to extract auction id" >&2
    exit 1
fi

AUCTION_LIST_STATUS="$(
    curl -sS -o "${AUCTION_LIST_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${TOKEN}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/admin/auctions?status=PENDING_START"
)"
[[ "${AUCTION_LIST_STATUS}" == "200" ]]
grep -q "\"auctionId\"[[:space:]]*:[[:space:]]*${AUCTION_ID}" "${AUCTION_LIST_BODY}"
grep -q "\"itemId\"[[:space:]]*:[[:space:]]*${ITEM_ID}" "${AUCTION_LIST_BODY}"

AUCTION_DETAIL_STATUS="$(
    curl -sS -o "${AUCTION_DETAIL_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${TOKEN}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/admin/auctions/${AUCTION_ID}"
)"
[[ "${AUCTION_DETAIL_STATUS}" == "200" ]]
grep -q "\"auctionId\"[[:space:]]*:[[:space:]]*${AUCTION_ID}" "${AUCTION_DETAIL_BODY}"
grep -q "\"sellerUsername\"[[:space:]]*:[[:space:]]*\"${REGISTER_USERNAME}\"" "${AUCTION_DETAIL_BODY}"

AUCTION_UPDATE_STATUS="$(
    curl -sS -o "${AUCTION_UPDATE_BODY}" -w "%{http_code}" \
        -H "Content-Type: application/json" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${TOKEN}" \
        -X PUT \
        --data "{\"startTime\":\"${AUCTION_UPDATE_START_TIME}\",\"endTime\":\"${AUCTION_UPDATE_END_TIME}\",\"startPrice\":150.00,\"bidStep\":15.00,\"antiSnipingWindowSeconds\":30,\"extendSeconds\":30}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/admin/auctions/${AUCTION_ID}"
)"
[[ "${AUCTION_UPDATE_STATUS}" == "200" ]]
grep -q '"status"[[:space:]]*:[[:space:]]*"PENDING_START"' "${AUCTION_UPDATE_BODY}"

AUCTION_CANCEL_STATUS="$(
    curl -sS -o "${AUCTION_CANCEL_BODY}" -w "%{http_code}" \
        -H "Content-Type: application/json" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${TOKEN}" \
        --data '{"reason":"http cancel"}' \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/admin/auctions/${AUCTION_ID}/cancel"
)"
[[ "${AUCTION_CANCEL_STATUS}" == "200" ]]
grep -q '"oldStatus"[[:space:]]*:[[:space:]]*"PENDING_START"' "${AUCTION_CANCEL_BODY}"
grep -q '"newStatus"[[:space:]]*:[[:space:]]*"CANCELLED"' "${AUCTION_CANCEL_BODY}"

REJECT_ITEM_TITLE="HTTP Reject Item ${REGISTER_USERNAME}"
REJECT_ITEM_CREATE_STATUS="$(
    curl -sS -o "${REJECT_ITEM_CREATE_BODY}" -w "%{http_code}" \
        -H "Content-Type: application/json" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${REGISTER_TOKEN}" \
        --data "{\"title\":\"${REJECT_ITEM_TITLE}\",\"description\":\"HTTP reject item\",\"categoryId\":1,\"startPrice\":50.00,\"coverImageUrl\":\"\"}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/items"
)"
[[ "${REJECT_ITEM_CREATE_STATUS}" == "200" ]]
REJECT_ITEM_ID="$(
    tr -d '\n' <"${REJECT_ITEM_CREATE_BODY}" |
        sed -n 's/.*"itemId"[[:space:]]*:[[:space:]]*\([0-9][0-9]*\).*/\1/p'
)"
if [[ -z "${REJECT_ITEM_ID}" ]]; then
    echo "failed to extract reject item id" >&2
    exit 1
fi

REJECT_ITEM_IMAGE_STATUS="$(
    curl -sS -o "${REJECT_ITEM_IMAGE_BODY}" -w "%{http_code}" \
        -H "Content-Type: application/json" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${REGISTER_TOKEN}" \
        --data "{\"imageUrl\":\"/uploads/http/${REGISTER_USERNAME}/reject.jpg\",\"isCover\":true}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/items/${REJECT_ITEM_ID}/images"
)"
[[ "${REJECT_ITEM_IMAGE_STATUS}" == "200" ]]

REJECT_ITEM_SUBMIT_STATUS="$(
    curl -sS -o "${REJECT_ITEM_SUBMIT_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${REGISTER_TOKEN}" \
        -X POST \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/items/${REJECT_ITEM_ID}/submit-audit"
)"
[[ "${REJECT_ITEM_SUBMIT_STATUS}" == "200" ]]

REJECT_AUDIT_STATUS="$(
    curl -sS -o "${REJECT_AUDIT_BODY}" -w "%{http_code}" \
        -H "Content-Type: application/json" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${TOKEN}" \
        --data '{"auditStatus":"REJECTED","reason":"image is not authentic"}' \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/admin/items/${REJECT_ITEM_ID}/audit"
)"
[[ "${REJECT_AUDIT_STATUS}" == "200" ]]
grep -q '"auditStatus"[[:space:]]*:[[:space:]]*"REJECTED"' "${REJECT_AUDIT_BODY}"
grep -q '"newStatus"[[:space:]]*:[[:space:]]*"REJECTED"' "${REJECT_AUDIT_BODY}"

REJECT_ITEM_DETAIL_STATUS="$(
    curl -sS -o "${REJECT_ITEM_DETAIL_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${REGISTER_TOKEN}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/items/${REJECT_ITEM_ID}"
)"
[[ "${REJECT_ITEM_DETAIL_STATUS}" == "200" ]]
grep -q '"itemStatus"[[:space:]]*:[[:space:]]*"REJECTED"' "${REJECT_ITEM_DETAIL_BODY}"
grep -q 'image is not authentic' "${REJECT_ITEM_DETAIL_BODY}"

AUDIT_LOGS_STATUS="$(
    curl -sS -o "${AUDIT_LOGS_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${TOKEN}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/admin/items/${REJECT_ITEM_ID}/audit-logs"
)"
[[ "${AUDIT_LOGS_STATUS}" == "200" ]]
grep -q '"auditResult"[[:space:]]*:[[:space:]]*"REJECTED"' "${AUDIT_LOGS_BODY}"

REGISTER_LOGOUT_STATUS="$(
    curl -sS -o "${REGISTER_LOGOUT_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${REGISTER_TOKEN}" \
        -X POST \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/auth/logout"
)"
[[ "${REGISTER_LOGOUT_STATUS}" == "200" ]]
grep -q '"success"[[:space:]]*:[[:space:]]*true' "${REGISTER_LOGOUT_BODY}"

REGISTER_LOGOUT_ME_STATUS="$(
    curl -sS -o "${REGISTER_LOGOUT_ME_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${REGISTER_TOKEN}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/auth/me"
)"
[[ "${REGISTER_LOGOUT_ME_STATUS}" == "401" ]]
grep -q '"code"[[:space:]]*:[[:space:]]*4105' "${REGISTER_LOGOUT_ME_BODY}"

SUPPORT_LOGIN_STATUS="$(
    curl -sS -o "${SUPPORT_LOGIN_BODY}" -w "%{http_code}" \
        -H "Content-Type: application/json" \
        -H "Accept: application/json" \
        --data '{"username":"support","password":"Support@123"}' \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/auth/login"
)"
[[ "${SUPPORT_LOGIN_STATUS}" == "200" ]]
SUPPORT_TOKEN="$(
    tr -d '\n' <"${SUPPORT_LOGIN_BODY}" |
        sed -n 's/.*"token"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p'
)"

SUPPORT_STATUS="$(
    curl -sS -o "${SUPPORT_STATUS_BODY}" -w "%{http_code}" \
        -H "Content-Type: application/json" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${SUPPORT_TOKEN}" \
        -X PATCH \
        --data '{"status":"FROZEN","reason":"support should be denied"}' \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/admin/users/${REGISTER_USER_ID}/status"
)"
[[ "${SUPPORT_STATUS}" == "403" ]]
grep -q '"code"[[:space:]]*:[[:space:]]*4108' "${SUPPORT_STATUS_BODY}"

ADMIN_STATUS="$(
    curl -sS -o "${ADMIN_STATUS_BODY}" -w "%{http_code}" \
        -H "Content-Type: application/json" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${TOKEN}" \
        -X PATCH \
        --data '{"status":"FROZEN","reason":"http regression"}' \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/admin/users/${REGISTER_USER_ID}/status"
)"
[[ "${ADMIN_STATUS}" == "200" ]]
grep -q '"oldStatus"[[:space:]]*:[[:space:]]*"ACTIVE"' "${ADMIN_STATUS_BODY}"
grep -q '"newStatus"[[:space:]]*:[[:space:]]*"FROZEN"' "${ADMIN_STATUS_BODY}"

FROZEN_LOGIN_STATUS="$(
    curl -sS -o "${FROZEN_LOGIN_BODY}" -w "%{http_code}" \
        -H "Content-Type: application/json" \
        -H "Accept: application/json" \
        --data "{\"username\":\"${REGISTER_USERNAME}\",\"password\":\"${REGISTER_PASSWORD}\"}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/auth/login"
)"
[[ "${FROZEN_LOGIN_STATUS}" == "401" ]]
grep -q '"code"[[:space:]]*:[[:space:]]*4106' "${FROZEN_LOGIN_BODY}"

CONTEXT_STATUS="$(
    curl -sS -o "${CONTEXT_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${TOKEN}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/system/context"
)"
[[ "${CONTEXT_STATUS}" == "200" ]]
grep -q '"authenticated"[[:space:]]*:[[:space:]]*true' "${CONTEXT_BODY}"
grep -q '"username"[[:space:]]*:[[:space:]]*"admin"' "${CONTEXT_BODY}"

UNAUTH_STATUS="$(
    curl -sS -o "${UNAUTH_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/system/context"
)"
[[ "${UNAUTH_STATUS}" == "401" ]]
grep -q '"code"[[:space:]]*:[[:space:]]*4103' "${UNAUTH_BODY}"

NOT_FOUND_STATUS="$(
    curl -sS -o "${NOT_FOUND_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/not-found"
)"
[[ "${NOT_FOUND_STATUS}" == "404" ]]
grep -q '"code"[[:space:]]*:[[:space:]]*4002' "${NOT_FOUND_BODY}"
grep -q '"message"[[:space:]]*:[[:space:]]*"route not found"' "${NOT_FOUND_BODY}"

echo "auction_http_baseline passed"
