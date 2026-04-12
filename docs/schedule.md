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
- 已有环境、数据库、认证、物品、拍卖、竞价模块设计文档
- 已落地 `CMakeLists.txt`、`src/`、`tests/`、`config/`、`scripts/` 等工程骨架
- 已完成配置加载、日志初始化、统一响应模型、错误码基线和冒烟测试
- Drogon 当前未安装在本机环境中，因此 HTTP 服务入口采用“检测到 Drogon 时启用，否则 fallback 到 bootstrap 校验模式”的实现策略

因此，真实进度应认定为：

- `S00-S05`：已完成
- `S06-S07`：设计文档已完成，代码未落地，整体应视为 `进行中`
- `S08-S15`：未开始

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

虽然 `S06-S07` 的设计文档已经存在，但这些步骤目前仍缺少真实代码实现；当前工程骨架、数据库基线、认证权限模块和物品审核模块已经分别在 `S02`、`S03`、`S04`、`S05` 落地完成，后续可以直接从 `S06` 继续推进。

建议严格按以下顺序推进实际开发：

1. `S06 -> S07 -> S08`
2. `S09 -> S10 -> S11`
3. `S12 -> S13 -> S14 -> S15`

当前最优先的实际下一步是：

- `S06`：实现拍卖管理模块代码，并复用当前认证、物品审核和数据库基线

## 5. Step-by-Step Schedule

