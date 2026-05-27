# Next.js 前端 UI/UX 落地 Schedule

## 1. 定位

本文档是 `/home/ljh/project/soft_course_design` 在后端 `v1.0` 基线之后的前端落地计划，用于指导 `frontend/` 下的 Next.js 多页面前端工程建设。

本阶段目标不是立刻补齐全部 Drogon HTTP 控制器，而是先完成一个能直接看到前端效果、能本地运行、能展示完整 UI/UX 方向的富客户端骨架，然后再逐步切换到真实后端接口。

## 2. 总体策略

- 前端工程放在 `frontend/`，不混入 C++ `src/`，不改 CMake 构建链。
- 第一阶段采用契约 Mock 优先：默认 `NEXT_PUBLIC_API_MODE=mock`，用 typed mock adapter 对齐后端文档契约。
- 保留 `live` 模式：当后端 HTTP 控制器补齐后，前端可切换到真实 `/api/...` 与 `/ws/...`。
- `live` 模式下后端接口缺失必须显式展示“API route not ready”，不能静默降级为 Mock 成功。
- 先完成可视化体验闭环，再逐步接入真实后端：UI Skeleton -> Mock 数据 -> live API -> WebSocket -> 端到端联调。
- MySQL 仍是拍卖、出价、订单、支付等交易事实唯一来源；前端乐观更新只能改善体验，不能替代后端确认。

## 3. 技术栈

- 工程骨架：Next.js 16 App Router
- UI 运行时：React 19
- 语言：TypeScript
- 样式：Tailwind CSS 3.4+
- 动画：Framer Motion
- 服务端状态：`@tanstack/react-query`
- 包管理：npm
- Node.js 要求：`>=20.9.0`
- 默认目录：`frontend/`
- 后端 API 基线：C++20 + Drogon + MySQL，当前后端 `v1.0` 主要具备服务层和文档契约，真实 HTTP 控制器需要后续逐步接入。

## 4. 视觉设计要求归档

### 4.1 设计愿景

整体风格必须现代、大气、优雅、浅色调，面向“校园/小型社区在线拍卖平台”。视觉气质参考高可信金融交易台、Apple 式克制空间、Stripe 式清晰信息架构，但不能直接复制任何品牌页面。

前端不能只是静态皮肤，必须体现严谨状态管理、清晰错误处理、交易安全感和并发拍卖场景下的可恢复交互。

### 4.2 全局设计系统

- 应用背景：`Slate 50 (#F8FAFC)`，禁止纯白铺满全屏背景。
- Surface：`#FFFFFF`，通过白色卡片与浅灰蓝背景建立空间层级。
- Border：`Slate 200 (#E2E8F0)`，常规边框 `1px`。
- Primary：`Electric Indigo (#4F46E5)`，用于主按钮、焦点、关键进程引导。
- Success：`Emerald Green (#10B981)`，浅底 `#D1FAE5`。
- Warning：`Amber Gold (#F59E0B)`，浅底 `#FEF3C7`。
- Danger：`Rose Red (#E11D48)`，浅底 `#FFE4E6`。
- 主标题/核心数字：`Slate 900 (#0F172A)`。
- 正文/标签：`Slate 600 (#475569)`。
- 辅助文本/Placeholder：`Slate 400 (#94A3B8)`。
- 字体：优先系统无衬线族；若引入 Web 字体，需保证可离线或可降级，不依赖付费服务。
- 全站价格、倒计时、时间戳、毫秒延迟、出价变化数字必须使用 `font-variant-numeric: tabular-nums;`，避免实时刷新抖动。
- 间距遵循 8px 网格：4、8、16、24、32、64。
- 基础卡片阴影：`0 1px 3px 0 rgba(0, 0, 0, 0.05)`。
- 悬浮卡片/Sticky 控制台阴影：`0 10px 15px -3px rgba(0, 0, 0, 0.05), 0 4px 6px -4px rgba(0, 0, 0, 0.05)`。
- Toast/Modal 阴影：`0 25px 50px -12px rgba(0, 0, 0, 0.08)`，并使用毛玻璃背景。
- 全局缓动：`transition: all 0.3s cubic-bezier(0.16, 1, 0.3, 1);`。

