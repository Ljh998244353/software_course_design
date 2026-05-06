# 在线拍卖平台 Agent 执行 Schedule

## 1. 文档目的

本文档是 `/home/ljh/project/soft_course_design` 的统一上下文入口，用于让后续 agent 快速确认：

- 项目是什么
- 当前真实进度到哪一步
- 当前应继续做什么
- 动手前必须理解哪些业务、架构和验证约束

本文件不再保留完整历史流水账。历史实现细节以代码、模块文档和测试为准；本文件只记录可恢复上下文和下一步执行入口。

## 2. 新会话固定恢复文本

后续每次进入本项目开始新一轮任务，先读取并遵循下面这段文本：

```text
这是 /home/ljh/project/soft_course_design 项目。

项目目标：课程设计用在线拍卖平台，浏览器访问，C++ 后端，Drogon HTTP 接入，MySQL 作为交易事实唯一来源，静态前端分为只读答辩演示台 `/demo` 和真实业务前端 `/app`。

当前状态：
- S00-S30 已完成。
- 当前处于课程设计交付和答辩复现状态。
- 当前下一步是按需答辩复现、演示或提交材料，不再有计划内新增开发步骤。

继续前先读取：
1. docs/schedule.md
2. docs/需求规格说明书.md
3. docs/系统概要设计报告.md
4. docs/接口联调记录.md
5. docs/测试计划与用例说明.md
6. docs/前端演示模块说明.md
7. 若继续 S30，还必须读取 docs/部署与答辩说明.md、docs/答辩演示操作手册.md、docs/前端演示模块说明.md、docs/测试计划与用例说明.md、scripts/deploy/、config/、sql/demo_data.sql、assets/app/。

执行原则：
- 用户未明确要求时，不默认新增功能。
- 若只是交付或答辩复现，优先使用 scripts/deploy/init_demo_env.sh、scripts/deploy/run_demo_server.sh、http://127.0.0.1:18080/demo、http://127.0.0.1:18080/app、scripts/deploy/show_demo_walkthrough.sh 和 scripts/deploy/verify_release.sh。
- 完整前端接入计划 S17-S30 已完成；用户未明确要求时，不新增业务功能。
- 任一 S17-S30 步骤完成后，同步更新 docs/schedule.md、相关模块文档、测试计划和 handoff。
- 如果 schedule.md、模块文档和代码现状不一致，优先以代码现状为准，并同步更新文档。
- 始终用中文回答；任何 sudo 操作必须先询问用户。
```

## 3. 当前进度速览

项目当前已经具备：

- CMake 工程骨架、配置加载、日志、错误码、统一响应模型和基础测试。
- MySQL schema、seed、Repository 基础封装和本地数据库验证脚本。
- 认证权限、拍品审核、拍卖管理、竞价通知、订单支付、评价、统计、运维异常等服务层模块。
- 主流程集成测试、高风险专项测试、HTTP 回归脚本、部署演示脚本和答辩演示数据。
- `/demo` 只读答辩演示台，可从 `auction_demo` 聚合展示完整业务闭环。
- `/app` 真实业务前端骨架，已接入认证、卖家拍品、管理员审核、管理端拍卖、公开拍卖、真实出价、轮询通知、订单支付、履约、评价、通知中心、用户信用、统计报表和运维异常页面，并完成全站 UX、状态处理和安全前端约束收口。

当前真实状态：

- `S00-S16`：课程设计原计划内步骤已完成。
- `S17-S28`：完整前端接入计划的前半段已完成。
- `S29`：完整前端部署、演示数据和答辩脚本接入已完成。
- `S30`：完整前端最终回归和 handoff 已完成。
- 当前下一步：按需进行答辩复现、演示运行或提交材料。

当前已接入的真实业务 HTTP 范围：

