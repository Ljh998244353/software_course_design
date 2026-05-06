#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HTTP_FIXTURE_DIR="${ROOT_DIR}/tests/http"
TMP_DIR="${ROOT_DIR}/build/test_http"
SERVER_HOST="127.0.0.1"
SERVER_PORT="${AUCTION_HTTP_TEST_PORT:-18082}"
TEST_DB_NAME="${AUCTION_TEST_DB_NAME:-${AUCTION_DB_NAME:-auction_system}}"

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
BIDDER_REGISTER_BODY="${TMP_DIR}/bidder_register.json"
BIDDER_LOGIN_BODY="${TMP_DIR}/bidder_login.json"
BUYER_DEMO_LOGIN_BODY="${TMP_DIR}/buyer_demo_login.json"
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
PUBLIC_AUCTION_CREATE_BODY="${TMP_DIR}/public_auction_create.json"
PUBLIC_AUCTION_LIST_BODY="${TMP_DIR}/public_auction_list.json"
PUBLIC_AUCTION_DETAIL_BODY="${TMP_DIR}/public_auction_detail.json"
PUBLIC_AUCTION_PRICE_BODY="${TMP_DIR}/public_auction_price.json"
PUBLIC_AUCTION_BIDS_BODY="${TMP_DIR}/public_auction_bids.json"
PUBLIC_AUCTION_INVALID_STATUS_BODY="${TMP_DIR}/public_auction_invalid_status.json"
PUBLIC_BID_SUCCESS_BODY="${TMP_DIR}/public_bid_success.json"
PUBLIC_BID_RETRY_BODY="${TMP_DIR}/public_bid_retry.json"
PUBLIC_BID_CONFLICT_BODY="${TMP_DIR}/public_bid_conflict.json"
PUBLIC_BID_LOW_BODY="${TMP_DIR}/public_bid_low.json"
PUBLIC_BID_MY_STATUS_BODY="${TMP_DIR}/public_bid_my_status.json"
PUBLIC_BID_PRICE_AFTER_BODY="${TMP_DIR}/public_bid_price_after.json"
PUBLIC_BID_BIDS_AFTER_BODY="${TMP_DIR}/public_bid_bids_after.json"
PUBLIC_BID_NOTIFICATION_BODY="${TMP_DIR}/public_bid_notification.json"
PUBLIC_BID_NOTIFICATION_READ_BODY="${TMP_DIR}/public_bid_notification_read.json"
PUBLIC_BID_EXPIRED_BODY="${TMP_DIR}/public_bid_expired.json"
PUBLIC_BID_FROZEN_BODY="${TMP_DIR}/public_bid_frozen.json"
ORDER_BUYER_LIST_BODY="${TMP_DIR}/order_buyer_list.json"
ORDER_SELLER_LIST_BODY="${TMP_DIR}/order_seller_list.json"
ORDER_DETAIL_BODY="${TMP_DIR}/order_detail.json"
ORDER_PAYMENT_STATUS_BODY="${TMP_DIR}/order_payment_status.json"
ORDER_PAY_BODY="${TMP_DIR}/order_pay.json"
ORDER_PAY_RETRY_BODY="${TMP_DIR}/order_pay_retry.json"
ORDER_UNAUTHORIZED_BODY="${TMP_DIR}/order_unauthorized.json"
ORDER_SHIP_BEFORE_PAY_BODY="${TMP_DIR}/order_ship_before_pay.json"
ORDER_CALLBACK_BODY="${TMP_DIR}/order_callback.json"
ORDER_CALLBACK_RETRY_BODY="${TMP_DIR}/order_callback_retry.json"
ORDER_PAYMENT_AFTER_BODY="${TMP_DIR}/order_payment_after.json"
ORDER_SHIP_WRONG_BODY="${TMP_DIR}/order_ship_wrong.json"
ORDER_SHIP_BODY="${TMP_DIR}/order_ship.json"
ORDER_SHIP_RETRY_BODY="${TMP_DIR}/order_ship_retry.json"
ORDER_CONFIRM_WRONG_BODY="${TMP_DIR}/order_confirm_wrong.json"
ORDER_CONFIRM_BODY="${TMP_DIR}/order_confirm.json"
ORDER_CONFIRM_RETRY_BODY="${TMP_DIR}/order_confirm_retry.json"
ORDER_DETAIL_COMPLETED_BODY="${TMP_DIR}/order_detail_completed.json"
ORDER_REVIEWS_EMPTY_BODY="${TMP_DIR}/order_reviews_empty.json"
ORDER_BUYER_REVIEW_BODY="${TMP_DIR}/order_buyer_review.json"
ORDER_BUYER_REVIEW_DUP_BODY="${TMP_DIR}/order_buyer_review_duplicate.json"
ORDER_SELLER_CREDIT_BODY="${TMP_DIR}/order_seller_credit.json"
ORDER_SELLER_REVIEW_BODY="${TMP_DIR}/order_seller_review.json"
ORDER_REVIEWS_AFTER_BODY="${TMP_DIR}/order_reviews_after.json"
ORDER_BUYER_CREDIT_BODY="${TMP_DIR}/order_buyer_credit.json"
ORDER_REVIEW_NOTIFICATION_BODY="${TMP_DIR}/order_review_notification.json"
ORDER_REVIEW_NOTIFICATION_READ_BODY="${TMP_DIR}/order_review_notification_read.json"
ORDER_REVIEW_NOT_OWNER_BODY="${TMP_DIR}/order_review_not_owner.json"
BIDDER_FREEZE_BODY="${TMP_DIR}/bidder_freeze.json"
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
grep -q "id=\"buyerAuctionList\"" "${APP_BODY}"
grep -q "id=\"orderList\"" "${APP_BODY}"
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
grep -q 'GET /api/auctions/{id}/bids' "${JS_ASSET_BODY}"
grep -q 'POST /api/auctions/{id}/bids' "${JS_ASSET_BODY}"
grep -q 'GET /api/auctions/{id}/my-bid' "${JS_ASSET_BODY}"
grep -q 'GET /api/notifications' "${JS_ASSET_BODY}"
grep -q 'GET /api/orders/mine' "${JS_ASSET_BODY}"
grep -q 'POST /api/orders/{id}/pay' "${JS_ASSET_BODY}"
grep -q 'POST /api/payments/callback' "${JS_ASSET_BODY}"
grep -q 'POST /api/orders/{id}/ship' "${JS_ASSET_BODY}"
grep -q 'POST /api/orders/{id}/confirm-receipt' "${JS_ASSET_BODY}"
grep -q 'POST /api/reviews' "${JS_ASSET_BODY}"
grep -q 'GET /api/orders/{id}/reviews' "${JS_ASSET_BODY}"
grep -q 'GET /api/users/{id}/reviews/summary' "${JS_ASSET_BODY}"
grep -q 'buyerBidAmountInput' "${JS_ASSET_BODY}"
grep -q 'payOrderButton' "${JS_ASSET_BODY}"
grep -q 'notificationCenterList' "${APP_BODY}"
grep -q 'data-route="notifications"' "${APP_BODY}"
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

