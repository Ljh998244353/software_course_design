# 前端 API Readiness

本文档记录 `frontend/` 当前真实后端接入状态。

## 接口接入矩阵

| 接口 | 方法 | 前端使用处 | 状态 | 后端控制器 |
|---|---|---|---|---|
| `/api/auth/login` | POST | `/auth/login` | 已接入 | `src/access/http/auth_http.cpp` |
| `/api/auth/me` | GET | 全局会话恢复 | 已接入 | `src/access/http/auth_http.cpp` |
| `/api/auctions` | GET | `/`、`/auction/hall` | 已接入 | `src/access/http/auction_http.cpp` |
| `/api/auctions/{id}` | GET | `/auction/detail/[id]` | 已接入 | `src/access/http/auction_http.cpp` |
| `/api/auctions/{id}/bids` | GET | 出价历史墙 | 已接入 | `src/access/http/bid_http.cpp` |
| `/api/auctions/{id}/bids` | POST | `PLACE BID` | 已接入 | `src/access/http/bid_http.cpp` |
| `/ws/auction/{id}` | WS | 详情实时价格 | 已接入，断线自动降级轮询 | `src/access/http/auction_ws.cpp` |
| `/api/items` | POST | `/auction/publish` | 已接入 | `src/access/http/item_http.cpp` |
| `/api/items/{id}/images` | POST | `/auction/publish` | 已接入 | `src/access/http/item_http.cpp` |
| `/api/items/{id}/submit-review` | POST | `/auction/publish` | 已接入 | `src/access/http/item_http.cpp` |
| `/api/orders/{id}` | GET | `/checkout/[orderId]` | 已接入 | `src/access/http/order_http.cpp` |
| `/api/orders/{id}/pay` | POST | `/checkout/[orderId]` | 已接入 | `src/access/http/order_http.cpp` |
| `/api/admin/items/pending` | GET | `/admin/dashboard` | 已接入 | `src/access/http/admin_http.cpp` |
| `/api/admin/items/{id}/approve` | POST | `/admin/dashboard` | 已接入 | `src/access/http/admin_http.cpp` |
| `/api/admin/items/{id}/reject` | POST | `/admin/dashboard` | 已接入 | `src/access/http/admin_http.cpp` |
| `/api/admin/statistics/daily` | GET | `/admin/dashboard` | 已接入 | `src/access/http/admin_http.cpp` |

## 当前状态

- 前端默认只连接真实后端，不再保留 Mock 主路径
- Auth、Auction、Bid、Publish、Checkout、Admin 和 WebSocket 已接入真实 Drogon 控制器
- 详情页优先使用真实 `/ws/auction/{id}`，连接失败时自动降级到轮询