### 4.3 强制 UX 韧性要求

- 全站异步加载优先使用与真实组件尺寸一致的 Skeleton Shimmer，不使用大面积空白或粗糙菊花图。
- WebSocket 断开或浏览器离线时，全局顶部下方展示 Amber 降级横幅。
- 竞价详情页在实时通道断开且价格可能不新鲜时，应锁定 `PLACE BID` 按钮或提示用户先同步权威价格。
- 接收到 `HTTP 409 Conflict` 时，必须执行乐观出价回滚并提示价格已被其他用户抢先更新。
- 接收到 `HTTP 429 Too Many Requests` 或用户 1 秒内连续加价超过 5 次时，必须展示限流冷却遮罩和倒计时。
- 所有关键交互支持键盘焦点、Enter 触发和 `aria-live` 状态播报。

## 5. 物理多页面路由要求

第一轮必须建立以下 7 个 App Router 物理页面：

| 页面 | 路由 | 目标 |
|---|---|---|
| 公共门户首页 | `/` | 游客入口、搜索、分类、拍品网格、品牌视觉主焦区 |
| 认证中心 | `/auth/login` | 登录/注册一体化视觉骨架、Mock 登录、Toast 和跳转 |
| 拍卖大厅 | `/auction/hall` | 登录后主阵地、筛选、实时拍品矩阵、短轮询结构 |
| 竞价详情 | `/auction/detail/[id]` | 核心实时竞价页、WebSocket/轮询、乐观出价和回滚 |
| 拍品发布 | `/auction/publish` | 卖家分步发布向导、草稿保存、幂等 Token |
| 订单支付 | `/checkout/[orderId]` | 封闭式支付页、支付方式选择、交易确认反馈 |
| 管理大盘 | `/admin/dashboard` | 管理员审核、KPI、运维终端、滑出式审核抽屉 |

## 6. 页面设计要求

### 6.1 `/` 公共门户首页

- 顶部吸顶导航栏，高度 64px，白色 Surface，Logo、400px 搜索框、登录/注册按钮。
- Hero Section 占据约 50% 视口高度，展示校园爆款或即将截标拍品。
- Category Bar 使用胶囊按钮：数码3C、运动装备、图书教材、生活闲置。
- Product Grid 使用 4 列白色扁平卡片。
- 卡片 Hover：上移 4px，边框转 Indigo，右下角浮现“查看竞价”箭头。
- 倒计时变色：大于 12 小时灰色，1-12 小时 Amber，小于 5 分钟 Rose 并脉冲。

### 6.2 `/auth/login` 认证中心

- 单列居中 + 左右非对称分割布局。
- 左侧 45% 为校园留白艺术视觉，右侧 55% 为登录卡片。
- 输入框 Focus 使用 `ring-2 ring-indigo-500/20 border-indigo-500`。
- 提交登录时按钮进入 Loading 状态。
- Mock 登录成功后显示 Emerald Toast，然后跳转 `/auction/hall`。

### 6.3 `/auction/hall` 拍卖大厅

- 左侧 260px 过滤面板：价格区间、卖家信用分、交易方式。
- 右侧 3 列实时拍品卡片矩阵。
- 右上角 Mini Live Ticker 展示最新竞价/落锤动态。
- React Query 使用 5 秒级短轮询思想，避免大厅对大量拍品建立巨量 WebSocket。
- Hover 超过 150ms 时预取详情数据。

### 6.4 `/auction/detail/[id]` 竞价详情