BIDDER_USERNAME="http_bidder_$(date +%s%N)"
BIDDER_PASSWORD="HttpBid1"
BIDDER_EMAIL="${BIDDER_USERNAME}@auction.local"
BIDDER_PHONE="138$(date +%N | tr -dc '0-9' | tail -c 8)"

BIDDER_REGISTER_STATUS="$(
    curl -sS -o "${BIDDER_REGISTER_BODY}" -w "%{http_code}" \
        -H "Content-Type: application/json" \
        -H "Accept: application/json" \
        --data "{\"username\":\"${BIDDER_USERNAME}\",\"password\":\"${BIDDER_PASSWORD}\",\"phone\":\"${BIDDER_PHONE}\",\"email\":\"${BIDDER_EMAIL}\",\"nickname\":\"HTTP Bidder\"}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/auth/register"
)"
[[ "${BIDDER_REGISTER_STATUS}" == "200" ]]
grep -q '"roleCode"[[:space:]]*:[[:space:]]*"USER"' "${BIDDER_REGISTER_BODY}"

BIDDER_USER_ID="$(
    tr -d '\n' <"${BIDDER_REGISTER_BODY}" |
        sed -n 's/.*"userId"[[:space:]]*:[[:space:]]*\([0-9][0-9]*\).*/\1/p'
)"
if [[ -z "${BIDDER_USER_ID}" ]]; then
    echo "failed to extract bidder user id" >&2
    exit 1
fi

BIDDER_LOGIN_STATUS="$(
    curl -sS -o "${BIDDER_LOGIN_BODY}" -w "%{http_code}" \
        -H "Content-Type: application/json" \
        -H "Accept: application/json" \
        --data "{\"username\":\"${BIDDER_USERNAME}\",\"password\":\"${BIDDER_PASSWORD}\"}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/auth/login"
)"
[[ "${BIDDER_LOGIN_STATUS}" == "200" ]]
BIDDER_TOKEN="$(
    tr -d '\n' <"${BIDDER_LOGIN_BODY}" |
        sed -n 's/.*"token"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p'
)"
if [[ -z "${BIDDER_TOKEN}" ]]; then
    echo "failed to extract bidder token" >&2
    exit 1
fi

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

PUBLIC_AUCTION_START_TIME="$(date -d '+20 minutes' '+%Y-%m-%d %H:%M:%S')"
PUBLIC_AUCTION_END_TIME="$(date -d '+50 minutes' '+%Y-%m-%d %H:%M:%S')"
PUBLIC_AUCTION_CREATE_STATUS="$(
    curl -sS -o "${PUBLIC_AUCTION_CREATE_BODY}" -w "%{http_code}" \
        -H "Content-Type: application/json" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${TOKEN}" \
        --data "{\"itemId\":${ITEM_ID},\"startTime\":\"${PUBLIC_AUCTION_START_TIME}\",\"endTime\":\"${PUBLIC_AUCTION_END_TIME}\",\"startPrice\":140.00,\"bidStep\":20.00,\"antiSnipingWindowSeconds\":60,\"extendSeconds\":60}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/admin/auctions"
)"
[[ "${PUBLIC_AUCTION_CREATE_STATUS}" == "200" ]]

PUBLIC_AUCTION_ID="$(
    tr -d '\n' <"${PUBLIC_AUCTION_CREATE_BODY}" |
        sed -n 's/.*"auctionId"[[:space:]]*:[[:space:]]*\([0-9][0-9]*\).*/\1/p'
)"
if [[ -z "${PUBLIC_AUCTION_ID}" ]]; then
    echo "failed to extract public auction id" >&2
    exit 1
fi

BUYER_DEMO_ID="$(
    mysql --socket="${SOCKET_FILE}" -uroot "${TEST_DB_NAME}" -Nse \
        "SELECT user_id FROM user_account WHERE username = 'buyer_demo' LIMIT 1"
)"
if [[ -z "${BUYER_DEMO_ID}" ]]; then
    echo "failed to load buyer_demo user id" >&2
    exit 1
fi

mysql --socket="${SOCKET_FILE}" -uroot "${TEST_DB_NAME}" <<SQL
UPDATE item
SET item_status = 'IN_AUCTION', updated_at = CURRENT_TIMESTAMP(3)
WHERE item_id = ${ITEM_ID};

