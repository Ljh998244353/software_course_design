# 在线拍卖平台 Agent 执行 Schedule

## 1. 文档目的

本文档用于把本项目拆解成可逐步交给 agent 执行的实际开发任务，并作为统一进度记录入口。

本次重写后的核心原则如下：

- 进度以实际代码落地为准，不再以“仅补充文档”视为步骤完成。
- 每一步只有在“代码实现 + 可构建/可运行 + 最小测试或验证 + 文档同步更新”同时满足时，才允许标记为 `已完成`。
- 仅完成设计文档、接口说明或状态机说明的步骤，统一标记为 `进行中`，不能标记为 `已完成`。
- 每一步完成后，必须记录实际代码位置和对应文档位置。
- 状态仅使用：`未开始`、`进行中`、`已完成`、`阻塞`。

## 2. 当前真实进度

截至当前仓库，实际状态如下：

- 已有需求基线文档：[需求规格说明书.md](/home/ljh/project/soft_course_design/docs/需求规格说明书.md)
- 已有架构基线文档：[系统概要设计报告.md](/home/ljh/project/soft_course_design/docs/系统概要设计报告.md)
- 已有环境、数据库、认证、物品、拍卖、竞价、订单与支付模块设计文档
- 已落地 `CMakeLists.txt`、`src/`、`tests/`、`config/`、`scripts/` 等工程骨架
- 已完成配置加载、日志初始化、统一响应模型、错误码基线和冒烟测试
- Drogon 当前已可在本机环境中被 CMake 检测到，HTTP 服务目标可启用；当前自动化验证仍以服务层和脚本闭环为主

因此，真实进度应认定为：

- `S00-S20`：已完成
- `S21-S30`：已规划，未开始，用于完整前端全量接入真实业务后端

## 3. 步骤完成判定

从本次改版起，每一步至少同时满足以下条件，才允许改为 `已完成`：

1. 已有实际代码、脚本或配置文件提交到仓库。
2. 已有明确的本地构建或运行方式，并完成最小验证。
3. 已有与本步对应的测试、冒烟验证或验证记录。
4. 已同步更新对应模块说明文档或接口说明文档。
5. `schedule.md` 已记录该步的代码路径、文档路径和进度备注。

特别说明：

- 仅新增 Markdown 文档，不算完成模块开发。
- 仅创建空目录或空文件，不算完成模块开发。
- 测试可以先小后大，但不能完全没有验证。

## 4. 当前推荐执行顺序

`S15` 已完成部署、演示与答辩交付；`S16` 已补充浏览器前端演示台和只读演示数据接口。当前工程骨架、数据库基线、认证权限模块、物品审核模块、拍卖管理模块、竞价模块、订单支付模块、评价模块、统计模块、运维异常模块、统一测试基线、高风险专项测试、最终演示脚本和前端演示入口均已具备可复用实现。

当前课程设计原计划内步骤均已完成：

1. `S00-S16`

当前完整前端扩展计划的已完成步骤如下：

1. `S17-S20`

当前完整前端扩展计划中待继续执行的步骤如下：

1. `S21-S30`

当前最优先的实际下一步是：

- `S21`：在 `S20` 已有管理员审核与管理端拍卖页面之上，接入买家拍卖大厅与详情页。

## 5. Step-by-Step Schedule

| 步骤 ID | 可分配给 agent 的实际目标 | 当前已有基础 | 需要实际完成的代码/脚本 | 完成判定 | 预计代码位置 | 完成后文档位置 | 当前状态 | 进度备注 |
|---|---|---|---|---|---|---|---|---|
| S00 | 冻结需求基线，确认角色、业务范围、性能与功能目标 | 已有需求文档 | 无新增代码要求 | 需求边界稳定，可作为后续开发输入 | 无 | [需求规格说明书.md](/home/ljh/project/soft_course_design/docs/需求规格说明书.md) | 已完成 | 这是需求基线步骤，不要求代码 |
| S01 | 冻结架构基线，确认模块边界、分层、状态机与关键链路 | 已有架构文档 | 无新增代码要求 | 架构边界稳定，可作为实现基线 | 无 | [系统概要设计报告.md](/home/ljh/project/soft_course_design/docs/系统概要设计报告.md) | 已完成 | 这是架构基线步骤，不要求代码 |
| S02 | 建立可编译、可启动的工程骨架 | 已有环境配置说明 | 创建 `CMakeLists.txt`、`src/main.cpp`、基础目录结构、配置加载、统一响应模型、错误码基线、日志初始化、最小启动入口 | 本地可完成配置和编译，服务可启动，至少完成一次工程冒烟验证 | `CMakeLists.txt`、`src/`、`config/`、`scripts/`、`tests/` | [环境配置说明.md](/home/ljh/project/soft_course_design/docs/环境配置说明.md) | 已完成 | 已落地工程骨架并通过 `cmake`、构建和 `ctest` 验证；当前本机已检测到 Drogon，可启用 HTTP 健康检查入口 |
| S03 | 落地数据库初始化脚本与持久层基础能力 | 已有数据库设计说明 | 创建建表脚本、索引脚本、种子数据脚本、数据库连接配置、Repository 基础封装、数据库连通验证 | MySQL 可初始化核心表，应用可连库，至少有数据库冒烟验证 | `sql/schema.sql`、`sql/seed.sql`、`src/repository/`、`src/common/db/` | [数据库设计说明.md](/home/ljh/project/soft_course_design/docs/数据库设计说明.md) | 已完成 | 已落地 15 张核心表、基础种子数据、MySQL C API 封装、Repository 基类、`--check-db` 和数据库冒烟测试 |
| S04 | 实现认证与权限模块代码 | 已有认证与权限模块设计文档 | 实现用户注册登录、密码哈希、Token 鉴权、中间件、RBAC、账号状态校验、认证相关测试 | 用户可注册/登录，受保护接口可鉴权，最小权限测试通过 | `src/modules/auth/`、`src/middleware/`、`tests/auth/` | [认证与权限模块说明.md](/home/ljh/project/soft_course_design/docs/认证与权限模块说明.md) | 已完成 | 已落地 MySQL 用户仓储、`SHA-512 crypt` 密码哈希、HMAC Token、进程内会话存储、鉴权中间件、管理员状态管理和认证自动化测试 |
| S05 | 实现物品发布与审核模块代码 | 已有物品与审核模块设计文档 | 实现拍品 CRUD、图片元数据管理、提交审核、审核流转、审核日志、最小模块测试 | 卖家可提交拍品，管理员可审核，状态流转和日志落库可验证 | `src/modules/item/`、`src/modules/audit/`、`tests/item/` | [物品与审核模块说明.md](/home/ljh/project/soft_course_design/docs/物品与审核模块说明.md) | 已完成 | 已落地 `ItemService`、`ItemAuditService`、`ItemRepository` 和自动化测试，完成草稿、图片元数据、提交审核、驳回/通过和审核日志闭环 |
| S06 | 实现拍卖管理模块代码 | 已有拍卖管理模块设计文档 | 实现活动创建、修改、取消、查询、开始调度、结束调度、活动状态切换、最小模块测试 | 管理员可创建活动，系统可按时间切换状态，拍卖和拍品状态协同可验证 | `src/modules/auction/`、`src/jobs/`、`tests/auction/` | [拍卖管理模块说明.md](/home/ljh/project/soft_course_design/docs/拍卖管理模块说明.md) | 已完成 | 已落地 `AuctionService`、`AuctionRepository`、`AuctionScheduler`、拍卖自动化测试和调度任务日志闭环 |
| S07 | 实现竞价与实时通知模块代码 | 已有竞价与实时通知模块设计文档 | 实现出价接口、行级锁事务、幂等键、延时保护、竞价历史、Redis 热点缓存、WebSocket 推送、最小并发测试 | 并发出价下最高价一致，延时保护生效，通知失败不回滚事务 | `src/modules/bid/`、`src/modules/notification/`、`src/ws/`、`tests/bid/` | [竞价与实时通知模块说明.md](/home/ljh/project/soft_course_design/docs/竞价与实时通知模块说明.md) | 已完成 | 已落地竞价服务、通知服务、缓存/推送抽象、仓储与自动化测试；当前未接入真实 Redis 客户端，缓存默认使用可替换的进程内实现 |
| S08 | 实现订单与支付结算模块代码 | 拍卖与竞价设计已具备输入条件 | 实现结束后生成订单、支付发起、支付回调、幂等处理、超时关闭、审计日志、最小模块测试 | 一场拍卖最多生成一笔订单，重复回调不重复记账 | `src/modules/order/`、`src/modules/payment/`、`tests/order/`、`tests/payment/` | `docs/订单与支付模块说明.md` | 已完成 | 已落地 `OrderService`、`PaymentService`、`OrderScheduler`、订单/支付仓储、mock 回调签名与自动化测试闭环 |
| S09 | 实现评价与反馈模块代码 | 已具备 `review` 表、订单终态字段和通知基础设施 | 实现互评、评价查询、评价约束、信用汇总基础能力、最小模块测试 | 买卖双方可在订单完成后互评，评价与订单正确绑定 | `src/modules/review/`、`tests/review/` | `docs/评价与反馈模块说明.md` | 已完成 | 已落地 `ReviewService`、`ReviewRepository`、自动评价方向推导、重复评价约束、信用汇总查询和自动化测试闭环 |
| S10 | 实现统计分析与报表模块代码 | 数据模型和业务链路设计已具备基础 | 实现日报表聚合、成交统计、流拍统计、出价统计、导出接口、统计任务测试 | 管理端可查看主要统计指标，统计结果可复算 | `src/modules/statistics/`、`src/jobs/`、`tests/statistics/` | `docs/统计分析与报表模块说明.md` | 已完成 | 已落地 `StatisticsService`、`StatisticsRepository`、`StatisticsScheduler`、CSV 导出与自动化测试闭环 |
| S11 | 实现系统监控与异常处理模块代码 | 已有顶层日志、任务、降级设计 | 实现操作日志、任务日志、通知失败重试、异常标记、补偿入口、降级处理、最小验证 | 关键异常可观测、可追踪、可重试 | `src/modules/ops/`、`src/repository/`、`src/modules/notification/`、`tests/ops/` | `docs/系统监控与异常处理模块说明.md` | 已完成 | 已落地统一运维服务、异常看板、手工异常标记、失败通知重试和补偿入口，并通过自动化验证 |
| S12 | 完成接口联调与主流程闭环 | 核心模块代码已具备后才能开始 | 联调认证、拍品、审核、活动、竞价、订单、支付、评价全链路，修正接口和状态切换问题 | 主流程可完整走通并可演示 | `src/`、`tests/integration/` | `docs/接口联调记录.md` | 已完成 | 已补齐支付后履约状态推进，新增 `tests/integration/full_flow_tests.cpp` 与 `scripts/test_integration.sh`，完成主流程联调与自动化验证 |
| S13 | 落地自动化测试基线 | 需要已有可运行工程 | 统一 CTest、断言式测试二进制、单元测试、模块测试、集成/契约测试入口、基础测试脚本和执行方式 | 测试框架可运行，核心模块都有最小自动化覆盖，且可按分层/模块统一执行 | `tests/`、`CMakeLists.txt`、`scripts/` | `docs/测试计划与用例说明.md` | 已完成 | 已统一 `CTest + test_*.sh + 断言式测试二进制` 基线，新增 suite/module 标签、MySQL 资源锁和 `scripts/test.sh` 分组入口；当前接口契约测试继续复用 `tests/integration/`，不额外创建空骨架 |
| S14 | 完成高风险专项测试 | 需要主链路代码和测试基线 | 执行并发出价、拍卖结束竞争、支付回调幂等、Redis 降级、通知失败、安全负向测试 | 高风险链路均有验证结果和问题闭环 | `tests/stress/`、`tests/integration/`、`tests/security/` | `docs/测试报告.md` | 已完成 | 已落地 `auction_high_risk_flow` 与 `auction_security_negative`，覆盖并发、结束竞争、支付幂等、缓存降级、通知失败重试和安全负向；同时修复出价金额三位小数校验缺陷 |
| S15 | 完成部署、演示与答辩交付 | 需要联调与专项测试完成 | 补齐部署脚本、初始化流程、演示数据、演示账号、答辩脚本与最终交付说明 | 系统可在目标环境复现并完成完整演示 | `scripts/deploy/`、`config/`、`sql/demo_data.sql` | `docs/部署与答辩说明.md` | 已完成 | 已落地演示初始化、服务启动、答辩提纲和最终发布验证脚本；最终回归 15/15 通过 |
| S16 | 补充浏览器前端演示台 | S15 演示数据、Drogon 健康检查入口和 MySQL 演示库已具备 | 新增静态前端、只读演示数据接口、前端数据验证测试、答辩操作文档 | 浏览器可打开 `/demo`，页面数据来自 `auction_demo`，前端专项测试通过，并同步文档 | `assets/demo/`、`src/access/http/`、`src/application/`、`tests/demo/`、`scripts/` | [前端演示模块说明.md](/home/ljh/project/soft_course_design/docs/前端演示模块说明.md)、[答辩演示操作手册.md](/home/ljh/project/soft_course_design/docs/答辩演示操作手册.md) | 已完成 | 已落地只读演示台、`/api/demo/dashboard`、`scripts/test.sh frontend` 和答辩操作手册；完整业务 HTTP 控制器批量接入仍作为后续扩展 |