- 认证与会话：`POST /api/auth/register`、`POST /api/auth/login`、`POST /api/auth/logout`、`GET /api/auth/me`、`PATCH /api/admin/users/{id}/status`。
- 系统上下文：`GET /api/system/context`。
- 卖家拍品：`POST /api/items`、`PUT /api/items/{id}`、`POST /api/items/{id}/images`、`DELETE /api/items/{itemId}/images/{imageId}`、`GET /api/items/mine`、`GET /api/items/{id}`、`POST /api/items/{id}/submit-audit`。
- 管理员审核与拍卖：`GET /api/admin/items/pending`、`POST /api/admin/items/{id}/audit`、`GET /api/admin/items/{id}/audit-logs`、`POST /api/admin/auctions`、`GET /api/admin/auctions`、`GET /api/admin/auctions/{id}`、`PUT /api/admin/auctions/{id}`、`POST /api/admin/auctions/{id}/cancel`。
- 公开拍卖：`GET /api/auctions`、`GET /api/auctions/{id}`、`GET /api/auctions/{id}/price`、`GET /api/auctions/{id}/bids`。
- 竞价与通知轮询：`POST /api/auctions/{id}/bids`、`GET /api/auctions/{id}/my-bid`、`GET /api/notifications`、`PATCH /api/notifications/{id}/read`。
- 订单支付和履约：`GET /api/orders/mine`、`GET /api/orders/{id}`、`POST /api/orders/{id}/pay`、`GET /api/orders/{id}/payment`、`POST /api/payments/callback`、`POST /api/orders/{id}/ship`、`POST /api/orders/{id}/confirm-receipt`。
- 评价、通知中心和信用：`POST /api/reviews`、`GET /api/orders/{id}/reviews`、`GET /api/users/{id}/reviews/summary`、`GET /api/notifications`、`PATCH /api/notifications/{id}/read`。
- 统计报表：`GET /api/admin/statistics/daily`、`POST /api/admin/statistics/daily/rebuild`、`GET /api/admin/statistics/daily/export`。
- 运维异常：`GET /api/admin/ops/operation-logs`、`GET /api/admin/ops/task-logs`、`GET /api/admin/ops/exceptions`、`POST /api/admin/ops/exceptions/mark`、`POST /api/admin/ops/notifications/retry`、`POST /api/admin/ops/compensations`。

仍待收口的真实业务页面范围：

- 无计划内待收口页面；后续只按用户明确要求做缺陷修复或可选增强。

最近一次记录的验证来自 S30：

- `bash -n scripts/deploy/init_demo_env.sh`
- `bash -n scripts/deploy/run_demo_server.sh`
- `bash -n scripts/deploy/show_demo_walkthrough.sh`
- `bash -n scripts/deploy/verify_release.sh`
- `bash -n scripts/test_ui.sh`
- `bash -n scripts/test.sh`
- `scripts/deploy/verify_release.sh`
- 结果：脚本语法检查通过；`scripts/deploy/verify_release.sh` 通过，已初始化 `auction_demo`，检查 `/healthz`、`/demo`、`/app`、两套静态资源和 `/api/demo/dashboard`，并串联 `frontend`、`ui`、`http`、`risk` 与全量 `ctest 18/18`。验证前仅处理了一个指向本项目 `build/test_mysql/data` 且 socket 已不可连的残留测试 `mysqld`；沙箱内启动 MySQL socket 被限制后，按权限流程在沙箱外完成最终验证。

## 4. 核心业务和架构约束

必须保持的项目边界：

- 项目是课程设计，不引入付费工具，不引入不必要的微服务、MQ、分布式锁、CQRS 或事件溯源。
- 后端优先保持模块化单体：`src/modules/` 承载业务服务，`src/repository/` 访问数据库，`src/access/http/` 做 HTTP 接入。
- MySQL 是拍卖、出价、订单、支付等交易事实的唯一来源。
- Redis 或进程内缓存只能加速读取或旁路预检查，不能决定出价合法性、结算结果或支付状态。
- `/demo` 是只读答辩演示台；完整业务页面必须使用真实 `/api/...` 接口，不能把 `/api/demo/dashboard` 当业务数据源。
- 当前前端路线是静态 HTML/CSS/JS 加 C++ 服务托管；除非用户明确要求，不引入 Node/Vite/React 构建链。

关键业务不变量：

- 同一拍卖任意时刻最多只有一个合法当前最高价和最高出价人。
- 出价记录插入、拍卖当前价更新、最高出价人更新和延时保护必须在同一事务内成功或失败。
- 拍卖结束后不能接受新的有效出价。
- 一场拍卖最多生成一笔订单。
- 支付回调必须幂等，重复回调不能重复记账或重复推进状态。
- 发货、收货、评价、统计、补偿等后续动作必须遵守订单和支付状态机。
- 所有写接口都需要认证、授权、输入校验和可审计错误处理。

## 5. Step 状态表

### 5.1 已完成基线步骤 S00-S16

