#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

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

}  // namespace

int main() {
    namespace fs = std::filesystem;

    const fs::path frontend_root = fs::path(AUCTION_PROJECT_ROOT) / "frontend";

    const std::string home_page = ReadText(frontend_root / "app" / "page.tsx");
    const std::string login_page = ReadText(frontend_root / "app" / "auth" / "login" / "page.tsx");
    const std::string hall_page = ReadText(frontend_root / "app" / "auction" / "hall" / "page.tsx");
    const std::string detail_page =
        ReadText(frontend_root / "app" / "auction" / "detail" / "[id]" / "page.tsx");
    const std::string publish_page =
        ReadText(frontend_root / "app" / "auction" / "publish" / "page.tsx");
    const std::string checkout_page =
        ReadText(frontend_root / "app" / "checkout" / "[orderId]" / "page.tsx");
    const std::string admin_page =
        ReadText(frontend_root / "app" / "admin" / "dashboard" / "page.tsx");
    const std::string nav_component =
        ReadText(frontend_root / "components" / "layout" / "site-nav.tsx");
    const std::string ws_hook =
        ReadText(frontend_root / "lib" / "realtime" / "use-auction-ws.ts");
    const std::string api_client =
        ReadText(frontend_root / "lib" / "api" / "client.ts");
    const std::string next_config = ReadText(frontend_root / "next.config.mjs");
    const std::string globals_css = ReadText(frontend_root / "app" / "globals.css");

    ExpectContains(home_page, "export const dynamic = \"force-dynamic\"");
    ExpectContains(home_page, "实时竞价工作台");
    ExpectContains(home_page, "进入拍卖大厅");

    ExpectContains(login_page, "export const dynamic = \"force-dynamic\"");
    ExpectContains(login_page, "真实账号进入正式业务工作台");

    ExpectContains(hall_page, "export const dynamic = \"force-dynamic\"");
    ExpectContains(hall_page, "详情页优先使用 WebSocket 实时通道");

    ExpectContains(detail_page, "export const dynamic = \"force-dynamic\"");
    ExpectContains(detail_page, "useAuctionWebSocket");
    ExpectContains(detail_page, "PLACE BID");

    ExpectContains(publish_page, "export const dynamic = \"force-dynamic\"");
    ExpectContains(publish_page, "PUBLISH WIZARD");
    ExpectContains(publish_page, "submitItemForReview");

    ExpectContains(checkout_page, "export const dynamic = \"force-dynamic\"");
    ExpectContains(checkout_page, "SECURE CHECKOUT");
    ExpectContains(checkout_page, "MOCK_WECHAT");
    ExpectContains(checkout_page, "MOCK_ALIPAY");

    ExpectContains(admin_page, "export const dynamic = \"force-dynamic\"");
    ExpectContains(admin_page, "ADMIN WORKBENCH");
    ExpectContains(admin_page, "ws gateway online");

    ExpectContains(nav_component, "拍卖大厅");
    ExpectContains(nav_component, "发布拍品");
    ExpectContains(nav_component, "管理大盘");

    ExpectContains(ws_hook, "/ws/auction/");
    ExpectContains(ws_hook, "setState(\"fallback\")");
    ExpectContains(ws_hook, "action: \"identify\"");

    ExpectContains(api_client, "liveFetch");
    ExpectContains(api_client, "/api/auctions");
    ExpectContains(api_client, "/api/orders/");
    ExpectContains(api_client, "/api/admin/items/");

    ExpectContains(next_config, "127.0.0.1");
    ExpectContains(globals_css, "auction-transition");
    ExpectContains(globals_css, "tabular-nums");

    std::cout << "auction_app_ui_acceptance passed\n";
    return 0;
}