## 6. 当前分配原则

为了方便直接分配给 agent 执行，后续每一步建议拆成“一个模块、一个明确目标、一个明确代码目录”的粒度。

按当前已确认进度，`S00-S20` 均已完成，`S21-S30` 为完整前端后续扩展计划且均未开始。完整前端接入步骤见本文第 13 节。

例如：

- 若后续新增步骤，仍应优先完成代码、脚本、验证记录和文档同步，再更新模块文档
- 若某一步尚未形成最小可验证闭环，则继续保持 `进行中`，不要提前改成 `已完成`
- 已完成步骤也要继续记录验证命令和 handoff，避免上下文压缩后丢失真实进度

## 7. 进度记录

| 日期 | 步骤 ID | 状态 | 更新内容 | 代码位置 | 文档位置 |
|---|---|---|---|---|---|
| 2026-04-12 | S00 | 已完成 | 需求基线已存在，可直接作为开发输入 | 无 | [需求规格说明书.md](/home/ljh/project/soft_course_design/docs/需求规格说明书.md) |
| 2026-04-12 | S01 | 已完成 | 架构基线已存在，可直接作为实现约束 | 无 | [系统概要设计报告.md](/home/ljh/project/soft_course_design/docs/系统概要设计报告.md) |
| 2026-04-12 | S02 | 已完成 | 已落地 `CMakeLists.txt`、`src/`、`tests/`、配置模板、日志与错误码基础代码、启动脚本和冒烟测试；已完成 `cmake`、构建与 `ctest` 验证 | [CMakeLists.txt](/home/ljh/project/soft_course_design/CMakeLists.txt) | [环境配置说明.md](/home/ljh/project/soft_course_design/docs/环境配置说明.md) |
| 2026-04-12 | S03 | 已完成 | 已落地 `sql/schema.sql`、`sql/seed.sql`、`src/common/db/`、`src/repository/`、`tests/integration/database_smoke_tests.cpp` 和 `scripts/test_db.sh`；已完成本地 MySQL schema/seed/连库验证 | [sql](/home/ljh/project/soft_course_design/sql) | [数据库设计说明.md](/home/ljh/project/soft_course_design/docs/数据库设计说明.md) |
| 2026-04-12 | S04 | 已完成 | 已落地 `src/modules/auth/`、`src/middleware/`、`tests/auth/auth_flow_tests.cpp` 和 `scripts/test_auth.sh`；已完成注册、登录、登出、Token 鉴权、RBAC 和管理员冻结/禁用闭环验证 | [auth_service.cpp](/home/ljh/project/soft_course_design/src/modules/auth/auth_service.cpp) | [认证与权限模块说明.md](/home/ljh/project/soft_course_design/docs/认证与权限模块说明.md) |
| 2026-04-12 | S05 | 已完成 | 已落地 `src/modules/item/`、`src/modules/audit/`、`src/repository/item_repository.*`、`tests/item/item_flow_tests.cpp` 和 `scripts/test_item.sh`；已完成草稿创建、图片元数据管理、提交审核、管理员驳回/通过、审核日志和驳回后重提闭环验证 | [item_service.cpp](/home/ljh/project/soft_course_design/src/modules/item/item_service.cpp) | [物品与审核模块说明.md](/home/ljh/project/soft_course_design/docs/物品与审核模块说明.md) |
| 2026-04-12 | S06 | 进行中 | 已有拍卖管理模块说明，但仓库中尚无活动与调度实现代码 | 待创建 `src/modules/auction/`、`src/jobs/` | [拍卖管理模块说明.md](/home/ljh/project/soft_course_design/docs/拍卖管理模块说明.md) |
| 2026-04-12 | S07 | 进行中 | 已有竞价与实时通知模块说明，但仓库中尚无竞价、缓存和 WebSocket 实现代码 | 待创建 `src/modules/bid/`、`src/modules/notification/`、`src/ws/` | [竞价与实时通知模块说明.md](/home/ljh/project/soft_course_design/docs/竞价与实时通知模块说明.md) |
| 2026-04-12 | S02-VERIFY | 已完成 | 顺序执行 `cmake -S . -B build`、`cmake --build build`、`ctest --test-dir build --output-on-failure`，2 个测试全部通过 | [build](/home/ljh/project/soft_course_design/build) | [schedule.md](/home/ljh/project/soft_course_design/docs/schedule.md) |
| 2026-04-12 | S03-VERIFY | 已完成 | 顺序执行 `ctest --test-dir build --output-on-failure -E auction_database_smoke`、`ctest --test-dir build --output-on-failure -R auction_database_smoke`、`AUCTION_APP_CONFIG=build/test_config/app.mysql.test.json ./build/bin/auction_app --check-db`；数据库冒烟测试通过，`--check-db` 返回 15 张表、1 个管理员和 4 个基础分类 | [test_db.sh](/home/ljh/project/soft_course_design/scripts/test_db.sh) | [数据库设计说明.md](/home/ljh/project/soft_course_design/docs/数据库设计说明.md) |
| 2026-04-12 | S04-VERIFY | 已完成 | 顺序执行 `ctest --test-dir build --output-on-failure`，4 个测试全部通过；其中 `auction_auth_flow` 已覆盖注册成功、重复账号、密码错误、Token 缺失、Token 过期、RBAC、冻结/禁用和登出失效 | [auth_flow_tests.cpp](/home/ljh/project/soft_course_design/tests/auth/auth_flow_tests.cpp) | [认证与权限模块说明.md](/home/ljh/project/soft_course_design/docs/认证与权限模块说明.md) |
| 2026-04-12 | S05-VERIFY | 已完成 | 顺序执行 `cmake -S . -B build`、`cmake --build build`、`scripts/test_item.sh`、`ctest --test-dir build --output-on-failure`；`auction_item_flow` 已覆盖草稿创建、图片元数据管理、提交审核、驳回原因校验、驳回后回退 `DRAFT`、再次提交和审核通过，当前 `ctest` 5/5 全部通过 | [item_flow_tests.cpp](/home/ljh/project/soft_course_design/tests/item/item_flow_tests.cpp) | [物品与审核模块说明.md](/home/ljh/project/soft_course_design/docs/物品与审核模块说明.md) |
| 2026-04-13 | S06 | 已完成 | 已落地 `src/modules/auction/`、`src/repository/auction_repository.*`、`src/jobs/auction_scheduler.*`、`tests/auction/auction_flow_tests.cpp` 和 `scripts/test_auction.sh`；已完成活动创建、修改、取消、管理端/公开端查询、开始调度、结束调度、`task_log` 记录和拍品状态协同闭环 | [auction_service.cpp](/home/ljh/project/soft_course_design/src/modules/auction/auction_service.cpp) | [拍卖管理模块说明.md](/home/ljh/project/soft_course_design/docs/拍卖管理模块说明.md) |
| 2026-04-13 | S06-VERIFY | 已完成 | 顺序执行 `cmake -S . -B build`、`cmake --build build`、`scripts/test_auction.sh`、`ctest --test-dir build --output-on-failure`；`auction_auction_flow` 已覆盖待开始活动创建/修改/取消、重复建拍限制、开始调度、无人出价流拍、有最高出价进入 `SETTLING`、延时保护顺延跳过和 `task_log` 校验，当前 `ctest` 6/6 全部通过 | [auction_flow_tests.cpp](/home/ljh/project/soft_course_design/tests/auction/auction_flow_tests.cpp) | [拍卖管理模块说明.md](/home/ljh/project/soft_course_design/docs/拍卖管理模块说明.md) |
| 2026-04-15 | S07 | 已完成 | 已落地 `src/modules/bid/`、`src/modules/notification/`、`src/ws/`、`src/repository/bid_repository.*`、`src/repository/notification_repository.*`、`tests/bid/bid_flow_tests.cpp` 和 `scripts/test_bid.sh`；已完成出价事务、幂等重试、延时保护、竞价历史、个人竞价状态、热点缓存刷新、被超越提醒、广播推送和通知失败补偿日志闭环 | [bid_service.cpp](/home/ljh/project/soft_course_design/src/modules/bid/bid_service.cpp) | [竞价与实时通知模块说明.md](/home/ljh/project/soft_course_design/docs/竞价与实时通知模块说明.md) |
| 2026-04-15 | S07-VERIFY | 已完成 | 顺序执行 `cmake -S . -B build`、`cmake --build build`、`scripts/test_bid.sh`、`ctest --test-dir build --output-on-failure`；`auction_bid_flow` 已覆盖首次出价、幂等重试、最低加价校验、延时保护、通知失败不回滚和最小并发竞价，当前 `ctest` 7/7 全部通过 | [bid_flow_tests.cpp](/home/ljh/project/soft_course_design/tests/bid/bid_flow_tests.cpp) | [竞价与实时通知模块说明.md](/home/ljh/project/soft_course_design/docs/竞价与实时通知模块说明.md) |
| 2026-04-15 | S08 | 已完成 | 已落地 `src/modules/order/`、`src/modules/payment/`、`src/repository/order_repository.*`、`src/repository/payment_repository.*`、`src/jobs/order_scheduler.*`、`tests/order/order_flow_tests.cpp`、`tests/payment/payment_flow_tests.cpp`、`scripts/test_order.sh` 和 `scripts/test_payment.sh`；已完成 `SETTLING` 建单、支付发起、回调验签与金额校验、重复成功回调幂等、超时关闭和站内通知/审计日志闭环 | [order_service.cpp](/home/ljh/project/soft_course_design/src/modules/order/order_service.cpp) | [订单与支付模块说明.md](/home/ljh/project/soft_course_design/docs/订单与支付模块说明.md) |
| 2026-04-15 | S08-VERIFY | 已完成 | 顺序执行 `cmake -S . -B build`、`cmake --build build`、`scripts/test_order.sh`、`scripts/test_payment.sh`、`ctest --test-dir build --output-on-failure`；`auction_order_flow` 已覆盖唯一建单与超时关闭，`auction_payment_flow` 已覆盖支付发起复用、成功回调、重复成功回调、金额不一致和验签失败，当前 `ctest` 9/9 全部通过 | [payment_flow_tests.cpp](/home/ljh/project/soft_course_design/tests/payment/payment_flow_tests.cpp) | [订单与支付模块说明.md](/home/ljh/project/soft_course_design/docs/订单与支付模块说明.md) |
| 2026-04-22 | S09 | 已完成 | 已落地 `src/modules/review/`、`src/repository/review_repository.*`、`tests/review/review_flow_tests.cpp` 和 `scripts/test_review.sh`；已完成互评提交、自动评价方向推导、重复评价约束、按订单查询互评记录、按用户汇总信用评分和第二条评价后订单推进到 `REVIEWED` 的闭环 | [review_service.cpp](/home/ljh/project/soft_course_design/src/modules/review/review_service.cpp) | [评价与反馈模块说明.md](/home/ljh/project/soft_course_design/docs/评价与反馈模块说明.md) |
| 2026-04-22 | S09-VERIFY | 已完成 | 顺序执行 `cmake -S . -B build`、`cmake --build build`、`scripts/test_review.sh`、`ctest --test-dir build --output-on-failure`；`auction_review_flow` 已覆盖买卖双方互评、重复评价、非订单参与方拒绝、订单状态约束和信用汇总；后续已修正 `auction_smoke_tests` 配置断言，`S09` 收尾时全量 `ctest` 为 10/10 通过 | [review_flow_tests.cpp](/home/ljh/project/soft_course_design/tests/review/review_flow_tests.cpp) | [评价与反馈模块说明.md](/home/ljh/project/soft_course_design/docs/评价与反馈模块说明.md) |
| 2026-04-22 | S10 | 已完成 | 已落地 `src/modules/statistics/`、`src/repository/statistics_repository.*`、`src/jobs/statistics_scheduler.*`、`tests/statistics/statistics_flow_tests.cpp` 和 `scripts/test_statistics.sh`；已完成日报聚合、成交/流拍/出价统计、管理员查询、CSV 导出、按天重跑覆盖和任务日志闭环 | [statistics_service.cpp](/home/ljh/project/soft_course_design/src/modules/statistics/statistics_service.cpp) | [统计分析与报表模块说明.md](/home/ljh/project/soft_course_design/docs/统计分析与报表模块说明.md) |
| 2026-04-22 | S10-VERIFY | 已完成 | 顺序执行 `cmake -S . -B build`、`cmake --build build`、`scripts/test_statistics.sh`、`ctest --test-dir build --output-on-failure`；`auction_statistics_flow` 已覆盖日报聚合、同日重跑覆盖、管理员查询、CSV 导出、日期范围校验和非管理员拒绝，当前全量 `ctest` 为 11/11 全部通过 | [statistics_flow_tests.cpp](/home/ljh/project/soft_course_design/tests/statistics/statistics_flow_tests.cpp) | [统计分析与报表模块说明.md](/home/ljh/project/soft_course_design/docs/统计分析与报表模块说明.md) |
| 2026-04-22 | S11 | 已完成 | 已落地 `src/modules/ops/`、`src/repository/ops_repository.*`、`tests/ops/ops_flow_tests.cpp` 和 `scripts/test_ops.sh`，并扩展 `src/modules/notification/` 与 `src/repository/notification_repository.*`；已完成操作日志查询、任务日志查询、异常看板、失败通知重试、手工异常标记和统一补偿入口闭环 | [ops_service.cpp](/home/ljh/project/soft_course_design/src/modules/ops/ops_service.cpp) | [系统监控与异常处理模块说明.md](/home/ljh/project/soft_course_design/docs/系统监控与异常处理模块说明.md) |
| 2026-04-22 | S11-VERIFY | 已完成 | 顺序执行 `cmake -S . -B build`、`cmake --build build`、`scripts/test_ops.sh`、`ctest --test-dir build --output-on-failure`；`auction_ops_flow` 已覆盖失败通知重试、Redis 降级手工标记、订单结算补偿、支付回调拒绝异常聚合、运维日志查询和权限校验，当前全量 `ctest` 为 12/12 全部通过 | [ops_flow_tests.cpp](/home/ljh/project/soft_course_design/tests/ops/ops_flow_tests.cpp) | [系统监控与异常处理模块说明.md](/home/ljh/project/soft_course_design/docs/系统监控与异常处理模块说明.md) |
| 2026-04-22 | S12 | 已完成 | 已扩展 `src/modules/order/`、`src/repository/order_repository.*`，补齐卖家发货与买家确认收货状态推进；已新增 `tests/integration/full_flow_tests.cpp` 和 `scripts/test_integration.sh`，完成认证、拍品、审核、活动、竞价、订单、支付、评价、统计、运维的主流程联调闭环 | [full_flow_tests.cpp](/home/ljh/project/soft_course_design/tests/integration/full_flow_tests.cpp) | [接口联调记录.md](/home/ljh/project/soft_course_design/docs/接口联调记录.md) |
| 2026-04-22 | S12-VERIFY | 已完成 | 顺序执行 `cmake -S . -B build`、`cmake --build build`、`scripts/test_integration.sh`、`ctest --test-dir build --output-on-failure`；`auction_integration_flow` 已覆盖支付后履约、互评、统计和运维查询联调，当前全量 `ctest` 为 13/13 全部通过 | [test_integration.sh](/home/ljh/project/soft_course_design/scripts/test_integration.sh) | [接口联调记录.md](/home/ljh/project/soft_course_design/docs/接口联调记录.md) |
| 2026-04-22 | S13 | 已完成 | 已扩展 `CMakeLists.txt` 和 `scripts/test.sh`，统一 `CTest` 标签、模块分组入口与 MySQL 资源锁；已新增 `docs/测试计划与用例说明.md`，明确当前测试分层、执行方式和覆盖边界 | [scripts/test.sh](/home/ljh/project/soft_course_design/scripts/test.sh) | [测试计划与用例说明.md](/home/ljh/project/soft_course_design/docs/测试计划与用例说明.md) |
| 2026-04-22 | S13-VERIFY | 已完成 | 顺序执行 `scripts/test.sh smoke`、`scripts/test.sh db`、`scripts/test.sh module`、`scripts/test.sh contract`、`ctest --test-dir build --output-on-failure`；已验证分层入口、`CTest` 标签筛选和 MySQL 共享资源锁，当前全量 `ctest` 为 13/13 全部通过 | [CMakeLists.txt](/home/ljh/project/soft_course_design/CMakeLists.txt) | [schedule.md](/home/ljh/project/soft_course_design/docs/schedule.md) |
| 2026-04-29 | S14 | 已完成 | 已新增高风险专项和安全负向测试，接入 `suite_risk`、`suite_stress`、`suite_security` 与 `scripts/test.sh risk/stress/security`；专项测试发现并修复出价金额三位小数校验缺陷 | [high_risk_flow_tests.cpp](/home/ljh/project/soft_course_design/tests/stress/high_risk_flow_tests.cpp) | [测试报告.md](/home/ljh/project/soft_course_design/docs/测试报告.md) |
| 2026-04-29 | S14-VERIFY | 已完成 | 顺序执行 `cmake -S . -B build`、`cmake --build build`、`scripts/test.sh risk`、`ctest --test-dir build --output-on-failure`；`auction_high_risk_flow` 和 `auction_security_negative` 均通过，专项回归为 2/2 通过，全量 `ctest` 为 15/15 通过 | [test_high_risk.sh](/home/ljh/project/soft_course_design/scripts/test_high_risk.sh) | [测试计划与用例说明.md](/home/ljh/project/soft_course_design/docs/测试计划与用例说明.md) |
| 2026-04-29 | S15 | 已完成 | 已新增 `scripts/deploy/` 部署演示脚本、`sql/demo_data.sql` 演示数据和 `docs/部署与答辩说明.md`；已调整 `config/nginx.conf` 为非特权演示端口 `18081` 并代理应用端口 `18080`；已同步环境配置说明和最终交付口径 | [init_demo_env.sh](/home/ljh/project/soft_course_design/scripts/deploy/init_demo_env.sh) | [部署与答辩说明.md](/home/ljh/project/soft_course_design/docs/部署与答辩说明.md) |
| 2026-04-29 | S15-VERIFY | 已完成 | 顺序执行 `cmake -S . -B build`、`cmake --build build`、`scripts/deploy/init_demo_env.sh`、`scripts/deploy/verify_release.sh`，并启动 `scripts/deploy/run_demo_server.sh` 后访问 `curl http://127.0.0.1:18080/healthz`；演示库 `auction_demo` schema/seed 检查通过，高风险专项 2/2 通过，全量 `ctest` 15/15 通过，健康检查返回 `status=ok` | [verify_release.sh](/home/ljh/project/soft_course_design/scripts/deploy/verify_release.sh) | [schedule.md](/home/ljh/project/soft_course_design/docs/schedule.md) |
| 2026-04-29 | S16 | 已完成 | 已新增 `assets/demo/` 浏览器前端演示台、`/api/demo/dashboard` 只读演示数据接口、`scripts/test.sh frontend` 前端专项验证和答辩操作手册；页面从 `auction_demo` 聚合展示拍品、拍卖、出价、订单、支付、评价、通知、任务日志、操作日志和统计日报 | [demo_http.cpp](/home/ljh/project/soft_course_design/src/access/http/demo_http.cpp) | [前端演示模块说明.md](/home/ljh/project/soft_course_design/docs/前端演示模块说明.md) |
| 2026-04-29 | S16-VERIFY | 已完成 | 顺序执行 `cmake -S . -B build`、`cmake --build build`、`scripts/test.sh frontend`、`ctest --test-dir build --output-on-failure` 和 `scripts/deploy/verify_release.sh`；启动演示服务后验证 `/healthz`、`/demo`、`/assets/demo/app.css`、`/api/demo/dashboard` 均返回 200；前端演示数据测试通过，全量 `ctest` 更新为 16/16 通过 | [test_demo_frontend.sh](/home/ljh/project/soft_course_design/scripts/test_demo_frontend.sh) | [答辩演示操作手册.md](/home/ljh/project/soft_course_design/docs/答辩演示操作手册.md) |
| 2026-04-29 | S17-S30-PLAN | 已完成 | 已追加完整前端全量接入真实业务后端的开发 schedule，明确先接业务 HTTP 控制器，再分角色落地前台、卖家、管理端、竞价、订单支付、评价、统计运维和最终验收 | [assets/app](/home/ljh/project/soft_course_design/assets/app) | [schedule.md](/home/ljh/project/soft_course_design/docs/schedule.md) |
| 2026-04-29 | S17 | 已完成 | 已新增真实业务 HTTP 接入基线：`/app`、`/assets/app/*`、`POST /api/auth/login`、`GET /api/system/context`、统一 JSON `404`；已新增 `HttpServiceContext`、请求解析/错误映射公共层和 `scripts/test_http.sh` | [business_http.cpp](/home/ljh/project/soft_course_design/src/access/http/business_http.cpp) | [前端演示模块说明.md](/home/ljh/project/soft_course_design/docs/前端演示模块说明.md) |
| 2026-04-29 | S17-VERIFY | 已完成 | 顺序执行 `cmake -S . -B build`、`cmake --build build`、`scripts/test_http.sh`、`scripts/test.sh http`、`ctest --test-dir build --output-on-failure`；`auction_http_baseline` 已覆盖 `/app`、`/app/`、`/assets/app/app.css`、`/assets/app/app.js`、登录、受保护上下文和统一 JSON `404`；全量 `ctest` 为 `17/17` 通过。本轮复核中，Codex 沙箱内本地 MySQL 绑定 Unix socket 受限，已在沙箱外完成验证。 | [test_http.sh](/home/ljh/project/soft_course_design/scripts/test_http.sh) | [测试计划与用例说明.md](/home/ljh/project/soft_course_design/docs/测试计划与用例说明.md) |
| 2026-04-29 | S18 | 已完成 | 已扩展真实认证 HTTP 接口：`POST /api/auth/register`、`POST /api/auth/logout`、`GET /api/auth/me`、`PATCH /api/admin/users/{id}/status`；已将 `/app` 升级为认证工作台，支持注册、登录、会话恢复、角色导航和路由守卫 | [auth_http.cpp](/home/ljh/project/soft_course_design/src/access/http/auth_http.cpp) | [认证与权限模块说明.md](/home/ljh/project/soft_course_design/docs/认证与权限模块说明.md) |
| 2026-04-29 | S18-VERIFY | 已完成 | 顺序执行 `cmake -S . -B build`、`cmake --build build`、`scripts/test_http.sh`、`scripts/test.sh http`、`ctest --test-dir build --output-on-failure`；`auction_http_baseline` 已覆盖注册、登录、当前用户、登出、管理员改状态、`SUPPORT` 越权拒绝、冻结后登录拒绝、受保护上下文和统一 JSON `404`；全量 `ctest` 为 `17/17` 通过。本轮验证中先清理了本项目 `build/test_mysql` 残留 `mysqld`，并在沙箱外完成依赖本地 MySQL/HTTP 的验证。 | [test_http.sh](/home/ljh/project/soft_course_design/scripts/test_http.sh) | [测试计划与用例说明.md](/home/ljh/project/soft_course_design/docs/测试计划与用例说明.md) |
| 2026-04-29 | S19 | 已完成 | 已新增卖家拍品真实 HTTP 控制器：创建、编辑、图片元数据、删除图片、我的拍品、详情和提交审核；已将 `/app` 卖家视图升级为拍品管理页面，支持状态筛选、表单编辑、图片元数据和驳回原因展示 | [item_http.cpp](/home/ljh/project/soft_course_design/src/access/http/item_http.cpp) | [物品与审核模块说明.md](/home/ljh/project/soft_course_design/docs/物品与审核模块说明.md) |
| 2026-04-29 | S19-VERIFY | 已完成 | 顺序执行 `cmake -S . -B build`、`cmake --build build`、`scripts/test_http.sh`、`ctest --test-dir build --output-on-failure`；`auction_http_baseline` 已覆盖拍品创建、非法字段、图片元数据、删除图片、编辑、列表、详情、提交审核和提审后禁止编辑；全量 `ctest` 为 `17/17` 通过。Codex 沙箱内首次启动本地 `mysqld` 失败，已按权限流程在沙箱外完成 HTTP 与全量验证。 | [test_http.sh](/home/ljh/project/soft_course_design/scripts/test_http.sh) | [测试计划与用例说明.md](/home/ljh/project/soft_course_design/docs/测试计划与用例说明.md) |
| 2026-04-29 | S20 | 已完成 | 已新增管理员审核和管理端拍卖真实 HTTP 控制器：待审列表、审核通过/驳回、审核日志、创建拍卖、管理端拍卖列表/详情、修改和取消；已将 `/app` 管理员视图升级为后台工作台，支持用户状态、待审拍品、审核日志、拍卖规则表单和拍卖列表详情 | [audit_http.cpp](/home/ljh/project/soft_course_design/src/access/http/audit_http.cpp)、[auction_admin_http.cpp](/home/ljh/project/soft_course_design/src/access/http/auction_admin_http.cpp) | [拍卖管理模块说明.md](/home/ljh/project/soft_course_design/docs/拍卖管理模块说明.md) |
| 2026-04-29 | S20-VERIFY | 已完成 | 顺序执行 `cmake -S . -B build`、`cmake --build build`、`bash -n scripts/test_http.sh`、`node --check assets/app/app.js`、`scripts/test_http.sh`、`ctest --test-dir build --output-on-failure`；`auction_http_baseline` 已覆盖管理员待审列表、审核通过、卖家查看通过结果、创建拍卖、管理端列表、详情、修改、取消、审核驳回、卖家查看驳回原因和审核日志；全量 `ctest` 为 `17/17` 通过。Codex 沙箱内本地 `mysqld` 启动失败，已按权限流程在沙箱外完成 HTTP 与全量验证，并清理本轮残留测试 `mysqld`。 | [test_http.sh](/home/ljh/project/soft_course_design/scripts/test_http.sh) | [测试计划与用例说明.md](/home/ljh/project/soft_course_design/docs/测试计划与用例说明.md) |
| 2026-04-12 | SCHEDULE | 已完成 | 按真实仓库状态重写 schedule，改为代码优先，并将仅文档完成的步骤重置为 `进行中` | 无 | [schedule.md](/home/ljh/project/soft_course_design/docs/schedule.md) |