| 步骤 | 状态 | 交付摘要 | 主要位置 |
|---|---|---|---|
| S00 | 已完成 | 冻结需求基线 | `docs/需求规格说明书.md` |
| S01 | 已完成 | 冻结架构基线 | `docs/系统概要设计报告.md` |
| S02 | 已完成 | 工程骨架、配置、日志、错误码、冒烟测试 | `CMakeLists.txt`、`src/`、`config/`、`tests/` |
| S03 | 已完成 | 数据库 schema、seed、Repository 基础和 DB 冒烟 | `sql/`、`src/common/db/`、`src/repository/` |
| S04 | 已完成 | 注册登录、密码哈希、Token、RBAC、账号状态 | `src/modules/auth/`、`src/middleware/`、`tests/auth/` |
| S05 | 已完成 | 拍品草稿、图片元数据、提交审核、审核日志 | `src/modules/item/`、`src/modules/audit/`、`tests/item/` |
| S06 | 已完成 | 拍卖创建、修改、取消、调度和状态协同 | `src/modules/auction/`、`src/jobs/`、`tests/auction/` |
| S07 | 已完成 | 出价事务、幂等、延时保护、通知和缓存抽象 | `src/modules/bid/`、`src/modules/notification/`、`src/ws/`、`tests/bid/` |
| S08 | 已完成 | 订单生成、支付发起、回调幂等、超时关闭 | `src/modules/order/`、`src/modules/payment/`、`tests/order/`、`tests/payment/` |
| S09 | 已完成 | 买卖双方互评、评价查询、信用汇总 | `src/modules/review/`、`tests/review/` |
| S10 | 已完成 | 日报聚合、成交/流拍/出价统计和 CSV 导出 | `src/modules/statistics/`、`tests/statistics/` |
| S11 | 已完成 | 操作日志、任务日志、异常看板、通知重试和补偿入口 | `src/modules/ops/`、`tests/ops/` |
| S12 | 已完成 | 认证、拍品、审核、拍卖、竞价、订单、支付、评价主流程联调 | `tests/integration/`、`scripts/test_integration.sh` |
| S13 | 已完成 | CTest、分层测试脚本、模块标签和测试计划 | `tests/`、`scripts/test.sh`、`docs/测试计划与用例说明.md` |
| S14 | 已完成 | 并发出价、结束竞争、支付幂等、缓存降级、通知失败和安全负向测试 | `tests/stress/`、`tests/security/` |
| S15 | 已完成 | 部署演示脚本、演示数据、答辩说明和发布验证 | `scripts/deploy/`、`sql/demo_data.sql`、`docs/部署与答辩说明.md` |
| S16 | 已完成 | `/demo` 只读浏览器演示台和 `/api/demo/dashboard` | `assets/demo/`、`src/access/http/`、`tests/demo/` |

### 5.2 完整前端接入步骤 S17-S30