| 步骤 ID | 可分配给 agent 的实际目标 | 当前已有基础 | 需要实际完成的代码/脚本 | 完成判定 | 预计代码位置 | 完成后文档位置 | 当前状态 | 进度备注 |
|---|---|---|---|---|---|---|---|---|
| S00 | 冻结需求基线，确认角色、业务范围、性能与功能目标 | 已有需求文档 | 无新增代码要求 | 需求边界稳定，可作为后续开发输入 | 无 | [需求规格说明书.md](/home/ljh/project/soft_course_design/docs/需求规格说明书.md) | 已完成 | 这是需求基线步骤，不要求代码 |
| S01 | 冻结架构基线，确认模块边界、分层、状态机与关键链路 | 已有架构文档 | 无新增代码要求 | 架构边界稳定，可作为实现基线 | 无 | [系统概要设计报告.md](/home/ljh/project/soft_course_design/docs/系统概要设计报告.md) | 已完成 | 这是架构基线步骤，不要求代码 |
| S02 | 建立可编译、可启动的工程骨架 | 已有环境配置说明 | 创建 `CMakeLists.txt`、`src/main.cpp`、基础目录结构、配置加载、统一响应模型、错误码基线、日志初始化、最小启动入口 | 本地可完成配置和编译，服务可启动，至少完成一次工程冒烟验证 | `CMakeLists.txt`、`src/`、`config/`、`scripts/`、`tests/` | [环境配置说明.md](/home/ljh/project/soft_course_design/docs/环境配置说明.md) | 已完成 | 已落地工程骨架并通过 `cmake`、构建和 `ctest` 验证；本机未安装 Drogon，当前默认运行在 bootstrap fallback 模式 |
| S03 | 落地数据库初始化脚本与持久层基础能力 | 已有数据库设计说明 | 创建建表脚本、索引脚本、种子数据脚本、数据库连接配置、Repository 基础封装、数据库连通验证 | MySQL 可初始化核心表，应用可连库，至少有数据库冒烟验证 | `sql/schema.sql`、`sql/seed.sql`、`src/repository/`、`src/common/db/` | [数据库设计说明.md](/home/ljh/project/soft_course_design/docs/数据库设计说明.md) | 已完成 | 已落地 15 张核心表、基础种子数据、MySQL C API 封装、Repository 基类、`--check-db` 和数据库冒烟测试 |
| S04 | 实现认证与权限模块代码 | 已有认证与权限模块设计文档 | 实现用户注册登录、密码哈希、Token 鉴权、中间件、RBAC、账号状态校验、认证相关测试 | 用户可注册/登录，受保护接口可鉴权，最小权限测试通过 | `src/modules/auth/`、`src/middleware/`、`tests/auth/` | [认证与权限模块说明.md](/home/ljh/project/soft_course_design/docs/认证与权限模块说明.md) | 已完成 | 已落地 MySQL 用户仓储、`SHA-512 crypt` 密码哈希、HMAC Token、进程内会话存储、鉴权中间件、管理员状态管理和认证自动化测试 |
| S05 | 实现物品发布与审核模块代码 | 已有物品与审核模块设计文档 | 实现拍品 CRUD、图片元数据管理、提交审核、审核流转、审核日志、最小模块测试 | 卖家可提交拍品，管理员可审核，状态流转和日志落库可验证 | `src/modules/item/`、`src/modules/audit/`、`tests/item/` | [物品与审核模块说明.md](/home/ljh/project/soft_course_design/docs/物品与审核模块说明.md) | 已完成 | 已落地 `ItemService`、`ItemAuditService`、`ItemRepository` 和自动化测试，完成草稿、图片元数据、提交审核、驳回/通过和审核日志闭环 |
| S06 | 实现拍卖管理模块代码 | 已有拍卖管理模块设计文档 | 实现活动创建、修改、取消、查询、开始调度、结束调度、活动状态切换、最小模块测试 | 管理员可创建活动，系统可按时间切换状态，拍卖和拍品状态协同可验证 | `src/modules/auction/`、`src/jobs/`、`tests/auction/` | [拍卖管理模块说明.md](/home/ljh/project/soft_course_design/docs/拍卖管理模块说明.md) | 进行中 | 目前只有状态机、接口和调度规则设计，没有真实活动代码 |
| S07 | 实现竞价与实时通知模块代码 | 已有竞价与实时通知模块设计文档 | 实现出价接口、行级锁事务、幂等键、延时保护、竞价历史、Redis 热点缓存、WebSocket 推送、最小并发测试 | 并发出价下最高价一致，延时保护生效，通知失败不回滚事务 | `src/modules/bid/`、`src/modules/notification/`、`src/ws/`、`tests/bid/` | [竞价与实时通知模块说明.md](/home/ljh/project/soft_course_design/docs/竞价与实时通知模块说明.md) | 进行中 | 目前只有事务边界和接口设计，没有真实竞价代码，这是当前最高风险代码步骤 |
| S08 | 实现订单与支付结算模块代码 | 拍卖与竞价设计已具备输入条件 | 实现结束后生成订单、支付发起、支付回调、幂等处理、超时关闭、审计日志、最小模块测试 | 一场拍卖最多生成一笔订单，重复回调不重复记账 | `src/modules/order/`、`src/modules/payment/`、`tests/order/`、`tests/payment/` | `docs/订单与支付模块说明.md` | 未开始 | 需先完成 `S07` 的最高价和结束态交接 |
| S09 | 实现评价与反馈模块代码 | 订单完成态定义待落地 | 实现互评、评价查询、评价约束、信用汇总基础能力、最小模块测试 | 买卖双方可在订单完成后互评，评价与订单正确绑定 | `src/modules/review/`、`tests/review/` | `docs/评价与反馈模块说明.md` | 未开始 | 依赖 `S08` 的订单终态 |
| S10 | 实现统计分析与报表模块代码 | 数据模型和业务链路设计已具备基础 | 实现日报表聚合、成交统计、流拍统计、出价统计、导出接口、统计任务测试 | 管理端可查看主要统计指标，统计结果可复算 | `src/modules/statistics/`、`src/jobs/statistics/`、`tests/statistics/` | `docs/统计分析与报表模块说明.md` | 未开始 | 建议在主链路闭环后推进 |
| S11 | 实现系统监控与异常处理模块代码 | 已有顶层日志、任务、降级设计 | 实现操作日志、任务日志、通知失败重试、异常标记、补偿入口、降级处理、最小验证 | 关键异常可观测、可追踪、可重试 | `src/modules/ops/`、`src/jobs/retry/`、`tests/ops/` | `docs/系统监控与异常处理模块说明.md` | 未开始 | 重点覆盖 Redis 降级、通知失败和任务补偿 |
| S12 | 完成接口联调与主流程闭环 | 核心模块代码已具备后才能开始 | 联调认证、拍品、审核、活动、竞价、订单、支付、评价全链路，修正接口和状态切换问题 | 主流程可完整走通并可演示 | `src/`、`tests/integration/` | `docs/接口联调记录.md` | 未开始 | 必须在 `S04-S11` 有实际代码后执行 |
| S13 | 落地自动化测试基线 | 需要已有可运行工程 | 建立 CTest/GoogleTest、单元测试、集成测试、接口测试目录、基础测试脚本和执行方式 | 测试框架可运行，核心模块都有最小自动化覆盖 | `tests/`、`CMakeLists.txt`、`scripts/test/` | `docs/测试计划与用例说明.md` | 未开始 | 虽然测试应随模块推进，但这里单列为统一收口步骤 |
| S14 | 完成高风险专项测试 | 需要主链路代码和测试基线 | 执行并发出价、拍卖结束竞争、支付回调幂等、Redis 降级、通知失败、安全负向测试 | 高风险链路均有验证结果和问题闭环 | `tests/stress/`、`tests/integration/`、`tests/security/` | `docs/测试报告.md` | 未开始 | 核心关注竞价、拍卖结束、支付回调 |
| S15 | 完成部署、演示与答辩交付 | 需要联调与专项测试完成 | 补齐部署脚本、初始化流程、演示数据、演示账号、答辩脚本与最终交付说明 | 系统可在目标环境复现并完成完整演示 | `scripts/deploy/`、`config/` | `docs/部署与答辩说明.md` | 未开始 | 最终交付前需完成一次全量回归 |