## 8. 后续更新规则

后续每完成一步，都按以下规则更新本文档：

1. 先核对该步是否已有实际代码进入仓库。
2. 再核对该步是否已有最小可运行验证或测试结果。
3. 满足后再将状态从 `进行中/未开始` 改为 `已完成`。
4. 在“进度记录”中追加一条记录，写明实际代码路径和文档路径。
5. 如果只新增或修改了文档，但代码未落地，则只能更新备注，不能将步骤改为 `已完成`。

## 9. 当前 Handoff 记录

以下 handoff 记录基于当前已确认的真实进度填写，可直接作为下一轮 agent 接手时的上下文入口。

## 模块 Handoff

### 1. 基本信息
- Step ID: S15
- 模块名称: 部署、演示与答辩交付
- 当前状态: 已完成
- 对应文档: `docs/部署与答辩说明.md`
- 对应代码目录: `scripts/deploy/` `config/` `sql/`
- 已有可复用基础目录: `scripts/` `config/` `sql/` `tests/` `build/`

### 2. 本次实际完成
- 已完成功能:
  - 已新增 `scripts/deploy/init_demo_env.sh`，统一执行构建、用户态 MySQL 初始化、演示配置生成、演示图片占位、演示数据导入、`--check-config` 和 `--check-db`
  - 已新增 `scripts/deploy/run_demo_server.sh`，使用 `build/demo_config/app.demo.json` 启动演示服务
  - 已新增 `scripts/deploy/show_demo_walkthrough.sh`，输出答辩展示顺序、演示账号和高风险说明
  - 已新增 `scripts/deploy/verify_release.sh`，串联演示初始化、高风险专项和全量 CTest
  - 已新增 `sql/demo_data.sql`，提供可重复导入的演示账号、拍品、拍卖、出价、订单、支付、评价、通知、任务日志、操作日志和统计日报
  - 已调整 `config/nginx.conf`，本地演示入口改为非特权端口 `18081`，代理应用端口 `18080`，并补齐 WebSocket 代理头
  - 已新增 `docs/部署与答辩说明.md`，并同步更新 `docs/环境配置说明.md` 和本文档
