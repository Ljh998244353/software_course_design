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
| 拍卖大厅 | `/auction/hall` | 登录后主阵地、URL 驱动搜索筛选、实时拍品矩阵、短轮询结构 |
| 竞价详情 | `/auction/detail/[id]` | 核心实时竞价页、WebSocket/轮询、真实 Tabs、乐观出价和回滚 |
| 拍品发布 | `/auction/publish` | 卖家分步发布向导、草稿保存、幂等 Token、建议竞价配置 |
| 订单支付 | `/checkout/[orderId]` | 封闭式支付页、支付方式选择、交易确认反馈 |
| 管理大盘 | `/admin/dashboard` | 管理员审核、KPI、拍卖监控、统计报表、只读运维面板 |

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
- React Query 使用 5 秒级短轮询思想，避免大厅对大量拍品建立巨量 WebSocket；筛选条件写入 URL 查询参数并可刷新恢复。
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

- 分步式 Wizard：`01 描述拍品 -> 02 上传实拍 -> 03 建议竞价配置 -> 04 检查并提交审核`。
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
NEXT_PUBLIC_API_BASE_URL=http://127.0.0.1:18080
NEXT_PUBLIC_WS_BASE_URL=ws://127.0.0.1:18080
```

### 7.2 当前接入策略

- 当前前端已固定走真实后端 API，不再保留 `mock-data.ts` 或前端 Mock 主路径。
- 支付环节仅保留正式测试支付模型，用于演示完整下单与回调链路，但产品入口、页面文案和接口流转均按真实系统组织。
- WebSocket 为详情页实时价格主通道；连接失败时前端自动降级为轮询，不再区分 mock/live 两套页面数据源。

### 7.3 首批对齐接口

| 接口 | 前端使用处 | 当前策略 |
|---|---|---|
| `POST /api/auth/login` | `/auth/login` | 已接入真实后端 |
| `GET /api/auth/me` | 全局会话恢复 | 已接入真实后端 |
| `GET /api/auctions` | `/`、`/auction/hall` | 已接入真实后端 |
| `GET /api/auctions/{id}` | `/auction/detail/[id]` | 已接入真实后端 |
| `GET /api/auctions/{id}/bids` | 出价历史墙 | 已接入真实后端 |
| `POST /api/auctions/{id}/bids` | `PLACE BID` | 已接入真实后端 |
| `/ws/auction/{id}` | 详情实时价格 | 已接入真实 WebSocket，失败时自动轮询降级 |
| `POST /api/items` | `/auction/publish` | 已接入真实后端 |
| `POST /api/orders/{id}/pay` | `/checkout/[orderId]` | 已接入真实测试支付模型 |
| `GET /api/admin/statistics/daily` | `/admin/dashboard` | 已接入真实后端 |

## 8. 阶段计划

### F16-0：前端工程骨架与设计系统

状态：已完成。

目标：创建 `frontend/` Next.js 工程、Tailwind 设计系统、Provider、全局布局和可运行首页。

完成判定：

- `frontend/package.json`、Next、TS、Tailwind 配置存在。
- `frontend/app/layout.tsx`、`providers.tsx`、`globals.css` 可运行。
- `/` 能显示设计系统风格首页。
- `npm run typecheck`、`npm run build` 通过。

实际完成：

- 已创建 `frontend/package.json`、`next.config.mjs`、`tsconfig.json`、`tailwind.config.ts`、`postcss.config.mjs` 和 `.env.example`。
- 已创建 `frontend/app/layout.tsx`、`frontend/app/providers.tsx`、`frontend/app/globals.css`，接入 React Query Provider 和浅色工业级设计系统。
- 已创建 `frontend/lib/api/client.ts`、`types/auction.ts`，当前仓库已在后续产品化收尾中删除前端 Mock 数据主路径并固定走真实后端接口。
- 后续已按安全修复要求升级到 Next.js `16.2.6`、React `19.2.6` 和对应 React 19 类型包；生产构建脚本固定为 `next build --webpack`，规避当前 WSL2/Codex 沙箱下 Turbopack 进程/端口限制。

### F16-1：7 个物理页面视觉骨架

状态：已完成。

目标：完成 7 个路由页面的视觉结构，能通过真实页面路由跳转浏览。

完成判定：

- `/`、`/auth/login`、`/auction/hall`、`/auction/detail/[id]`、`/auction/publish`、`/checkout/[orderId]`、`/admin/dashboard` 均可访问。
- 页面结构符合第 6 节要求。
- 全站 Skeleton、Toast、基础组件可复用。
- `npm run typecheck`、`npm run build` 通过。

实际完成：

- 已落地 `/`、`/auth/login`、`/auction/hall`、`/auction/detail/[id]`、`/auction/publish`、`/checkout/[orderId]`、`/admin/dashboard` 七个物理页面。
- 已新增导航、降级横幅、拍卖卡片、按钮、Toast、Skeleton 等基础组件。
- 首页、登录、大厅、详情、发布、支付、管理页均已具备真实产品页面结构，并可通过路由跳转访问。

### F16-2：竞价详情核心状态机

状态：已完成。

目标：实现详情页 React Query 数据流、乐观 pending 出价、409 回滚、429 限流和 WebSocket 降级 UI。

完成判定：

- 真实接口和实时通道下可稳定处理成功出价、409、429 与 WebSocket close。
- 出价 pending 行、成功态、回滚撤销、Shake、Toast、冷却遮罩可见。
- 断连横幅和轮询状态可见。
- 核心状态逻辑有最小测试或可复现实验步骤。

实际完成：

- 详情页已使用 React Query 读取拍卖详情和出价历史，并以 4 秒短轮询模拟实时刷新。
- 前端详情页已接入真实 `placeBid`、出价历史和 `/ws/auction/{id}`；页面已实现 pending 出价行、成功刷新、409 回滚 Toast 与 Shake、429 冷却遮罩和倒计时、实时通道降级提示。

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
- 管理页“退出后台”现已收口为仅离开管理视图并返回前台，不再触发全局登出。
- 管理页“最近处理记录”现已持久化到浏览器本地存储，重新进入后台或重新登录后仍可保留最近审核结果。
- 管理页已补齐管理员创建拍卖入口：可基于已批准拍品直接填写起止时间、起拍价、加价幅度和延时规则，并调用真实 `POST /api/admin/auctions` 创建正式拍卖。
- 管理页审核抽屉现已渲染真实封面图：`GET /api/admin/items/pending` 已补回 `cover_image_url`，抽屉只有在缺图时才显示占位背景。
- 管理页创建拍卖链路现已补齐跨域预检：管理端拍卖 HTTP 控制器已为 `POST/GET/PUT` 路由注册 `OPTIONS`，浏览器不再把创建拍卖误判成“无法连接后端服务”。
- 发布页图片卡片右上角现已支持单张撤回删除，并会同步更新本地草稿状态。
- 发布页添加图片前现已校验 URL 协议、常见图片扩展名/格式参数，以及资源是否真的可加载；非法或不可访问图片会直接提示错误。
- 首页卡片与详情页主图现已对用户自填图片 URL 使用普通 `<img>`，避免第三方图片域名触发 `next/image` 运行时白名单错误。
- 会话恢复阶段现已统一显示加载占位：`AuthProvider` 启动时先从 `sessionStorage` 恢复 token，导航条不会在刷新瞬间误闪“登录/注册”。
- 首页现已与大厅保持一致，只展示 `RUNNING` 拍卖，不再把 `PENDING_START` 误展示为“正在竞价”。
- 详情页出价按钮现已依据服务端 `acceptingBids` 标记控制，不再只靠本地字符串判断状态。
- 管理端创建拍卖默认开始时间已由 10 分钟后调整为 1 分钟后，减少“建完拍卖却长期不可竞价”的误判。

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

### F17：真实后端 HTTP 与 WebSocket 接入及虚标功能补全

状态：已完成主要闭环，剩余以回归和体验打磨为主。

目标：按页面优先级逐步把 Mock API 替换为真实 Drogon HTTP 接口。

建议顺序：

1. Auth：`POST /api/auth/login`、`GET /api/auth/me`。 ✅ 已完成
2. Auction list/detail：`GET /api/auctions`、`GET /api/auctions/{id}`。 ✅ 已完成

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

#### F17-Auction 实际完成

- 已创建 `src/access/http/auction_http.h` 和 `src/access/http/auction_http.cpp`，注册 2 个路由：
  - `GET /api/auctions` - 公开拍卖列表（支持 keyword、status、page_no、page_size 查询参数）
  - `GET /api/auctions/{id}` - 公开拍卖详情
- 已更新 `src/common/runtime/application_bootstrap.cpp`，实例化 `AuctionService` 并注册路由
- 已更新 `CMakeLists.txt` 添加 `src/access/http/auction_http.cpp`
- 已更新 `frontend/types/auction.ts`：新增 `AuctionSummaryRaw` 和 `AuctionDetailRaw` 类型
- 已更新 `frontend/lib/api/client.ts`：新增 `mapAuctionSummary` 和 `mapAuctionDetail` 映射函数，live 模式下自动将后端 `PublicAuctionSummary`/`PublicAuctionDetail` 响应转换为前端 `AuctionItem` 类型
- 已修复 `GET /api/auctions/{id}` Drogon 路径参数绑定问题，详情路由现在显式接收 `{id}` 参数
- 已修复 live 模式拍卖图片地址归一化：空图使用占位图，`/uploads/...` 自动拼接后端 base URL，并允许本地后端图片源
- 已更新 `frontend/API_READINESS.md`：标记 Auction list/detail 为已接入
- 验证：`cmake --build build`、`scripts/test.sh smoke`、前端 `npm run typecheck` 与 `npm run build` 均通过

#### F17-Bid 实际完成

- 已创建 `src/access/http/bid_http.h` 和 `src/access/http/bid_http.cpp`，注册 2 个路由：
  - `GET /api/auctions/{id}/bids` - 公开出价历史（支持 page_no、page_size 分页）
  - `POST /api/auctions/{id}/bids` - 提交出价（需 Bearer Token，请求体含 `request_id` 和 `bid_amount`）
- 已更新 `src/common/runtime/application_bootstrap.cpp`，实例化 `InMemoryAuctionEventGateway`、`NotificationService`、`InMemoryBidCacheStore`、`BidService` 并注册路由
- 已更新 `CMakeLists.txt` 添加 `src/access/http/bid_http.cpp`
- 已更新 `frontend/types/auction.ts`：新增 `BidHistoryEntryRaw`、`BidHistoryResponseRaw`、`PlaceBidResultRaw` 类型
- 已更新 `frontend/lib/api/client.ts`：新增 `mapBidHistoryEntry` 和 `mapPlaceBidResult` 映射函数；`getBids` live 模式正确解析后端 `{ records, page_no, page_size }` 响应；`placeBid` live 模式发送 `{ request_id, bid_amount }` 请求体并映射 `PlaceBidResult` 响应
- 已修复 `liveFetch` 错误消息格式：HTTP 状态码前缀（如 `409 ...`），确保详情页 409/429 错误处理在 live 模式下正常工作
- 已更新 `frontend/API_READINESS.md`：标记 Bid list/place 为已接入
- 验证：`cmake --build build`、`scripts/test.sh smoke`、前端 `npm run typecheck` 与 `npm run build` 均通过

#### F17-Bid Code Review 修复

- 已修复 HTTP 数字参数解析缺陷：`src/access/http/bid_http.cpp` 的 `auction_id`、`page_no`、`page_size` 现在要求整段字符串均为合法数字，`1abc`、`2xyz` 等畸形输入不再被 `std::stoull/std::stoi` 静默截断接受
- 已同步修复相邻 Auction HTTP 控制器：`src/access/http/auction_http.cpp` 的拍卖详情 `auction_id` 以及列表分页参数同样执行严格数字解析
- 验证：`git diff --check`、`cmake --build build`、`scripts/test.sh smoke`、前端 `npm run typecheck` 与 `npm run build` 均通过

#### F17-Publish 实际完成

- 已创建 `src/access/http/item_http.h` 和 `src/access/http/item_http.cpp`，注册 3 个路由：
  - `POST /api/items` - 创建草稿拍品（需 Bearer Token，请求体含 `title`、`description`、`category_id`、`start_price`、`cover_image_url`）
  - `PUT /api/items/{id}` - 修改拍品（需 Bearer Token，请求体字段均可选）
  - `POST /api/items/{id}/submit-review` - 提交审核（需 Bearer Token）
- 已更新 `src/common/runtime/application_bootstrap.cpp`，实例化 `ItemService` 并注册路由
- 已更新 `CMakeLists.txt` 添加 `src/access/http/item_http.cpp`
- 已更新 `frontend/types/auction.ts`：新增 `CreateItemRaw`、`UpdateItemRaw`、`SubmitReviewRaw` 类型
- 已更新 `frontend/lib/api/client.ts`：新增 `createItem()` 和 `submitItemForReview()` 函数，mock/live 模式均支持
- 已更新 `frontend/app/auction/publish/page.tsx`：实现真实 API 提交流程（创建草稿 -> 提交审核）、加载状态和错误 Toast
- 已更新 `frontend/API_READINESS.md`：标记 Item create/update/submit-review 为已接入
- 验证：`cmake --build build`、`scripts/test.sh smoke`、前端 `npm run typecheck` 与 `npm run build` 均通过

#### F17-Publish-FIX 实际完成

- 修复 live 发布流程缺陷：`cover_image_url` 只写入拍品主表，不会产生 `item_image` 元数据；而 `SubmitForAudit` 明确要求至少存在一条图片元数据
- 已在 `src/access/http/item_http.cpp` 新增 `POST /api/items/{id}/images`，复用 `ItemService::AddItemImage` 写入图片 URL 元数据并维护封面
- 已更新前端 `create -> add images -> submit-review` 提交流程；提交审核失败后复用本地已创建的 `item_id` 和已写入图片 URL，避免重试时重复创建草稿或重复写图
- 已修复发布页重复图片 URL 的前端状态缺陷：草稿恢复与手动添加都会去重，图片网格渲染 key 不再直接使用 URL，避免 React 因重复 key 报错
- 已新增服务层回归断言：仅提供 `cover_image_url` 但未写 `item_image` 时，提交审核必须失败
- 已同步 `frontend/API_READINESS.md` 与 `docs/物品与审核模块说明.md`
- 验证：`git diff --check`、`cmake --build build`、`scripts/test.sh smoke`、前端 `npm run typecheck` 与 `npm run build` 均通过；`ctest --test-dir build --output-on-failure -R auction_item_flow` 初次因残留测试 MySQL `Unable to lock ./ibdata1 error: 11` 失败，停止残留 `mysqld --datadir=build/test_mysql/data` 进程后重跑通过

3. Bid：`POST /api/auctions/{id}/bids`、`GET /api/auctions/{id}/bids`。 ✅ 已完成
4. Publish：`POST /api/items`、图片元数据、提交审核。 ✅ 已完成
5. Checkout：`GET /api/orders/{id}`、`POST /api/orders/{id}/pay`。 ✅ 已完成
6. Admin：统计、待审、运维数据。 ✅ 已完成
7. WebSocket：`/ws/auction/{id}`。 ✅ 已完成

#### F17 Checkout/Admin Code Review 修复

- 修复 Checkout live 支付渠道被忽略的问题：前端支付方式现在映射为 `MOCK_WECHAT` / `MOCK_ALIPAY` 并随 `pay_channel` 提交。
- 修复 Checkout 金额展示与后端支付金额不一致的问题：前端总计对齐后端 `final_amount/pay_amount`，当前无单独服务费落库字段时管理费显示为 0。
- 修复 Admin 审核按钮只本地移除行的问题：批准/驳回现在调用 `approveItem` / `rejectItem`，接口成功后才更新列表，并展示审核失败错误。
- 修复 Admin 统计接口“已接入但页面未使用”的问题：管理页 KPI 现在查询 `GET /api/admin/statistics/daily`，Mock 模式也返回可展示统计数据。
- 验证：`git diff --check`、`cmake --build build`、`scripts/test.sh smoke`、前端 `npm run typecheck`、前端 `npm run build` 均通过。

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
- 下一步: F17 按 Auth、Auction、Bid、Publish、Checkout、Admin、WebSocket 顺序接入真实后端，保留 Mock 模式。Auth/Auction/Bid/Publish/Checkout/Admin 已完成，下一步 WebSocket。
- 下一步必须先读: `docs/schedule.md`、`docs/frontend-next-schedule.md`、`frontend/API_READINESS.md`、`docs/接口联调记录.md`。
- 2026-06-03 补充修复：大厅页已区分“首屏加载 / 刷新中 / 错误 / 空结果”状态；当真实后端当前无拍卖数据时，不再持续显示 Skeleton，而是展示可操作空态。顶部导航的访客会话初始态也已修正，未登录用户首屏直接可见 `登录/注册` 入口。
- 2026-06-03 补充修复：登录页已修正浏览器自动填充后的提交态，按钮不再因为 React state 未同步而持续灰掉；表单补齐 `required`、`name` 和 `autoComplete`，并在页面文案中明确默认管理员账号为 `admin / Admin@123`。
- 2026-06-03 补充修复：登录错误已改为分型提示。前端会根据后端返回的 `401/404/5xx` 与业务码 `4001/4102/4106/4107/5004/5005` 等，明确提示是“用户名或密码错误”“账号被冻结/禁用”“数据库不可用”“接口未注册”还是“后端服务不可达/超时”。
- 2026-06-03 补充修复：登录提交阶段已改为直接从 `FormData` 读取表单值，并把 form method 固定为 `post`；即便浏览器自动填充未触发 `onChange`，也不会再因为 state 为空而提交错误或把凭据拼进 URL。
- 2026-06-03 补充修复：测试基础设施已开始兼容受限环境。测试库启动脚本优先走 Unix socket，若 socket bind 被环境拒绝则自动回退本地 TCP；本轮在 Codex 沙箱内继续验证时，环境同时限制 socket 与 TCP 监听，最终联通门禁需在沙箱外复验。
- 2026-06-03 补充修复：最终真实联调门禁已补齐后端 HTTP 合同缺口，订单列表/详情/支付状态/发货/确认收货、管理员用户状态、登出响应和统一 JSON 404 均可由真实 Drogon 接口返回；验证通过 `scripts/test.sh http`、`scripts/test.sh risk`、`ctest --test-dir build --output-on-failure`、`cd frontend && npm run typecheck`、`cd frontend && npm run build`。其中 MySQL 相关门禁在 Codex 沙箱内会受本地监听限制，已在沙箱外权限下完成复验。