- 6:4 左右双栏。
- 左侧：高清图集、缩略图、描述/参数/卖家评价 Tabs。
- 右侧：Sticky 竞价控制台、最高价大屏、出价历史墙、快捷加价按钮、自定义金额输入、`PLACE BID` 主按钮。
- 页面加载时尝试建立 `/ws/auction/{id}`；不可用时进入轮询降级。
- 收到价格广播时，大数字触发上翻或闪光高亮动效。
- 出价点击后立即插入灰色 pending 出价行，不等待后端响应。
- 成功后 pending 行转 Emerald 成功态，并刷新权威价格。
- `409 Conflict` 后撤销 pending 行、恢复权威价格、控制台 Shake 260ms、展示 Rose 阻断 Toast。
- `429` 后展示冷却遮罩与倒计时。

### 6.5 `/auction/publish` 拍品发布

- 分步式 Wizard：`01 描述拍品 -> 02 上传实拍 -> 03 制定竞价契约 -> 04 检查并上架`。
- 拖拽上传池使用虚线边框，Drag Over 时变 Indigo。
- 图片以 120x120 卡片展示；第一张显示“封面首图”。
- 首轮可以先做图片 URL/占位上传，不接真实二进制上传。
- 表单 `onBlur` 写入 `localStorage`，显示“草稿已自动保存”。
- 进入发布流时生成 UUID v4 幂等令牌，提交时随请求传递。

### 6.6 `/checkout/[orderId]` 支付页

- Distraction-free Layout，不显示普通全局导航。
- 居中账单卡片：落锤价、管理费、总计。
- 支付方式 Radio Card：校园一卡通、微信/支付宝模拟钱包。
- 选中态使用 Indigo 边框和 Emerald 对勾缩放动效。
- 确认支付后展示全屏白色遮罩、骨架进度线。
- Mock 成功后展示绿色徽章或对勾完成态。

### 6.7 `/admin/dashboard` 管理大盘

- 左侧深色 `Slate 900` 管理侧边栏，宽 240px。
- 右侧主工作台包含 KPI 卡片、高密度审核表格、黑色运维终端组件。
- 点击审核表格行时右侧滑出 500px 毛玻璃抽屉。
- 抽屉展示大图、OCR/关键词提示、驳回与批准按钮。
- Mock 操作后对应行 Fade-out 并折叠高度。

## 7. 前端 API 契约策略

### 7.1 环境变量

```env
NEXT_PUBLIC_API_MODE=mock
NEXT_PUBLIC_API_BASE_URL=http://127.0.0.1:18080
NEXT_PUBLIC_WS_BASE_URL=ws://127.0.0.1:18080
```

### 7.2 模式定义

- `mock`：默认，所有页面通过 `frontend/lib/api/client.ts` 调用 `frontend/lib/api/mock-data.ts` 中的 typed mock 数据，确保不依赖尚未补齐的 HTTP 控制器即可看到完整前端效果。
- `live`：调用真实后端文档契约接口；如果后端返回 404/网络错误，应显示“API route not ready”。

### 7.3 首批对齐接口

| 接口 | 前端使用处 | 当前策略 |
|---|---|---|
| `POST /api/auth/login` | `/auth/login` | Mock 优先，live 保留 |
| `GET /api/auth/me` | 全局会话恢复 | Mock 优先，live 保留 |
| `GET /api/auctions` | `/`、`/auction/hall` | Mock 优先，live 保留 |
| `GET /api/auctions/{id}` | `/auction/detail/[id]` | Mock 优先，live 保留 |
| `GET /api/auctions/{id}/bids` | 出价历史墙 | Mock 优先，live 保留 |
| `POST /api/auctions/{id}/bids` | `PLACE BID` | Mock 优先，必须模拟 409/429 |
| `/ws/auction/{id}` | 详情实时价格 | Mock/轮询优先，live 保留 |
| `POST /api/items` | `/auction/publish` | Mock 优先，live 保留 |
| `POST /api/orders/{id}/pay` | `/checkout/[orderId]` | Mock 优先，live 保留 |
| `GET /api/admin/statistics/daily` | `/admin/dashboard` | Mock 优先，live 保留 |

## 8. 阶段计划