- 实际修改文件:
  - [init_demo_env.sh](/home/ljh/project/soft_course_design/scripts/deploy/init_demo_env.sh)
  - [run_demo_server.sh](/home/ljh/project/soft_course_design/scripts/deploy/run_demo_server.sh)
  - [show_demo_walkthrough.sh](/home/ljh/project/soft_course_design/scripts/deploy/show_demo_walkthrough.sh)
  - [verify_release.sh](/home/ljh/project/soft_course_design/scripts/deploy/verify_release.sh)
  - [demo_data.sql](/home/ljh/project/soft_course_design/sql/demo_data.sql)
  - [nginx.conf](/home/ljh/project/soft_course_design/config/nginx.conf)
  - [部署与答辩说明.md](/home/ljh/project/soft_course_design/docs/部署与答辩说明.md)
  - [环境配置说明.md](/home/ljh/project/soft_course_design/docs/环境配置说明.md)
  - [schedule.md](/home/ljh/project/soft_course_design/docs/schedule.md)
- 未完成功能:
  - 无计划内未完成功能
- 明确不在本步处理的内容:
  - Drogon 控制器批量接入
  - 真实前端页面开发
  - 真实 200 并发在线性能压测

### 3. 关键设计决定
- 决定 1: S15 继续复用 `scripts/bootstrap.sh`、`scripts/db/setup_local_mysql.sh`、`sql/schema.sql`、`sql/seed.sql` 和 `scripts/test.sh`
- 原因: 现有构建、数据库和测试基线已经稳定，最终交付应降低额外环境差异
- 影响范围:
  - 演示环境和测试环境共用同一套 schema、seed 和用户态 MySQL 启动方式
  - 最终回归继续使用 CTest 标签体系，不引入新测试框架

- 决定 2: 演示数据库默认使用独立库名 `auction_demo`
- 原因: 避免演示数据污染自动化测试默认库 `auction_system`
- 影响范围:
  - `init_demo_env.sh` 通过 `AUCTION_DEMO_DB_NAME` 可切换演示库
  - `verify_release.sh` 可先初始化演示库，再运行测试库回归

- 决定 3: Nginx 模板使用 `18081` 作为本地演示入口
- 原因: 避免监听 80 端口触发 sudo 需求，符合本仓库非 sudo 操作约束
- 影响范围:
  - 应用仍监听 `127.0.0.1:18080`
  - Nginx 只作为可选反向代理模板，不影响应用直接启动和健康检查

### 4. 验证结果
- 执行命令:
  - `cmake -S . -B build`
  - `cmake --build build`
  - `scripts/deploy/init_demo_env.sh`
  - `scripts/deploy/verify_release.sh`
  - `scripts/deploy/run_demo_server.sh`
  - `curl http://127.0.0.1:18080/healthz`
  - `ctest --test-dir build --output-on-failure`
- 结果:
  - CMake 配置成功，Drogon 已被检测到，HTTP 服务目标启用
  - 演示库 `auction_demo` 初始化成功，schema 表数 15/15，seed 检查通过
  - `scripts/test.sh risk` 为 2/2 通过
  - 全量 `ctest` 为 15/15 通过
  - `/healthz` 返回 `code=0`、`status=ok`、`drogonEnabled=true`
- 未执行的测试:
  - 200 并发在线性能测试
- 原因:
  - 真实 200 并发在线压测超出课程设计当前自动化回归范围

### 5. 当前风险/阻塞
- 风险 1: 真实 HTTP 控制器仍未批量接入，当前契约测试仍是服务层映射验证
- 风险 2: 当前 Redis 仍为可替换旁路能力，高风险验证使用进程内缓存失败注入
- 风险 3: 若用户态 MySQL 残留进程占用 `build/test_mysql/data/ibdata1`，初始化脚本会因 InnoDB 锁失败而中断
- 阻塞项:
  - 无
- 需要注意的坑:
  - 若测试脚本卡住，优先检查 `build/test_mysql/data/ibdata1` 是否被残留 `mysqld` 占用
  - 在 Codex 沙箱中运行依赖本地 MySQL 的入口时，可能需要沙箱外权限
  - `scripts/deploy/run_demo_server.sh` 会长驻占用 `127.0.0.1:18080`，验证后需停止进程

### 6. 下一步
- 下一步 Step ID: S17
- 下一步目标: S16 已完成浏览器只读演示台；若继续开发完整前端，从真实业务 HTTP 接入基线开始
- 建议先读文件:
  - [schedule.md](/home/ljh/project/soft_course_design/docs/schedule.md)
  - [部署与答辩说明.md](/home/ljh/project/soft_course_design/docs/部署与答辩说明.md)
  - [测试报告.md](/home/ljh/project/soft_course_design/docs/测试报告.md)
  - [测试计划与用例说明.md](/home/ljh/project/soft_course_design/docs/测试计划与用例说明.md)
  - [接口联调记录.md](/home/ljh/project/soft_course_design/docs/接口联调记录.md)
  - [环境配置说明.md](/home/ljh/project/soft_course_design/docs/环境配置说明.md)
  - [scripts/deploy](/home/ljh/project/soft_course_design/scripts/deploy)
  - [demo_data.sql](/home/ljh/project/soft_course_design/sql/demo_data.sql)
  - [CMakeLists.txt](/home/ljh/project/soft_course_design/CMakeLists.txt)

## 模块 Handoff

### 1. 基本信息
- Step ID: S16
- 模块名称: 前端演示台与答辩操作手册
- 当前状态: 已完成
- 对应文档: `docs/前端演示模块说明.md`、`docs/答辩演示操作手册.md`
- 对应代码目录: `assets/demo/`、`src/access/http/`、`src/application/`、`tests/demo/`

### 2. 本次实际完成
- 已完成功能:
  - 新增浏览器入口 `/demo` 和首页 `/`
  - 新增静态资源 `/assets/demo/app.css`、`/assets/demo/app.js`
  - 新增只读演示数据接口 `/api/demo/dashboard`
  - 新增前端演示数据聚合服务 `BuildDemoDashboardResponse`
  - 新增 `scripts/test.sh frontend` 和 `auction_demo_dashboard` 测试
  - 更新部署演示脚本、最终验证脚本、测试文档、环境文档和 schedule
- 实际修改文件:
  - [index.html](/home/ljh/project/soft_course_design/assets/demo/index.html)
  - [app.css](/home/ljh/project/soft_course_design/assets/demo/app.css)
  - [app.js](/home/ljh/project/soft_course_design/assets/demo/app.js)
  - [demo_http.cpp](/home/ljh/project/soft_course_design/src/access/http/demo_http.cpp)
  - [demo_dashboard_service.cpp](/home/ljh/project/soft_course_design/src/application/demo_dashboard_service.cpp)
  - [demo_dashboard_tests.cpp](/home/ljh/project/soft_course_design/tests/demo/demo_dashboard_tests.cpp)
  - [test_demo_frontend.sh](/home/ljh/project/soft_course_design/scripts/test_demo_frontend.sh)
  - [前端演示模块说明.md](/home/ljh/project/soft_course_design/docs/前端演示模块说明.md)
  - [答辩演示操作手册.md](/home/ljh/project/soft_course_design/docs/答辩演示操作手册.md)
- 未完成功能:
  - 真实业务写接口的 HTTP 控制器批量接入
  - 完整生产级前端登录、发布、审核、出价、支付页面
- 明确不在本步处理的内容:
  - 真实支付平台接入
  - Redis 客户端接入
  - 200 并发在线性能压测

### 3. 关键设计决定
- 决定 1: 前端演示台只读取 `auction_demo` 数据，不提供写交易状态的按钮
- 原因: 避免演示误改拍卖、订单、支付事实，保持 MySQL 事实源和自动化测试结果一致
- 影响范围:
  - 页面展示来自 `/api/demo/dashboard`
  - 核心业务写路径继续由服务层和测试验证

