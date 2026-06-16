# 在线拍卖平台 Agent 执行 Schedule

## 新会话最小恢复卡片

- 当前主 Step: `Final-Closeout`
- 当前唯一活动任务: 答辩演示流程与浏览器自动化验证收口
- 当前执行阶段: `Defense-Demo-E2E-Closeout`
- 当前状态: `已完成`
- 当前真实状态:
  - `S00-S15` 已完成
  - `F16` 已完成
  - `F17` 真实 HTTP 与真实 WebSocket 已接入完成
  - `R2/R3/R6/R7` 最终收口已完成
- 当前唯一主入口:
  - 前端: `frontend/`，默认地址 `http://127.0.0.1:3000`
  - 后端: `./build/bin/auction_app` 提供真实 `/api/...` 与 `/ws/...`
  - 一键启动: `./start.sh --config config/app.local.json`
- 当前支付口径:
  - 保留本地测试支付模型
  - 前端支付页传 `confirm_success=true` 时由后端本地 mock 成功回调推进订单
  - 未接入真实第三方支付平台
- 最近一次已通过验证:
  - `bash -n start.sh`
  - `bash -n scripts/test.sh`
  - `bash -n scripts/test_frontend.sh`
  - `bash -n scripts/deploy/verify_release.sh`
  - `git diff --check`
  - `./start.sh --config config/app.local.json`
  - `scripts/deploy/verify_release.sh`
  - `cmake --build build`
  - `ctest --test-dir build --output-on-failure`
  - `scripts/test.sh http`
  - `scripts/test.sh risk`
  - `scripts/test.sh item`
  - `scripts/test.sh auction`
  - `scripts/test.sh bid`
  - `scripts/test.sh payment`
  - `scripts/test.sh integration`
  - `scripts/test.sh frontend`
  - `scripts/test.sh e2e`
  - `cd frontend && npm run test:e2e -- tests/e2e/defense-flow.spec.ts`
  - `cd frontend && npm run typecheck`
  - `cd frontend && npm run build`
  - `cd frontend && npm run test:e2e`
  - `git diff --check`
- 当前阻塞/风险:
  - Codex 沙箱内可能限制本地 MySQL socket/TCP 监听；依赖 MySQL 的测试需在本机非沙箱环境复验
  - `build/local_mysql/runtime` 是 `start.sh` fallback 的本地持久运行库，不应作为普通缓存清理
  - `build/test_mysql/run-*` 是测试临时库，可由测试脚本自行管理
  - 当前仓库仍保留部分历史 `/app` 与 `/demo` 说明，但不能写成当前主验收入口
- 下一步唯一动作:
  - 后续仅按新增验收反馈做增量维护
- 优先阅读文件:
  - [schedule.md](/home/ljh/project/soft_course_design/docs/schedule.md)
  - [frontend-next-schedule.md](/home/ljh/project/soft_course_design/docs/frontend-next-schedule.md)
  - [frontend/API_READINESS.md](/home/ljh/project/soft_course_design/frontend/API_READINESS.md)
  - [部署与答辩说明.md](/home/ljh/project/soft_course_design/docs/部署与答辩说明.md)
  - [接口联调记录.md](/home/ljh/project/soft_course_design/docs/接口联调记录.md)

## 当前 Handoff

### 1. 基本信息

- Step ID: `Final-Closeout/Defense-Demo`
- 模块名称: 答辩演示流程与浏览器自动化验证收口
- 当前状态: `已完成`
- 统一进度文档: [schedule.md](/home/ljh/project/soft_course_design/docs/schedule.md)
- 前端接入状态: [frontend-next-schedule.md](/home/ljh/project/soft_course_design/docs/frontend-next-schedule.md)、[frontend/API_READINESS.md](/home/ljh/project/soft_course_design/frontend/API_READINESS.md)

### 2. 本轮实际处理

- 新增并跑通答辩浏览器链路:
  - `frontend/tests/e2e/defense-flow.spec.ts` 覆盖注册、登录、发布、审核、建拍、双买家出价、支付、发货、收货、评价、通知、统计和运维入口
  - `frontend/public/demo-auction.svg` 作为现场发布页稳定图片 URL，避免依赖外网图片
- 更新答辩演示文档:
  - `docs/答辩演示操作手册.md` 已改为逐步操作脚本，包含每一步按钮、账号、字段值、预期验证点和自动化命令
  - `docs/演示账号与全功能测试流程.md` 已同步短时拍卖、真实调度结算和本地演示图口径
  - `frontend/API_READINESS.md` 与 `docs/frontend-next-schedule.md` 已记录专用 E2E 与演示资产
- 已验证:
  - `cd frontend && npm run test:e2e -- tests/e2e/defense-flow.spec.ts`
  - `scripts/test.sh e2e`
  - `cd frontend && npm run typecheck`
  - `git diff --check`
- 清理可再生成资源:
  - 已删除 `frontend/.next`
  - 已删除 `.cache/clangd`
  - 未清理 `build/`、`data/`、`config/app.local.json`，避免破坏本地运行库、上传数据或本地配置
- 清理冗余文档:
  - 已删除完成后不再作为恢复入口的一次性计划文档
  - `schedule.md` 已收缩为短恢复入口，不再保留长历史流水
  - `docs/frontend-next-schedule.md` 已收缩为前端最终态与维护规则
- 文档口径要求:
  - 当前主入口必须写作 `frontend/` + 真实 HTTP/WS
  - `/app` 只能作为旧阶段兼容/历史页面说明
  - `/demo` 只能作为历史只读演示形态说明
  - 支付只能写作本地测试支付模型或本地 mock 成功闭环
