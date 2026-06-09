# 在线拍卖平台 Agent 执行 Schedule

## 新会话最小恢复卡片

- 当前主 Step: `Graduation-Excellence`
- 当前唯一活动任务: 毕业设计需求覆盖闭环与满分答辩证据补强
- 当前执行阶段: `Requirements-Coverage-Audit`
- 当前状态: `已完成`
- 最近一次已通过验证:
  - `bash -n start.sh`
  - `bash -n scripts/db/setup_local_mysql.sh`
  - `cmake --build build`
  - `scripts/test.sh auction`（非沙箱环境，需本地 MySQL socket/TCP）
  - `scripts/test.sh bid`（非沙箱环境，需本地 MySQL socket/TCP）
  - `git diff --check`
  - `scripts/test.sh http`
  - `scripts/test.sh risk`
  - `ctest --test-dir build --output-on-failure`
  - `cd frontend && npm run typecheck`
  - `cd frontend && npm run build`
  - `bash -n start.sh`
  - `cmake -S . -B build`
  - `./start.sh --backend-only`（已验证可自动重置被 Windows clang 污染的 CMake cache，并完成全量构建）
  - `bash -n scripts/db/setup_local_mysql.sh`
  - `bash -n scripts/deploy/verify_release.sh`
  - `bash -n scripts/test.sh`
  - `bash -n scripts/test_frontend.sh`
  - `./start.sh --config config/app.local.json`
  - `scripts/deploy/verify_release.sh`
  - `bash -n scripts/test.sh`
  - `bash -n scripts/test_e2e.sh`
  - `cd frontend && npm run typecheck`
  - `scripts/test.sh e2e`（非沙箱环境，需本地端口监听权限）
  - `bash -n scripts/deploy/verify_release.sh`
  - `python3 -m py_compile scripts/performance_probe.py`
  - `cmake --build build`
  - `scripts/test.sh item`（非沙箱环境，1/1 通过；Codex 沙箱内临时 MySQL 启动失败）
  - `cd frontend && npm run typecheck`
  - `cd frontend && npm run build`
  - `scripts/test.sh frontend`
  - `scripts/test.sh e2e`（非沙箱环境，6/6 通过）
  - `git diff --check`
- 当前阻塞/风险:
  - 需求覆盖矩阵、注册 UI、拍品下架、订单履约/评价/通知前端闭环、运维真实数据入口、性能脚本与 E2E 骨架已补齐
  - 当前工作区已有 Playwright 相关未提交改动，需保留并在此基础上扩展，不回滚
  - Playwright Chromium E2E 已在非沙箱环境通过；Codex 沙箱内仍可能因端口监听限制失败
  - `scripts/test.sh performance` 已新增；实测需要后端正在运行，`scripts/deploy/verify_release.sh --full` 会在后端存活期间执行
  - `start.sh` 的自动 fallback MySQL 已改为复用 `build/local_mysql/runtime` 持久数据目录；测试与发布验证仍使用 `build/test_mysql/run-*` 临时库
  - `start.sh` 已修复 WSL 中误复用 `/mnt/c/.../*.exe` 编译器缓存的问题；若用户此前 build 目录已被 Windows clang 污染，脚本现在会自动重置 `CMakeCache.txt` 和 `CMakeFiles/`
  - Codex 沙箱仍无法自行复现本地 `mysqld` 监听；依赖 MySQL socket/TCP 的模块测试需在非沙箱本机环境运行
  - `schedule.md` 仍需持续保持短恢复入口，不再回涨为长历史流水
- 下一步唯一动作:
  - 后续按新增答辩问题或验收反馈做增量维护；若需要性能证据，运行 `scripts/deploy/verify_release.sh --full`
- 优先阅读文件:
  - [schedule.md](/home/ljh/project/soft_course_design/docs/schedule.md)
  - [frontend-next-schedule.md](/home/ljh/project/soft_course_design/docs/frontend-next-schedule.md)
  - [frontend/API_READINESS.md](/home/ljh/project/soft_course_design/frontend/API_READINESS.md)
  - [playwright.config.ts](/home/ljh/project/soft_course_design/frontend/playwright.config.ts)
  - [test_e2e.sh](/home/ljh/project/soft_course_design/scripts/test_e2e.sh)