- 决定 2: `/api/demo/dashboard` 使用只读 SQL 聚合演示数据
- 原因: 答辩需要快速说明完整闭环，单接口能降低现场操作复杂度
- 影响范围:
  - 页面可一次性展示账号、拍卖、出价、订单、支付、评价、通知、任务日志、操作日志和统计日报
  - 不改变既有模块服务和交易状态机

### 4. 验证结果
- 执行命令:
  - `cmake -S . -B build`
  - `cmake --build build`
  - `scripts/test.sh frontend`
  - `ctest --test-dir build --output-on-failure`
  - `scripts/deploy/verify_release.sh`
  - `scripts/deploy/run_demo_server.sh`
  - `curl --max-time 5 -I http://127.0.0.1:18080/demo`
  - `curl --max-time 5 -i http://127.0.0.1:18080/api/demo/dashboard`
- 结果:
  - 前端演示数据测试通过
  - 全量 `ctest` 更新为 16/16 通过
  - `/demo`、`/assets/demo/app.css`、`/api/demo/dashboard` 均返回 200
  - `scripts/deploy/verify_release.sh` 通过
- 未执行的测试:
  - 浏览器自动化 UI 测试
- 原因:
  - 当前演示台为静态资源加只读接口，课程设计阶段以构建、接口数据和人工浏览器演示验收

### 5. 当前风险/阻塞
- 风险 1: 业务 HTTP 写接口仍未批量接入，除 `/demo`、`/healthz` 和 `/api/demo/dashboard` 外的业务 API 仍可能返回 404
- 风险 2: `/demo` 依赖 Drogon；无 Drogon 环境只能完成构建和服务层验证，不能长驻 HTTP 服务
- 阻塞项:
  - 无
- 需要注意的坑:
  - 演示前必须先执行 `scripts/deploy/init_demo_env.sh`
  - `scripts/deploy/run_demo_server.sh` 会占用 `127.0.0.1:18080`

### 6. 下一步
- 下一步 Step ID: S17
- 下一步目标: 建立真实业务 HTTP 接入基线和完整前端工程骨架
- 建议先读文件:
  - [schedule.md](/home/ljh/project/soft_course_design/docs/schedule.md)
  - [schedule.md 第 13 节](/home/ljh/project/soft_course_design/docs/schedule.md)
  - [前端演示模块说明.md](/home/ljh/project/soft_course_design/docs/前端演示模块说明.md)
  - [答辩演示操作手册.md](/home/ljh/project/soft_course_design/docs/答辩演示操作手册.md)
  - [部署与答辩说明.md](/home/ljh/project/soft_course_design/docs/部署与答辩说明.md)
  - [demo_http.cpp](/home/ljh/project/soft_course_design/src/access/http/demo_http.cpp)
  - [demo_dashboard_service.cpp](/home/ljh/project/soft_course_design/src/application/demo_dashboard_service.cpp)
  - [assets/demo](/home/ljh/project/soft_course_design/assets/demo)

## 模块 Handoff

### 1. 基本信息
- Step ID: S17
- 模块名称: 真实业务 HTTP 接入基线与完整前端工程骨架
- 当前状态: 已完成
- 对应文档: `docs/接口联调记录.md`、`docs/前端演示模块说明.md`、`docs/测试计划与用例说明.md`
- 对应代码目录: `src/access/http/`、`src/common/http/`、`assets/app/`、`tests/http/`、`scripts/`

### 2. 本次实际完成
- 已完成功能:
  - 新增完整前端骨架入口 `/app` 和静态资源 `/assets/app/app.css`、`/assets/app/app.js`
  - 新增真实业务登录接口 `POST /api/auth/login`
  - 新增受保护测试接口 `GET /api/system/context`
  - 新增统一 JSON 请求解析、异常到错误码映射、错误码到 HTTP 状态映射和认证上下文缓存
  - 新增 `HttpServiceContext`，统一装配认证、中间件、通知、拍卖、订单、支付、评价、统计和运维服务
  - 新增统一 JSON `404`，未注册 `/api/...` 不再直接返回裸 `404 Not Found`
  - 新增 `tests/http/auth_login_request.json`、`scripts/test_http.sh`、`scripts/test.sh http` 和 `suite_http`
- 实际修改文件:
  - [index.html](/home/ljh/project/soft_course_design/assets/app/index.html)
  - [app.css](/home/ljh/project/soft_course_design/assets/app/app.css)
  - [app.js](/home/ljh/project/soft_course_design/assets/app/app.js)
  - [app_http.cpp](/home/ljh/project/soft_course_design/src/access/http/app_http.cpp)
  - [auth_http.cpp](/home/ljh/project/soft_course_design/src/access/http/auth_http.cpp)
  - [system_http.cpp](/home/ljh/project/soft_course_design/src/access/http/system_http.cpp)
  - [business_http.cpp](/home/ljh/project/soft_course_design/src/access/http/business_http.cpp)
  - [api_error_http.cpp](/home/ljh/project/soft_course_design/src/access/http/api_error_http.cpp)
  - [http_service_context.cpp](/home/ljh/project/soft_course_design/src/common/http/http_service_context.cpp)
  - [http_utils.cpp](/home/ljh/project/soft_course_design/src/common/http/http_utils.cpp)
  - [test_http.sh](/home/ljh/project/soft_course_design/scripts/test_http.sh)
  - [auth_login_request.json](/home/ljh/project/soft_course_design/tests/http/auth_login_request.json)
  - [CMakeLists.txt](/home/ljh/project/soft_course_design/CMakeLists.txt)
  - [test.sh](/home/ljh/project/soft_course_design/scripts/test.sh)
- 未完成功能:
  - 注册、登出、当前用户接口和前端登录态保持
  - 卖家拍品、管理员审核建拍、买家浏览出价、订单支付、评价、统计运维页面
- 明确不在本步处理的内容:
  - 完整业务 HTTP 控制器批量接入
  - 浏览器自动化 UI 测试
  - 200 并发在线性能压测

### 3. 关键设计决定
- 决定 1: `S17` 只补真实 HTTP 地基，不提前吞掉 `S18` 的完整认证前端范围
- 原因: 需要先保证后续页面有稳定入口、统一响应和最小受保护链路，再逐步接入注册、登出、路由守卫和角色导航
- 影响范围:
  - 当前仅新增 `POST /api/auth/login` 和 `GET /api/system/context`
  - 其他真实业务 `/api/...` 路由仍待后续步骤继续接入

- 决定 2: 为接入层新增 `HttpServiceContext` 和 `http_utils`
- 原因: 后续 `S18-S30` 会连续新增多个控制器；若继续在 `bootstrap` 或控制器中零散 new 服务，会迅速失控
- 影响范围:
  - 统一服务装配、请求解析、认证上下文和错误映射
  - 后续控制器可以直接复用当前公共层，不必重复实现

### 4. 验证结果
- 执行命令:
  - `cmake -S . -B build`
  - `cmake --build build`
  - `scripts/test_http.sh`
  - `scripts/test.sh http`
  - `ctest --test-dir build --output-on-failure`
- 结果:
  - `/app`、`/app/`、`/assets/app/app.css`、`/assets/app/app.js` 返回 `200`
  - `POST /api/auth/login` 可成功返回 Token
  - `GET /api/system/context` 带 Token 返回成功，缺少 Token 返回统一 `401`
  - 未注册 `/api/not-found` 返回统一 JSON `404`
  - `auction_http_baseline` 通过
  - 全量 `ctest` 更新为 `17/17` 通过
- 未执行的测试:
  - 页面级浏览器自动化
- 原因:
  - `S17` 当前目标是最小真实 HTTP 基线，不是完整页面验收

### 5. 当前风险/阻塞
- 风险 1: 当前只完成最小登录和受保护基线，除 `POST /api/auth/login`、`GET /api/system/context` 和只读演示接口外，其余业务 HTTP 控制器仍未接入
- 风险 2: `scripts/test_http.sh` 依赖本地测试 MySQL 和本地监听端口，在 Codex 沙箱内可能需要沙箱外权限
- 风险 3: 若 `build/test_mysql/data/ibdata1` 被残留 `mysqld` 占用，测试脚本会卡在本地 MySQL 初始化
- 阻塞项:
  - 无
- 需要注意的坑:
  - 若 HTTP 测试卡住，先检查 `lsof build/test_mysql/data/ibdata1`
  - `/demo` 仍保留为只读答辩演示台，不要把 `/api/demo/dashboard` 当成真实业务数据源

### 6. 下一步
- 下一步 Step ID: S18（已完成，最新交接见下方 S18 Handoff）
- 下一步目标: 接入注册、登录态保持、角色导航和路由守卫；当前实际下一步为 S19
- 建议先读文件:
  - [schedule.md](/home/ljh/project/soft_course_design/docs/schedule.md)
  - [接口联调记录.md](/home/ljh/project/soft_course_design/docs/接口联调记录.md)
  - [前端演示模块说明.md](/home/ljh/project/soft_course_design/docs/前端演示模块说明.md)
  - [认证与权限模块说明.md](/home/ljh/project/soft_course_design/docs/认证与权限模块说明.md)
  - [auth_http.cpp](/home/ljh/project/soft_course_design/src/access/http/auth_http.cpp)
  - [system_http.cpp](/home/ljh/project/soft_course_design/src/access/http/system_http.cpp)
  - [http_service_context.cpp](/home/ljh/project/soft_course_design/src/common/http/http_service_context.cpp)
  - [assets/app](/home/ljh/project/soft_course_design/assets/app)

## 模块 Handoff

### 1. 基本信息
- Step ID: S18
- 模块名称: 认证、会话和角色路由
- 当前状态: 已完成
- 对应文档: `docs/认证与权限模块说明.md`、`docs/前端演示模块说明.md`、`docs/接口联调记录.md`、`docs/测试计划与用例说明.md`
- 对应代码目录: `src/access/http/`、`assets/app/`、`tests/http/`、`scripts/`

### 2. 本次实际完成
- 已完成功能:
  - 新增 `POST /api/auth/register`
  - 新增 `POST /api/auth/logout`
  - 新增 `GET /api/auth/me`
  - 新增 `PATCH /api/admin/users/{id}/status`
  - `/app` 已支持注册、登录、登出、会话恢复、角色导航和 hash 路由守卫
  - `scripts/test_http.sh` 已覆盖注册、登录态恢复、登出失效、管理员状态变更、`SUPPORT` 越权拒绝和冻结后登录拒绝
- 实际修改文件:
  - [auth_http.cpp](/home/ljh/project/soft_course_design/src/access/http/auth_http.cpp)
  - [index.html](/home/ljh/project/soft_course_design/assets/app/index.html)
  - [app.css](/home/ljh/project/soft_course_design/assets/app/app.css)
  - [app.js](/home/ljh/project/soft_course_design/assets/app/app.js)
  - [test_http.sh](/home/ljh/project/soft_course_design/scripts/test_http.sh)
  - [认证与权限模块说明.md](/home/ljh/project/soft_course_design/docs/认证与权限模块说明.md)
  - [前端演示模块说明.md](/home/ljh/project/soft_course_design/docs/前端演示模块说明.md)
  - [接口联调记录.md](/home/ljh/project/soft_course_design/docs/接口联调记录.md)
  - [测试计划与用例说明.md](/home/ljh/project/soft_course_design/docs/测试计划与用例说明.md)
  - [schedule.md](/home/ljh/project/soft_course_design/docs/schedule.md)
- 未完成功能:
  - `GET /api/admin/users` 用户列表查询暂未接入
  - 管理员审核建拍已在 S20 接入；买家浏览出价、订单支付、评价、统计运维页面仍待 S21-S25
- 明确不在本步处理的内容:
  - 拍品 CRUD HTTP 控制器
  - 真实出价、订单支付和评价页面
  - 浏览器自动化 UI 测试

### 3. 关键设计决定
- 决定 1: 继续复用 `AuthService` 和 `AuthMiddleware`，HTTP 层只做请求解析、统一响应和路径参数转换
- 原因: 服务层已经完成密码哈希、Token、会话、RBAC、状态变更和审计日志，S18 不重复实现业务规则
- 影响范围:
  - HTTP 认证接口与服务层状态机保持一致
  - 后续业务控制器继续通过 `HttpServiceContext` 复用认证上下文

- 决定 2: `/app` 继续采用静态 HTML/CSS/JS，不引入前端构建链
- 原因: 当前课程设计答辩环境以本地 C++ 服务和静态资源托管为主，减少 Node/Vite 依赖
- 影响范围:
  - 前端会话保存到 `sessionStorage`
  - 角色路由使用 hash 路由和前端守卫实现

