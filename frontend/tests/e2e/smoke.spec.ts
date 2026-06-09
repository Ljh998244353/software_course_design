import { expect, test } from "@playwright/test";
import type { Page } from "@playwright/test";

async function mockAuthenticatedApi(page: Page, role = "USER") {
  const envelope = <T,>(data: T) => ({ code: 0, message: "OK", data });
  await page.addInitScript(() => {
    window.sessionStorage.setItem("auth_token", "playwright-token");
  });
  await page.route("**/api/auth/me", (route) =>
    route.fulfill({
      contentType: "application/json",
      body: JSON.stringify(envelope({
        user_id: role === "ADMIN" ? 1 : 2,
        username: role === "ADMIN" ? "admin" : "seller",
        nickname: role === "ADMIN" ? "管理员" : "卖家",
        role_code: role,
        status: "ACTIVE",
      })),
    }),
  );
}

test("home page exposes the auction entry points", async ({ page }) => {
  await page.goto("/");

  await expect(page.getByRole("link", { name: "进入拍卖大厅" })).toBeVisible();
  await expect(page.getByRole("link", { name: "发布拍品" }).first()).toBeVisible();
});

test("login page renders the real account form", async ({ page }) => {
  await page.goto("/auth/login");

  await expect(page.getByRole("heading", { name: "登录 / 注册" })).toBeVisible();
  await expect(page.locator('input[name="username"]')).toBeVisible();
  await expect(page.locator('input[name="password"]')).toBeVisible();
  await expect(page.getByRole("button", { name: "进入拍卖大厅" })).toBeVisible();
  await page.getByRole("button", { name: "注册" }).click();
  await expect(page.locator('input[name="nickname"]')).toBeVisible();
  await expect(page.locator('input[name="email"]')).toBeVisible();
  await expect(page.getByRole("button", { name: "注册并进入大厅" })).toBeVisible();
});

test("auction hall route is available", async ({ page }) => {
  await page.goto("/auction/hall");

  await expect(page.getByRole("heading", { name: "拍卖大厅" })).toBeVisible();
  await expect(page.getByText("详情页优先使用 WebSocket 实时通道")).toBeVisible();
});

test("seller item offline workflow is reachable", async ({ page }) => {
  await mockAuthenticatedApi(page);
  await page.route("**/api/items/mine", (route) =>
    route.fulfill({
      contentType: "application/json",
      body: JSON.stringify({
        code: 0,
        message: "OK",
        data: {
          items: [{
            item_id: 91,
            seller_id: 2,
            category_id: 1,
            title: "可下架拍品",
            start_price: 120,
            item_status: "READY_FOR_AUCTION",
            reject_reason: "",
            cover_image_url: "https://images.unsplash.com/photo-1516321318423-f06f85e504b3?auto=format&fit=crop&w=900&q=80",
            updated_at: "2026-06-09T10:00:00Z",
          }],
          total: 1,
        },
      }),
    }),
  );
  await page.route("**/api/items/91/offline", (route) =>
    route.fulfill({
      contentType: "application/json",
      body: JSON.stringify({ code: 0, message: "OK", data: { item_id: 91, item_status: "OFFLINE", updated_at: "2026-06-09T10:01:00Z" } }),
    }),
  );

  await page.goto("/account/items");
  await expect(page.getByRole("heading", { name: "我的拍品" })).toBeVisible();
  await expect(page.getByRole("button", { name: "下架" })).toBeVisible();
});

test("order, review and notification entries are reachable", async ({ page }) => {
  await mockAuthenticatedApi(page);
  await page.route("**/api/orders/mine**", (route) =>
    route.fulfill({
      contentType: "application/json",
      body: JSON.stringify({
        code: 0,
        message: "OK",
        data: {
          records: [{
            orderId: 7,
            orderNo: "ORD7",
            auctionId: 3,
            buyerId: 4,
            sellerId: 2,
            finalAmount: 260,
            orderStatus: "COMPLETED",
            payDeadlineAt: "2026-06-09T11:00:00Z",
            paidAt: "2026-06-09T10:00:00Z",
            shippedAt: "2026-06-09T10:20:00Z",
            completedAt: "2026-06-09T10:40:00Z",
            createdAt: "2026-06-09T09:00:00Z",
            itemTitle: "已完成订单",
            coverImageUrl: "https://images.unsplash.com/photo-1516321318423-f06f85e504b3?auto=format&fit=crop&w=900&q=80",
            buyerUsername: "buyer",
            sellerUsername: "seller",
            latestPayment: null,
          }],
          pageNo: 1,
          pageSize: 20,
        },
      }),
    }),
  );
  await page.route("**/api/notifications**", (route) =>
    route.fulfill({
      contentType: "application/json",
      body: JSON.stringify({
        code: 0,
        message: "OK",
        data: {
          list: [{
            notificationId: 5,
            noticeType: "OUTBID",
            title: "价格变更",
            content: "你的关注拍卖出现新出价",
            bizType: "AUCTION",
            bizId: 3,
            readStatus: "UNREAD",
            pushStatus: "SENT",
            createdAt: "2026-06-09T10:00:00Z",
            readAt: "",
          }],
          total: 1,
          unreadCount: 1,
          limit: 20,
        },
      }),
    }),
  );

  await page.goto("/orders");
  await expect(page.getByRole("heading", { name: "我的订单" })).toBeVisible();
  await expect(page.getByRole("button", { name: "评价" })).toBeVisible();

  await page.goto("/notifications");
  await expect(page.getByRole("heading", { name: "站内通知" })).toBeVisible();
  await expect(page.getByRole("button", { name: "已读" })).toBeVisible();
});

test("admin ops panel uses real API contract entries", async ({ page }) => {
  await mockAuthenticatedApi(page, "ADMIN");
  await page.route("**/api/admin/items/pending", (route) =>
    route.fulfill({ contentType: "application/json", body: JSON.stringify({ code: 0, message: "OK", data: [] }) }),
  );
  await page.route("**/api/admin/auctions**", (route) =>
    route.fulfill({ contentType: "application/json", body: JSON.stringify({ code: 0, message: "OK", data: { list: [], total: 0, pageNo: 1, pageSize: 20 } }) }),
  );
  await page.route("**/api/admin/statistics/daily**", (route) =>
    route.fulfill({ contentType: "application/json", body: JSON.stringify({ code: 0, message: "OK", data: [] }) }),
  );
  await page.route("**/api/admin/ops/operation-logs**", (route) =>
    route.fulfill({ contentType: "application/json", body: JSON.stringify({ code: 0, message: "OK", data: { list: [], total: 0 } }) }),
  );
  await page.route("**/api/admin/ops/task-logs**", (route) =>
    route.fulfill({ contentType: "application/json", body: JSON.stringify({ code: 0, message: "OK", data: { list: [], total: 0 } }) }),
  );
  await page.route("**/api/admin/ops/exceptions**", (route) =>
    route.fulfill({ contentType: "application/json", body: JSON.stringify({ code: 0, message: "OK", data: { list: [], total: 0 } }) }),
  );

  await page.goto("/admin/dashboard");
  await page.getByRole("button", { name: "运维终端" }).click();
  await expect(page.getByRole("heading", { name: "操作日志" })).toBeVisible();
  await expect(page.getByRole("button", { name: "通知重试" })).toBeVisible();
});