| 步骤 | 状态 | 目标 | 主要代码位置 | 文档位置 |
|---|---|---|---|---|
| S17 | 已完成 | 真实业务 HTTP 接入基线、`/app`、登录、系统上下文、统一 JSON 404 | `src/access/http/`、`src/common/http/`、`assets/app/`、`tests/http/` | `docs/接口联调记录.md`、`docs/前端演示模块说明.md` |
| S18 | 已完成 | 注册、登录态、登出、当前用户、角色路由、管理员用户状态 | `src/access/http/auth_http.*`、`assets/app/`、`scripts/test_http.sh` | `docs/认证与权限模块说明.md`、`docs/前端演示模块说明.md` |
| S19 | 已完成 | 卖家拍品创建、编辑、图片元数据、列表、详情、提交审核 | `src/access/http/item_http.*`、`assets/app/`、`scripts/test_http.sh` | `docs/物品与审核模块说明.md`、`docs/前端演示模块说明.md` |
| S20 | 已完成 | 管理员待审、审核、审核日志、创建/修改/取消拍卖、管理端拍卖列表详情 | `src/access/http/audit_http.*`、`src/access/http/auction_admin_http.*`、`assets/app/` | `docs/物品与审核模块说明.md`、`docs/拍卖管理模块说明.md` |
| S21 | 已完成 | 买家公开拍卖大厅、详情、当前价、出价历史 | `src/access/http/auction_public_http.*`、`assets/app/` | `docs/拍卖管理模块说明.md`、`docs/前端演示模块说明.md` |
| S22 | 已完成 | 真实出价、我的出价状态、通知轮询、通知已读 | `src/access/http/bid_http.*`、`src/access/http/notification_http.*`、`assets/app/` | `docs/竞价与实时通知模块说明.md`、`docs/测试计划与用例说明.md` |
| S23 | 已完成 | 订单、支付和履约页面 | `src/access/http/order_http.*`、`src/access/http/payment_http.*`、`assets/app/`、`scripts/test_http.sh` | `docs/订单与支付模块说明.md`、`docs/前端演示模块说明.md` |
| S24 | 已完成 | 评价、通知中心和用户信用展示 | `src/access/http/review_http.*`、`src/access/http/notification_http.*`、`assets/app/`、`tests/http/` | `docs/评价与反馈模块说明.md`、`docs/竞价与实时通知模块说明.md` |
| S25 | 已完成 | 统计报表和运维异常页面 | `src/access/http/statistics_http.*`、`src/access/http/ops_http.*`、`assets/app/`、`tests/http/` | `docs/统计分析与报表模块说明.md`、`docs/系统监控与异常处理模块说明.md` |
| S26 | 已完成 | 全站 UX、状态处理和安全前端约束 | `assets/app/`、`tests/http/`、`scripts/` | `docs/前端演示模块说明.md`、`docs/测试计划与用例说明.md` |
| S27 | 已完成 | 真实 HTTP 接口自动化回归收口 | `tests/http/`、`scripts/test_http.sh`、`scripts/test.sh`、`CMakeLists.txt` | `docs/测试计划与用例说明.md`、`docs/接口联调记录.md` |
| S28 | 已完成 | 前端页面级验收脚本 | `scripts/test_ui.sh`、`tests/ui/`、`assets/app/` | `docs/测试计划与用例说明.md`、`docs/前端演示模块说明.md` |
| S29 | 已完成 | 完整前端部署、演示数据和答辩脚本接入 | `scripts/deploy/`、`config/`、`sql/demo_data.sql`、`assets/app/` | `docs/部署与答辩说明.md`、`docs/答辩演示操作手册.md` |
| S30 | 已完成 | 完整前端最终回归和 handoff | 全仓库 | `docs/schedule.md`、`docs/部署与答辩说明.md`、`docs/答辩演示操作手册.md` |

## 6. 当前任务：已完成 S30

### 6.1 S30 目标

在 S29 已经完成 `/demo` 与 `/app` 双入口部署演示链路的基础上，执行完整最终回归，确认课程设计交付材料、演示脚本、测试入口和 handoff 一致。

S30 已完成。当前仓库进入课程设计交付和答辩复现状态。

### 6.2 S30 开始前必须读取

- `docs/部署与答辩说明.md`
- `docs/答辩演示操作手册.md`
- `docs/前端演示模块说明.md`
- `scripts/deploy/`
- `config/`
- `sql/demo_data.sql`
- `assets/app/`

### 6.3 S30 建议范围

- 保持当前静态 HTML/CSS/JS 路线，不引入 Node/Vite/React 构建链。
- 执行 `scripts/deploy/verify_release.sh` 和必要的分层测试，确认 `/demo`、`/app`、真实 HTTP 基线、前端页面级验收和高风险专项仍通过。
- 复核部署与答辩说明、答辩演示操作手册、前端演示模块说明和测试计划是否与代码现状一致。
- 输出最终 handoff，明确验证结果、剩余风险和交付边界。

S30 不新增业务功能；只有发现最终回归、文档一致性或答辩复现链路缺陷时才做最小修复。

S30 实际未新增业务功能，未引入付费工具、Node/Vite/React 构建链、微服务、MQ、分布式锁、CQRS 或事件溯源。

### 6.4 S30 风险点

- 不要为了最终交付引入付费服务或额外前端构建链。
- `/demo` 只能作为只读答辩演示台，不能替代 `/app` 真实业务前端入口。
- 不要为了演示便利放宽后端 RBAC、支付验签、状态机或审计约束。
- 部署脚本应优先复用现有本地 MySQL、CMake、Drogon 服务和静态资源托管方式。

## 7. 常用命令和验证规则

常用本地命令：