UPDATE auction
SET status = 'RUNNING',
    start_time = DATE_SUB(CURRENT_TIMESTAMP(3), INTERVAL 5 MINUTE),
    end_time = DATE_ADD(CURRENT_TIMESTAMP(3), INTERVAL 25 MINUTE),
    current_price = 180.00,
    highest_bidder_id = ${BUYER_DEMO_ID},
    version = version + 1,
    updated_at = CURRENT_TIMESTAMP(3)
WHERE auction_id = ${PUBLIC_AUCTION_ID};

INSERT INTO bid_record (
    auction_id,
    bidder_id,
    request_id,
    bid_amount,
    bid_status,
    bid_time
) VALUES (
    ${PUBLIC_AUCTION_ID},
    ${BUYER_DEMO_ID},
    'HTTP-PUBLIC-BID-${PUBLIC_AUCTION_ID}',
    180.00,
    'WINNING',
    CURRENT_TIMESTAMP(3)
);
SQL

PUBLIC_AUCTION_LIST_STATUS="$(
    curl -sS -o "${PUBLIC_AUCTION_LIST_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/auctions?status=RUNNING&keyword=HTTP%20Seller"
)"
[[ "${PUBLIC_AUCTION_LIST_STATUS}" == "200" ]]
grep -q "\"auctionId\"[[:space:]]*:[[:space:]]*${PUBLIC_AUCTION_ID}" "${PUBLIC_AUCTION_LIST_BODY}"
grep -q '"acceptingBids"[[:space:]]*:[[:space:]]*true' "${PUBLIC_AUCTION_LIST_BODY}"

PUBLIC_AUCTION_DETAIL_STATUS="$(
    curl -sS -o "${PUBLIC_AUCTION_DETAIL_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/auctions/${PUBLIC_AUCTION_ID}"
)"
[[ "${PUBLIC_AUCTION_DETAIL_STATUS}" == "200" ]]
grep -q "\"auctionId\"[[:space:]]*:[[:space:]]*${PUBLIC_AUCTION_ID}" "${PUBLIC_AUCTION_DETAIL_BODY}"
grep -q '"status"[[:space:]]*:[[:space:]]*"RUNNING"' "${PUBLIC_AUCTION_DETAIL_BODY}"
grep -q '"highestBidderMasked"[[:space:]]*:' "${PUBLIC_AUCTION_DETAIL_BODY}"

PUBLIC_AUCTION_PRICE_STATUS="$(
    curl -sS -o "${PUBLIC_AUCTION_PRICE_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/auctions/${PUBLIC_AUCTION_ID}/price"
)"
[[ "${PUBLIC_AUCTION_PRICE_STATUS}" == "200" ]]
grep -q '"currentHighestPrice"[[:space:]]*:[[:space:]]*180' "${PUBLIC_AUCTION_PRICE_BODY}"
grep -q '"minimumNextBid"[[:space:]]*:[[:space:]]*200' "${PUBLIC_AUCTION_PRICE_BODY}"

PUBLIC_AUCTION_BIDS_STATUS="$(
    curl -sS -o "${PUBLIC_AUCTION_BIDS_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/auctions/${PUBLIC_AUCTION_ID}/bids?pageSize=5"
)"
[[ "${PUBLIC_AUCTION_BIDS_STATUS}" == "200" ]]
grep -q '"bidStatus"[[:space:]]*:[[:space:]]*"WINNING"' "${PUBLIC_AUCTION_BIDS_BODY}"
grep -q '"bidAmount"[[:space:]]*:[[:space:]]*180' "${PUBLIC_AUCTION_BIDS_BODY}"

PUBLIC_BID_REQUEST_ID="HTTP-BID-${PUBLIC_AUCTION_ID}"
PUBLIC_BID_SUCCESS_STATUS="$(
    curl -sS -o "${PUBLIC_BID_SUCCESS_BODY}" -w "%{http_code}" \
        -H "Content-Type: application/json" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${BIDDER_TOKEN}" \
        --data "{\"requestId\":\"${PUBLIC_BID_REQUEST_ID}\",\"bidAmount\":220.00}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/auctions/${PUBLIC_AUCTION_ID}/bids"
)"
[[ "${PUBLIC_BID_SUCCESS_STATUS}" == "200" ]]
grep -q '"code"[[:space:]]*:[[:space:]]*0' "${PUBLIC_BID_SUCCESS_BODY}"
grep -q '"bidAmount"[[:space:]]*:[[:space:]]*220' "${PUBLIC_BID_SUCCESS_BODY}"
grep -q '"bidStatus"[[:space:]]*:[[:space:]]*"WINNING"' "${PUBLIC_BID_SUCCESS_BODY}"
grep -q '"currentHighestPrice"[[:space:]]*:[[:space:]]*220' "${PUBLIC_BID_SUCCESS_BODY}"

PUBLIC_BID_RETRY_STATUS="$(
    curl -sS -o "${PUBLIC_BID_RETRY_BODY}" -w "%{http_code}" \
        -H "Content-Type: application/json" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${BIDDER_TOKEN}" \
        --data "{\"requestId\":\"${PUBLIC_BID_REQUEST_ID}\",\"bidAmount\":220.00}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/auctions/${PUBLIC_AUCTION_ID}/bids"
)"
[[ "${PUBLIC_BID_RETRY_STATUS}" == "200" ]]
grep -q '"code"[[:space:]]*:[[:space:]]*0' "${PUBLIC_BID_RETRY_BODY}"
grep -q '"bidAmount"[[:space:]]*:[[:space:]]*220' "${PUBLIC_BID_RETRY_BODY}"

