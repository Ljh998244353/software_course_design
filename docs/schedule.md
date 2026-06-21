# 在线拍卖平台 Agent 执行 Schedule

## 新会话最小恢复卡片

- 当前主 Step: `Final-Closeout`
- 当前唯一活动任务: `final-test` 4 人份统一全量测试总报告与证据收口
- 当前执行阶段: `Final-Test-4Person-Report-Passed`
- 当前状态: `4 人份统一总报告已完成；后端 CTest/HTTP/risk/security、前端 typecheck/build/Playwright、发布门禁、快速性能探针、JMeter 全量负载和 JUnit + Selenium WebDriver 黑盒专项已通过`
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
  - `cd slides && npm run build`
  - `git diff --check`
  - `cmake -S . -B build -DCMAKE_CXX_COMPILER=/usr/bin/c++ -DCMAKE_C_COMPILER=/usr/bin/cc`
  - `cmake --build build`
  - `ctest --test-dir build --output-on-failure`
  - `cd frontend && npm run typecheck`
  - `cd frontend && npm run build`
  - `cd frontend && npm run test:e2e`
  - `scripts/deploy/verify_release.sh --full`
  - `docs/hw/jmeter-test.sh`
  - `final-test/selenium/run-selenium.sh`
  - `FINAL_TEST_RUN_ID=finaltest-20260621-1444 final-test/selenium/run-selenium.sh`
- 当前阻塞/风险:
  - Codex 沙箱内可能限制本地 MySQL socket/TCP 监听；依赖 MySQL 的测试需在本机非沙箱环境复验
  - `build/local_mysql/runtime` 是 `start.sh` fallback 的本地持久运行库，不应作为普通缓存清理
  - `build/test_mysql/run-*` 是测试临时库，可由测试脚本自行管理
  - 当前仓库仍保留部分历史 `/app` 与 `/demo` 说明，但不能写成当前主验收入口
  - 当前 `frontend/tests/e2e/defense-flow.spec.ts` 属于 Playwright 浏览器交互与接口契约测试，真实后端事实仍以 HTTP/CTest/JMeter 验证为准
  - `docs/hw/` 与根目录 `jmeter.log` 已作为本地测试材料/日志加入 `.gitignore`，不会进入普通 Git 提交清单
  - Codex 沙箱内 CTest 的 MySQL 依赖测试会因 mysqld TCP socket `Operation not permitted` 失败；本轮已在非沙箱环境复验通过
  - JMeter 全量负载专项已在非沙箱临时 MySQL/后端环境执行通过，结果归档到 `final-test/performance/jmeter/finaltest-20260618-1425/`
  - JUnit + Selenium WebDriver 工程已纳入 `final-test/selenium/` 并通过，结果归档到 `final-test/selenium/junit-report/finaltest-20260618-220300/`
  - 本轮 `finaltest-20260621-1444` 已完成 4 人份统一总报告、JMeter 归档和 Selenium 归档
  - `scripts/db/setup_local_mysql.sh` 已修复测试 MySQL 默认目录 `run-$$` 可能复用导致的 `ibdata1` lock 冲突，改为短随机运行目录
  - Selenium 4.21 对缓存 Chrome 149 输出 CDP 版本 warning；当前用例仅使用普通 WebDriver 页面操作，不影响通过结论
  - 用户反馈的 `BUG-FE-001` 通知中心已读显示无法连接后端、`BUG-FE-002` 管理员统计页疑似不可用均已复现、修复并回归通过
- 下一步唯一动作:
  - 若最终提交要求 Word 文件，再确认导出工具和封面格式后从 `final-test/软件系统综合课程设计-全量测试总报告.md` 导出 `.docx`
