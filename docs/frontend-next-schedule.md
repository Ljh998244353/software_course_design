# Next.js 前端最终接入状态

## 1. 定位

本文档记录 `frontend/` 当前真实前端交付状态。它不再作为 F16/F17 的逐步开发流水使用；历史阶段只保留摘要，具体事实以当前代码、[frontend/API_READINESS.md](/home/ljh/project/soft_course_design/frontend/API_READINESS.md) 和各模块文档为准。

当前口径：

- 前端唯一主入口为 `frontend/`
- 页面默认连接真实 Drogon HTTP 与 WebSocket
- 不再保留前端 Mock 主路径
- 支付仅保留本地测试支付模型，用于演示订单支付闭环
- MySQL 仍是拍卖、出价、订单、支付等交易事实唯一来源

## 2. 技术栈

- 工程骨架：Next.js 16 App Router
- UI 运行时：React 19
- 语言：TypeScript
- 样式：Tailwind CSS 3.4+
- 动画：Framer Motion
- 服务端状态：`@tanstack/react-query`
- 包管理：npm
- Node.js 要求：`>=20.9.0`
- 默认目录：`frontend/`
- 构建脚本：`next build --webpack`

## 3. 当前页面

| 页面 | 路由 | 主要能力 |
|---|---|---|
| 公共门户首页 | `/` | 活动入口、搜索、分类、运行中拍卖展示 |
| 认证中心 | `/auth/login` | 登录、注册、会话恢复 |
| 拍卖大厅 | `/auction/hall` | URL 驱动搜索筛选、活动列表、空态分流 |
| 竞价详情 | `/auction/detail/[id]` | 详情、出价历史、真实出价、WebSocket/轮询降级 |
| 拍品发布 | `/auction/publish` | 发布向导、草稿、图片 URL 校验、提交审核 |
| 我的拍品 | `/account/items` | 卖家拍品列表、下架入口 |
| 我的订单 | `/orders` | 订单列表、发货、确认收货、评价入口 |
| 订单支付 | `/checkout/[orderId]` | 本地测试支付确认、订单状态推进 |
| 通知中心 | `/notifications` | 通知列表、未读筛选、已读操作 |
| 管理大盘 | `/admin/dashboard` | 审核、建拍、统计、运维日志、补偿与通知重试 |

当前答辩演示还包含一个本地静态图片资产：

- `frontend/public/demo-auction.svg`：发布拍品页面的稳定图片 URL，现场填写 `http://127.0.0.1:3000/demo-auction.svg`

## 4. 真实接口状态

前端使用的真实接口矩阵维护在 [frontend/API_READINESS.md](/home/ljh/project/soft_course_design/frontend/API_READINESS.md)。

当前已接入能力包括：

- Auth：注册、登录、登出、当前用户
- Auction：公开列表、详情、当前价格、管理端建拍/修改/取消
- Bid：出价历史、提交出价、实时价格推送
- Item：发布、图片元数据、提交审核、我的拍品、下架
- Order：订单列表、订单详情、支付、发货、确认收货
- Review：订单评价
- Notification：通知列表、未读红点、右下角弹窗、已读
- Statistics：管理端统计日报
- Ops：操作日志、任务日志、异常列表、通知重试、补偿入口
- WebSocket：`/ws/auction/{id}`，连接失败时自动降级轮询

## 5. 已完成阶段摘要

- `F16-0`：工程骨架、Tailwind 设计系统、全局 Provider、首页完成
- `F16-1`：计划内 7 个物理页面完成
- `F16-2`：竞价详情状态机、乐观出价、409 回滚、429 冷却和实时降级 UI 完成
- `F16-3`：发布、支付、管理交互闭环完成
- `F16-4`：前端运行说明、API readiness 和本地验证记录完成
- `F16-5`：Next.js 16、React 19、安全升级和 webpack 构建路径完成
- `F17`：真实 HTTP、WebSocket、订单履约、评价、通知、统计和运维入口接入完成

## 6. 当前验证命令

```bash
cd frontend
npm run typecheck
npm run build
npm run test:e2e
npm run test:e2e -- tests/e2e/defense-flow.spec.ts
```

涉及后端接口或全链路时补充：

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
scripts/test.sh http
scripts/test.sh risk
scripts/deploy/verify_release.sh
```

## 7. 维护规则

1. 前端新增页面或接口时，同步更新本文档和 [frontend/API_READINESS.md](/home/ljh/project/soft_course_design/frontend/API_READINESS.md)。
2. 当前页面不得重新引入前端 Mock 主路径；测试支付除外。
3. 文档中的当前主入口统一写作 `frontend/` 或 `http://127.0.0.1:3000`。
4. 历史 `/app` 和 `/demo` 只能作为旧阶段说明，不得写成当前验收入口。
5. 交易事实、出价合法性、订单状态和支付状态以 MySQL 后端确认为准，前端乐观 UI 不能替代服务端结果。
6. 答辩浏览器链路维护在 `frontend/tests/e2e/defense-flow.spec.ts`；若页面按钮文案、路由或接口契约变化，必须同步更新该用例和 [答辩演示操作手册.md](/home/ljh/project/soft_course_design/docs/答辩演示操作手册.md)。