## 6. 当前分配原则

为了方便直接分配给 agent 执行，后续每一步建议拆成“一个模块、一个明确目标、一个明确代码目录”的粒度。

按当前已确认进度，下一阶段应从 `S06` 开始拆分执行。

例如：

- `S06-S07` 每一步都应优先完成控制器、服务、仓储、测试四件套，再更新模块文档
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
- Step ID: S06
- 模块名称: 拍卖管理模块
- 当前状态: 进行中
- 对应文档: [拍卖管理模块说明.md](/home/ljh/project/soft_course_design/docs/拍卖管理模块说明.md)
- 对应代码目录: `src/modules/auction/` `tests/auction/`
- 已有可复用基础目录: `src/common/db/` `src/repository/` `src/modules/auth/` `src/modules/item/` `src/modules/audit/` `src/middleware/`

### 2. 本次实际完成
- 已完成功能:
  - `S00-S05` 已完成
  - 已落地 `src/modules/item/item_service.*`
  - 已落地 `src/modules/audit/item_audit_service.*`
  - 已落地 `src/repository/item_repository.*`
  - 已落地 `tests/item/item_flow_tests.cpp` 与 `scripts/test_item.sh`
  - 已完成草稿创建、图片元数据管理、提交审核、待审列表、管理员驳回/通过、审核日志和驳回后重提闭环验证
- 实际修改文件:
  - [item_service.cpp](/home/ljh/project/soft_course_design/src/modules/item/item_service.cpp)
  - [item_audit_service.cpp](/home/ljh/project/soft_course_design/src/modules/audit/item_audit_service.cpp)
  - [item_repository.cpp](/home/ljh/project/soft_course_design/src/repository/item_repository.cpp)
  - [item_flow_tests.cpp](/home/ljh/project/soft_course_design/tests/item/item_flow_tests.cpp)
  - [test_item.sh](/home/ljh/project/soft_course_design/scripts/test_item.sh)
  - [setup_local_mysql.sh](/home/ljh/project/soft_course_design/scripts/db/setup_local_mysql.sh)
  - [error_code.h](/home/ljh/project/soft_course_design/src/common/errors/error_code.h)
  - [CMakeLists.txt](/home/ljh/project/soft_course_design/CMakeLists.txt)
  - [物品与审核模块说明.md](/home/ljh/project/soft_course_design/docs/物品与审核模块说明.md)
  - [schedule.md](/home/ljh/project/soft_course_design/docs/schedule.md)
- 未完成功能:
  - 拍卖活动创建、修改、取消、查询
  - 开始/结束调度与活动状态切换
  - 拍卖管理模块测试
- 明确不在本步处理的内容:
  - 竞价、订单、支付等后续业务模块实现

### 3. 关键设计决定
- 决定 1: `S05` 继续复用 `AuthMiddleware`，在服务层直接消费 Bearer Token
- 原因: 当前本机未接入 Drogon HTTP 路由，但仍需先把登录态和 RBAC 真实跑通
- 影响范围:
  - `S06` 可继续沿用“service + middleware”的最小闭环模式
  - 后续 HTTP 接入只需做薄路由适配

- 决定 2: 图片能力当前仅落地 `image_url` 元数据管理，不实现真实文件上传
- 原因: 当前 schema 只持久化元数据，`S05` 优先保证拍品状态流转和审核闭环
- 影响范围:
  - 后续接入对象存储或本地上传时，应复用当前 `ItemService` 的校验和事务边界

