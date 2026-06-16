import { expect, test } from "@playwright/test";
import type { Page, Route } from "@playwright/test";

type DemoUser = {
  id: number;
  username: string;
  password: string;
  nickname: string;
  role: "ADMIN" | "USER" | "SUPPORT";
};

type DemoState = {
  itemStatus: "NONE" | "PENDING_AUDIT" | "READY_FOR_AUCTION";
  auctionCreated: boolean;
  currentPrice: number;
  highestBidder: string;
  bids: Array<{ id: number; amount: number; bidder: string; status: "WINNING" | "OUTBID" }>;
  orderCreated: boolean;
  orderStatus: "PENDING_PAYMENT" | "PAID" | "SHIPPED" | "COMPLETED" | "REVIEWED";
  notificationRead: boolean;
};

const demoImageUrl = "http://127.0.0.1:3000/demo-auction.svg";
const demoTitle = "答辩演示 iPad Air 5 64G 蓝色";
const demoDescription = "99 新，无拆修，支持犀浦校区当面验货，适合课程答辩完整链路演示。";

const users: Record<string, DemoUser> = {
  admin: { id: 1, username: "admin", password: "Admin@123", nickname: "管理员", role: "ADMIN" },
  seller_demo: { id: 2, username: "seller_demo", password: "Seller@123", nickname: "演示卖家", role: "USER" },
  buyer_demo: { id: 3, username: "buyer_demo", password: "Buyer@123", nickname: "演示买家", role: "USER" },
  viewer_demo: { id: 4, username: "viewer_demo", password: "Viewer@123", nickname: "第二买家", role: "USER" },
};

const envelope = <T,>(data: T) => ({ code: 0, message: "OK", data });

function userFromRequest(route: Route): DemoUser | null {
  const header = route.request().headers().authorization;
  const token = header?.replace(/^Bearer\s+/i, "");
  if (!token?.startsWith("token:")) return null;
  return users[token.slice("token:".length)] ?? null;
}

async function bodyOf<T>(route: Route): Promise<T> {
  return route.request().postDataJSON() as T;
}

async function fulfillJson<T>(route: Route, data: T, status = 200) {
  await route.fulfill({
    status,
    contentType: "application/json",
    body: JSON.stringify(status >= 400 ? data : envelope(data)),
  });
}

function auctionSummary(state: DemoState) {
  return {
    auctionId: 7001,
    itemId: 501,
    title: demoTitle,
    categoryName: "数码设备",
    coverImageUrl: demoImageUrl,
    status: "RUNNING",
    startPrice: 300,
    currentPrice: state.currentPrice,
    bidStep: 20,
    sellerUsername: "seller_demo",
    sellerRating: 4.9,
    sellerDeals: 12,
    watcherCount: 28,
    tradeMode: "MEETUP",
    location: "犀浦校区东门",
    tagsJson: JSON.stringify(["99新", "可当面验货", "答辩演示"]),
    description: demoDescription,
    startTime: "2026-06-16 10:00:00",
    endTime: "2026-06-16 23:30:00",
    acceptingBids: true,
  };
}

function orderRecord(state: DemoState) {
  return {
    orderId: 9001,
    orderNo: "DEMO-ORDER-9001",
    auctionId: 7001,
    buyerId: users.viewer_demo.id,
    sellerId: users.seller_demo.id,
    finalAmount: state.currentPrice,
    orderStatus: state.orderStatus,
    payDeadlineAt: "2026-06-16T23:59:00Z",
    paidAt: state.orderStatus === "PENDING_PAYMENT" ? "" : "2026-06-16T10:20:00Z",
    shippedAt: ["SHIPPED", "COMPLETED", "REVIEWED"].includes(state.orderStatus) ? "2026-06-16T10:30:00Z" : "",
    completedAt: ["COMPLETED", "REVIEWED"].includes(state.orderStatus) ? "2026-06-16T10:40:00Z" : "",
    createdAt: "2026-06-16T10:15:00Z",
    itemTitle: demoTitle,
    coverImageUrl: demoImageUrl,
    buyerUsername: "viewer_demo",
    sellerUsername: "seller_demo",
    latestPayment: state.orderStatus === "PENDING_PAYMENT" ? null : {
      paymentId: 9901,
      payStatus: "SUCCESS",
      payChannel: "MOCK_WECHAT",
      paidAt: "2026-06-16T10:20:00Z",
    },
  };
}