- 优先阅读文件:
  - [schedule.md](/home/ljh/project/soft_course_design/docs/schedule.md)
  - [frontend-next-schedule.md](/home/ljh/project/soft_course_design/docs/frontend-next-schedule.md)
  - [frontend/API_READINESS.md](/home/ljh/project/soft_course_design/frontend/API_READINESS.md)
  - [final-test/全量测试计划书.md](/home/ljh/project/soft_course_design/final-test/全量测试计划书.md)
  - [final-test/软件系统综合课程设计-全量测试总报告.md](/home/ljh/project/soft_course_design/final-test/软件系统综合课程设计-全量测试总报告.md)
  - [final-test/缺陷与修复记录.md](/home/ljh/project/soft_course_design/final-test/缺陷与修复记录.md)
  - [部署与答辩说明.md](/home/ljh/project/soft_course_design/docs/部署与答辩说明.md)
  - [接口联调记录.md](/home/ljh/project/soft_course_design/docs/接口联调记录.md)

## 当前 Handoff

### 1. 基本信息

- Step ID: `Final-Test/Planning`
- 模块名称: 4 人份统一全量测试总报告、测试记录和证据归档
- 当前状态: `统一总报告、主线门禁、JMeter 负载专项和 Selenium 黑盒专项已通过`
- 统一进度文档: [schedule.md](/home/ljh/project/soft_course_design/docs/schedule.md)
- 前端接入状态: [frontend-next-schedule.md](/home/ljh/project/soft_course_design/docs/frontend-next-schedule.md)、[frontend/API_READINESS.md](/home/ljh/project/soft_course_design/frontend/API_READINESS.md)

### 2. 本轮实际处理