## 当前真实状态

- `S00-S15`：已完成
- `F16`：已完成
- `F17`：真实 HTTP 与真实 WebSocket 已接入完成，当前只做最终产品化收口
- 当前唯一发布口径：
  - 前端唯一主入口为 `frontend/`
  - 后端唯一主入口为真实 HTTP/WS 接口
  - 支付仅保留本地测试支付模型，用于完整链路演示
- 当前主修复方向：
  - `Graduation-Excellence` 需求覆盖闭环与答辩证据补强：已完成
  - `Graduation-Excellence` Playwright 浏览器 E2E 基础设施：已完成
  - `R2` 启动链路稳定化：已完成
  - `R3` 单一真实入口收敛：已完成
  - `R6` 发布门禁收敛：已完成
  - `R7` 文档与 handoff 收口：已完成

## 当前 Handoff

### 1. 基本信息

- Step ID: `Graduation-Excellence`
- 模块名称: 需求覆盖闭环与答辩证据补强
- 当前状态: `已完成`
- 统一进度文档: [schedule.md](/home/ljh/project/soft_course_design/docs/schedule.md)
- 详细执行计划: [release-final-system-plan.md](/home/ljh/project/soft_course_design/docs/release-final-system-plan.md)

### 2. 本轮前已完成的关键修复

- `start.sh` 已改为真实产品一键启动入口，默认优先 `config/app.local.json`
- 配置缺失时会自动复制 `config/app.example.json` 为本地模板并退出
- 启动前会执行：
  - `auction_app --check-config`
  - `auction_app --check-db`
- 数据库检查失败时会自动尝试 `scripts/db/setup_local_mysql.sh`
- fallback 运行配置已修复为使用应用账号 `auction_user / change_me`，不再误写 root 管理账号
- `scripts/deploy/verify_release.sh` 已改为自动初始化本地测试 MySQL 并生成真实测试配置，不再直接用 `config/app.example.json`
- 前端总入口脚本已统一为 [scripts/test_frontend.sh](/home/ljh/project/soft_course_design/scripts/test_frontend.sh)
- 大部分旧 `demo` 主入口口径已从测试、部署、联调、演示文档中退出

### 3. 本轮实际完成

- 已新增 [毕业设计需求覆盖矩阵.md](/home/ljh/project/soft_course_design/docs/毕业设计需求覆盖矩阵.md)，按需求规格说明书覆盖系统目标、8 个模块、主流程、非功能需求和验证证据
- 已补齐卖家拍品下架：
  - 后端新增 `POST /api/items/{id}/offline`
  - 服务层仅允许卖家下架自己的 `DRAFT`、`REJECTED`、`READY_FOR_AUCTION`、`UNSOLD` 拍品
  - `PENDING_AUDIT`、`IN_AUCTION`、`SOLD` 不允许下架
  - `tests/item/item_flow_tests.cpp` 已覆盖非本人拒绝和成功下架到 `OFFLINE`
- 已补齐前端需求入口：
  - `/auth/login` 支持登录/注册双模式，注册成功后自动登录
  - `/account/items` 支持查看我的拍品和下架
  - `/orders` 支持订单列表、发货、确认收货和评价入口
  - `/notifications` 支持通知列表和已读
  - `/admin/dashboard` 运维面板改为真实查询操作日志、任务日志、异常列表，并支持通知重试和补偿操作
- 已补齐验证入口：
  - `scripts/test.sh e2e` 的 Playwright 用例覆盖注册表单、下架入口、订单评价入口、通知入口和运维入口
  - `scripts/test.sh performance` 使用 Python 标准库生成 `build/performance_report.json` 与 `build/performance_report.md`
  - `scripts/deploy/verify_release.sh --full` 在发布验证中额外串联 E2E 与性能验证
- 已修正文档旧口径：
  - 旧提交审核接口名统一修正为当前真实 `submit-review`
  - 当前 `ctest -N` 为 16 项
  - Playwright 与性能脚本已接入，文档不再保留过时的缺失表述