async function setupDefenseApi(page: Page, state: DemoState) {
  await page.route("**/api/**", async (route) => {
    const request = route.request();
    const url = new URL(request.url());
    const path = url.pathname;
    const method = request.method();
    const user = userFromRequest(route);

    if (path === "/api/auth/register" && method === "POST") {
      const body = await bodyOf<{ username: string; password: string; nickname?: string }>(route);
      users[body.username] = {
        id: body.username === "viewer_demo" ? 4 : 20,
        username: body.username,
        password: body.password,
        nickname: body.nickname || body.username,
        role: "USER",
      };
      await fulfillJson(route, {
        user_id: users[body.username].id,
        username: body.username,
        role_code: "USER",
        status: "ACTIVE",
      });
      return;
    }

    if (path === "/api/auth/login" && method === "POST") {
      const body = await bodyOf<{ username: string; password: string }>(route);
      const loginUser = users[body.username];
      if (!loginUser || loginUser.password !== body.password) {
        await fulfillJson(route, { code: 4102, message: "invalid credentials" }, 401);
        return;
      }
      await fulfillJson(route, {
        token: `token:${loginUser.username}`,
        expire_at: "2026-06-17T00:00:00Z",
        user_info: {
          user_id: loginUser.id,
          username: loginUser.username,
          nickname: loginUser.nickname,
          role_code: loginUser.role,
          status: "ACTIVE",
        },
      });
      return;
    }

    if (path === "/api/auth/me" && method === "GET") {
      if (!user) {
        await fulfillJson(route, { code: 401, message: "unauthorized" }, 401);
        return;
      }
      await fulfillJson(route, {
        user_id: user.id,
        username: user.username,
        nickname: user.nickname,
        role_code: user.role,
        status: "ACTIVE",
      });
      return;
    }

    if (path === "/api/auth/logout" && method === "POST") {
      await fulfillJson(route, { message: "OK" });
      return;
    }

    if (path === "/api/items" && method === "POST") {
      state.itemStatus = "PENDING_AUDIT";
      const body = await bodyOf<{ title: string; start_price: number; suggested_bid_step: number }>(route);
      expect(body.title).toBe(demoTitle);
      expect(body.start_price).toBe(300);
      expect(body.suggested_bid_step).toBe(20);
      await fulfillJson(route, { item_id: 501, item_status: "DRAFT", created_at: "2026-06-16T10:00:00Z" });
      return;
    }

    if (path === "/api/items/501/images" && method === "POST") {
      const body = await bodyOf<{ image_url: string; is_cover?: boolean }>(route);
      expect(body.image_url).toBe(demoImageUrl);
      await fulfillJson(route, {
        image_id: 601,
        item_id: 501,
        image_url: body.image_url,
        sort_no: 1,
        is_cover: true,
        created_at: "2026-06-16T10:01:00Z",
      });
      return;
    }

    if (path === "/api/items/501/submit-review" && method === "POST") {
      state.itemStatus = "PENDING_AUDIT";
      await fulfillJson(route, { item_id: 501, item_status: "PENDING_AUDIT", submitted_at: "2026-06-16T10:02:00Z" });
      return;
    }

    if (path === "/api/admin/items/pending" && method === "GET") {
      await fulfillJson(route, state.itemStatus === "PENDING_AUDIT" ? [{
        item_id: 501,
        seller_id: users.seller_demo.id,
        seller_username: "seller_demo",
        seller_nickname: "演示卖家",
        category_id: 1,
        title: demoTitle,
        start_price: 300,
        cover_image_url: demoImageUrl,
        created_at: "2026-06-16T10:02:00Z",
      }] : []);
      return;
    }

    if (path === "/api/admin/items/501/approve" && method === "POST") {
      state.itemStatus = "READY_FOR_AUCTION";
      await fulfillJson(route, {
        item_id: 501,
        old_status: "PENDING_AUDIT",
        new_status: "READY_FOR_AUCTION",
        audit_result: "APPROVED",
        audited_at: "2026-06-16T10:05:00Z",
      });
      return;
    }

    if (path === "/api/admin/auctions" && method === "POST") {
      const body = await bodyOf<{ itemId: number; startPrice: number; bidStep: number }>(route);
      expect(body.itemId).toBe(501);
      expect(body.startPrice).toBe(300);
      expect(body.bidStep).toBe(20);
      state.auctionCreated = true;
      state.currentPrice = 300;
      await fulfillJson(route, {
        auctionId: 7001,
        itemId: 501,
        status: "RUNNING",
        currentPrice: 300,
        createdAt: "2026-06-16T10:06:00Z",
      });
      return;
    }

    if (path === "/api/admin/auctions" && method === "GET") {
      await fulfillJson(route, {
        list: state.auctionCreated ? [{
          auctionId: 7001,
          itemId: 501,
          sellerId: users.seller_demo.id,
          title: demoTitle,
          coverImageUrl: demoImageUrl,
          status: "RUNNING",
          startPrice: 300,
          currentPrice: state.currentPrice,
          bidStep: 20,
          highestBidderId: state.highestBidder ? users[state.highestBidder].id : null,
          startTime: "2026-06-16T10:00:00Z",
          endTime: "2026-06-16T23:30:00Z",
          updatedAt: "2026-06-16T10:06:00Z",
        }] : [],
        total: state.auctionCreated ? 1 : 0,
        pageNo: 1,
        pageSize: 20,
      });
      return;
    }

    if (path === "/api/auctions" && method === "GET") {
      await fulfillJson(route, {
        list: state.auctionCreated ? [auctionSummary(state)] : [],
        total: state.auctionCreated ? 1 : 0,
        pageNo: 1,
        pageSize: 24,
      });
      return;
    }

    if (path === "/api/auctions/7001" && method === "GET") {
      await fulfillJson(route, {
        ...auctionSummary(state),
        antiSnipingWindowSeconds: 300,
        extendSeconds: 180,
        highestBidderMasked: state.highestBidder ? `${state.highestBidder.slice(0, 1)}***` : "",
      });
      return;
    }

    if (path === "/api/auctions/7001/bids" && method === "GET") {
      await fulfillJson(route, {
        list: state.bids.map((bid) => ({
          bidId: bid.id,
          bidAmount: bid.amount,
          bidStatus: bid.status,
          bidTime: "2026-06-16T10:12:00Z",
          bidderMasked: `${bid.bidder.slice(0, 1)}***`,
        })),
        total: state.bids.length,
        pageNo: 1,
        pageSize: 20,
      });
      return;
    }

    if (path === "/api/auctions/7001/bids" && method === "POST") {
      const body = await bodyOf<{ bid_amount: number }>(route);
      expect(user?.role).toBe("USER");
      state.bids = state.bids.map((bid) => ({ ...bid, status: "OUTBID" }));
      state.currentPrice = body.bid_amount;
      state.highestBidder = user?.username ?? "buyer_demo";
      const bid = {
        id: 8000 + state.bids.length + 1,
        amount: body.bid_amount,
        bidder: state.highestBidder,
        status: "WINNING" as const,
      };
      state.bids = [bid, ...state.bids];
      if (state.highestBidder === "viewer_demo") {
        state.orderCreated = true;
        state.orderStatus = "PENDING_PAYMENT";
      }
      await fulfillJson(route, {
        bid_id: bid.id,
        auction_id: 7001,
        bid_amount: bid.amount,
        bid_status: "WINNING",
        current_price: state.currentPrice,
        highest_bidder_masked: "v***",
        end_time: "2026-06-16T23:30:00Z",
        extended: false,
        server_time: "2026-06-16T10:12:00Z",
      });
      return;
    }

    if (path === "/api/orders/mine" && method === "GET") {
      const records = state.orderCreated && user && ["viewer_demo", "seller_demo"].includes(user.username)
        ? [orderRecord(state)]
        : [];
      await fulfillJson(route, { records, pageNo: 1, pageSize: 20 });
      return;
    }

    if (path === "/api/orders/9001" && method === "GET") {
      const order = orderRecord(state);
      await fulfillJson(route, {
        order_id: order.orderId,
        order_no: order.orderNo,
        auction_id: order.auctionId,
        item_title: order.itemTitle,
        buyer_id: order.buyerId,
        seller_id: order.sellerId,
        final_amount: order.finalAmount,
        order_status: order.orderStatus,
        pay_deadline_at: order.payDeadlineAt,
        paid_at: order.paidAt,
        created_at: order.createdAt,
      });
      return;
    }

    if (path === "/api/orders/9001/pay" && method === "POST") {
      const body = await bodyOf<{ confirm_success?: boolean; pay_channel?: string }>(route);
      expect(body.confirm_success).toBe(true);
      expect(body.pay_channel).toBe("MOCK_WECHAT");
      state.orderStatus = "PAID";
      await fulfillJson(route, {
        payment_id: 9901,
        order_id: 9001,
        order_no: "DEMO-ORDER-9001",
        payment_no: "DEMO-PAY-9901",
        pay_channel: "MOCK_WECHAT",
        pay_amount: state.currentPrice,
        pay_status: "SUCCESS",
        pay_url: "mockpay://checkout",
        expire_at: "2026-06-16T23:59:00Z",
        reused_existing: false,
      });
      return;
    }

    if (path === "/api/orders/9001/ship" && method === "POST") {
      state.orderStatus = "SHIPPED";
      await fulfillJson(route, { order_id: 9001, old_status: "PAID", new_status: "SHIPPED" });
      return;
    }

    if (path === "/api/orders/9001/confirm-receipt" && method === "POST") {
      state.orderStatus = "COMPLETED";
      await fulfillJson(route, { order_id: 9001, old_status: "SHIPPED", new_status: "COMPLETED" });
      return;
    }

    if (path === "/api/reviews" && method === "POST") {
      const body = await bodyOf<{ orderId: number; rating: number; content: string }>(route);
      expect(body.orderId).toBe(9001);
      expect(body.rating).toBe(5);
      state.orderStatus = "REVIEWED";
      await fulfillJson(route, {
        reviewId: 10001,
        orderId: 9001,
        reviewerId: users.viewer_demo.id,
        revieweeId: users.seller_demo.id,
        reviewType: "BUYER_TO_SELLER",
        rating: body.rating,
        content: body.content,
        createdAt: "2026-06-16T10:45:00Z",
        orderStatusAfter: "REVIEWED",
        orderMarkedReviewed: true,
      });
      return;
    }

    if (path === "/api/notifications" && method === "GET") {
      const hasNotification = state.orderStatus === "REVIEWED" && user?.username === "seller_demo";
      await fulfillJson(route, {
        list: hasNotification ? [{
          notificationId: 12001,
          noticeType: "ORDER_REVIEW_RECEIVED",
          title: "收到新的交易评价",
          content: `${demoTitle} 的买家已提交 5 星评价`,
          bizType: "ORDER",
          bizId: 9001,
          readStatus: state.notificationRead ? "READ" : "UNREAD",
          pushStatus: "SENT",
          createdAt: "2026-06-16T10:45:00Z",
          readAt: state.notificationRead ? "2026-06-16T10:46:00Z" : "",
        }] : [],
        total: hasNotification ? 1 : 0,
        unreadCount: hasNotification && !state.notificationRead ? 1 : 0,
        limit: 20,
      });
      return;
    }

    if (path === "/api/notifications/12001/read" && method === "PATCH") {
      state.notificationRead = true;
      await fulfillJson(route, {
        notificationId: 12001,
        readStatus: "READ",
        readAt: "2026-06-16T10:46:00Z",
      });
      return;
    }

    if (path === "/api/admin/statistics/daily" && method === "GET") {
      await fulfillJson(route, [{
        stat_id: 301,
        stat_date: "2026-06-16",
        auction_count: state.auctionCreated ? 1 : 0,
        sold_count: state.orderCreated ? 1 : 0,
        unsold_count: 0,
        bid_count: state.bids.length,
        gmv_amount: state.orderCreated ? state.currentPrice : 0,
        created_at: "2026-06-16T11:00:00Z",
      }]);
      return;
    }

    if (path === "/api/admin/ops/operation-logs" && method === "GET") {
      await fulfillJson(route, {
        list: [{ operationLogId: 1, moduleName: "auction", operationName: "defense_demo", bizKey: "auction:7001", result: "SUCCESS", detail: "答辩链路完成", createdAt: "2026-06-16T11:00:00Z" }],
        total: 1,
      });
      return;
    }

    if (path === "/api/admin/ops/task-logs" && method === "GET") {
      await fulfillJson(route, {
        list: [{ taskLogId: 1, taskType: "ORDER_SETTLEMENT", bizKey: "auction:7001", taskStatus: "SUCCESS", retryCount: 0, lastError: "", createdAt: "2026-06-16T11:00:00Z" }],
        total: 1,
      });
      return;
    }

    if (path === "/api/admin/ops/exceptions" && method === "GET") {
      await fulfillJson(route, { list: [], total: 0 });
      return;
    }

    if (path === "/api/admin/ops/notifications/retry" && method === "POST") {
      await fulfillJson(route, { scanned: 1, succeeded: 1, failed: 0, skipped: 0 });
      return;
    }

    if (path === "/api/admin/ops/compensations" && method === "POST") {
      const body = await bodyOf<{ compensationType: string }>(route);
      expect(body.compensationType).toBe("ORDER_SETTLEMENT");
      await fulfillJson(route, { scanned: 1, succeeded: 1, failed: 0, skipped: 0 });
      return;
    }

    await fulfillJson(route, { code: 404, message: `unhandled ${method} ${path}` }, 404);
  });
}

