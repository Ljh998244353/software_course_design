#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::string ReadText(const std::filesystem::path& path) {
    std::ifstream input(path);
    assert(input.good());

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

void ExpectContains(const std::string& text, const std::string& needle) {
    assert(text.find(needle) != std::string::npos);
}

void ExpectContainsInOrder(const std::string& text, const std::vector<std::string>& fragments) {
    std::size_t cursor = 0;
    for (const auto& fragment : fragments) {
        const auto position = text.find(fragment, cursor);
        assert(position != std::string::npos);
        cursor = position + fragment.size();
    }
}

std::string FindTagById(const std::string& html, const std::string& id) {
    const std::string marker = "id=\"" + id + "\"";
    const auto marker_position = html.find(marker);
    assert(marker_position != std::string::npos);

    const auto tag_start = html.rfind('<', marker_position);
    const auto tag_end = html.find('>', marker_position);
    assert(tag_start != std::string::npos);
    assert(tag_end != std::string::npos);
    return html.substr(tag_start, tag_end - tag_start + 1);
}

std::string FindNavTag(const std::string& html, const std::string& route) {
    const std::string marker = "data-route=\"" + route + "\"";
    const auto marker_position = html.find(marker);
    assert(marker_position != std::string::npos);

    const auto tag_start = html.rfind('<', marker_position);
    const auto tag_end = html.find('>', marker_position);
    assert(tag_start != std::string::npos);
    assert(tag_end != std::string::npos);
    return html.substr(tag_start, tag_end - tag_start + 1);
}

void ExpectRouteView(const std::string& html, const std::string& route) {
    ExpectContains(html, "data-view=\"" + route + "\"");
}

void ExpectNavRole(
    const std::string& html,
    const std::string& route,
    const std::string& roles
) {
    const std::string tag = FindNavTag(html, route);
    ExpectContains(tag, "data-route=\"" + route + "\"");
    ExpectContains(tag, "data-roles=\"" + roles + "\"");
}

void ExpectInputRule(
    const std::string& html,
    const std::string& id,
    const std::vector<std::string>& required_fragments
) {
    const std::string tag = FindTagById(html, id);
    for (const auto& fragment : required_fragments) {
        ExpectContains(tag, fragment);
    }
}

void ExpectAllIds(const std::string& html, const std::vector<std::string>& ids) {
    for (const auto& id : ids) {
        ExpectContains(html, "id=\"" + id + "\"");
    }
}

void ExpectAllTexts(const std::string& text, const std::vector<std::string>& needles) {
    for (const auto& needle : needles) {
        ExpectContains(text, needle);
    }
}

} // namespace