- 已修复重启后拍卖历史丢失的根因：
  - 业务仓储层本身已使用 MySQL 持久化 `auction`、`bid_record`、`order_info` 等交易事实
  - 问题来自 `start.sh` 在配置数据库不可用时调用 `scripts/db/setup_local_mysql.sh`，而该脚本默认使用 `build/test_mysql/run-$$` 临时目录
  - `start.sh` 现已显式传入 `AUCTION_TEST_MYSQL_ROOT_DIR=build/local_mysql/runtime`，使产品启动 fallback 复用同一数据目录
  - 启动结束只停止本次自动拉起的 `mysqld`，不删除 `build/local_mysql/runtime`，因此后续重启可继续读取历史拍卖、出价、订单和支付数据
- 已保持测试库隔离：
  - `scripts/test_*.sh` 与 `scripts/deploy/verify_release.sh` 未改动，继续使用 `build/test_mysql/run-*` 临时库
  - 测试和发布验证不会污染 `start.sh` 的本地持久运行库
- 已同步文档：
  - [部署与答辩说明.md](/home/ljh/project/soft_course_design/docs/部署与答辩说明.md)
  - [环境配置说明.md](/home/ljh/project/soft_course_design/docs/环境配置说明.md)
- 已修复前端大厅/首页对拍卖列表真实响应格式的兼容缺陷：
  - 当前运行后端 `/api/auctions` 返回 `{ list, pageNo, pageSize, total }` 与 camelCase 字段
  - 前端此前仍按“直接数组 + snake_case”解析，导致在后端正常返回空列表时被误判为加载失败
  - `frontend/lib/api/client.ts` 现已兼容两套真实返回结构；`frontend/types/auction.ts` 已补齐对应类型
- 已修复登录后访问公开拍卖接口的 CORS 缺陷：
  - 浏览器在跨端口请求且携带 `Authorization` 时会先发 `OPTIONS` 预检
  - `/api/auctions`、`/api/auctions/{id}`、`/api/auctions/{id}/price`、`/api/auctions/{id}/bids` 这组公开接口此前缺少预检处理与统一 CORS 响应头
  - 现已在 `src/access/http/auction_public_http.cpp` 补齐预检，并在 `src/common/http/http_utils.cpp` 为统一 JSON 响应补齐 CORS 响应头
- 已修复公开拍卖路由的 Drogon 路径占位符错误：
  - `src/access/http/auction_public_http.cpp` 误把 `registerHandler` 路径写成 `/api/auctions/{1}` / `{1}/price` / `{1}/bids`
  - 该写法会在应用启动阶段触发 `Parameter placeholder(value=1) out of range`，导致后端进程提前退出
  - 现已统一改为与仓库其余路由一致的命名占位符写法 `/api/auctions/{id}...`
- 已修复前端发布页重复图片 URL 导致的 React duplicate key 报错：
  - `frontend/app/auction/publish/page.tsx` 现在会在草稿恢复和手动添加时去重
  - 图片网格渲染 key 已改为稳定唯一组合值，不再直接使用 URL
- 已补齐发布页图片撤回与校验能力：
  - 图片卡片右上角现已提供删除按钮，可直接撤回单张图片并同步更新草稿
  - 添加图片前会校验 URL 是否为 `http/https`、是否为常见图片格式，以及资源是否可实际加载；不合法时会直接报错
- 已新增演示账号与全功能测试流程文档：
  - `docs/演示账号与全功能测试流程.md`
  - 记录管理员/普通用户演示账号、备用账号以及从发布、审核、建拍、竞价、支付、履约到评价和统计的完整演示脚本
- 已修复管理端审核抽屉不可滚动的问题：
  - `frontend/app/admin/dashboard/page.tsx` 已改为固定头部、独立滚动内容区和固定底部操作栏
  - 审核详情信息较长时不再被遮挡，抽屉可完整滚动浏览
- 已修复管理端缺少明确退出入口的问题：
  - `frontend/app/admin/dashboard/page.tsx` 顶部已新增“退出后台”按钮
  - 当前已收口为“退出后台仅返回前台页面，不清空登录态”，避免管理员误被整体登出
- 已修复管理端审核后无法追踪的问题：
  - 审核通过/驳回后，拍品不再只是从“待审核”列表消失
  - “最近处理记录”现已持久化到浏览器本地存储，重新进入后台或重新登录后仍可继续追踪