### 4. 验证结果
- 执行命令:
  - `cmake -S . -B build`
  - `cmake --build build`
  - `scripts/test_http.sh`
  - `scripts/test.sh http`
  - `ctest --test-dir build --output-on-failure`
- 结果:
  - `auction_http_baseline` 通过
  - HTTP 注册、登录、当前用户、登出、管理员状态变更、越权拒绝和统一 `404` 均通过
  - 全量 `ctest` 为 `17/17` 通过
- 未执行的测试:
  - 页面级浏览器自动化
- 原因:
  - `S18` 当前目标是认证前端与真实 HTTP 回归，不是完整 UI 自动化验收

### 5. 当前风险/阻塞
- 风险 1: 当前 HTTP 回归仍集中在认证模块，后续业务控制器尚未接入
- 风险 2: `scripts/test_http.sh` 依赖本地 MySQL 和本地 HTTP 监听，Codex 沙箱内可能无法启动 Unix socket
- 阻塞项:
  - 无
- 需要注意的坑:
  - 若 `build/test_mysql/data/ibdata1` 被残留 `mysqld` 占用，需要先停止命令行明确指向本项目 `build/test_mysql` 的残留进程

### 6. 下一步
- 下一步 Step ID: S19（已完成，最新交接见下方 S19 Handoff）
- 下一步目标: 接入卖家拍品发布与管理页面；当前实际下一步为 S20
- 建议先读文件:
  - [schedule.md](/home/ljh/project/soft_course_design/docs/schedule.md)
  - [物品与审核模块说明.md](/home/ljh/project/soft_course_design/docs/物品与审核模块说明.md)
  - [前端演示模块说明.md](/home/ljh/project/soft_course_design/docs/前端演示模块说明.md)
  - [item_service.cpp](/home/ljh/project/soft_course_design/src/modules/item/item_service.cpp)
  - [item_flow_tests.cpp](/home/ljh/project/soft_course_design/tests/item/item_flow_tests.cpp)
  - [assets/app](/home/ljh/project/soft_course_design/assets/app)

## 模块 Handoff

### 1. 基本信息
- Step ID: S19
- 模块名称: 卖家拍品发布与管理页面
- 当前状态: 已完成
- 对应文档: `docs/物品与审核模块说明.md`、`docs/前端演示模块说明.md`、`docs/接口联调记录.md`、`docs/测试计划与用例说明.md`
- 对应代码目录: `src/access/http/`、`assets/app/`、`tests/http/`、`scripts/`

### 2. 本次实际完成
- 已完成功能:
  - 新增 `src/access/http/item_http.*`
  - 新增 `POST /api/items`
  - 新增 `PUT /api/items/{id}`
  - 新增 `POST /api/items/{id}/images`
  - 新增 `DELETE /api/items/{itemId}/images/{imageId}`
  - 新增 `GET /api/items/mine`
  - 新增 `GET /api/items/{id}`
  - 新增 `POST /api/items/{id}/submit-audit`
  - `/app` 卖家视图支持拍品创建、编辑、图片元数据、列表筛选、详情查看、驳回原因展示和提交审核
  - `scripts/test_http.sh` 已覆盖卖家拍品真实 HTTP 回归
- 实际修改文件:
  - [item_http.cpp](/home/ljh/project/soft_course_design/src/access/http/item_http.cpp)
  - [item_http.h](/home/ljh/project/soft_course_design/src/access/http/item_http.h)
  - [business_http.cpp](/home/ljh/project/soft_course_design/src/access/http/business_http.cpp)
  - [CMakeLists.txt](/home/ljh/project/soft_course_design/CMakeLists.txt)
  - [index.html](/home/ljh/project/soft_course_design/assets/app/index.html)
  - [app.css](/home/ljh/project/soft_course_design/assets/app/app.css)
  - [app.js](/home/ljh/project/soft_course_design/assets/app/app.js)
  - [test_http.sh](/home/ljh/project/soft_course_design/scripts/test_http.sh)
  - [物品与审核模块说明.md](/home/ljh/project/soft_course_design/docs/物品与审核模块说明.md)
  - [前端演示模块说明.md](/home/ljh/project/soft_course_design/docs/前端演示模块说明.md)
  - [接口联调记录.md](/home/ljh/project/soft_course_design/docs/接口联调记录.md)
  - [测试计划与用例说明.md](/home/ljh/project/soft_course_design/docs/测试计划与用例说明.md)
  - [schedule.md](/home/ljh/project/soft_course_design/docs/schedule.md)
- 未完成功能:
  - 管理员待审列表、审核通过/驳回和拍卖创建页面已在 S20 完成
  - 真实文件 multipart 上传仍未接入，本步只处理图片元数据
- 明确不在本步处理的内容:
  - 管理员审核建拍
  - 买家拍卖大厅与出价
  - 订单支付、评价、统计运维页面
  - 页面级浏览器自动化 UI 测试

### 3. 关键设计决定
- 决定 1: HTTP 层复用 `ItemService`，不在控制器中重写所有权、状态机和图片完整性规则
- 原因: S05 服务层已具备拍品草稿、图片元数据、提审和状态约束，HTTP 层应只负责 JSON 解析、路径参数和统一响应
- 影响范围:
  - 卖家只能操作自己的拍品
  - 提审后禁止编辑、缺图禁止提审等规则继续由服务层保证

- 决定 2: `/app` 继续使用静态 HTML/CSS/JS，不引入前端构建链
- 原因: 当前课程答辩环境以本地 C++ 服务和静态资源托管为主，保持 S17-S18 的低依赖路线
- 影响范围:
  - 卖家页面通过 hash 路由和原有 `sessionStorage` Token 访问真实 `/api/items...` 接口
  - 前端使用 DOM API 渲染拍品和图片，避免直接插入未转义 HTML

### 4. 验证结果
- 执行命令:
  - `cmake -S . -B build`
  - `cmake --build build`
  - `scripts/test_http.sh`
  - `ctest --test-dir build --output-on-failure`
- 结果:
  - `auction_http_baseline` 通过
  - HTTP 回归已覆盖拍品创建、非法字段、图片元数据、删除图片、编辑、列表、详情、提交审核和提审后禁止编辑
  - 全量 `ctest` 为 `17/17` 通过
- 未执行的测试:
  - 页面级浏览器自动化
- 原因:
  - S19 验收重点是真实 HTTP 接口和静态前端接入，页面级自动化规划在 S28

### 5. 当前风险/阻塞
- 风险 1: 图片能力当前只保存 `imageUrl` 元数据，尚未接入真实 multipart 上传和文件清理任务
- 风险 2: 管理员审核和管理端建拍已在 S20 接入，公开拍卖大厅和真实出价仍待 S21-S22
- 风险 3: `scripts/test_http.sh` 依赖本地 MySQL 和 HTTP 监听，Codex 沙箱内可能无法启动用户态 `mysqld`
- 阻塞项:
  - 无
- 需要注意的坑:
  - 若 HTTP 测试提示 `Failed to start mysqld daemon`，按权限流程在沙箱外运行或检查 `build/test_mysql` 残留进程

### 6. 下一步
- 下一步 Step ID: S21（S20 已完成，最新交接见下方 S20 Handoff）
- 下一步目标: 接入买家拍卖大厅与详情页
- 建议先读文件:
  - [schedule.md](/home/ljh/project/soft_course_design/docs/schedule.md)
  - [物品与审核模块说明.md](/home/ljh/project/soft_course_design/docs/物品与审核模块说明.md)
  - [拍卖管理模块说明.md](/home/ljh/project/soft_course_design/docs/拍卖管理模块说明.md)
  - [item_http.cpp](/home/ljh/project/soft_course_design/src/access/http/item_http.cpp)
  - [item_audit_service.cpp](/home/ljh/project/soft_course_design/src/modules/audit/item_audit_service.cpp)
  - [auction_service.cpp](/home/ljh/project/soft_course_design/src/modules/auction/auction_service.cpp)
  - [assets/app](/home/ljh/project/soft_course_design/assets/app)

## 模块 Handoff

### 1. 基本信息
- Step ID: S20
- 模块名称: 管理员审核与拍卖管理页面
- 当前状态: 已完成
- 对应文档: `docs/物品与审核模块说明.md`、`docs/拍卖管理模块说明.md`、`docs/前端演示模块说明.md`、`docs/接口联调记录.md`、`docs/测试计划与用例说明.md`
- 对应代码目录: `src/access/http/`、`assets/app/`、`tests/http/`、`scripts/`

### 2. 本次实际完成
- 已完成功能:
  - 新增 `src/access/http/audit_http.*`
  - 新增 `src/access/http/auction_admin_http.*`
  - 新增 `GET /api/admin/items/pending`
  - 新增 `POST /api/admin/items/{id}/audit`
  - 新增 `GET /api/admin/items/{id}/audit-logs`
  - 新增 `POST /api/admin/auctions`
  - 新增 `GET /api/admin/auctions`
  - 新增 `GET /api/admin/auctions/{id}`
  - 新增 `PUT /api/admin/auctions/{id}`
  - 新增 `POST /api/admin/auctions/{id}/cancel`
  - `/app` 管理员视图支持用户状态、待审核拍品、审核结果提交、审核日志、拍卖规则表单、管理端拍卖列表和详情
  - `scripts/test_http.sh` 已覆盖管理员审核和管理端拍卖真实 HTTP 回归
- 实际修改文件:
  - [audit_http.cpp](/home/ljh/project/soft_course_design/src/access/http/audit_http.cpp)
  - [audit_http.h](/home/ljh/project/soft_course_design/src/access/http/audit_http.h)
  - [auction_admin_http.cpp](/home/ljh/project/soft_course_design/src/access/http/auction_admin_http.cpp)
  - [auction_admin_http.h](/home/ljh/project/soft_course_design/src/access/http/auction_admin_http.h)
  - [business_http.cpp](/home/ljh/project/soft_course_design/src/access/http/business_http.cpp)
  - [CMakeLists.txt](/home/ljh/project/soft_course_design/CMakeLists.txt)
  - [index.html](/home/ljh/project/soft_course_design/assets/app/index.html)
  - [app.css](/home/ljh/project/soft_course_design/assets/app/app.css)
  - [app.js](/home/ljh/project/soft_course_design/assets/app/app.js)
  - [test_http.sh](/home/ljh/project/soft_course_design/scripts/test_http.sh)
  - [物品与审核模块说明.md](/home/ljh/project/soft_course_design/docs/物品与审核模块说明.md)
  - [拍卖管理模块说明.md](/home/ljh/project/soft_course_design/docs/拍卖管理模块说明.md)
  - [前端演示模块说明.md](/home/ljh/project/soft_course_design/docs/前端演示模块说明.md)
  - [接口联调记录.md](/home/ljh/project/soft_course_design/docs/接口联调记录.md)
  - [测试计划与用例说明.md](/home/ljh/project/soft_course_design/docs/测试计划与用例说明.md)
  - [schedule.md](/home/ljh/project/soft_course_design/docs/schedule.md)
- 未完成功能:
  - 公开拍卖大厅和买家详情页仍待 S21
  - 真实出价和实时通知页面仍待 S22
- 明确不在本步处理的内容:
  - 公开拍卖接口
  - 买家出价
  - 订单支付、评价、统计运维页面
  - 页面级浏览器自动化 UI 测试

### 3. 关键设计决定
- 决定 1: HTTP 层复用 `ItemAuditService` 和 `AuctionService`，不在控制器中重写审核状态机、建拍冲突校验和时间价格规则
- 原因: S05/S06 服务层已具备事务边界、RBAC、状态机和错误码约束，HTTP 层只应负责参数解析和统一响应
- 影响范围:
  - 审核通过/驳回和拍卖创建/修改/取消继续由服务层保证一致性
  - `SUPPORT` 只读查询与 `ADMIN` 写操作权限保持服务层口径

- 决定 2: `/app` 管理员页面继续沿用静态 HTML/CSS/JS
- 原因: 与 S17-S19 保持同一低依赖前端路线，减少课程答辩环境复杂度
- 影响范围:
  - 管理端页面使用现有 hash 路由、`sessionStorage` Token 和 DOM API 渲染
  - 管理台不引入 Node/Vite/React 构建链