### F16-0：前端工程骨架与设计系统

状态：已完成。

目标：创建 `frontend/` Next.js 工程、Tailwind 设计系统、Provider、全局布局、Mock API 基础和可运行首页。

完成判定：

- `frontend/package.json`、Next、TS、Tailwind 配置存在。
- `frontend/app/layout.tsx`、`providers.tsx`、`globals.css` 可运行。
- `/` 能显示设计系统风格首页。
- `npm run typecheck`、`npm run build` 通过。

实际完成：

- 已创建 `frontend/package.json`、`next.config.mjs`、`tsconfig.json`、`tailwind.config.ts`、`postcss.config.mjs` 和 `.env.example`。
- 已创建 `frontend/app/layout.tsx`、`frontend/app/providers.tsx`、`frontend/app/globals.css`，接入 React Query Provider 和浅色工业级设计系统。
- 已创建 `frontend/lib/api/client.ts`、`mock-data.ts`、`types/auction.ts`，默认 Mock 模式可运行，live 模式保留真实接口调用入口。
- 后续已按安全修复要求升级到 Next.js `16.2.6`、React `19.2.6` 和对应 React 19 类型包；生产构建脚本固定为 `next build --webpack`，规避当前 WSL2/Codex 沙箱下 Turbopack 进程/端口限制。

### F16-1：7 个物理页面视觉骨架

状态：已完成。

目标：完成 7 个路由页面的视觉结构，能通过 Mock 数据跳转浏览。

完成判定：

- `/`、`/auth/login`、`/auction/hall`、`/auction/detail/[id]`、`/auction/publish`、`/checkout/[orderId]`、`/admin/dashboard` 均可访问。
- 页面结构符合第 6 节要求。
- 全站 Skeleton、Toast、基础组件可复用。
- `npm run typecheck`、`npm run build` 通过。

实际完成：

- 已落地 `/`、`/auth/login`、`/auction/hall`、`/auction/detail/[id]`、`/auction/publish`、`/checkout/[orderId]`、`/admin/dashboard` 七个物理页面。
- 已新增导航、降级横幅、拍卖卡片、按钮、Toast、Skeleton 等基础组件。
- 首页、登录、大厅、详情、发布、支付、管理页均使用 Mock 数据并可通过路由跳转访问。

### F16-2：竞价详情核心状态机

状态：已完成。

目标：实现详情页 React Query 数据流、乐观 pending 出价、409 回滚、429 限流、WebSocket 降级 UI。

完成判定：

- Mock adapter 可稳定模拟成功出价、409、429、WebSocket close。
- 出价 pending 行、成功态、回滚撤销、Shake、Toast、冷却遮罩可见。
- 断连横幅和轮询状态可见。
- 核心状态逻辑有最小测试或可复现实验步骤。

实际完成：

- 详情页已使用 React Query 读取拍卖详情和出价历史，并以 4 秒短轮询模拟实时刷新。
- `placeBid` Mock 已支持成功、`409 Conflict` 和 `429 Too Many Requests` 分支；金额可被 13 整除时模拟 409，可被 17 整除时模拟 429。
- 页面已实现 pending 出价行、成功刷新、409 回滚 Toast 与 Shake、429 冷却遮罩和倒计时、实时通道降级提示。

### F16-3：发布、支付、管理交互闭环

状态：已完成。

目标：完善发布向导、支付流程、管理员审核抽屉的 Mock 交互闭环。

完成判定：

- 发布页可保存草稿、生成幂等 Token、Mock 提交。
- 支付页可选择通道、进入支付中、完成成功态。
- 管理页可打开抽屉、Mock 审核并移除行。
- `npm run typecheck`、`npm run build` 通过。

实际完成：

- 发布页已实现四步向导、图片 URL 占位上传、`localStorage` 草稿保存和 UUID v4 幂等令牌。
- 支付页已实现支付方式选择、支付处理中状态和 Mock 成功态。
- 管理页已实现 KPI、待审表格、500px 审核抽屉、批准/驳回后行淡出移除。