- 新增全量测试计划:
  - 本轮按用户要求完成 `final-test/软件系统综合课程设计-全量测试总报告.md`：统一成一份报告，内部按 4 人份组织认证/物品审核、拍卖竞价、订单支付通知、统计运维性能安全四类任务，每份均包含 20+ 测试用例、测试方法、脚本映射和证据索引
  - 本轮批次 `finaltest-20260621-1444` 已通过：`ctest --test-dir build --output-on-failure` 16/16、`scripts/test.sh http`、`scripts/test.sh risk`、`scripts/test.sh security`、`cd frontend && npm run typecheck`、`cd frontend && npm run build`、`cd frontend && npm run test:e2e`、`scripts/deploy/verify_release.sh --full`、`docs/hw/jmeter-test.sh`、`FINAL_TEST_RUN_ID=finaltest-20260621-1444 final-test/selenium/run-selenium.sh`
  - 本轮 JMeter 3840 样本 0 失败，归档到 `final-test/performance/jmeter/finaltest-20260621-1444/`
  - 本轮 Selenium 4/4 通过，归档到 `final-test/selenium/junit-report/finaltest-20260621-1444/`、`screenshots/`、`console/`、`logs/`
  - 本轮修复测试环境缺陷 `BUG-TEST-002`：`scripts/db/setup_local_mysql.sh` 默认运行目录由 `run-$$` 改为短随机目录，避免 PID 复用造成测试 MySQL 数据目录锁冲突，并随机化 TCP fallback 端口
  - `final-test/全量测试计划书.md` 已覆盖测试目标、范围、工具、环境、黑盒/白盒/单元/浏览器/性能/安全/可靠性方法、执行顺序、结果归档结构和通过准则
  - 本轮继续强化计划：新增需求/模块/风险覆盖追踪矩阵、测试数据隔离策略、分层门禁、API 契约记录要求、故障注入矩阵、Selenium 负向/多浏览器/响应式/会话失效用例、性能执行规则、flaky 处理和 P0/P1 准入规则
  - 当前计划明确区分现有可直接执行入口与后续执行阶段需要补充的浏览器真实后端观察、截图、日志和缺陷记录
  - 已按用户要求将黑盒浏览器主工具调整为 `JUnit 5 + Selenium WebDriver`，Playwright 保留为 trace/截图/快速回归辅助工具
  - 已新增 Selenium 黑盒测试表，覆盖认证、发布、审核建拍、竞价、通知已读、管理员统计、订单履约、评价、RBAC 和 WebSocket 降级
  - 已新增 `final-test/缺陷与修复记录.md`，先登记 `BUG-FE-001` 通知已读无法连接后端和 `BUG-FE-002` 管理员统计页疑似不可用
  - 已新增 `final-test/测试执行记录.md`，记录 `finaltest-20260618-1425` 批次 T0 环境与基线确认结果
  - 已将 `docs/hw/` 与根目录 `jmeter.log` 加入 `.gitignore`，确认 `git check-ignore` 命中；`docs/hw/` 不再出现在普通待提交清单
  - 已完成 T0 准备检查：`ctest --test-dir build -N` 注册 16 项，`build/bin/auction_app` 可执行，`frontend/node_modules` 存在，JMeter 位于 `/opt/jmeter/bin/jmeter`
  - T1 静态/语法检查通过：`bash -n start.sh`、`bash -n scripts/test.sh`、`bash -n scripts/test_frontend.sh`、`bash -n scripts/deploy/verify_release.sh`、`git diff --check`
  - T2 构建通过：初次构建因旧缓存 Windows clang 失败，切换 Linux `/usr/bin/c++` 后构建通过
  - T3 后端门禁通过：沙箱内 MySQL 依赖测试因 socket 权限失败；非沙箱首轮发现 `BUG-TEST-001` 测试数据缺少合法 `suggested_bid_step`；修复后 `ctest --test-dir build --output-on-failure` 16/16 通过
  - T7/T8 前端通过：`cd frontend && npm run typecheck`、`cd frontend && npm run build`、`cd frontend && npm run test:e2e` 均通过，Playwright 10/10
  - T9 快速性能探针通过：`/healthz` 20 并发 100 请求 P95 42.84ms；`/api/auctions` 200 并发 200 请求 P95 88.52ms
  - T10 发布门禁通过：`scripts/deploy/verify_release.sh --full` 通过，包含前端 release-check、真实后端健康检查、性能探针、HTTP、risk/security、完整 CTest 和 Playwright E2E
  - T11 JMeter 全量负载专项通过：`docs/hw/jmeter-test.sh` 运行 3840 个样本，0 失败；结果归档到 `final-test/performance/jmeter/finaltest-20260618-1425/`
  - 本轮按练习 3 DOCX 和 `docs/hw/3-性能测试（Jmeter） .pdf` 补齐 `docs/hw/练习3个人负载测试计划书-在线拍卖平台.md` 与 `docs/hw/练习3个人实验报告-负载测试.md`，统一为 2026-06-18 全场景 JMeter 实测口径：3840 样本、0 失败、CPU 平均 busy 2.21%、峰值 13.00%，复现入口为 `docs/hw/jmeter-test.sh`
  - 已同步更新 `docs/hw/assets/hw3-load-test-results.png`，结果图覆盖列表延迟/吞吐量、详情/通知/出价/混合业务响应时间和资源采样摘要
  - 本轮按用户要求在 `docs/hw/练习3个人实验报告-负载测试.md` 末尾追加“测试设计代码”附录，包含 JMeter 场景配置、非 GUI 执行参数、测试数据准备、JTL 汇总统计和资源采样代码片段
  - 本轮按 `docs/hw/测试方法.pdf` 中场景法规范，在 `docs/hw/练习2个人测试报告-竞价与拍卖.md` 末尾追加火车票退票业务黑盒测试补充作业，覆盖基本流、备选流、场景组合、12 条测试用例和关键时间/尾数取整边界
  - 本轮为 `docs/hw/final/软件系统综合课程设计——练习2（白盒+单元测试） .docx` 重新生成 PlantUML 控制流图，修正 U2/U3 图中文字截断和缺失状态问题，并保留 `.puml` 源文件与 PNG 导出图
  - 已新增并通过 JUnit + Selenium WebDriver 黑盒专项：`final-test/selenium/run-selenium.sh`
  - Selenium 覆盖认证会话/RBAC、通知已读回归、管理员统计回归、卖家发布-管理员审核建拍闭环
  - Selenium 批次 `finaltest-20260618-220300` 结果为 4/4 通过，报告归档到 `final-test/selenium/junit-report/finaltest-20260618-220300/`
  - Selenium 同步复现并修复 `BUG-FE-001` 与 `BUG-FE-002`