### 4. 验证结果
- 执行命令:
  - `cmake -S . -B build`
  - `cmake --build build`
  - `bash -n scripts/test_http.sh`
  - `node --check assets/app/app.js`
  - `scripts/test_http.sh`
  - `ctest --test-dir build --output-on-failure`
- 结果:
  - `auction_http_baseline` 通过
  - HTTP 回归已覆盖待审列表、审核通过、卖家查看通过结果、拍卖创建、列表、详情、修改、取消、审核驳回、卖家查看驳回原因和审核日志
  - 全量 `ctest` 为 `17/17` 通过
- 未执行的测试:
  - 页面级浏览器自动化
- 原因:
  - S20 验收重点是真实 HTTP 接口和静态前端接入，页面级自动化规划在 S28

### 5. 当前风险/阻塞
- 风险 1: 公开拍卖大厅尚未接入，买家暂不能从页面浏览 S20 创建的拍卖
- 风险 2: 出价和实时通知仍待 S22，管理端当前只负责审核和建拍
- 风险 3: `scripts/test_http.sh` 和全量 `ctest` 依赖本地 MySQL 与 HTTP 监听，Codex 沙箱内可能无法启动用户态 `mysqld`
- 阻塞项:
  - 无
- 需要注意的坑:
  - 若测试提示 `Failed to start mysqld daemon` 或 `Unable to lock ./ibdata1`，先检查并清理本项目 `build/test_mysql` 残留 `mysqld`，再按权限流程在沙箱外运行

### 6. 下一步
- 下一步 Step ID: S21
- 下一步目标: 接入买家拍卖大厅与详情页
- 建议先读文件:
  - [schedule.md](/home/ljh/project/soft_course_design/docs/schedule.md)
  - [拍卖管理模块说明.md](/home/ljh/project/soft_course_design/docs/拍卖管理模块说明.md)
  - [前端演示模块说明.md](/home/ljh/project/soft_course_design/docs/前端演示模块说明.md)
  - [auction_admin_http.cpp](/home/ljh/project/soft_course_design/src/access/http/auction_admin_http.cpp)
  - [auction_service.cpp](/home/ljh/project/soft_course_design/src/modules/auction/auction_service.cpp)
  - [auction_repository.cpp](/home/ljh/project/soft_course_design/src/repository/auction_repository.cpp)
  - [app.js](/home/ljh/project/soft_course_design/assets/app/app.js)

## 10. 模块 Handoff 模板

每完成一个模块，建议至少按以下模板补齐一次 handoff 记录，再进行 Codex 上下文压缩：

```md
## 模块 Handoff

### 1. 基本信息
- Step ID: SXX
- 模块名称:
- 当前状态: 已完成 / 进行中 / 阻塞
- 对应文档: docs/xxx.md
- 对应代码目录: src/xxx/ tests/xxx/

### 2. 本次实际完成
- 已完成功能:
- 实际修改文件:
  - path1
  - path2
- 未完成功能:
- 明确不在本步处理的内容:

### 3. 关键设计决定
- 决定 1:
- 原因:
- 影响范围:

- 决定 2:
- 原因:
- 影响范围:

### 4. 验证结果
- 执行命令:
  - `cmake -S . -B build`
  - `cmake --build build`
  - `ctest --test-dir build --output-on-failure`
- 结果:
- 未执行的测试:
- 原因:

### 5. 当前风险/阻塞
- 风险 1:
- 风险 2:
- 阻塞项:
- 需要注意的坑:

### 6. 下一步
- 下一步 Step ID:
- 下一步目标:
- 建议先读文件:
  - docs/schedule.md
  - docs/xxx.md
  - src/xxx/
```

## 11. 压缩前检查清单

每次完成一个模块并准备进行 Codex 上下文压缩前，建议先检查以下事项：

```md
- [ ] schedule.md 已更新状态
- [ ] 模块文档已更新“已实现/未实现”
- [ ] 验证命令和结果已记录
- [ ] 下一步已写清楚
- [ ] 关键决定已写进代码/文档，不只存在聊天里
```

## 12. 新会话固定恢复文本

后续每次进入本项目的新一轮 agent 执行，开始前都应先读取并遵循下面这段固定文本：

```text
这是 /home/ljh/project/soft_course_design 项目。

请先读取并对齐以下内容后再继续：
1. docs/schedule.md
2. docs/部署与答辩说明.md
3. docs/环境配置说明.md
4. docs/测试报告.md
5. docs/测试计划与用例说明.md
6. docs/接口联调记录.md
7. docs/系统概要设计报告.md
8. docs/前端演示模块说明.md
9. docs/答辩演示操作手册.md
10. docs/schedule.md 第 13 节“全量接入后端的完整前端开发 Schedule”
11. 对应代码目录 assets/demo/、src/access/http/、src/application/、src/modules/、scripts/、scripts/deploy/、config/、sql/、tests/、CMakeLists.txt
12. 若开始完整前端开发，还要检查计划目录 assets/app/、tests/http/ 是否已存在；不存在则按 S17 创建

当前正在做：完整前端全量接入真实业务后端的扩展计划，下一步从 S21 开始
当前状态：S00-S20 已完成；S21-S30 已规划但均未开始

本次若需要继续：
- 先确认用户指定的新目标，不要默认新增功能
- 若只是交付或答辩复现，优先使用 `scripts/deploy/init_demo_env.sh`、`scripts/deploy/run_demo_server.sh`、`http://127.0.0.1:18080/demo`、`scripts/deploy/show_demo_walkthrough.sh` 和 `scripts/deploy/verify_release.sh`
- 若继续开发完整前端，先执行 `S21`，在 `S20` 已有管理员审核与管理端拍卖页面之上接入买家拍卖大厅与详情页，再继续出价、订单支付、评价、统计运维等页面开发
- 任一 S17-S30 步骤完成后，同步更新 `docs/schedule.md`、相关模块文档、测试计划和 handoff

注意约束：
- 以实际代码落地为准，不以仅写文档算完成
- 任何 sudo 先问我，不用sudo，所有操作你都可以直接执行
- 始终用中文
```

历史 S15 恢复重点：

```text
S15 已完成。已落地 `scripts/deploy/`、`sql/demo_data.sql`、`docs/部署与答辩说明.md`，并调整 `config/nginx.conf` 为本地非特权演示端口 `18081` 代理应用端口 `18080`。

最近一次验证：
- `cmake -S . -B build`
- `cmake --build build`
- `scripts/deploy/init_demo_env.sh`
- `scripts/deploy/verify_release.sh`
- `scripts/deploy/run_demo_server.sh`
- `curl http://127.0.0.1:18080/healthz`

结果：演示库 `auction_demo` 初始化成功，高风险专项 2/2 通过，全量 CTest 15/15 通过，健康检查返回 `status=ok`。

注意：若用户态 MySQL 初始化失败并提示 `Unable to lock ./ibdata1`，优先检查是否有残留 `mysqld` 占用 `build/test_mysql/data`。
```

历史 S16 恢复重点：

```text
S16 已完成。已落地 `assets/demo/` 浏览器前端演示台、`src/access/http/demo_http.*`、`src/application/demo_dashboard_service.*`、`tests/demo/demo_dashboard_tests.cpp`、`scripts/test_demo_frontend.sh`、`docs/前端演示模块说明.md` 和 `docs/答辩演示操作手册.md`。

演示入口：
- `http://127.0.0.1:18080/demo`
- `curl http://127.0.0.1:18080/api/demo/dashboard`

验证入口：
- `scripts/test.sh frontend`
- `ctest --test-dir build --output-on-failure`

结果：前端演示数据测试通过，全量 CTest 更新为 16/16 通过。
HTTP 验证：`/healthz`、`/demo`、`/assets/demo/app.css`、`/api/demo/dashboard` 均返回 200。
最终脚本：`scripts/deploy/verify_release.sh` 通过。

注意：后续已接入认证和卖家拍品真实业务接口；未接入的其他 `/api/...` 仍会返回统一 JSON `404`，属于当前边界。
```

历史 S17-S30 恢复重点：

```text
S17-S20 已完成，S21-S30 是后续完整前端全量接入真实业务后端计划，当前均未开始。

执行顺序要求：
- 已完成 S17，当前已具备 `/app`、`/assets/app/*`、`POST /api/auth/login`、`GET /api/system/context` 和统一 JSON `404`。
- 已完成 S18，当前已具备 `POST /api/auth/register`、`POST /api/auth/logout`、`GET /api/auth/me`、`PATCH /api/admin/users/{id}/status`、前端会话保持、角色导航和路由守卫。
- 已完成 S19，当前已具备 `POST /api/items`、`PUT /api/items/{id}`、`POST /api/items/{id}/images`、`DELETE /api/items/{itemId}/images/{imageId}`、`GET /api/items/mine`、`GET /api/items/{id}`、`POST /api/items/{id}/submit-audit` 和卖家拍品管理页面。
- 已完成 S20，当前已具备 `GET /api/admin/items/pending`、`POST /api/admin/items/{id}/audit`、`GET /api/admin/items/{id}/audit-logs`、`POST /api/admin/auctions`、`GET /api/admin/auctions`、`GET /api/admin/auctions/{id}`、`PUT /api/admin/auctions/{id}`、`POST /api/admin/auctions/{id}/cancel` 和管理员审核建拍页面。
- 下一步从 S21 开始，再按角色和业务流开发完整前端：买家浏览与出价、订单支付、评价通知、统计运维。
- 当前 `/demo` 仍保留为只读答辩演示台，不要用它替代真实业务页面。