PUBLIC_BID_CONFLICT_STATUS="$(
    curl -sS -o "${PUBLIC_BID_CONFLICT_BODY}" -w "%{http_code}" \
        -H "Content-Type: application/json" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${BIDDER_TOKEN}" \
        --data "{\"requestId\":\"${PUBLIC_BID_REQUEST_ID}\",\"bidAmount\":240.00}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/auctions/${PUBLIC_AUCTION_ID}/bids"
)"
[[ "${PUBLIC_BID_CONFLICT_STATUS}" == "409" ]]
grep -q '"code"[[:space:]]*:[[:space:]]*4408' "${PUBLIC_BID_CONFLICT_BODY}"

PUBLIC_BID_LOW_STATUS="$(
    curl -sS -o "${PUBLIC_BID_LOW_BODY}" -w "%{http_code}" \
        -H "Content-Type: application/json" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${BIDDER_TOKEN}" \
        --data "{\"requestId\":\"HTTP-BID-LOW-${PUBLIC_AUCTION_ID}\",\"bidAmount\":230.00}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/auctions/${PUBLIC_AUCTION_ID}/bids"
)"
[[ "${PUBLIC_BID_LOW_STATUS}" == "400" ]]
grep -q '"code"[[:space:]]*:[[:space:]]*4406' "${PUBLIC_BID_LOW_BODY}"

PUBLIC_BID_MY_STATUS="$(
    curl -sS -o "${PUBLIC_BID_MY_STATUS_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${BIDDER_TOKEN}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/auctions/${PUBLIC_AUCTION_ID}/my-bid"
)"
[[ "${PUBLIC_BID_MY_STATUS}" == "200" ]]
grep -q '"myHighestBid"[[:space:]]*:[[:space:]]*220' "${PUBLIC_BID_MY_STATUS_BODY}"
grep -q '"isCurrentHighest"[[:space:]]*:[[:space:]]*true' "${PUBLIC_BID_MY_STATUS_BODY}"

PUBLIC_BID_PRICE_AFTER_STATUS="$(
    curl -sS -o "${PUBLIC_BID_PRICE_AFTER_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/auctions/${PUBLIC_AUCTION_ID}/price"
)"
[[ "${PUBLIC_BID_PRICE_AFTER_STATUS}" == "200" ]]
grep -q '"currentHighestPrice"[[:space:]]*:[[:space:]]*220' "${PUBLIC_BID_PRICE_AFTER_BODY}"
grep -q '"minimumNextBid"[[:space:]]*:[[:space:]]*240' "${PUBLIC_BID_PRICE_AFTER_BODY}"

PUBLIC_BID_BIDS_AFTER_STATUS="$(
    curl -sS -o "${PUBLIC_BID_BIDS_AFTER_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/auctions/${PUBLIC_AUCTION_ID}/bids?pageSize=5"
)"
[[ "${PUBLIC_BID_BIDS_AFTER_STATUS}" == "200" ]]
grep -q '"bidStatus"[[:space:]]*:[[:space:]]*"WINNING"' "${PUBLIC_BID_BIDS_AFTER_BODY}"
grep -q '"bidStatus"[[:space:]]*:[[:space:]]*"OUTBID"' "${PUBLIC_BID_BIDS_AFTER_BODY}"
grep -q '"bidAmount"[[:space:]]*:[[:space:]]*220' "${PUBLIC_BID_BIDS_AFTER_BODY}"

BUYER_DEMO_LOGIN_STATUS="$(
    curl -sS -o "${BUYER_DEMO_LOGIN_BODY}" -w "%{http_code}" \
        -H "Content-Type: application/json" \
        -H "Accept: application/json" \
        --data '{"username":"buyer_demo","password":"Buyer@123"}' \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/auth/login"
)"
[[ "${BUYER_DEMO_LOGIN_STATUS}" == "200" ]]
BUYER_DEMO_TOKEN="$(
    tr -d '\n' <"${BUYER_DEMO_LOGIN_BODY}" |
        sed -n 's/.*"token"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p'
)"
if [[ -z "${BUYER_DEMO_TOKEN}" ]]; then
    echo "failed to extract buyer_demo token" >&2
    exit 1
fi

PUBLIC_BID_NOTIFICATION_STATUS="$(
    curl -sS -o "${PUBLIC_BID_NOTIFICATION_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${BUYER_DEMO_TOKEN}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/notifications?limit=10&bizType=AUCTION&bizId=${PUBLIC_AUCTION_ID}"
)"
[[ "${PUBLIC_BID_NOTIFICATION_STATUS}" == "200" ]]
grep -q '"noticeType"[[:space:]]*:[[:space:]]*"OUTBID"' "${PUBLIC_BID_NOTIFICATION_BODY}"
grep -q '"readStatus"[[:space:]]*:[[:space:]]*"UNREAD"' "${PUBLIC_BID_NOTIFICATION_BODY}"

OUTBID_NOTIFICATION_ID="$(
    tr -d '\n' <"${PUBLIC_BID_NOTIFICATION_BODY}" |
        sed -n 's/.*"notificationId"[[:space:]]*:[[:space:]]*\([0-9][0-9]*\).*/\1/p'
)"
if [[ -z "${OUTBID_NOTIFICATION_ID}" ]]; then
    echo "failed to extract outbid notification id" >&2
    exit 1
fi

PUBLIC_BID_NOTIFICATION_READ_STATUS="$(
    curl -sS -o "${PUBLIC_BID_NOTIFICATION_READ_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${BUYER_DEMO_TOKEN}" \
        -X PATCH \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/notifications/${OUTBID_NOTIFICATION_ID}/read"
)"
[[ "${PUBLIC_BID_NOTIFICATION_READ_STATUS}" == "200" ]]
grep -q '"readStatus"[[:space:]]*:[[:space:]]*"READ"' "${PUBLIC_BID_NOTIFICATION_READ_BODY}"