async function loginAs(page: Page, username: string, password: string) {
  await page.goto("/auth/login");
  await page.locator('input[name="username"]').fill(username);
  await page.locator('input[name="password"]').fill(password);
  await page.getByRole("button", { name: "进入拍卖大厅" }).click();
  await page.waitForURL("**/auction/hall");
}

test.describe("defense demo full browser workflow", () => {
  test("runs the complete defense flow from register to ops evidence", async ({ page }) => {
    test.setTimeout(120_000);

    const state: DemoState = {
      itemStatus: "NONE",
      auctionCreated: false,
      currentPrice: 300,
      highestBidder: "",
      bids: [],
      orderCreated: false,
      orderStatus: "PENDING_PAYMENT",
      notificationRead: false,
    };
    await setupDefenseApi(page, state);

    await page.goto("/auth/login");
    await page.getByRole("button", { name: "注册" }).click();
    await page.locator('input[name="username"]').fill("viewer_demo");
    await page.locator('input[name="nickname"]').fill("第二买家");
    await page.locator('input[name="email"]').fill("viewer_demo@example.test");
    await page.locator('input[name="phone"]').fill("13800000004");
    await page.locator('input[name="password"]').fill("Viewer@123");
    await page.getByRole("button", { name: "注册并进入大厅" }).click();
    await page.waitForURL("**/auction/hall");
    await expect(page.getByRole("heading", { name: "拍卖大厅" })).toBeVisible();

    await loginAs(page, "seller_demo", "Seller@123");
    await page.goto("/auction/publish");
    await page.getByLabel("拍品标题").fill(demoTitle);
    await page.getByLabel("分类 ID").fill("1");
    await page.getByLabel("拍品描述").fill(demoDescription);
    await page.getByRole("button", { name: "下一步" }).click();
    await page.getByPlaceholder("https://images.unsplash.com/...").fill(demoImageUrl);
    await page.getByRole("button", { name: "添加图片" }).click();
    await expect(page.getByText("图片已添加")).toBeVisible();
    await page.getByRole("button", { name: "下一步" }).click();
    await page.getByLabel("起拍价").fill("300");
    await page.getByLabel("建议最小加价幅度").fill("20");
    await page.getByLabel("建议延时保护窗口（秒）").fill("300");
    await page.getByLabel("建议顺延秒数").fill("180");
    await page.getByLabel("交付地点").fill("犀浦校区东门");
    await page.getByLabel("标签（逗号分隔）").fill("99新,可当面验货,答辩演示");
    await page.getByRole("button", { name: "下一步" }).click();
    await expect(page.getByText("图片数量")).toBeVisible();
    await page.getByRole("button", { name: "提交审核", exact: true }).click();
    await expect(page.getByText(/已进入审核队列/)).toBeVisible();

    await loginAs(page, "admin", "Admin@123");
    await page.goto("/admin/dashboard");
    await expect(page.getByRole("heading", { name: "审核与运维大盘" })).toBeVisible();
    await page.getByText(demoTitle).first().click();
    await expect(page.getByText("REVIEW DRAWER")).toBeVisible();
    await page.getByRole("button", { name: "批准" }).click();
    await expect(page.getByText("已批准")).toBeVisible();
    await page.getByRole("button", { name: "创建拍卖" }).click();
    await page.getByRole("button", { name: "确认创建拍卖" }).click();
    await expect(page.getByText(/已创建拍卖 #7001/)).toBeVisible();
    await page.getByRole("button", { name: "拍卖监控" }).click();
    await expect(page.getByRole("heading", { name: "拍卖监控" })).toBeVisible();
    await expect(page.getByText(demoTitle)).toBeVisible();

    await loginAs(page, "buyer_demo", "Buyer@123");
    await expect(page.getByRole("link", { name: new RegExp(demoTitle) })).toBeVisible();
    await page.getByRole("link", { name: new RegExp(demoTitle) }).click();
    await expect(page.getByRole("heading", { name: demoTitle })).toBeVisible();
    await page.locator('input[inputmode="numeric"]').fill("340");
    await page.getByRole("button", { name: "PLACE BID" }).click();
    await expect(page.getByText("出价成功，权威价格已刷新")).toBeVisible();

    await loginAs(page, "viewer_demo", "Viewer@123");
    await page.goto("/auction/detail/7001");
    await page.locator('input[inputmode="numeric"]').fill("380");
    await page.getByRole("button", { name: "PLACE BID" }).click();
    await expect(page.getByText("出价成功，权威价格已刷新")).toBeVisible();
    await expect(page.getByText("¥380").first()).toBeVisible();

    await page.goto("/orders");
    await expect(page.getByRole("heading", { name: "我的订单" })).toBeVisible();
    await page.getByRole("link", { name: "支付" }).click();
    await expect(page.getByRole("heading", { name: "订单支付" })).toBeVisible();
    await page.getByRole("button", { name: "确认支付" }).click();
    await expect(page.getByRole("heading", { name: "支付成功" })).toBeVisible();

    await loginAs(page, "seller_demo", "Seller@123");
    await page.goto("/orders");
    await page.getByRole("button", { name: "发货" }).click();
    await expect(page.getByText("已确认发货")).toBeVisible();
    await expect(page.getByText("已发货")).toBeVisible();

    await loginAs(page, "viewer_demo", "Viewer@123");
    await page.goto("/orders");
    await page.getByRole("button", { name: "收货" }).click();
    await expect(page.getByText("已确认收货")).toBeVisible();
    await expect(page.getByText("已完成")).toBeVisible();
    await page.getByRole("button", { name: "评价" }).click();
    await expect(page.getByText("已评价")).toBeVisible();

    await loginAs(page, "seller_demo", "Seller@123");
    await page.goto("/notifications");
    await expect(page.getByRole("heading", { name: "站内通知" })).toBeVisible();
    await expect(page.getByRole("heading", { name: "收到新的交易评价" })).toBeVisible();
    await page.getByRole("button", { name: "已读" }).click();
    await expect(page.getByText("已标记为已读")).toBeVisible();

    await loginAs(page, "admin", "Admin@123");
    await page.goto("/admin/dashboard");
    await page.getByRole("button", { name: "统计报表" }).click();
    await expect(page.getByText("今日 GMV")).toBeVisible();
    await expect(page.getByText("¥380").first()).toBeVisible();
    await page.getByRole("button", { name: "运维终端" }).click();
    await expect(page.getByRole("heading", { name: "操作日志" })).toBeVisible();
    await page.getByRole("button", { name: "订单结算补偿" }).click();
    await expect(page.getByText("补偿：成功 1，跳过 0")).toBeVisible();
    await page.getByRole("button", { name: "通知重试" }).click();
    await expect(page.getByText("通知：成功 1，失败 0")).toBeVisible();
  });
});