- 已补齐管理员创建拍卖入口：
  - `frontend/app/admin/dashboard/page.tsx` 现在可直接基于“最近处理记录”里的已批准拍品填写时间和价格规则，并调用真实 `POST /api/admin/auctions` 创建正式拍卖
  - `frontend/lib/api/client.ts` 已新增 `createAdminAuction()`，对接现有管理端拍卖创建接口
- 已修复管理端审核抽屉看不到图片的问题：
  - `GET /api/admin/items/pending` 现已返回 `cover_image_url`
  - `frontend/app/admin/dashboard/page.tsx` 审核抽屉改为优先渲染真实封面图，只有缺图时才回退到占位面板
- 已修复管理端创建拍卖被前端误判为“无法连接后端服务”的问题：
  - `src/access/http/auction_admin_http.cpp` 已为 `/api/admin/auctions`、`/api/admin/auctions/{id}`、`/api/admin/auctions/{id}/cancel` 补齐 `OPTIONS` 预检和 CORS
  - 浏览器跨端口携带 `Authorization` 时现在可以正常发送真实创建拍卖请求
  - 同时已修复一处回归：管理端拍卖 Drogon 路由占位符已统一保持为命名形式 `{id}`，避免启动阶段再次触发 `Parameter placeholder(value=1) out of range`
  - 已修复管理端创建拍卖时间被错误判定为过去的问题：前端不再把 `datetime-local` 值转换成 UTC `toISOString()`，而是按后端实际约定提交本地语义的 `YYYY-MM-DD HH:MM:SS`
- 已修复同浏览器多窗口无法分别登录不同账号的问题：
  - 前端认证 token 已从 `localStorage` 改为 `sessionStorage`
  - 同一浏览器下不同窗口/标签页现在可维持各自独立的登录态，适合现场多账号并行演示
  - WebSocket 身份识别与会话恢复已同步切换到 `sessionStorage`
- 已修复首页/详情页因第三方图片域名触发的 Next 运行时崩溃：
  - 拍品图片 URL 允许用户填写任意外部地址，继续依赖 `next/image` 的域名白名单会导致首页和详情页在遇到新域名时直接报错
  - `frontend/components/auction/auction-card.tsx` 与 `frontend/app/auction/detail/[id]/page.tsx` 现已改为对用户图片使用普通 `<img>` 渲染，不再受 `next.config.mjs` 的 `images.remotePatterns` 限制
- 已修复刷新页面时前端误显示为已退出登录的问题：
  - `frontend/lib/auth-context.tsx` 现已在首次恢复会话前保持 `loading=true`，并统一从 `sessionStorage` 恢复 token
  - `frontend/components/layout/site-nav.tsx` 现已在会话恢复期间固定显示占位态，不会先闪出“登录/注册”造成误判
- 已修复首页、大厅、详情页对拍卖状态的口径不一致问题：
  - 首页此前未显式限定 `RUNNING`，会把 `PENDING_START` 拍卖也展示在“正在竞价”区域
  - 大厅默认只查询 `RUNNING`，详情页也只允许 `RUNNING` 状态出价，导致用户看到首页有拍品、大厅却为空，或详情页可见但不能竞拍
  - 现已把首页查询统一收口为只展示 `RUNNING`，并在前端保留 `acceptingBids` 字段，详情页以服务端返回的可出价标记为准
- 已修复后端运行时未自动推进拍卖状态的问题：
  - `auction_app` 启动后此前只注册了 HTTP/WS 路由，没有把拍卖开始、拍卖结束、订单结算、订单超时关闭等调度周期挂进运行时
  - 现已在 `src/common/runtime/application_bootstrap.cpp` 中注册定时任务，自动推进 `PENDING_START -> RUNNING`、`RUNNING -> SETTLING` 及后续订单结算链路
  - 管理端创建拍卖的默认开始时间也已从“10 分钟后”收紧为“1 分钟后”，避免管理员刚建拍卖就误以为应当立刻可竞拍
