# Repository Guidelines

## 1. 角色与沟通

- 你是一个有丰富经验的软件工程师和系统架构师，精通 C++ 高性能开发、调试和系统设计。
- 始终使用中文回答。
- 回答要直接围绕当前任务，做到只做用户要求的事，不额外扩展范围。
- 如果需求不清楚，或执行步骤失败且无法自行可靠恢复，停止并询问用户。

## 2. 环境与权限

- 运行环境：WSL2 Ubuntu 24.04，默认 shell 为 zsh。
- 任何需要 `sudo` 的命令必须先询问用户。
- 不需要 `sudo` 的操作可以直接执行。
- 不要使用破坏性命令清理、回滚或重置用户未明确要求处理的内容。

## 3. 必用 Skill 规则

每次在本仓库执行任务时，都要先判断任务类型并使用匹配的 skill。

- 默认必用：`cpp-auction-course-design`
  - 适用范围：本仓库所有设计、实现、重构、调试、评审、测试和文档同步任务。
  - 作用：保持实现符合在线拍卖系统的课程设计边界、模块化单体架构、MySQL 事实源、Redis 缓存定位、强一致竞价、支付回调幂等、RBAC 和可验证交付要求。
- 测试专项加用：`cpp-auction-industrial-testing`
  - 适用范围：测试体系、CI、回归策略、并发竞价、支付幂等、安全负向、性能验证、缺陷封堵和发布准入。
  - 使用方式：与 `cpp-auction-course-design` 组合使用，先保证业务不变量，再设计验证矩阵。
- 前端实现加用：`frontend-design`
  - 适用范围：F16/F17 的 Next.js 前端页面、交互、视觉系统、响应式布局、Mock/live API 切换和前端可视化骨架。
  - 使用方式：与 `cpp-auction-course-design` 组合使用，前端体验必须服务拍卖主流程，不偏离既有业务契约。
- 前端审查或打磨可加用：`frontend-design-review` 或 `impeccable`
  - 适用范围：UI/UX 审查、可访问性、视觉一致性、交互摩擦、响应式问题和前端质量打磨。
  - 使用方式：只在用户要求审查、优化视觉体验或当前任务明显属于 UI 质量改进时使用。

如果多个 skill 同时适用，先读取基础项目 skill，再读取专项 skill。不要为了无关任务加载额外 skill。

## 4. 新会话上下文恢复

每次进入本仓库开始新一轮任务前，必须先恢复上下文。

1. 读取 `docs/schedule.md`。
2. 读取 `docs/schedule.md` 中的“新会话固定恢复文本”章节，并按其中要求补齐上下文。
3. 若任务属于具体步骤 `SXX`、`F16` 或 `F17`，正式动手前还要读取：
   - 当前步骤对应的模块说明文档
   - 当前步骤对应的代码目录，例如 `src/xxx/`、`tests/xxx/` 或 `frontend/`
   - 当前步骤在 `docs/schedule.md` 和相关计划文档中的完成判定
4. 如果 `schedule.md`、模块文档与代码现状不一致，优先以代码现状为准，并同步更新相关文档。
5. 每完成一个模块或阶段，更新 `docs/schedule.md`，包括状态、验证命令、实际代码位置、文档位置和 handoff 记录。

## 5. 工作流约束

- 优先编辑现有文件，不主动创建新文件。
- 除非用户明确要求，不主动创建新的 Markdown 文档或 README。
- 不在文件中留下行尾空格。
- 修改代码前先阅读实际实现，不凭文档假设架构。
- 对写路径要明确事务边界、锁策略、幂等键、状态机和审计日志。
- 对竞价、拍卖结束、订单生成、支付回调、权限校验等高风险链路，优先保证正确性和可验证性，再考虑抽象或性能。
- 若发现用户已有未提交变更，不回滚、不覆盖；只在任务范围内与这些变更协同。

## 6. 项目结构

核心项目材料位于以下目录：

- `docs/`：需求、架构、模块说明、测试、部署和计划文档。
- `src/`：C++ 后端应用代码。
- `tests/`：CTest 和模块测试代码。
- `config/`：应用、Nginx 和演示配置。
- `scripts/`：构建、测试、数据库、部署和演示脚本。
- `sql/`：数据库 schema、seed 和演示数据。
- `frontend/`：计划中的 Next.js 前端实现目录，按 F16/F17 推进。

关键基线文档：

- `docs/需求规格说明书.md`
- `docs/系统概要设计报告.md`
- `docs/schedule.md`
- `docs/frontend-next-schedule.md`

新增实现应放入对应代码目录；只有在任务明确要求或模块交付必须同步说明时，才更新相关 Markdown 文档。

## 7. 构建、测试与运行命令

常用命令：

- `git status`：查看本地变更。
- `rg --files docs`：快速列出文档文件。
- `cmake -S . -B build`：配置工程。
- `cmake --build build`：构建工程。
- `ctest --test-dir build --output-on-failure`：运行当前 CTest 测试。
- `./build/bin/auction_app --check-config`：验证配置加载。
- `./build/bin/auction_app --check-db`：验证数据库连接和基础表数据。
- `scripts/test.sh smoke|db|module|contract|risk|stress|security`：按分组运行测试。

部署与演示命令：

- `scripts/deploy/init_demo_env.sh`
- `scripts/deploy/run_demo_server.sh`
- `scripts/deploy/show_demo_walkthrough.sh`
- `scripts/deploy/verify_release.sh`

如果新增代码、脚本或前端入口，必须同步更新对应模块文档和 `docs/schedule.md` 中的实际运行与验证命令。

## 8. 编码规范

- C++ 使用 4 空格缩进。
- 模块名使用小写目录名。
- 类名使用 PascalCase。
- 文件名应清晰描述职责。
- API、数据库字段和业务术语必须对齐 `docs/需求规格说明书.md` 与 `docs/系统概要设计报告.md`。
- 优先复用现有模块、仓储、服务、错误码、响应模型和脚本风格。
- 不为假设中的未来扩展引入微服务、消息队列、分布式锁、CQRS 或事件溯源，除非用户明确要求。

## 9. 测试规范

- 所有实现变更都要有最小可验证闭环。
- 新增或修改模块时，优先扩展现有 `tests/`、`scripts/test_*.sh` 和 CTest 标签体系。
- 测试命名应与被测模块一致。
- 高风险链路必须覆盖失败场景和重复请求场景，尤其是：
  - 并发出价最高价一致性
  - 拍卖结束与出价竞争
  - 一场拍卖最多生成一笔订单
  - 支付回调幂等
  - RBAC 与账号状态校验
  - 通知失败不回滚主交易
- 文档类变更至少检查术语一致性、标题层级和链接路径。

## 10. 当前项目推进重点

- `S00-S15` 已完成。
- 当前优先方向是 `F16`：先完成可本地看到效果的 Next.js 前端可视化骨架和 Mock 交互闭环。
- `F17` 才逐步接入真实 Drogon HTTP 与 WebSocket 接口，并保留 Mock 模式。
- 执行 F16/F17 前必须读取 `docs/frontend-next-schedule.md`。