mysql --socket="${SOCKET_FILE}" -uroot "${TEST_DB_NAME}" <<SQL
UPDATE auction
SET end_time = DATE_SUB(CURRENT_TIMESTAMP(3), INTERVAL 1 SECOND),
    updated_at = CURRENT_TIMESTAMP(3)
WHERE auction_id = ${PUBLIC_AUCTION_ID};
SQL

PUBLIC_BID_EXPIRED_STATUS="$(
    curl -sS -o "${PUBLIC_BID_EXPIRED_BODY}" -w "%{http_code}" \
        -H "Content-Type: application/json" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${BIDDER_TOKEN}" \
        --data "{\"requestId\":\"HTTP-BID-EXPIRED-${PUBLIC_AUCTION_ID}\",\"bidAmount\":240.00}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/auctions/${PUBLIC_AUCTION_ID}/bids"
)"
[[ "${PUBLIC_BID_EXPIRED_STATUS}" == "400" ]]
grep -q '"code"[[:space:]]*:[[:space:]]*4403' "${PUBLIC_BID_EXPIRED_BODY}"

ORDER_NO="HTTP-ORDER-${PUBLIC_AUCTION_ID}-$(date +%s%N)"
mysql --socket="${SOCKET_FILE}" -uroot "${TEST_DB_NAME}" <<SQL
UPDATE item
SET item_status = 'IN_AUCTION', updated_at = CURRENT_TIMESTAMP(3)
WHERE item_id = ${ITEM_ID};

UPDATE auction
SET status = 'SETTLING',
    current_price = 220.00,
    highest_bidder_id = ${BIDDER_USER_ID},
    end_time = DATE_SUB(CURRENT_TIMESTAMP(3), INTERVAL 2 SECOND),
    version = version + 1,
    updated_at = CURRENT_TIMESTAMP(3)
WHERE auction_id = ${PUBLIC_AUCTION_ID};

INSERT INTO order_info (
    order_no,
    auction_id,
    buyer_id,
    seller_id,
    final_amount,
    order_status,
    pay_deadline_at
) VALUES (
    '${ORDER_NO}',
    ${PUBLIC_AUCTION_ID},
    ${BIDDER_USER_ID},
    ${REGISTER_USER_ID},
    220.00,
    'PENDING_PAYMENT',
    DATE_ADD(CURRENT_TIMESTAMP(3), INTERVAL 30 MINUTE)
);
SQL

ORDER_ID="$(
    mysql --socket="${SOCKET_FILE}" -uroot "${TEST_DB_NAME}" -Nse \
        "SELECT order_id FROM order_info WHERE order_no = '${ORDER_NO}' LIMIT 1"
)"
if [[ -z "${ORDER_ID}" ]]; then
    echo "failed to extract http order id" >&2
    exit 1
fi

ORDER_BUYER_LIST_STATUS="$(
    curl -sS -o "${ORDER_BUYER_LIST_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${BIDDER_TOKEN}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/orders/mine?role=buyer&status=PENDING_PAYMENT"
)"
[[ "${ORDER_BUYER_LIST_STATUS}" == "200" ]]
grep -q "\"orderId\"[[:space:]]*:[[:space:]]*${ORDER_ID}" "${ORDER_BUYER_LIST_BODY}"
grep -q '"orderStatus"[[:space:]]*:[[:space:]]*"PENDING_PAYMENT"' "${ORDER_BUYER_LIST_BODY}"

ORDER_SELLER_LIST_STATUS="$(
    curl -sS -o "${ORDER_SELLER_LIST_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${REGISTER_TOKEN}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/orders/mine?role=seller"
)"
[[ "${ORDER_SELLER_LIST_STATUS}" == "200" ]]
grep -q "\"orderId\"[[:space:]]*:[[:space:]]*${ORDER_ID}" "${ORDER_SELLER_LIST_BODY}"

ORDER_DETAIL_STATUS="$(
    curl -sS -o "${ORDER_DETAIL_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${BIDDER_TOKEN}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/orders/${ORDER_ID}"
)"
[[ "${ORDER_DETAIL_STATUS}" == "200" ]]
grep -q "\"orderNo\"[[:space:]]*:[[:space:]]*\"${ORDER_NO}\"" "${ORDER_DETAIL_BODY}"
grep -q '"latestPayment"[[:space:]]*:[[:space:]]*null' "${ORDER_DETAIL_BODY}"

ORDER_UNAUTHORIZED_STATUS="$(
    curl -sS -o "${ORDER_UNAUTHORIZED_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${BUYER_DEMO_TOKEN}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/orders/${ORDER_ID}"
)"
[[ "${ORDER_UNAUTHORIZED_STATUS}" == "403" ]]
grep -q '"code"[[:space:]]*:[[:space:]]*4503' "${ORDER_UNAUTHORIZED_BODY}"

ORDER_PAYMENT_STATUS="$(
    curl -sS -o "${ORDER_PAYMENT_STATUS_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${BIDDER_TOKEN}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/orders/${ORDER_ID}/payment"
)"
[[ "${ORDER_PAYMENT_STATUS}" == "200" ]]
grep -q '"latestPayment"[[:space:]]*:[[:space:]]*null' "${ORDER_PAYMENT_STATUS_BODY}"

ORDER_PAY_STATUS="$(
    curl -sS -o "${ORDER_PAY_BODY}" -w "%{http_code}" \
        -H "Content-Type: application/json" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${BIDDER_TOKEN}" \
        --data '{"payChannel":"MOCK_ALIPAY"}' \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/orders/${ORDER_ID}/pay"
)"
[[ "${ORDER_PAY_STATUS}" == "200" ]]
grep -q '"payStatus"[[:space:]]*:[[:space:]]*"WAITING_CALLBACK"' "${ORDER_PAY_BODY}"
grep -q '"reusedExisting"[[:space:]]*:[[:space:]]*false' "${ORDER_PAY_BODY}"