- 新增答辩 Slidev 材料:
  - `slides/slides.md` 覆盖项目背景、需求边界、架构、数据模型、竞价一致性、支付幂等、前端接入、演示流程和测试证据
  - `slides/assets/architecture-stack.png`、`slides/assets/auction-flow-ai.png`、`slides/assets/consistency-ai.png` 用于具体技术栈架构、业务闭环和竞价一致性说明
  - `slides/assets/architecture-backdrop.svg`、`slides/assets/auction-flow-backdrop.svg`、`slides/assets/consistency-backdrop.svg` 保留为可编辑备用图
  - `slides/slides.md` 已将字体栈调整为中文优先：`Noto Sans SC` / `Source Han Sans SC` / `Microsoft YaHei` / `PingFang SC`，标题使用中文衬线优先 fallback
  - `slides/theme/` 为从本地 academic deck master 复制的主题副本，master theme 未移动或修改
  - 已验证 `cd slides && npm run build`
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
- 全量测试计划与记录:
  - `final-test/全量测试计划书.md`
  - `final-test/软件系统综合课程设计-全量测试总报告.md`
  - `final-test/测试执行记录.md`
  - `final-test/缺陷与修复记录.md`
  - `final-test/selenium/pom.xml`
  - `final-test/selenium/run-selenium.sh`
  - `final-test/selenium/src/test/java/local/auction/FinalSeleniumTest.java`
- 答辩演示与浏览器验证:
  - `slides/slides.md`
  - `slides/assets/architecture-stack.png`
  - `slides/assets/auction-flow-ai.png`
  - `slides/assets/consistency-ai.png`
  - `slides/assets/architecture-backdrop.svg`
  - `slides/assets/auction-flow-backdrop.svg`
  - `slides/assets/consistency-backdrop.svg`
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
- 答辩 Slidev 验证:
  - `cd slides && npm run build`
  - `cd slides && npm run dev`
- 发布验证:
  - `./start.sh --config config/app.local.json`
  - `scripts/deploy/verify_release.sh`
  - `scripts/deploy/verify_release.sh --full`
- Selenium 黑盒验证:
  - `final-test/selenium/run-selenium.sh`

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
4. 若任务属于 final-test 全量测试执行或记录维护，再读取 final-test/全量测试计划书.md 和 final-test/缺陷与修复记录.md
5. 若任务属于答辩 slide 维护，再读取 slides/slides.md
6. 若任务属于后端启动/验证，再读取 start.sh、scripts/、scripts/deploy/、config/、sql/、tests/、CMakeLists.txt

当前主任务：final-test 批次 finaltest-20260618-1425 已完成后端、前端、快速性能、发布门禁、JMeter 全量负载和 JUnit + Selenium WebDriver 黑盒专项；Selenium 通过批次为 finaltest-20260618-220300
当前状态：S00-S15 已完成；F16 已完成；F17 真实 HTTP/WS 已接入完成；R2/R3/R6/R7 最终收口已完成

当前必须遵守：
- 以代码现状为准
- schedule.md 只保留短恢复入口，不再扩写成长历史流水
- 单一真实业务入口：frontend + 真实 HTTP/WS
- 答辩 Slidev 入口：slides/slides.md
- 全量测试记录入口：final-test/
- 黑盒浏览器主工具：JUnit 5 + Selenium WebDriver；Playwright 为辅助证据工具
- 支付保留，但只做本地测试支付模型
- 不清理 build/local_mysql/runtime，避免破坏本地持久演示数据
- 每次改变实现状态、验证状态、handoff、下一步，都同步更新 docs/schedule.md
- 始终用中文
```
