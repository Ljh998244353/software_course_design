# 最终上线完整系统大型修复计划

## 1. 目标

本计划用于把当前仓库收敛为最终本地可运行、可验收、可恢复上下文的完整系统版本，核心目标如下：

- 只保留真实业务前端作为唯一主入口
- 支付功能保留，但只做本地 mock 成功闭环，不接第三方平台
- `start.sh` 成为默认一键启动入口，并能自动处理本地配置与数据库 fallback
- `scripts/deploy/verify_release.sh` 成为最终验收入口
- 所有修复步骤都可被后续 agent 直接逐条执行

## 2. 最终交付定义

满足以下条件时，视为“最终上线完整系统”完成：

- 前端 `typecheck` 与 `build` 通过
- 真实 HTTP 合同回归通过
- `risk` 回归通过
- 全量 `ctest` 通过
- `start.sh` 在本地能完成一键启动
- 独立 `demo` 主入口已退出当前主链路
- 文档、脚本、恢复卡片与代码现状一致

## 3. 非目标与边界

- 不接入真实第三方支付平台
- 不做 200 并发性能上线验证
- 不做浏览器驱动级截图或压测系统建设
- 不引入新的架构形态，例如 MQ、微服务、分布式锁

## 4. 分阶段执行清单

### R1 当前计划与恢复入口固化

目标：
- 把本计划留存在仓库
- 让后续 agent 通过 `docs/schedule.md` 能恢复到本计划

执行步骤：
- `R1.1` 新增 `docs/release-final-system-plan.md`
- `R1.2` 更新 `docs/schedule.md` 的恢复卡片、优先阅读文件、handoff 指向本计划
- `R1.3` 约定后续每完成一个阶段，都同步更新本计划和 `schedule.md`

完成判定：
- 新会话先读 `schedule.md` 时能定位到本计划
- 本计划足够细，可直接按步骤执行

### R2 启动链路稳定化

目标：
- `start.sh` 真正成为一键启动入口

执行步骤：
- `R2.1` 默认优先使用 `config/app.local.json`
- `R2.2` 配置缺失时自动生成模板并退出
- `R2.3` 启动前执行 `--check-config`
- `R2.4` 启动前执行 `--check-db`
- `R2.5` 若数据库检查失败，自动调用仓库用户态本地 MySQL 初始化脚本
- `R2.6` 生成临时运行配置并继续启动
- `R2.7` 自动写入前端 `.env.local`
- `R2.8` 完成 PID、socket、日志、退出清理治理

完成判定：
- 新环境首次运行不会直接报错中断在“配置不存在”
- 未手工准备 MySQL 时，脚本可 fallback 到仓库本地测试库
- 登录不再因为模板配置导致 500

当前状态：
- 已完成代码修复：
  - `start.sh` 现在先做 `--check-config` / `--check-db`，失败时自动尝试 `scripts/db/setup_local_mysql.sh`
  - fallback 运行配置现在正确写入应用账号 `auction_user/change_me`，不再误用 root 管理账号
  - fallback 失败时会输出最近 MySQL 错误日志路径，并明确识别“当前环境禁止 mysqld 监听 socket/TCP”的限制
- 当前剩余事项：
  - 需要在本机正常 shell 中实际执行 `./start.sh --config config/app.local.json`，确认在非沙箱环境可真正完成自动拉库后启动

### R3 单一真实入口收敛

目标：
- 退出独立 `demo` 主入口

执行步骤：
- `R3.1` 确认启动路径和 HTTP 注册中不再把 `/demo` 作为当前主入口
- `R3.2` 清理发布脚本和测试脚本里仍把 `/demo` 当作主门禁的检查
- `R3.3` 清理只服务 `demo` 的主流程测试入口
- `R3.4` 更新文档，把 `demo` 改成历史口径而非当前口径

完成判定：
- 当前主脚本、主测试、主文档只承认真实业务入口

### R4 支付收敛为本地 mock 成功

目标：
- 保留支付功能，但完全本地可演示

执行步骤：
- `R4.1` 收敛支付页与支付按钮文案
- `R4.2` 支付发起统一映射为本地成功链路
- `R4.3` 支付成功后保证订单、履约、评价、统计都闭环
- `R4.4` 若保留支付回调，仅作为内部 mock 成功实现

完成判定：
- 点击支付后系统能稳定进入 `PAID`
- 不再依赖真实第三方支付

### R5 真实主链路排雷