PAYMENT_NO="$(
    tr -d '\n' <"${ORDER_PAY_BODY}" |
        sed -n 's/.*"paymentNo"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p'
)"
if [[ -z "${PAYMENT_NO}" ]]; then
    echo "failed to extract payment no" >&2
    exit 1
fi

ORDER_PAY_RETRY_STATUS="$(
    curl -sS -o "${ORDER_PAY_RETRY_BODY}" -w "%{http_code}" \
        -H "Content-Type: application/json" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${BIDDER_TOKEN}" \
        --data '{"payChannel":"MOCK_ALIPAY"}' \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/orders/${ORDER_ID}/pay"
)"
[[ "${ORDER_PAY_RETRY_STATUS}" == "200" ]]
grep -q '"reusedExisting"[[:space:]]*:[[:space:]]*true' "${ORDER_PAY_RETRY_BODY}"

ORDER_SHIP_BEFORE_PAY_STATUS="$(
    curl -sS -o "${ORDER_SHIP_BEFORE_PAY_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${REGISTER_TOKEN}" \
        -X POST \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/orders/${ORDER_ID}/ship"
)"
[[ "${ORDER_SHIP_BEFORE_PAY_STATUS}" == "400" ]]
grep -q '"code"[[:space:]]*:[[:space:]]*4502' "${ORDER_SHIP_BEFORE_PAY_BODY}"

CALLBACK_TRANSACTION_NO="HTTP-TXN-${PUBLIC_AUCTION_ID}-$(date +%s%N)"
CALLBACK_PAYLOAD="COURSE_AUCTION|${ORDER_NO}|${PAYMENT_NO}|${CALLBACK_TRANSACTION_NO}|SUCCESS|220.00"
CALLBACK_SIGNATURE="$(
    printf '%s' "${CALLBACK_PAYLOAD}" |
        openssl dgst -sha256 -hmac "change_me_auth_secret" |
        awk '{print $NF}'
)"

ORDER_CALLBACK_STATUS="$(
    curl -sS -o "${ORDER_CALLBACK_BODY}" -w "%{http_code}" \
        -H "Content-Type: application/json" \
        -H "Accept: application/json" \
        --data "{\"paymentNo\":\"${PAYMENT_NO}\",\"orderNo\":\"${ORDER_NO}\",\"merchantNo\":\"COURSE_AUCTION\",\"transactionNo\":\"${CALLBACK_TRANSACTION_NO}\",\"payStatus\":\"SUCCESS\",\"payAmount\":220.00,\"signature\":\"${CALLBACK_SIGNATURE}\"}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/payments/callback"
)"
[[ "${ORDER_CALLBACK_STATUS}" == "200" ]]
grep -q '"orderStatus"[[:space:]]*:[[:space:]]*"PAID"' "${ORDER_CALLBACK_BODY}"
grep -q '"payStatus"[[:space:]]*:[[:space:]]*"SUCCESS"' "${ORDER_CALLBACK_BODY}"
grep -q '"idempotent"[[:space:]]*:[[:space:]]*false' "${ORDER_CALLBACK_BODY}"

ORDER_CALLBACK_RETRY_STATUS="$(
    curl -sS -o "${ORDER_CALLBACK_RETRY_BODY}" -w "%{http_code}" \
        -H "Content-Type: application/json" \
        -H "Accept: application/json" \
        --data "{\"paymentNo\":\"${PAYMENT_NO}\",\"orderNo\":\"${ORDER_NO}\",\"merchantNo\":\"COURSE_AUCTION\",\"transactionNo\":\"${CALLBACK_TRANSACTION_NO}\",\"payStatus\":\"SUCCESS\",\"payAmount\":220.00,\"signature\":\"${CALLBACK_SIGNATURE}\"}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/payments/callback"
)"
[[ "${ORDER_CALLBACK_RETRY_STATUS}" == "200" ]]
grep -q '"idempotent"[[:space:]]*:[[:space:]]*true' "${ORDER_CALLBACK_RETRY_BODY}"

ORDER_PAYMENT_AFTER_STATUS="$(
    curl -sS -o "${ORDER_PAYMENT_AFTER_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${BIDDER_TOKEN}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/orders/${ORDER_ID}/payment"
)"
[[ "${ORDER_PAYMENT_AFTER_STATUS}" == "200" ]]
grep -q '"orderStatus"[[:space:]]*:[[:space:]]*"PAID"' "${ORDER_PAYMENT_AFTER_BODY}"
grep -q '"payStatus"[[:space:]]*:[[:space:]]*"SUCCESS"' "${ORDER_PAYMENT_AFTER_BODY}"

ORDER_SHIP_WRONG_STATUS="$(
    curl -sS -o "${ORDER_SHIP_WRONG_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${BIDDER_TOKEN}" \
        -X POST \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/orders/${ORDER_ID}/ship"
)"
[[ "${ORDER_SHIP_WRONG_STATUS}" == "403" ]]
grep -q '"code"[[:space:]]*:[[:space:]]*4503' "${ORDER_SHIP_WRONG_BODY}"

ORDER_SHIP_STATUS="$(
    curl -sS -o "${ORDER_SHIP_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${REGISTER_TOKEN}" \
        -X POST \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/orders/${ORDER_ID}/ship"
)"
[[ "${ORDER_SHIP_STATUS}" == "200" ]]
grep -q '"oldStatus"[[:space:]]*:[[:space:]]*"PAID"' "${ORDER_SHIP_BODY}"
grep -q '"newStatus"[[:space:]]*:[[:space:]]*"SHIPPED"' "${ORDER_SHIP_BODY}"