- `git status`
- `cmake -S . -B build`
- `cmake --build build`
- `ctest --test-dir build --output-on-failure`
- `scripts/test.sh smoke`
- `scripts/test.sh module`
- `scripts/test.sh contract`
- `scripts/test.sh http`
- `scripts/test.sh ui`
- `scripts/test_http.sh`
- `./build/bin/auction_app --check-config`
- `scripts/deploy/init_demo_env.sh`
- `scripts/deploy/run_demo_server.sh`
- `scripts/deploy/verify_release.sh`

验证和更新规则：

1. 只写文档不算模块完成。
2. 只创建空目录或空文件不算模块完成。
3. 每个步骤完成时必须有代码、可运行验证、测试或冒烟记录、对应模块文档更新和 schedule 更新。
4. 依赖本地 MySQL 或 HTTP 监听的测试在 Codex 沙箱内可能失败；如果是沙箱权限导致，按权限流程请求在沙箱外执行，不要用 sudo 绕过。
5. 若 `build/test_mysql/data/ibdata1` 被残留 `mysqld` 锁住，只清理本项目测试 MySQL 进程，不要影响系统级 MySQL。
6. 修改前先检查工作区，不能回滚用户已有修改。

## 8. 后续更新规则

每完成一个步骤，至少更新以下内容：

- 本文件的当前进度、Step 状态表和当前任务。
- 对应模块文档的“已实现/未实现”、接口、错误场景和验证入口。
- `docs/测试计划与用例说明.md` 中的测试覆盖和执行方式。
- 必要时更新 `docs/接口联调记录.md`、`docs/部署与答辩说明.md` 或 `docs/答辩演示操作手册.md`。

每次 handoff 保持精简，写清楚：

```md
## 模块 Handoff

- Step ID:
- 当前状态:
- 已完成:
- 修改文件:
- 验证命令:
- 验证结果:
- 未覆盖风险:
- 下一步:
- 下一步必须先读:
```

历史细节不要继续堆在本文件中；能从代码、测试和模块文档恢复的内容，不要重复粘贴。

## 9. 最近模块 Handoff

- Step ID: S30
- 当前状态: 已完成
- 已完成: 复核 S30 必读上下文、部署与答辩说明、答辩操作手册、前端说明、测试计划、部署脚本、配置、演示 SQL 和 `/app` 静态资源；执行完整发布验证，确认 `/demo` 只读演示台、`/app` 真实业务工作台、真实 HTTP 基线、前端页面级验收、高风险专项和全量 CTest 均通过；同步更新最终 handoff 和交付边界。
- 修改文件: `docs/schedule.md`、`docs/接口联调记录.md`、`docs/部署与答辩说明.md`、`docs/答辩演示操作手册.md`、`docs/前端演示模块说明.md`、`docs/测试计划与用例说明.md`、`docs/测试报告.md`。
- 验证命令: `bash -n scripts/deploy/init_demo_env.sh`、`bash -n scripts/deploy/run_demo_server.sh`、`bash -n scripts/deploy/show_demo_walkthrough.sh`、`bash -n scripts/deploy/verify_release.sh`、`bash -n scripts/test.sh`、`bash -n scripts/test_ui.sh`、`scripts/deploy/verify_release.sh`。
- 验证结果: 脚本语法检查通过；`scripts/deploy/verify_release.sh` 通过，已初始化 `auction_demo`，检查 `/healthz`、`/demo`、`/app`、`/assets/demo/app.css`、`/assets/demo/app.js`、`/assets/app/app.css`、`/assets/app/app.js` 和 `/api/demo/dashboard`，再执行 `scripts/test.sh frontend`、`scripts/test.sh ui`、`scripts/test.sh http`、`scripts/test.sh risk` 和全量 `ctest --test-dir build --output-on-failure`；最终 `ctest 18/18` 通过。
- 未覆盖风险: 未新增浏览器驱动截图、真实点击流、前端性能压测或 200 并发在线验证；当前课程设计交付以静态页面合同、真实 HTTP 回归、高风险专项和全量 CTest 作为验收证据。当前支付仍为 mock 支付适配器，不连接真实第三方支付平台。
- 下一步: 按需进行答辩复现、演示运行或提交材料；若用户后续要求新增功能或缺陷修复，先重新读取本文件和相关模块文档。
- 下一步必须先读: `docs/schedule.md`、`docs/需求规格说明书.md`、`docs/系统概要设计报告.md`、`docs/部署与答辩说明.md`、`docs/答辩演示操作手册.md`、`docs/前端演示模块说明.md`、`docs/测试计划与用例说明.md`。
