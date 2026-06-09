# 前端 API Readiness

本文档记录 `frontend/` 当前真实后端接入状态。

## 接口接入矩阵

| 接口 | 方法 | 前端使用处 | 状态 | 后端控制器 |
|---|---|---|---|---|
| `/api/auth/login` | POST | `/auth/login` | 已接入 | `src/access/http/auth_http.cpp` |
| `/api/auth/me` | GET | 全局会话恢复 | 已接入 | `src/access/http/auth_http.cpp` |
| `/api/auctions` | GET | `/`、`/auction/hall` | 已接入，支持大厅分类/价格/信用/交易方式筛选 | `src/access/http/auction_public_http.cpp` |
| `/api/auctions/{id}` | GET | `/auction/detail/[id]` | 已接入 | `src/access/http/auction_http.cpp` |
| `/api/auctions/{id}/bids` | GET | 出价历史墙 | 已接入 | `src/access/http/bid_http.cpp` |
| `/api/auctions/{id}/bids` | POST | `PLACE BID` | 已接入 | `src/access/http/bid_http.cpp` |
| `/ws/auction/{id}` | WS | 详情实时价格 | 已接入，断线自动降级轮询 | `src/access/http/auction_ws.cpp` |
| `/api/items` | POST | `/auction/publish` | 已接入 | `src/access/http/item_http.cpp` |
| `/api/items/{id}/images` | POST | `/auction/publish` | 已接入 | `src/access/http/item_http.cpp` |
| `/api/items/{id}/submit-review` | POST | `/auction/publish` | 已接入 | `src/access/http/item_http.cpp` |
| `/api/items/mine` | GET | `/account/items` | 已接入 | `src/access/http/item_http.cpp` |
| `/api/items/{id}/offline` | POST | `/account/items` | 已接入 | `src/access/http/item_http.cpp` |
| `/api/orders/mine` | GET | `/orders` | 已接入 | `src/access/http/order_http.cpp` |
| `/api/orders/{id}` | GET | `/checkout/[orderId]` | 已接入 | `src/access/http/order_http.cpp` |
| `/api/orders/{id}/pay` | POST | `/checkout/[orderId]` | 已接入，前端传 `confirm_success=true` 走虚拟支付成功闭环 | `src/access/http/order_http.cpp` |
| `/api/orders/{id}/ship` | POST | `/orders` | 已接入 | `src/access/http/order_http.cpp` |
| `/api/orders/{id}/confirm-receipt` | POST | `/orders` | 已接入 | `src/access/http/order_http.cpp` |
| `/api/reviews` | POST | `/orders` | 已接入 | `src/access/http/review_http.cpp` |
| `/api/notifications` | GET | `/notifications`、全局导航、右下角弹窗 | 已接入，支持未读筛选和红点计数 | `src/access/http/notification_http.cpp` |
| `/api/notifications/{id}/read` | PATCH | `/notifications` | 已接入，已补齐 CORS 预检和命名占位符路由 | `src/access/http/notification_http.cpp` |
| `/api/admin/items/pending` | GET | `/admin/dashboard` | 已接入 | `src/access/http/admin_http.cpp` |
| `/api/admin/items/{id}/approve` | POST | `/admin/dashboard` | 已接入 | `src/access/http/admin_http.cpp` |
| `/api/admin/items/{id}/reject` | POST | `/admin/dashboard` | 已接入 | `src/access/http/admin_http.cpp` |
| `/api/admin/statistics/daily` | GET | `/admin/dashboard` | 已接入 | `src/access/http/admin_http.cpp` |
| `/api/admin/ops/operation-logs` | GET | `/admin/dashboard` | 已接入 | `src/access/http/ops_http.cpp` |
| `/api/admin/ops/task-logs` | GET | `/admin/dashboard` | 已接入 | `src/access/http/ops_http.cpp` |
| `/api/admin/ops/exceptions` | GET | `/admin/dashboard` | 已接入 | `src/access/http/ops_http.cpp` |
| `/api/admin/ops/notifications/retry` | POST | `/admin/dashboard` | 已接入 | `src/access/http/ops_http.cpp` |
| `/api/admin/ops/compensations` | POST | `/admin/dashboard` | 已接入 | `src/access/http/ops_http.cpp` |

## 当前状态

- 前端默认只连接真实后端，不再保留 Mock 主路径
- Auth、Auction、Bid、Publish、Seller Items、Orders、Notifications、Checkout、Admin Ops 和 WebSocket 已接入真实 Drogon 控制器
- 拍卖大厅筛选项已对齐后端真实字段：分类、价格区间、卖家评分、有成交记录和交易方式都会进入 `/api/auctions` 查询参数
- Checkout 当前使用本地虚拟支付确认，点击“确认支付”后由后端 mock 成功回调推进订单到 `PAID`
- 全局导航会轮询未读通知并显示红点计数，新增未读通知会在右下角弹窗提示；通知页点击“已读”走真实 `PATCH /api/notifications/{id}/read`
- 详情页优先使用真实 `/ws/auction/{id}`，连接失败时自动降级到轮询