### F16-4：前端文档与本地演示验证

状态：已完成。

目标：补齐前端 README、API readiness、设计系统说明和本地运行记录。

完成判定：

- `frontend/README.md` 说明安装、运行、构建和 mock/live 模式。
- `frontend/API_READINESS.md` 记录接口接入矩阵和后端 HTTP 控制器缺口。
- `docs/schedule.md` 已同步当前前端进度和 handoff。
- 本地能通过 `npm install`、`npm run typecheck`、`npm run build`。

实际完成：

- 已创建 `frontend/README.md`，覆盖安装、运行、构建、mock/live 模式切换、页面路由和目录结构说明。
- 已创建 `frontend/API_READINESS.md`，记录 12 个接口接入矩阵、后端控制器缺口和 F17 建议接入顺序。
- `docs/schedule.md` 已同步 F16 全部完成状态和 handoff。
- `npm run typecheck` 与 `npm run build` 通过，8 个 App Router 页面全部生成。

### F16-5：Next 16 安全升级与构建验证

状态：已完成。

目标：按用户要求把前端升级到 Next.js 16，清理 Next.js 14 依赖链的 high 风险，并同步验证与记录。

实际完成：

- 已升级 `next` 到 `^16.2.6`、`react`/`react-dom` 到 `^19.2.6`、`@types/react`/`@types/react-dom` 到 React 19 对应版本。
- 已更新 `frontend/package-lock.json`，并保留 Mock/live API 策略不变。
- 已将 `npm run build` 固定为 `next build --webpack`；Next.js 16 默认 Turbopack 在当前 WSL2/Codex 沙箱中会因创建进程/绑定端口受限失败，webpack 构建路径通过验证。
- 已在 `next.config.mjs` 设置 `outputFileTracingRoot` 为 `frontend/`，避免 Next.js 16 因上级目录 lockfile 误判 workspace root。
- Next.js 16 会生成 `.next/types`，`npm run typecheck` 已改为 `next typegen && tsc --noEmit`，可在 fresh clone 没有 `.next` 的状态下独立执行。

验证结果：

- 无 `.next` 状态下 `npm run typecheck` 通过，先生成 route types 再执行 `tsc --noEmit`。
- `npm run build` 通过，Next.js `16.2.6 (webpack)` 生成 8 个 App Router 页面，其中 7 个为计划内业务页面。
- `npm audit --json` 执行成功，high/critical 为 0，剩余 2 个 moderate，来源为 Next.js `16.2.6` 内部依赖 `postcss 8.4.31` 的 advisory；当前 npm 给出的 force fix 会降级到异常的 Next `9.3.3`，不采用。

### F17：真实后端 HTTP 控制器逐步接入

状态：进行中。

目标：按页面优先级逐步把 Mock API 替换为真实 Drogon HTTP 接口。

建议顺序：

1. Auth：`POST /api/auth/login`、`GET /api/auth/me`。 ✅ 已完成

#### F17-Auth 实际完成

- 已创建 `src/access/http/auth_http.h` 和 `src/access/http/auth_http.cpp`，注册 4 个路由：
  - `POST /api/auth/register` - 用户注册
  - `POST /api/auth/login` - 用户登录
  - `POST /api/auth/logout` - 用户登出（需 Bearer Token）
  - `GET /api/auth/me` - 获取当前用户（需 Bearer Token）