关键边界：
- MySQL 仍是交易事实唯一来源。
- Redis 只能作为缓存或旁路，不能决定出价合法性。
- 出价、结算、支付回调等写路径必须保持服务层事务和幂等约束。
- 当前已接入的真实业务 HTTP 包括认证、用户状态、卖家拍品、管理员审核和管理端拍卖接口；公开拍卖、出价、订单支付、评价、统计运维等业务 `/api/...` 仍待后续步骤接入。
```

说明如下：

- 当当前步骤变化时，要把这里的模块文档和代码目录替换成新的真实目标
- 若当前步骤尚未创建代码目录，也必须先读取 `docs/schedule.md` 和对应模块文档，再决定下一步

## 13. 全量接入后端的完整前端开发 Schedule

本节用于规划 `S17-S30`，目标是把当前只读答辩演示台扩展为完整浏览器前端，并接入真实业务后端接口。当前 `S17-S20` 已完成，`S21-S30` 均为 `未开始`。

### 13.1 目标与边界

- 目标：提供面向买家、卖家、管理员、运维/客服的完整浏览器前端，覆盖注册登录、拍品发布、审核、拍卖配置、浏览出价、订单支付、履约、评价、通知、统计和异常处理。
- 前提：当前后端服务层已具备主流程能力，Drogon 接入层已落地 `/healthz`、`/demo`、`/api/demo/dashboard`、认证、卖家拍品、管理员审核和管理端拍卖接口；后续仍需继续补公开拍卖、出价、订单支付、评价、统计运维等真实业务 HTTP 控制器。
- 边界：`/demo` 保留为只读答辩演示台；完整前端应使用真实业务 `/api/...` 接口，不应复用 `/api/demo/dashboard` 作为业务数据源。
- 技术取向：除非用户明确要求引入 Node/Vite/React，否则优先沿用静态 HTML/CSS/JS 加 C++ 后端静态资源托管，降低课程答辩环境复杂度。
- 数据约束：MySQL 仍是交易事实唯一来源；Redis 只能作为缓存或旁路能力，不能决定出价、结算和支付状态。

### 13.2 完成判定

任一 `S17-S30` 步骤只有同时满足以下条件，才允许改为 `已完成`：

1. 后端真实接口、前端页面、脚本或配置已有实际代码落地。
2. 页面访问不再依赖手工改 URL 或直接查库，功能通过真实 `/api/...` 接口完成。
3. 至少有一个可重复执行的验证入口，优先接入 `CTest`、`scripts/test.sh` 或专用脚本。
4. 相关接口、页面、错误处理和演示方式已同步到模块文档或 `docs/前端演示模块说明.md`。
5. `docs/schedule.md` 已追加进度记录和 handoff，说明实际代码路径、验证命令和未覆盖风险。

### 13.3 扩展步骤总表

| 步骤 ID | 可分配给 agent 的实际目标 | 前置条件 | 需要实际完成的代码/脚本 | 完成判定 | 预计代码位置 | 完成后文档位置 | 当前状态 |
|---|---|---|---|---|---|---|---|
| S17 | 建立真实业务 HTTP 接入基线和完整前端工程骨架 | S16 已完成，服务层模块可复用 | 注册业务路由分组、统一请求解析、统一错误映射、认证上下文、静态完整前端入口、最小 API 客户端、HTTP 冒烟测试脚本 | `/api/auth/login` 之外至少有一个受保护测试接口可返回统一响应；未知接口 404 有明确 JSON 错误；完整前端首页可打开 | `src/access/http/`、`src/common/http/`、`assets/app/`、`tests/http/`、`scripts/test_http.sh` | `docs/接口联调记录.md`、`docs/前端演示模块说明.md`、`docs/schedule.md` | 已完成 |
| S18 | 接入认证、会话和角色路由 | S17 | 实现注册、登录、登出、当前用户、管理员用户状态接口；前端登录页、注册页、会话保持、角色导航、路由守卫 | 用户可注册登录；不同角色看到不同入口；未登录访问受保护页面会跳转登录 | `src/access/http/auth_http.*`、`assets/app/`、`tests/http/` | `docs/认证与权限模块说明.md`、`docs/前端演示模块说明.md` | 已完成 |
| S19 | 接入卖家拍品发布与管理页面 | S18 | 实现拍品创建、编辑、图片元数据、我的拍品、提交审核接口；前端卖家拍品列表、表单、状态提示和驳回原因展示 | 卖家可从页面创建拍品并提交审核；非法字段显示业务错误；数据落库可查 | `src/access/http/item_http.*`、`assets/app/`、`tests/http/` | `docs/物品与审核模块说明.md`、`docs/前端演示模块说明.md` | 已完成 |
| S20 | 接入管理员审核与拍卖管理页面 | S19 | 实现待审拍品、审核通过/驳回、创建/修改/取消拍卖、管理端拍卖查询接口；前端管理台表格、审核弹窗、拍卖规则表单 | 管理员可审核拍品并创建拍卖；卖家可看到审核结果；活动状态与拍品状态一致 | `src/access/http/audit_http.*`、`src/access/http/auction_admin_http.*`、`assets/app/`、`tests/http/` | `docs/拍卖管理模块说明.md`、`docs/物品与审核模块说明.md` | 已完成 |
| S21 | 接入买家拍卖大厅与详情页 | S20 | 实现公开拍卖列表、详情、出价历史、当前最高价查询接口；前端首页、筛选、详情页、倒计时和状态标签 | 买家可浏览拍卖、进入详情、看到当前最高价和历史出价；结束或未开始活动按钮状态正确 | `src/access/http/auction_public_http.*`、`assets/app/`、`tests/http/` | `docs/拍卖管理模块说明.md`、`docs/前端演示模块说明.md` | 未开始 |
| S22 | 接入真实出价和实时通知 | S21 | 实现出价接口、幂等键传递、出价错误映射、WebSocket 或轮询通知入口；前端出价面板、价格刷新、被超越提示和失败重试提示 | 并发出价后页面最高价与数据库一致；低价、过期、冻结用户和重复幂等键均有明确提示 | `src/access/http/bid_http.*`、`src/access/http/notification_http.*`、`src/ws/`、`assets/app/`、`tests/http/` | `docs/竞价与实时通知模块说明.md`、`docs/测试计划与用例说明.md` | 未开始 |
| S23 | 接入订单、支付和履约页面 | S22 | 实现我的订单、支付发起、mock 支付结果展示、卖家发货、买家确认收货接口；前端订单列表、订单详情、支付状态页和履约操作 | 获胜买家可完成支付流程；卖家可发货；买家可确认收货；重复支付或非法状态操作被拒绝 | `src/access/http/order_http.*`、`src/access/http/payment_http.*`、`assets/app/`、`tests/http/` | `docs/订单与支付模块说明.md`、`docs/前端演示模块说明.md` | 未开始 |
| S24 | 接入评价、通知中心和用户信用展示 | S23 | 实现评价提交、订单评价查询、用户信用汇总、通知列表、通知已读接口；前端评价表单、通知中心和个人信用卡片 | 订单完成后买卖双方可互评；重复评价被拒绝；通知能按用户展示并标记已读 | `src/access/http/review_http.*`、`src/access/http/notification_http.*`、`assets/app/`、`tests/http/` | `docs/评价与反馈模块说明.md`、`docs/竞价与实时通知模块说明.md` | 未开始 |
| S25 | 接入统计报表和运维异常页面 | S24 | 实现统计日报、CSV 导出、操作日志、任务日志、异常看板、通知重试、补偿入口接口；前端管理员统计页和运维工作台 | 管理员可查看统计；运维可查看日志并执行允许的补偿动作；越权访问被拒绝 | `src/access/http/statistics_http.*`、`src/access/http/ops_http.*`、`assets/app/`、`tests/http/` | `docs/统计分析与报表模块说明.md`、`docs/系统监控与异常处理模块说明.md` | 未开始 |
| S26 | 完成全站 UX、状态处理和安全前端约束 | S18-S25 | 统一 loading、empty、error、toast、表单校验、金额格式、时间显示、鉴权过期处理、XSS 安全渲染和 404 页面 | 主要页面在无数据、接口失败、权限不足、会话过期时都有可解释反馈；金额和时间展示一致 | `assets/app/`、`tests/http/`、`scripts/` | `docs/前端演示模块说明.md`、`docs/测试计划与用例说明.md` | 未开始 |
| S27 | 建立真实 HTTP 接口自动化回归 | S18-S25 | 为认证、拍品、审核、拍卖、出价、订单支付、评价、统计运维补 curl 或 CTest 级接口测试；接入 `scripts/test.sh http` | 真实 HTTP 主流程可自动跑通；不再只依赖服务层契约映射；全量 `ctest` 包含 HTTP 回归 | `tests/http/`、`scripts/test_http.sh`、`scripts/test.sh`、`CMakeLists.txt` | `docs/测试计划与用例说明.md`、`docs/接口联调记录.md` | 未开始 |
| S28 | 建立前端页面级验收脚本 | S26-S27 | 增加静态资源完整性检查、关键页面可达性检查、必要时增加轻量浏览器自动化；接入 `scripts/test.sh ui` 或 `scripts/test.sh frontend-full` | 启动服务后首页、登录页、拍卖大厅、详情页、订单页、管理台均可访问且核心接口返回 200/业务错误 | `scripts/test_ui.sh`、`tests/ui/`、`assets/app/` | `docs/测试计划与用例说明.md`、`docs/前端演示模块说明.md` | 未开始 |
| S29 | 接入部署、演示数据和答辩脚本 | S27-S28 | 扩展演示初始化、服务启动、Nginx 模板、答辩 walkthrough、发布验证脚本，让完整前端成为默认演示入口，保留 `/demo` 只读入口 | `scripts/deploy/verify_release.sh` 能验证完整前端；答辩脚本能演示真实写操作和只读总览两条路径 | `scripts/deploy/`、`config/`、`sql/demo_data.sql`、`assets/app/` | `docs/部署与答辩说明.md`、`docs/答辩演示操作手册.md` | 未开始 |
| S30 | 完成完整前端最终回归和 handoff | S29 | 执行全量构建、HTTP 回归、UI 验收、高风险专项、全量 CTest 和发布验证；补齐最终 handoff | 完整前端可在本机从初始化到浏览器演示复现；`docs/schedule.md` 记录全部验证命令和剩余风险 | 全仓库 | `docs/schedule.md`、`docs/部署与答辩说明.md`、`docs/答辩演示操作手册.md` | 未开始 |

### 13.4 必须覆盖的业务接口矩阵

| 模块 | 前端必须接入的接口范围 | 关键风险点 |
|---|---|---|
| 认证与权限 | `POST /api/auth/register`、`POST /api/auth/login`、`POST /api/auth/logout`、`GET /api/auth/me`、管理员用户状态管理 | Token 过期、冻结账号、RBAC 越权 |
| 拍品与审核 | 拍品创建、编辑、图片元数据、提交审核、审核通过、审核驳回、我的拍品、待审列表 | 卖家只能操作自己的拍品；审核状态不可跳转 |
| 拍卖管理 | 管理端创建/修改/取消拍卖，公开列表，公开详情，调度后状态查询 | 拍品与拍卖状态一致；已开始活动不可非法修改 |
| 竞价与通知 | 提交出价、出价历史、当前最高价、WebSocket 或轮询价格更新、通知列表 | 最高价一致性、幂等键、延时保护、通知失败不回滚 |
| 订单与支付 | 我的订单、支付发起、支付状态、支付回调、卖家发货、买家确认收货、超时关闭 | 一场拍卖最多一单；回调幂等；非法状态操作拒绝 |
| 评价与信用 | 提交评价、订单评价查询、用户信用汇总 | 只能交易双方评价；一方一次；订单终态约束 |
| 统计与运维 | 统计日报、CSV 导出、操作日志、任务日志、异常看板、通知重试、补偿入口 | 管理和运维权限隔离；补偿动作可审计 |

### 13.5 推荐执行顺序

1. `S17` 已完成，已补后端 HTTP 接入基线和前端工程骨架。
2. `S18` 已完成，已补认证、会话和角色路由。
3. `S19` 已完成，已补卖家拍品发布与管理页面。
4. `S20` 已完成，已补管理员审核与管理端拍卖页面。
5. 当前继续执行 `S21-S24`，按业务闭环推进买家浏览出价、订单支付、履约评价。
6. 再执行 `S25-S26`，补管理端统计运维和全站体验、安全状态处理。
7. 最后执行 `S27-S30`，把真实 HTTP 回归、页面验收、部署演示和最终 handoff 收口。

### 13.6 S17 启动前检查清单

```md
- [ ] 已读取 `docs/schedule.md` 第 13 节
- [ ] 已读取 `docs/接口联调记录.md`
- [ ] 已读取 `docs/前端演示模块说明.md`
- [ ] 已确认当前 `/api/demo/dashboard` 只是只读演示接口
- [ ] 已确认业务 HTTP 控制器仍在分步接入，未接入的其他 `/api/...` 404 是当前边界
- [ ] 已查看 `src/access/http/health_http.*` 和 `src/access/http/demo_http.*`
- [ ] 已查看目标模块服务层，例如 `src/modules/auth/`、`src/modules/item/`、`src/modules/auction/`、`src/modules/bid/`
- [ ] 已查看对应测试目录，优先复用现有服务层测试数据和断言
```

### 13.7 计划态 Handoff

```text
Step ID: S20-S30
模块名称: 完整前端全量接入真实业务后端
当前状态: S17-S20 已完成；S21-S30 未开始
下一步 Step ID: S21
下一步目标: 接入买家拍卖大厅与详情页

当前已知事实:
- S00-S20 已完成。
- 现有 `/demo` 是只读答辩演示台，不是完整生产前端。
- 当前已落地真实业务基线路由：`/app`、`/app/`、`/assets/app/app.css`、`/assets/app/app.js`、`POST /api/auth/login`、`GET /api/system/context`。
- 当前已落地认证会话路由：`POST /api/auth/register`、`POST /api/auth/logout`、`GET /api/auth/me`、`PATCH /api/admin/users/{id}/status`。
- 当前已落地卖家拍品路由：`POST /api/items`、`PUT /api/items/{id}`、`POST /api/items/{id}/images`、`DELETE /api/items/{itemId}/images/{imageId}`、`GET /api/items/mine`、`GET /api/items/{id}`、`POST /api/items/{id}/submit-audit`。
- 当前已落地管理员审核与管理端拍卖路由：`GET /api/admin/items/pending`、`POST /api/admin/items/{id}/audit`、`GET /api/admin/items/{id}/audit-logs`、`POST /api/admin/auctions`、`GET /api/admin/auctions`、`GET /api/admin/auctions/{id}`、`PUT /api/admin/auctions/{id}`、`POST /api/admin/auctions/{id}/cancel`。
- 未知 `/api/...` 已统一返回 JSON `404`。
- 公开拍卖、竞价、订单、支付等完整业务页面仍待继续接入。

建议先读:
- docs/schedule.md
- docs/接口联调记录.md
- docs/前端演示模块说明.md
- docs/认证与权限模块说明.md
- docs/物品与审核模块说明.md
- docs/拍卖管理模块说明.md
- docs/竞价与实时通知模块说明.md
- docs/订单与支付模块说明.md
- src/access/http/
- src/modules/
- tests/

完成 S21 时必须更新:
- docs/schedule.md
- docs/接口联调记录.md
- docs/前端演示模块说明.md
- docs/测试计划与用例说明.md
```