目标：
- 逐模块完成真实业务路径稳定化

执行步骤：
- `R5.1` 认证与会话
- `R5.2` 拍品与审核
- `R5.3` 拍卖与大厅
- `R5.4` 竞价与 WS 降级
- `R5.5` 订单、支付 mock、履约
- `R5.6` 评价与信用
- `R5.7` 统计与运维

完成判定：
- 所有主流程在真实 API 下可闭环

### R6 测试与发布门禁收敛

目标：
- 把最终门禁统一到真实系统

执行步骤：
- `R6.1` 收敛 `scripts/test.sh` 子入口
- `R6.2` 收敛 `scripts/deploy/verify_release.sh`
- `R6.3` 删除依赖旧 `demo` 主路径的验收逻辑
- `R6.4` 固定最终验证命令矩阵

完成判定：
- 以下命令全部通过：
  - `cd frontend && npm run typecheck`
  - `cd frontend && npm run build`
  - `scripts/test.sh http`
  - `scripts/test.sh risk`
  - `ctest --test-dir build --output-on-failure`
  - `scripts/deploy/verify_release.sh`

当前状态：
- 已完成代码修复：
  - `scripts/deploy/verify_release.sh` 现已改为自动初始化本地测试 MySQL，并通过 `scripts/db/write_test_config.sh` 生成真实测试配置
  - 发布门禁不再直接使用 `config/app.example.json` 作为后端运行配置
- 当前剩余事项：
  - 受当前 Codex 沙箱禁止 mysqld 监听影响，只完成了 `bash -n` 级静态验证
  - 需要在本机正常 shell 中实际执行 `scripts/deploy/verify_release.sh` 完成最终验收

### R7 文档与 handoff 收口

目标：
- 确保计划、代码、文档、恢复文本一致

执行步骤：
- `R7.1` 更新 `docs/schedule.md`
- `R7.2` 更新 `docs/frontend-next-schedule.md`
- `R7.3` 更新部署、联调、测试报告与 readiness 文档
- `R7.4` 清理“当前仍保留 demo 主入口”的描述

完成判定：
- 新会话恢复文本与仓库真实状态一致

## 5. 测试与验收矩阵

- 启动：
  - 无本地配置时自动生成模板
  - 有本地配置但数据库不可达时自动 fallback
  - 一键启动后前后端都可访问

- 真实业务：
  - 登录
  - 审核
  - 建拍
  - 出价
  - 支付 mock 成功
  - 发货
  - 收货
  - 评价
  - 统计

- 故障：
  - 数据库不可用
  - 会话失效
  - WS 不可用
  - 重复支付
  - 残留 mysqld 锁文件

## 6. 文档同步规则

- 每完成一个阶段，必须同时更新：
  - `docs/release-final-system-plan.md`
  - `docs/schedule.md`
- 若是前端或 F17 相关收尾，还要同步：
  - `docs/frontend-next-schedule.md`
  - `frontend/API_READINESS.md`

## 7. 当前执行位置模板

每轮完成后必须更新以下字段：

- 当前阶段：`R?.?`
- 当前状态：`进行中 / 已完成 / 阻塞`
- 最近通过的验证命令：
- 下一步唯一动作：
- 当前风险：

## 8. 当前执行位置

- 当前阶段：`R3 / R7`
- 当前状态：`进行中`
- 最近通过的验证命令：
  - `bash -n start.sh`
  - `bash -n scripts/db/setup_local_mysql.sh`
  - `bash -n scripts/deploy/verify_release.sh`
  - `bash -n scripts/test.sh`
  - `bash -n scripts/test_frontend.sh`
  - `scripts/test.sh http`
  - `scripts/test.sh risk`
  - `ctest --test-dir build --output-on-failure`
  - `cd frontend && npm run typecheck`
  - `cd frontend && npm run build`
- 下一步唯一动作：
  - 继续清理残余文档中的旧 `demo` 当前态口径
  - 在本机正常 shell 中执行 `./start.sh --config config/app.local.json`
  - 随后执行 `scripts/deploy/verify_release.sh`
- 当前风险：
  - 当前 Codex 沙箱禁止 `mysqld` 绑定 Unix socket 和 TCP 端口，因此无法在沙箱内完成 MySQL fallback 成功启动验证
  - 旧 `demo` 口径已从主测试入口、主发布文档和恢复入口中退出；当前剩余命中大多属于明确标注的历史交付说明