- 已更新 `src/common/runtime/application_bootstrap.cpp`，创建 `InMemoryAuthSessionStore`、`AuthService`、`AuthMiddleware` 实例并注册路由
- 已更新 `CMakeLists.txt` 添加 `src/access/http/auth_http.cpp`
- 已更新 `frontend/lib/api/client.ts`：`login()` 增加 `password` 参数，新增 `getMe()` 和 `logout()`，mock/live 模式均自动管理 token 存储，live 错误优先展示后端统一响应 message
- Auth HTTP 响应已补齐本地前端 live 模式所需 CORS 响应头和 `OPTIONS` 预检处理
- 已更新 `frontend/app/auth/login/page.tsx`：传递 password 并处理登录错误
- 已更新 `frontend/types/auction.ts`：新增 `UserProfile` 和 `LoginResponse` 类型
- 验证：`cmake --build build`、非数据库冒烟 CTest、前端 `npm run typecheck` 与 `npm run build` 通过；当前本机全量 `ctest --test-dir build --output-on-failure` 在数据库用例启动临时 MySQL 时失败，错误日志为 `Unable to lock ./ibdata1 error: 11`，需清理或重建 `build/test_mysql` 后重跑数据库套件
2. Auction list/detail：`GET /api/auctions`、`GET /api/auctions/{id}`。
3. Bid：`POST /api/auctions/{id}/bids`、`GET /api/auctions/{id}/bids`。
4. Publish：`POST /api/items`、图片元数据、提交审核。
5. Checkout：订单查询、支付发起。
6. Admin：统计、待审、运维数据。
7. WebSocket：`/ws/auction/{id}`。

完成判定：

- 每接入一组真实 API，都保留 Mock 模式。
- live 模式下对应页面能跑通真实后端。
- 更新 `frontend/API_READINESS.md` 和对应后端模块文档。
- 后端和前端验证均有记录。

## 9. 建议文件结构

```text
frontend/
  package.json
  next.config.mjs
  tsconfig.json
  tailwind.config.ts
  postcss.config.mjs
  app/
    globals.css
    layout.tsx
    providers.tsx
    page.tsx
    auth/login/page.tsx
    auction/hall/page.tsx
    auction/detail/[id]/page.tsx
    auction/publish/page.tsx
    checkout/[orderId]/page.tsx
    admin/dashboard/page.tsx
  components/
    layout/
    ui/
    auction/
    auth/
    publish/
    checkout/
    admin/
  hooks/
  lib/
    api/
    query/
    realtime/
    utils/
  types/
```

## 10. 验证命令

前端阶段：

```bash
cd frontend
npm install
npm run typecheck
npm run build
npm run dev
```

后端未修改时不强制运行 CTest。若修改后端或接入真实 API，则补充：

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## 11. 当前 Handoff

- Step ID: F16（含 F16-0 到 F16-5）
- 当前状态: 已完成
- 已完成: 创建 `frontend/` Next.js 工程、Tailwind 设计系统、React Query Provider、Mock API、7 个物理页面、竞价详情核心状态机、发布/支付/管理 Mock 交互闭环、前端 README/API_READINESS，并升级到 Next.js 16 与 React 19。
- 修改文件: `frontend/`、`.gitignore`、`docs/frontend-next-schedule.md`、`docs/schedule.md`。
- 验证命令:
  - `cd frontend && npm install`
  - `cd frontend && npm install next@^16 react@^19 react-dom@^19 @types/react@^19 @types/react-dom@^19`
  - `cd frontend && npm run typecheck`
  - `cd frontend && npm run build`
  - `cd frontend && npm audit --json`
- 验证结果:
  - Next.js 已升级到 `16.2.6`，React 已升级到 `19.2.6`
  - `npm run typecheck` 通过
  - `npm run build` 通过，使用 webpack 构建生成 8 个 App Router 页面，其中 7 个为计划内业务页面
  - `npm audit --json` 剩余 2 个 moderate，high/critical 为 0
- 未覆盖风险:
  - 尚未接入真实 Drogon HTTP 控制器和 WebSocket
  - `npm audit` 剩余 2 个 moderate，等待 Next.js 后续补丁释放可用修复版本
- 下一步: F17 按 Auth、Auction、Bid、Publish、Checkout、Admin、WebSocket 顺序接入真实后端，保留 Mock 模式。
- 下一步必须先读: `docs/schedule.md`、`docs/frontend-next-schedule.md`、`frontend/API_READINESS.md`、`docs/接口联调记录.md`。