ORDER_SHIP_RETRY_STATUS="$(
    curl -sS -o "${ORDER_SHIP_RETRY_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${REGISTER_TOKEN}" \
        -X POST \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/orders/${ORDER_ID}/ship"
)"
[[ "${ORDER_SHIP_RETRY_STATUS}" == "400" ]]
grep -q '"code"[[:space:]]*:[[:space:]]*4502' "${ORDER_SHIP_RETRY_BODY}"

ORDER_CONFIRM_WRONG_STATUS="$(
    curl -sS -o "${ORDER_CONFIRM_WRONG_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${REGISTER_TOKEN}" \
        -X POST \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/orders/${ORDER_ID}/confirm-receipt"
)"
[[ "${ORDER_CONFIRM_WRONG_STATUS}" == "403" ]]
grep -q '"code"[[:space:]]*:[[:space:]]*4503' "${ORDER_CONFIRM_WRONG_BODY}"

ORDER_CONFIRM_STATUS="$(
    curl -sS -o "${ORDER_CONFIRM_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${BIDDER_TOKEN}" \
        -X POST \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/orders/${ORDER_ID}/confirm-receipt"
)"
[[ "${ORDER_CONFIRM_STATUS}" == "200" ]]
grep -q '"oldStatus"[[:space:]]*:[[:space:]]*"SHIPPED"' "${ORDER_CONFIRM_BODY}"
grep -q '"newStatus"[[:space:]]*:[[:space:]]*"COMPLETED"' "${ORDER_CONFIRM_BODY}"

ORDER_CONFIRM_RETRY_STATUS="$(
    curl -sS -o "${ORDER_CONFIRM_RETRY_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${BIDDER_TOKEN}" \
        -X POST \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/orders/${ORDER_ID}/confirm-receipt"
)"
[[ "${ORDER_CONFIRM_RETRY_STATUS}" == "400" ]]
grep -q '"code"[[:space:]]*:[[:space:]]*4502' "${ORDER_CONFIRM_RETRY_BODY}"

ORDER_DETAIL_COMPLETED_STATUS="$(
    curl -sS -o "${ORDER_DETAIL_COMPLETED_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${BIDDER_TOKEN}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/orders/${ORDER_ID}"
)"
[[ "${ORDER_DETAIL_COMPLETED_STATUS}" == "200" ]]
grep -q '"orderStatus"[[:space:]]*:[[:space:]]*"COMPLETED"' "${ORDER_DETAIL_COMPLETED_BODY}"
grep -q '"payStatus"[[:space:]]*:[[:space:]]*"SUCCESS"' "${ORDER_DETAIL_COMPLETED_BODY}"

ORDER_REVIEWS_EMPTY_STATUS="$(
    curl -sS -o "${ORDER_REVIEWS_EMPTY_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${BIDDER_TOKEN}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/orders/${ORDER_ID}/reviews"
)"
[[ "${ORDER_REVIEWS_EMPTY_STATUS}" == "200" ]]
grep -q '"total"[[:space:]]*:[[:space:]]*0' "${ORDER_REVIEWS_EMPTY_BODY}"

ORDER_BUYER_REVIEW_STATUS="$(
    curl -sS -o "${ORDER_BUYER_REVIEW_BODY}" -w "%{http_code}" \
        -H "Content-Type: application/json" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${BIDDER_TOKEN}" \
        --data "{\"orderId\":${ORDER_ID},\"rating\":5,\"content\":\"HTTP buyer review\"}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/reviews"
)"
[[ "${ORDER_BUYER_REVIEW_STATUS}" == "200" ]]
grep -q '"reviewType"[[:space:]]*:[[:space:]]*"BUYER_TO_SELLER"' "${ORDER_BUYER_REVIEW_BODY}"
grep -q '"orderStatusAfter"[[:space:]]*:[[:space:]]*"COMPLETED"' "${ORDER_BUYER_REVIEW_BODY}"
grep -q '"orderMarkedReviewed"[[:space:]]*:[[:space:]]*false' "${ORDER_BUYER_REVIEW_BODY}"

ORDER_BUYER_REVIEW_DUP_STATUS="$(
    curl -sS -o "${ORDER_BUYER_REVIEW_DUP_BODY}" -w "%{http_code}" \
        -H "Content-Type: application/json" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${BIDDER_TOKEN}" \
        --data "{\"orderId\":${ORDER_ID},\"rating\":4,\"content\":\"duplicate\"}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/reviews"
)"
[[ "${ORDER_BUYER_REVIEW_DUP_STATUS}" == "409" ]]
grep -q '"code"[[:space:]]*:[[:space:]]*4704' "${ORDER_BUYER_REVIEW_DUP_BODY}"

ORDER_SELLER_CREDIT_STATUS="$(
    curl -sS -o "${ORDER_SELLER_CREDIT_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${BIDDER_TOKEN}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/users/${REGISTER_USER_ID}/reviews/summary"
)"
[[ "${ORDER_SELLER_CREDIT_STATUS}" == "200" ]]
grep -q '"totalReviews"[[:space:]]*:[[:space:]]*1' "${ORDER_SELLER_CREDIT_BODY}"
grep -q '"positiveReviews"[[:space:]]*:[[:space:]]*1' "${ORDER_SELLER_CREDIT_BODY}"

ORDER_REVIEW_NOT_OWNER_STATUS="$(
    curl -sS -o "${ORDER_REVIEW_NOT_OWNER_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${BUYER_DEMO_TOKEN}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/orders/${ORDER_ID}/reviews"
)"
[[ "${ORDER_REVIEW_NOT_OWNER_STATUS}" == "403" ]]
grep -q '"code"[[:space:]]*:[[:space:]]*4503' "${ORDER_REVIEW_NOT_OWNER_BODY}"