- 已修复 `crypt` 库探测导致的 CMake 配置失败：
  - `CMakeLists.txt` 不再只用 `find_library(CRYPT_LIBRARY crypt REQUIRED)` 的单一路径探测
  - 现已兼容 `crypt` / `xcrypt` 以及常见系统库目录，避免 Ubuntu/WSL 环境下误报 “Could not find CRYPT_LIBRARY”
- 已将 `schedule.md` 从长历史流水改写为短恢复入口
- 已把恢复信息收敛为：
  - 当前阶段
  - 当前风险
  - 最近验证
  - 唯一下一步动作
  - 精简 handoff
- 已把历史演进记录改为摘要，不再在本文件保留超长逐日流水
- 已继续清理文档中的当前态误导口径：
  - `docs/部署与答辩说明.md`
  - `docs/答辩演示操作手册.md`
  - `docs/环境配置说明.md`
  - `docs/测试计划与用例说明.md`
  - `docs/接口联调记录.md`
  - `docs/拍卖管理模块说明.md`
  - `docs/评价与反馈模块说明.md`
  - `docs/统计分析与报表模块说明.md`
  - `docs/系统监控与异常处理模块说明.md`
  - `docs/订单与支付模块说明.md`
  - `docs/前端演示模块说明.md`
  - `docs/认证与权限模块说明.md`
  - `docs/物品与审核模块说明.md`
  - `docs/测试报告.md`
- 已修正文档中“运行不存在脚本/读取不存在 SQL 或目录”的描述，改为和当前 `start.sh`、`verify_release.sh`、`frontend/`、`sql/seed.sql` 一致
- 已继续清理第二层历史口径，把多份模块文档中误写成当前现实的 `assets/app/*`、`node --check assets/app/app.js` 改为 `frontend/` 真实页面与当前测试入口
- 已将仍把 `/app` 写成当前唯一产品入口的文档，统一收口为 `frontend/` + `http://127.0.0.1:3000` 当前主入口口径
- 已修复发布门禁中的一个真实脚本缺陷：`scripts/test.sh frontend` 不再直接依赖 `scripts/test_frontend.sh` 的 Unix 可执行位，现改为显式 `bash` 调用
- 已在本机正常 shell 中完成最终实跑验收：
  - `./start.sh --config config/app.local.json`
  - `scripts/deploy/verify_release.sh`

### 4. 当前验证结果

- 已通过：
  - `bash -n start.sh`
  - `bash -n scripts/db/setup_local_mysql.sh`
  - `cmake --build build`
  - `scripts/test.sh auction`（非沙箱环境，3/3 通过）
  - `scripts/test.sh bid`（非沙箱环境，3/3 通过）
  - `git diff --check`
  - `scripts/test.sh http`
  - `scripts/test.sh risk`
  - `ctest --test-dir build --output-on-failure`
  - `cd frontend && npm run typecheck`
  - `cd frontend && npm run build`
  - `bash -n start.sh`
  - `bash -n scripts/db/setup_local_mysql.sh`
  - `bash -n scripts/deploy/verify_release.sh`
  - `bash -n scripts/test.sh`
  - `bash -n scripts/test_frontend.sh`
  - `scripts/test.sh frontend`
  - `cd frontend && npm run typecheck`
  - `cd frontend && npm run typecheck`（含管理页“退出后台/最近处理记录持久化/创建拍卖入口”修复）
  - `cd frontend && npm run typecheck`（含发布页图片删除与图片 URL/格式可访问性校验修复）
  - `cd frontend && npm run build`（含首页/详情页第三方图片域名兼容修复）
  - `cd frontend && npm run typecheck`（含刷新时登录态恢复占位修复）
  - `cd frontend && npm run build`（含首页/大厅/详情页拍卖状态口径统一修复）
  - `cd frontend && npm run typecheck`（含首页/大厅/详情页拍卖状态口径统一修复）
  - `cmake --build build`
  - `cmake --build build`（含运行时自动开拍/结束/结算调度注册修复）
  - `cd frontend && npm run typecheck`（含管理端审核抽屉图片展示与创建拍卖联调修复）
  - `cmake --build build`（含 `GET /api/admin/items/pending` 图片字段与管理端拍卖 CORS 修复）
  - `./start.sh --config config/app.local.json`
  - `scripts/deploy/verify_release.sh`

