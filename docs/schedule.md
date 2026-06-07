# 在线拍卖平台 Agent 执行 Schedule

## 新会话最小恢复卡片

- 当前主 Step: `Release-Final-System-Fix`
- 当前唯一活动任务: 按 [release-final-system-plan.md](/home/ljh/project/soft_course_design/docs/release-final-system-plan.md) 执行最终上线完整系统修复
- 当前执行阶段: `R3 / R7`
- 当前状态: `进行中`
- 最近一次已通过验证:
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
- 当前阻塞/风险:
  - `start.sh` 与 `scripts/deploy/verify_release.sh` 的本地 MySQL fallback 代码已修复，但当前 Codex 沙箱禁止 `mysqld` 监听 Unix socket/TCP，无法在沙箱内完成最终成功启动验证
  - `schedule.md` 仍需持续保持短恢复入口，不再回涨为长历史流水
- 下一步唯一动作:
  - 继续收口文档口径，确保 `demo` 只作为历史说明出现
  - 在本机正常 shell 中执行 `./start.sh --config config/app.local.json`
  - 在本机正常 shell 中执行 `scripts/deploy/verify_release.sh`
- 优先阅读文件:
  - [schedule.md](/home/ljh/project/soft_course_design/docs/schedule.md)
  - [release-final-system-plan.md](/home/ljh/project/soft_course_design/docs/release-final-system-plan.md)
  - [frontend-next-schedule.md](/home/ljh/project/soft_course_design/docs/frontend-next-schedule.md)
  - [frontend/API_READINESS.md](/home/ljh/project/soft_course_design/frontend/API_READINESS.md)
  - [start.sh](/home/ljh/project/soft_course_design/start.sh)

## 当前真实状态

- `S00-S15`：已完成
- `F16`：已完成
- `F17`：真实 HTTP 与真实 WebSocket 已接入完成，当前只做最终产品化收口
- 当前唯一发布口径：
  - 前端唯一主入口为 `frontend/`
  - 后端唯一主入口为真实 HTTP/WS 接口
  - 支付仅保留本地测试支付模型，用于完整链路演示
- 当前主修复方向：
  - `R2` 启动链路稳定化：代码修复已完成，等待非沙箱最终实跑验证
  - `R3` 单一真实入口收敛：持续进行中，重点是清理文档历史口径
  - `R6` 发布门禁收敛：代码修复已完成，等待非沙箱最终实跑验证
  - `R7` 文档与 handoff 收口：进行中

## 当前 Handoff

### 1. 基本信息

- Step ID: `Release-Final-System-Fix`
- 模块名称: 最终上线完整系统修复
- 当前状态: `进行中`
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

### 4. 当前验证结果

- 已通过：
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
- 已确认但受环境限制未完成：
  - `./start.sh --config config/app.local.json --backend-only --no-build`
  - 结果：已能进入 fallback，并明确报出当前沙箱禁止 `mysqld` 监听 socket/TCP
- 待在本机正常 shell 中完成：
  - `scripts/deploy/verify_release.sh`

### 5. 当前风险/阻塞

- 当前 Codex 沙箱禁止本地 `mysqld` 建立 Unix socket/TCP 监听，因此无法在沙箱内完成最终启动成功验证
- `schedule.md` 作为统一进度真源，后续只能继续保持“短恢复入口 + 当前状态摘要”结构，避免再次膨胀
- 仓库中剩余 `demo` 命中大多属于历史交付说明；继续清理时要保留“历史边界”语义，不能误改代码现状
- `start.sh` 已在本机正常 shell 中成功完成自动拉起本地 MySQL、启动后端和启动前端；当前剩余真实验收阻塞已缩小为 `scripts/deploy/verify_release.sh` 一条链路
- 当前剩余命中基本属于明确标注的历史边界说明，例如旧 `/demo`、`sql/demo_data.sql` 和 `/app` 兼容路由历史；这部分可以保留，但不能再写成当前主链路

### 6. 下一步

- 下一步 Step ID: `R3 / R7`
- 下一步目标:
  - 在本机正常 shell 中重新执行 `scripts/deploy/verify_release.sh`，确认发布门禁全链路通过
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
- `2026-06-07`：完成 `start.sh`、本地 MySQL fallback、`verify_release.sh`、前端测试脚本名收口等最终修复

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
当前状态：S00-S15 已完成；F16 已完成；F17 真实 HTTP/WS 已接入完成；当前正在做 R2/R3/R6/R7 最终收口

当前必须遵守：
- 以代码现状为准
- schedule.md 只保留短恢复入口，不再扩写成长历史流水
- 单一真实业务入口：frontend + 真实 HTTP/WS
- 支付保留，但只做本地测试支付模型
- 每次改变实现状态、验证状态、handoff、下一步，都同步更新 docs/schedule.md
- 始终用中文
```