ORDER_REVIEW_NOTIFICATION_STATUS="$(
    curl -sS -o "${ORDER_REVIEW_NOTIFICATION_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${REGISTER_TOKEN}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/notifications?limit=10&bizType=ORDER&bizId=${ORDER_ID}"
)"
[[ "${ORDER_REVIEW_NOTIFICATION_STATUS}" == "200" ]]
grep -q '"noticeType"[[:space:]]*:[[:space:]]*"ORDER_REVIEW_RECEIVED"' "${ORDER_REVIEW_NOTIFICATION_BODY}"
grep -q '"readStatus"[[:space:]]*:[[:space:]]*"UNREAD"' "${ORDER_REVIEW_NOTIFICATION_BODY}"

REVIEW_NOTIFICATION_ID="$(
    tr -d '\n' <"${ORDER_REVIEW_NOTIFICATION_BODY}" |
        sed -n 's/.*"notificationId"[[:space:]]*:[[:space:]]*\([0-9][0-9]*\).*/\1/p'
)"
if [[ -z "${REVIEW_NOTIFICATION_ID}" ]]; then
    echo "failed to extract review notification id" >&2
    exit 1
fi

ORDER_REVIEW_NOTIFICATION_READ_STATUS="$(
    curl -sS -o "${ORDER_REVIEW_NOTIFICATION_READ_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${REGISTER_TOKEN}" \
        -X PATCH \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/notifications/${REVIEW_NOTIFICATION_ID}/read"
)"
[[ "${ORDER_REVIEW_NOTIFICATION_READ_STATUS}" == "200" ]]
grep -q '"readStatus"[[:space:]]*:[[:space:]]*"READ"' "${ORDER_REVIEW_NOTIFICATION_READ_BODY}"

ORDER_SELLER_REVIEW_STATUS="$(
    curl -sS -o "${ORDER_SELLER_REVIEW_BODY}" -w "%{http_code}" \
        -H "Content-Type: application/json" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${REGISTER_TOKEN}" \
        --data "{\"orderId\":${ORDER_ID},\"rating\":4,\"content\":\"HTTP seller review\"}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/reviews"
)"
[[ "${ORDER_SELLER_REVIEW_STATUS}" == "200" ]]
grep -q '"reviewType"[[:space:]]*:[[:space:]]*"SELLER_TO_BUYER"' "${ORDER_SELLER_REVIEW_BODY}"
grep -q '"orderStatusAfter"[[:space:]]*:[[:space:]]*"REVIEWED"' "${ORDER_SELLER_REVIEW_BODY}"
grep -q '"orderMarkedReviewed"[[:space:]]*:[[:space:]]*true' "${ORDER_SELLER_REVIEW_BODY}"

ORDER_REVIEWS_AFTER_STATUS="$(
    curl -sS -o "${ORDER_REVIEWS_AFTER_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${BIDDER_TOKEN}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/orders/${ORDER_ID}/reviews"
)"
[[ "${ORDER_REVIEWS_AFTER_STATUS}" == "200" ]]
grep -q '"orderStatus"[[:space:]]*:[[:space:]]*"REVIEWED"' "${ORDER_REVIEWS_AFTER_BODY}"
grep -q '"total"[[:space:]]*:[[:space:]]*2' "${ORDER_REVIEWS_AFTER_BODY}"
grep -q '"BUYER_TO_SELLER"' "${ORDER_REVIEWS_AFTER_BODY}"
grep -q '"SELLER_TO_BUYER"' "${ORDER_REVIEWS_AFTER_BODY}"

ORDER_BUYER_CREDIT_STATUS="$(
    curl -sS -o "${ORDER_BUYER_CREDIT_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${REGISTER_TOKEN}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/users/${BIDDER_USER_ID}/reviews/summary"
)"
[[ "${ORDER_BUYER_CREDIT_STATUS}" == "200" ]]
grep -q '"totalReviews"[[:space:]]*:[[:space:]]*1' "${ORDER_BUYER_CREDIT_BODY}"
grep -q '"averageRating"[[:space:]]*:[[:space:]]*4' "${ORDER_BUYER_CREDIT_BODY}"

BIDDER_FREEZE_STATUS="$(
    curl -sS -o "${BIDDER_FREEZE_BODY}" -w "%{http_code}" \
        -H "Content-Type: application/json" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${TOKEN}" \
        -X PATCH \
        --data '{"status":"FROZEN","reason":"http bid frozen regression"}' \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/admin/users/${BIDDER_USER_ID}/status"
)"
[[ "${BIDDER_FREEZE_STATUS}" == "200" ]]

PUBLIC_BID_FROZEN_STATUS="$(
    curl -sS -o "${PUBLIC_BID_FROZEN_BODY}" -w "%{http_code}" \
        -H "Content-Type: application/json" \
        -H "Accept: application/json" \
        -H "Authorization: Bearer ${BIDDER_TOKEN}" \
        --data "{\"requestId\":\"HTTP-BID-FROZEN-${PUBLIC_AUCTION_ID}\",\"bidAmount\":260.00}" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/auctions/${PUBLIC_AUCTION_ID}/bids"
)"
[[ "${PUBLIC_BID_FROZEN_STATUS}" == "401" ]]
grep -Eq '"code"[[:space:]]*:[[:space:]]*(4105|4106)' "${PUBLIC_BID_FROZEN_BODY}"

PUBLIC_AUCTION_INVALID_STATUS="$(
    curl -sS -o "${PUBLIC_AUCTION_INVALID_STATUS_BODY}" -w "%{http_code}" \
        -H "Accept: application/json" \
        "http://${SERVER_HOST}:${SERVER_PORT}/api/auctions?status=BAD_STATUS"
)"
[[ "${PUBLIC_AUCTION_INVALID_STATUS}" == "400" ]]
grep -q '"code"[[:space:]]*:[[:space:]]*4001' "${PUBLIC_AUCTION_INVALID_STATUS_BODY}"

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