- 决定 3: 被驳回拍品在卖家再次编辑或增删图片时自动回退到 `DRAFT`
- 原因: 对齐模块文档里的“修改后重提”业务流，避免 `REJECTED` 状态下出现不可再次提交的中间态
- 影响范围:
  - `S06` 只需消费 `READY_FOR_AUCTION` 拍品，不必额外兼容驳回态变更细节

- 决定 4: 本地测试 MySQL 的运行时 socket 改为 `mysql-runtime.sock`
- 原因: 旧固定 socket 在当前环境下可能残留 `.lock` 文件，导致 `mysqld` 无法重新绑定
- 影响范围:
  - `scripts/test_db.sh`、`scripts/test_auth.sh`、`scripts/test_item.sh` 都继续复用同一套测试库配置生成逻辑

### 4. 验证结果
- 执行命令:
  - `cmake -S . -B build`
  - `cmake --build build`
  - `scripts/test_item.sh`
  - `ctest --test-dir build --output-on-failure`
- 结果:
  - 配置成功
  - 构建成功
  - `auction_item_flow_tests passed`
  - 全部测试 5/5 通过
  - 数据库冒烟测试通过
  - 认证测试通过，已覆盖注册成功、重复账号、密码错误、Token 缺失、Token 过期、RBAC、冻结/禁用和登出失效
  - 物品与审核测试通过，已覆盖草稿创建、图片元数据管理、提交审核、驳回原因校验、驳回后回退 `DRAFT`、再次提交和审核通过
- 未执行的测试:
  - Drogon HTTP 路由真实启动测试
  - Redis 连通测试
- 原因:
  - 本机当前未安装 Drogon
  - 当前 `S05` 不依赖 Redis，图片文件本体上传也尚未接入

### 5. 当前风险/阻塞
- 风险 1: Drogon 目前不在本机环境里，HTTP 模式尚未实际验证
- 风险 2: 当前图片能力只管理元数据，不包含真实文件上传与清理
- 阻塞项:
  - 无硬阻塞，可直接继续 `S06`
- 需要注意的坑:
  - 不能再把“仅文档完成”误记成“模块已完成”
  - `S06` 的活动创建只能消费 `READY_FOR_AUCTION` 拍品
  - 后续写路径继续坚持预编译 SQL，不要回退到字符串拼接
  - 不要绕过当前 `ItemService` / `ItemAuditService` 已有状态约束直接写 `item` 表
  - 后续每一步都要同步写入代码路径、测试结果和下一步

### 6. 下一步
- 下一步 Step ID: S06
- 下一步目标: 实现活动创建、修改、取消、查询、开始/结束调度和活动状态切换
- 建议先读文件:
  - [schedule.md](/home/ljh/project/soft_course_design/docs/schedule.md)
  - [拍卖管理模块说明.md](/home/ljh/project/soft_course_design/docs/拍卖管理模块说明.md)
  - [物品与审核模块说明.md](/home/ljh/project/soft_course_design/docs/物品与审核模块说明.md)
  - [数据库设计说明.md](/home/ljh/project/soft_course_design/docs/数据库设计说明.md)
  - `src/modules/item/`
  - `src/modules/audit/`
  - `src/common/db/`
  - `src/repository/`
  - `src/modules/auth/`

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
2. docs/拍卖管理模块说明.md
3. 对应代码目录 src/common/db/、src/repository/、src/modules/item/、src/modules/audit/

当前正在做：S06
当前状态：进行中

本次需要你继续：
- 开始 `S06`
- 先实现活动创建、修改、取消、查询、开始/结束调度和最小模块测试
- 复用 `src/common/db/`、`src/repository/`、`src/modules/item/`、`src/modules/audit/` 和现有 `auction`、`item` 表
- 仅允许 `READY_FOR_AUCTION` 拍品创建拍卖活动
- 完成后更新 `docs/拍卖管理模块说明.md` 与 `docs/schedule.md`

注意约束：
- 以实际代码落地为准，不以仅写文档算完成
- 任何 sudo 和删除操作先问我
- 始终用中文
```

说明如下：

- 当当前步骤变化时，要把这里的模块文档和代码目录替换成新的真实目标
- 若当前步骤尚未创建代码目录，也必须先读取 `docs/schedule.md` 和对应模块文档，再决定下一步