- 已同步修正:
  - `README.md`
  - `frontend/README.md`
  - `docs/认证与权限模块说明.md`
  - `docs/物品与审核模块说明.md`
  - `docs/拍卖管理模块说明.md`
  - `docs/竞价与实时通知模块说明.md`
  - `docs/订单与支付模块说明.md`
  - `docs/评价与反馈模块说明.md`
  - `docs/统计分析与报表模块说明.md`
  - `docs/系统监控与异常处理模块说明.md`
  - `docs/测试计划与用例说明.md`
  - `docs/部署与答辩说明.md`
  - `docs/接口联调记录.md`
  - `docs/演示账号与全功能测试流程.md`

### 3. 当前交付清单

- 后端模块:
  - `src/modules/auth`
  - `src/modules/item`
  - `src/modules/audit`
  - `src/modules/auction`
  - `src/modules/bid`
  - `src/modules/order`
  - `src/modules/payment`
  - `src/modules/review`
  - `src/modules/notification`
  - `src/modules/statistics`
  - `src/modules/ops`
- HTTP/WS 接入:
  - `src/access/http/auth_http.*`
  - `src/access/http/item_http.*`
  - `src/access/http/admin_http.*`
  - `src/access/http/auction_admin_http.*`
  - `src/access/http/auction_public_http.*`
  - `src/access/http/auction_http.*`
  - `src/access/http/bid_http.*`
  - `src/access/http/order_http.*`
  - `src/access/http/payment_http.*`
  - `src/access/http/review_http.*`
  - `src/access/http/notification_http.*`
  - `src/access/http/statistics_http.*`
  - `src/access/http/ops_http.*`
  - `src/access/http/auction_ws.*`
- 前端页面:
  - `frontend/app/page.tsx`
  - `frontend/app/auth/login/page.tsx`
  - `frontend/app/auction/hall/page.tsx`
  - `frontend/app/auction/detail/[id]/page.tsx`
  - `frontend/app/auction/publish/page.tsx`
  - `frontend/app/account/items/page.tsx`
  - `frontend/app/orders/page.tsx`
  - `frontend/app/checkout/[orderId]/page.tsx`
  - `frontend/app/notifications/page.tsx`
  - `frontend/app/admin/dashboard/page.tsx`
- 答辩演示与浏览器验证:
  - `docs/答辩演示操作手册.md`
  - `docs/演示账号与全功能测试流程.md`
  - `frontend/public/demo-auction.svg`
  - `frontend/tests/e2e/defense-flow.spec.ts`

## 当前验证入口

- 快速静态验证:
  - `bash -n start.sh`
  - `bash -n scripts/test.sh`
  - `bash -n scripts/test_frontend.sh`
  - `bash -n scripts/deploy/verify_release.sh`
  - `git diff --check`
- 后端验证:
  - `cmake -S . -B build`
  - `cmake --build build`
  - `ctest --test-dir build --output-on-failure`
  - `scripts/test.sh http`
  - `scripts/test.sh risk`
- 前端验证:
  - `cd frontend && npm run typecheck`
  - `cd frontend && npm run build`
  - `cd frontend && npm run test:e2e`
  - `cd frontend && npm run test:e2e -- tests/e2e/defense-flow.spec.ts`
- 发布验证:
  - `./start.sh --config config/app.local.json`
  - `scripts/deploy/verify_release.sh`
  - `scripts/deploy/verify_release.sh --full`

## 历史阶段摘要

- `S00-S15`：后端、数据库、核心模块、测试基线、部署与答辩材料完成
- `F16`：Next.js 前端工程、设计系统、主页面骨架和本地交互闭环完成
- `F17`：真实 Drogon HTTP、WebSocket、通知、订单履约、评价、统计与运维入口完成
- `R2/R3/R6/R7`：启动、单一真实入口、发布门禁、handoff 收口完成

历史细节以各模块文档、测试文档、联调记录、代码和测试文件为准，本文件不再追加长流水。

## 后续更新规则

1. 任何改变实现状态、验证状态、handoff、下一步或前端 readiness 的任务，都必须同步更新本文件。
2. `schedule.md` 只保留恢复必需信息，不再追加逐日流水。
3. 若任务涉及前端或 F17 接入口径，还必须同步：
   - [frontend-next-schedule.md](/home/ljh/project/soft_course_design/docs/frontend-next-schedule.md)
   - [frontend/API_READINESS.md](/home/ljh/project/soft_course_design/frontend/API_READINESS.md)
4. 若代码与文档不一致，先以代码现状为准，再同步修正文档。

## 新会话固定恢复文本

```text
这是 /home/ljh/project/soft_course_design 项目。

开始前先读取：
1. docs/schedule.md
2. docs/frontend-next-schedule.md
3. frontend/API_READINESS.md
4. 若任务属于后端启动/验证，再读取 start.sh、scripts/、scripts/deploy/、config/、sql/、tests/、CMakeLists.txt

当前主任务：最终收尾后的增量维护
当前状态：S00-S15 已完成；F16 已完成；F17 真实 HTTP/WS 已接入完成；R2/R3/R6/R7 最终收口已完成

当前必须遵守：
- 以代码现状为准
- schedule.md 只保留短恢复入口，不再扩写成长历史流水
- 单一真实业务入口：frontend + 真实 HTTP/WS
- 支付保留，但只做本地测试支付模型
- 不清理 build/local_mysql/runtime，避免破坏本地持久演示数据
- 每次改变实现状态、验证状态、handoff、下一步，都同步更新 docs/schedule.md
- 始终用中文
```