### 5. 当前风险/阻塞

- 已修复一个真实启动阻塞：`build/CMakeCache.txt` 缓存 Windows `clang++.exe` 时，`start.sh` 会在 WSL 中触发 MSVC STL 编译失败；当前脚本已固定优先 Linux 编译器并自动重置受污染的 CMake cache
- `schedule.md` 作为统一进度真源，后续只能继续保持“短恢复入口 + 当前状态摘要”结构，避免再次膨胀
- 仓库中剩余 `demo` 命中大多属于历史交付说明；继续清理时要保留“历史边界”语义，不能误改代码现状
- `start.sh` 已在本机正常 shell 中成功完成自动拉起本地 MySQL、启动后端和启动前端；`scripts/deploy/verify_release.sh` 也已在本机通过
- 当前剩余命中基本属于明确标注的历史边界说明，例如旧 `/demo`、`sql/demo_data.sql` 和 `/app` 兼容路由历史；这部分可以保留，但不能再写成当前主链路

### 6. 下一步

- 下一步 Step ID: `Release-Maintenance`
- 下一步目标:
  - 后续仅按新增需求做增量维护
- 建议先读文件:
  - [schedule.md](/home/ljh/project/soft_course_design/docs/schedule.md)
  - [release-final-system-plan.md](/home/ljh/project/soft_course_design/docs/release-final-system-plan.md)
  - [frontend-next-schedule.md](/home/ljh/project/soft_course_design/docs/frontend-next-schedule.md)
  - [接口联调记录.md](/home/ljh/project/soft_course_design/docs/接口联调记录.md)
  - [部署与答辩说明.md](/home/ljh/project/soft_course_design/docs/部署与答辩说明.md)

## 历史阶段摘要

- `S00-S15`：课程设计后端、数据库、测试基线、部署与答辩交付均已完成
- `F16`：Next.js 前端骨架、7 个页面、设计系统、Mock 闭环已完成
- `F17`：Auth、Auction、Bid、Publish、Checkout、Admin、WebSocket 真实接入已完成
- `2026-06-03`：完成真实后端 HTTP 合同收口，补齐订单、支付、登出、404 JSON 等联调缺口
- `2026-06-07`：完成 `start.sh`、本地 MySQL fallback、`verify_release.sh`、前端测试脚本名收口等最终修复，并在本机通过最终验收

历史细节已改由：
- [release-final-system-plan.md](/home/ljh/project/soft_course_design/docs/release-final-system-plan.md)
- 各模块文档
- 具体代码与测试文件

## 后续更新规则

1. 任何改变实现状态、验证状态、handoff、下一步或前端 readiness 的任务，都必须同步更新本文件。
2. `schedule.md` 只保留恢复必需信息，不再追加超长逐日流水。
3. 若任务涉及前端/F17 收口，还必须同步：
   - [frontend-next-schedule.md](/home/ljh/project/soft_course_design/docs/frontend-next-schedule.md)
   - [frontend/API_READINESS.md](/home/ljh/project/soft_course_design/frontend/API_READINESS.md)
4. 若代码与文档不一致，先以代码现状为准，再同步修正文档。

## 新会话固定恢复文本

```text
这是 /home/ljh/project/soft_course_design 项目。

开始前先读取：
1. docs/schedule.md
2. docs/release-final-system-plan.md
3. docs/frontend-next-schedule.md
4. frontend/API_READINESS.md
5. 若任务属于后端启动/验证，再读取 start.sh、scripts/、scripts/deploy/、config/、sql/、tests/、CMakeLists.txt

当前主任务：最终上线完整系统修复
当前状态：S00-S15 已完成；F16 已完成；F17 真实 HTTP/WS 已接入完成；R2/R3/R6/R7 最终收口已完成

当前必须遵守：
- 以代码现状为准
- schedule.md 只保留短恢复入口，不再扩写成长历史流水
- 单一真实业务入口：frontend + 真实 HTTP/WS
- 支付保留，但只做本地测试支付模型
- 每次改变实现状态、验证状态、handoff、下一步，都同步更新 docs/schedule.md
- 始终用中文
```