int main() {
    namespace fs = std::filesystem;

    const fs::path app_root = fs::path(AUCTION_PROJECT_ROOT) / "assets/app";
    const std::string html = ReadText(app_root / "index.html");
    const std::string js = ReadText(app_root / "app.js");
    const std::string css = ReadText(app_root / "app.css");

    ExpectContains(html, "<title>在线拍卖平台业务工作台</title>");
    ExpectContains(html, "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
    ExpectContains(html, "<link rel=\"stylesheet\" href=\"/assets/app/app.css\">");
    ExpectContains(html, "<script src=\"/assets/app/app.js\"></script>");

    for (const auto& route : {
             "gate",
             "home",
             "buyer",
             "orders",
             "notifications",
             "seller",
             "admin",
             "support",
             "api",
         }) {
        ExpectRouteView(html, route);
    }

    ExpectNavRole(html, "home", "USER,ADMIN,SUPPORT");
    ExpectNavRole(html, "buyer", "USER");
    ExpectNavRole(html, "orders", "USER");
    ExpectNavRole(html, "notifications", "USER,ADMIN,SUPPORT");
    ExpectNavRole(html, "seller", "USER");
    ExpectNavRole(html, "admin", "ADMIN");
    ExpectNavRole(html, "support", "ADMIN,SUPPORT");
    ExpectNavRole(html, "api", "USER,ADMIN,SUPPORT");

    ExpectContains(js, "buyer: [\"USER\"]");
    ExpectContains(js, "orders: [\"USER\"]");
    ExpectContains(js, "admin: [\"ADMIN\"]");
    ExpectContains(js, "support: [\"ADMIN\", \"SUPPORT\"]");
    ExpectContains(js, "function canEnter(route)");
    ExpectContains(js, "function visibleRoutesForRole(roleCode)");
    ExpectContains(js, "toast(\"当前角色无权访问\")");
    ExpectContains(js, "toast(\"请先登录\")");
    ExpectContains(js, "panel.querySelectorAll(\"input, select, textarea, button\")");
    ExpectContains(js, "control.disabled = !isAdmin");

    ExpectAllIds(html, {
        "loginForm",
        "registerForm",
        "globalStatusBadge",
        "sessionLabel",
        "logoutButton",
        "profileName",
        "buyerFilterForm",
        "buyerAuctionList",
        "buyerAuctionDetailPanel",
        "orderFilterForm",
        "orderList",
        "orderDetailPanel",
        "notificationFilterForm",
        "notificationCenterList",
        "itemForm",
        "imageForm",
        "sellerItemList",
        "itemDetailPanel",
        "adminPendingItemList",
        "auditForm",
        "auctionForm",
        "adminAuctionList",
        "statisticsFilterForm",
        "statisticsSummaryPanel",
        "statisticsList",
        "opsLogFilterForm",
        "opsExceptionList",
        "markExceptionForm",
        "opsOperationLogList",
        "opsTaskLogList",
        "compensationForm",
        "retryNotificationForm",
        "endpointList",
        "responseViewer",
        "toast",
    });

    ExpectInputRule(html, "usernameInput", {"maxlength=\"64\"", "required"});
    ExpectInputRule(html, "passwordInput", {"maxlength=\"128\"", "required"});
    ExpectInputRule(html, "registerPhoneInput", {"pattern=\"[0-9]{11}\"", "maxlength=\"11\"", "required"});
    ExpectInputRule(html, "registerEmailInput", {"type=\"email\"", "maxlength=\"120\"", "required"});
    ExpectInputRule(html, "registerPasswordInput", {"minlength=\"8\"", "maxlength=\"128\"", "required"});
    ExpectInputRule(html, "itemTitleInput", {"maxlength=\"200\"", "required"});
    ExpectInputRule(html, "itemDescriptionInput", {"maxlength=\"2000\"", "required"});
    ExpectInputRule(html, "itemStartPriceInput", {"type=\"number\"", "min=\"0.01\"", "step=\"0.01\"", "required"});
    ExpectInputRule(html, "imageUrlInput", {"type=\"url\"", "maxlength=\"500\"", "required"});
    ExpectInputRule(html, "auctionStartTimeInput", {"type=\"datetime-local\"", "required"});
    ExpectInputRule(html, "auctionEndTimeInput", {"type=\"datetime-local\"", "required"});
    ExpectInputRule(html, "auctionStartPriceInput", {"type=\"number\"", "min=\"0.01\"", "step=\"0.01\"", "required"});
    ExpectInputRule(html, "auctionBidStepInput", {"type=\"number\"", "min=\"0.01\"", "step=\"0.01\"", "required"});
    ExpectInputRule(html, "statisticsStartDateInput", {"type=\"date\"", "required"});
    ExpectInputRule(html, "statisticsEndDateInput", {"type=\"date\"", "required"});
    ExpectInputRule(html, "opsLimitInput", {"type=\"number\"", "min=\"1\"", "max=\"100\"", "required"});
    ExpectInputRule(html, "compensationLimitInput", {"type=\"number\"", "min=\"1\"", "max=\"100\"", "required"});
    ExpectInputRule(html, "retryNotificationLimitInput", {"type=\"number\"", "min=\"1\"", "max=\"100\"", "required"});

    ExpectContains(html, "data-role-entry=\"USER\"");
    ExpectContains(html, "data-role-entry=\"ADMIN\"");
    ExpectContains(html, "data-role-entry=\"SUPPORT\"");
    ExpectContains(html, "data-admin-only");
    ExpectContains(css, ".is-locked::after");
    ExpectContains(css, "content: \"仅管理员\"");

    ExpectAllTexts(js, {
        "POST /api/auth/register",
        "POST /api/auth/login",
        "POST /api/items",
        "POST /api/admin/items/{id}/audit",
        "POST /api/admin/auctions",
        "GET /api/auctions/{id}/price",
        "POST /api/auctions/{id}/bids",
        "GET /api/notifications",
        "GET /api/orders/mine",
        "POST /api/orders/{id}/pay",
        "POST /api/orders/{id}/ship",
        "POST /api/orders/{id}/confirm-receipt",
        "POST /api/reviews",
        "GET /api/users/{id}/reviews/summary",
        "GET /api/admin/statistics/daily",
        "POST /api/admin/ops/notifications/retry",
        "POST /api/admin/ops/compensations",
        "GET /api/system/context",
    });

    ExpectContains(js, "setRequestBusy(true)");
    ExpectContains(js, "document.body.classList.toggle(\"is-busy\"");
    ExpectContains(js, "document.body.setAttribute(\"aria-busy\"");
    ExpectContains(js, "form.reportValidity()");
    ExpectContains(js, "validateAuctionForm()");
    ExpectContains(js, "结束时间必须晚于开始时间");
    ExpectContains(js, "requireFiniteNumber(\"#buyerBidAmountInput\", \"出价金额\", 0.01)");
    ExpectContains(js, "requireFiniteNumber(\"#opsLimitInput\", \"返回条数\", 1, 100)");
    ExpectContains(js, "window.setInterval");
    ExpectContains(js, "refreshSelectedPublicAuction({ silent: true })");

    ExpectContains(css, "body.is-busy .status-badge");
    ExpectContains(css, "@media (max-width: 980px)");
    ExpectContains(css, "@media (max-width: 640px)");
    ExpectContainsInOrder(css, {
        "@media (max-width: 980px)",
        ".topbar",
        "grid-template-columns: 1fr;",
    });
    ExpectContainsInOrder(css, {
        "@media (max-width: 980px)",
        ".seller-layout",
        ".admin-layout",
        ".ops-layout",
        "grid-template-columns: 1fr;",
    });
    ExpectContainsInOrder(css, {
        "@media (max-width: 640px)",
        ".nav-button",
        ".primary-button",
        "width: 100%;",
    });
    ExpectContainsInOrder(css, {
        "@media (max-width: 640px)",
        ".item-card-head",
        ".image-row",
        "grid-template-columns: 1fr;",
    });
    ExpectContains(css, "overflow-wrap: anywhere");
    ExpectContains(css, "min-width: 0");

    std::cout << "auction_app_ui_acceptance passed\n";
    return 0;
}
