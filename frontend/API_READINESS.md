# 前端 API Readiness

本文档记录前端各接口的接入状态和后端 HTTP 控制器缺口。

## 接口接入矩阵

| 接口 | 方法 | 前端使用处 | Mock 状态 | Live 状态 | 后端控制器 |
|---|---|---|---|---|---|
| `/api/auth/login` | POST | `/auth/login` | 已实现 | 已接入 | `src/access/http/auth_http.cpp` |
| `/api/auth/me` | GET | 全局会话恢复 | 已实现 | 已接入 | `src/access/http/auth_http.cpp` |
| `/api/auctions` | GET | `/`、`/auction/hall` | 已实现 | 已接入 | `src/access/http/auction_http.cpp` |
| `/api/auctions/{id}` | GET | `/auction/detail/[id]` | 已实现 | 已接入 | `src/access/http/auction_http.cpp` |
| `/api/auctions/{id}/bids` | GET | 出价历史墙 | 已实现 | 待接入 | 待创建 |
| `/api/auctions/{id}/bids` | POST | `PLACE BID` | 已实现（含 409/429） | 待接入 | 待创建 |
| `/ws/auction/{id}` | WS | 详情实时价格 | 未实现（轮询降级） | 待接入 | 待创建 |
| `/api/items` | POST | `/auction/publish` | 已实现 | 待接入 | 待创建 |
| `/api/orders/{id}` | GET | `/checkout/[orderId]` | 已实现 | 待接入 | 待创建 |
| `/api/orders/{id}/pay` | POST | `/checkout/[orderId]` | 已实现 | 待接入 | 待创建 |
| `/api/admin/items/pending` | GET | `/admin/dashboard` | 已实现 | 待接入 | 待创建 |
| `/api/admin/statistics/daily` | GET | `/admin/dashboard` | 未实现 | 待接入 | 待创建 |

## 后端 HTTP 控制器缺口

当前后端 v1.0 具备完整的服务层（`src/modules/`）和仓储层，但 Drogon HTTP 控制器尚未批量接入。以下控制器需要在 F17 阶段逐步补齐：

### Auth 控制器（已接入）
- `POST /api/auth/login` - 用户登录 ✅
- `GET /api/auth/me` - 获取当前用户信息 ✅
- `POST /api/auth/register` - 用户注册 ✅
- `POST /api/auth/logout` - 用户登出 ✅

### Auction 控制器
- `GET /api/auctions` - 拍卖列表（支持筛选） ✅
- `GET /api/auctions/{id}` - 拍卖详情 ✅
- `POST /api/auctions` - 创建拍卖（管理员）

### Bid 控制器
- `GET /api/auctions/{id}/bids` - 出价历史
- `POST /api/auctions/{id}/bids` - 提交出价

### Item 控制器
- `POST /api/items` - 创建拍品
- `PUT /api/items/{id}` - 修改拍品
- `POST /api/items/{id}/submit-review` - 提交审核

### Order 控制器
- `GET /api/orders/{id}` - 订单详情
- `POST /api/orders/{id}/pay` - 发起支付

### Admin 控制器
- `GET /api/admin/items/pending` - 待审拍品列表
- `POST /api/admin/items/{id}/approve` - 审核通过
- `POST /api/admin/items/{id}/reject` - 审核驳回
- `GET /api/admin/statistics/daily` - 日报统计

### WebSocket
- `/ws/auction/{id}` - 拍卖实时价格推送

## F17 接入顺序建议

1. **Auth** - 登录/会话恢复，前端可先跑通认证流程
2. **Auction list/detail** - 拍卖列表和详情，前端首页和大厅可切换 live
3. **Bid** - 出价接口，竞价详情页核心功能
4. **Publish** - 拍品发布和审核
5. **Checkout** - 订单查询和支付
6. **Admin** - 管理后台数据
7. **WebSocket** - 实时价格推送，替换轮询降级

## 当前状态

- **Mock 模式**: 完整可用，7 个页面均可通过 mock 数据正常交互；登录态会写入本地 token，导航会话恢复可用
- **Live 模式**: Auth 登录、会话恢复、登出和注册已接入真实 Drogon 控制器；Auction 列表和详情已接入真实 Drogon 控制器；Bid、Publish、Checkout、Admin 和 WebSocket 仍等待 F17 后续接入
- **WebSocket**: 前端已有降级 UI（Amber 横幅 + 轮询），等待 F17 接入真实通道
