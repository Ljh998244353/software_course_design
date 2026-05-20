# 在线拍卖平台

本项目是一个课程设计用在线拍卖平台，面向校园或小型社区场景，支持卖家发布拍品、管理员审核、买家竞价、系统结算、订单支付、履约评价、统计报表和运维异常处理等完整业务闭环。

系统后端采用 `C++20` 实现，使用 Drogon 提供 HTTP 接入，MySQL 作为拍卖、出价、订单、支付等交易事实的唯一来源。前端为静态 HTML/CSS/JavaScript，由后端统一托管，包含只读答辩演示台 `/demo` 和真实业务工作台 `/app`。

## 功能概览

- 用户与权限：注册、登录、登出、当前用户、角色权限、用户状态管理。
- 物品与审核：卖家拍品草稿、图片元数据、提交审核、管理员审核和审核日志。
- 拍卖管理：管理员创建、修改、取消拍卖，系统按状态机推进拍卖状态。
- 竞价与通知：买家浏览拍卖、查看当前价、提交出价、查询出价历史和站内通知。
- 订单与支付：拍卖成交后生成订单，支持 mock 支付、支付回调幂等、发货、确认收货。
- 评价与信用：买卖双方互评、评价查询、用户信用汇总。
- 统计与运维：统计日报、CSV 导出、操作日志、任务日志、异常看板、通知重试和补偿入口。
- 自动化验证：覆盖 CTest、HTTP 回归、UI 合同、高风险专项和发布验证脚本。

## 技术栈

- 语言：C++20
- 构建：CMake
- HTTP 框架：Drogon
- 数据库：MySQL 8
- 前端：静态 HTML/CSS/JavaScript
- 测试：CTest、自定义 Shell 回归脚本
- 可选能力：Redis、Nginx

## 目录结构

```text
.
├── assets/              # /demo 与 /app 前端静态资源
├── config/              # 应用和 Nginx 配置模板
├── docs/                # 需求、设计、测试、部署和答辩文档
├── scripts/             # 构建、测试、部署和演示脚本
├── sql/                 # schema、seed 和演示数据
├── src/                 # C++ 后端源码
├── tests/               # 单元、模块、集成、HTTP、UI 和风险测试
├── CMakeLists.txt       # CMake 工程入口
└── README.md            # 项目说明
```

## 环境要求

推荐使用：

- WSL2 Ubuntu 24.04
- zsh 或兼容 Bash 的 Shell
- CMake
- GCC 或 Clang，支持 C++20
- MySQL 8 客户端与 `mysqld`
- `ctest`、`curl`、`openssl`
- 可选：Drogon、Nginx、Redis

演示初始化脚本使用用户态本地 MySQL 数据目录，不需要 `sudo`。

## 快速开始

### 1. 进入项目目录

```bash
cd /home/ljh/project/soft_course_design
```

### 2. 构建工程

```bash
./scripts/bootstrap.sh
```

该命令会创建 `build/`、`logs/`、`data/uploads/`，并执行 CMake 配置与构建。

也可以手动执行：

```bash
cmake -S . -B build
cmake --build build
```

### 3. 初始化演示环境

```bash
./scripts/deploy/init_demo_env.sh
```

该脚本会：

- 构建当前工程。
- 初始化本地用户态 MySQL。
- 创建并导入演示库 `auction_demo`。
- 导入 `sql/schema.sql`、`sql/seed.sql` 和 `sql/demo_data.sql`。
- 生成演示配置 `build/demo_config/app.demo.json`。
- 执行配置检查和数据库检查。

可选环境变量：

```bash
AUCTION_DEMO_DB_NAME=auction_demo
AUCTION_DEMO_DB_USER=auction_user
AUCTION_DEMO_DB_PASSWORD=change_me
AUCTION_DEMO_SERVER_HOST=127.0.0.1
AUCTION_DEMO_SERVER_PORT=18080
```

### 4. 启动演示服务

```bash
./scripts/deploy/run_demo_server.sh
```

服务默认监听：

```text
http://127.0.0.1:18080
```

健康检查：

```bash
curl http://127.0.0.1:18080/healthz
```

预期响应包含 `status=ok`。

## 演示入口

启动服务后，在浏览器打开：

```text
http://127.0.0.1:18080/demo
http://127.0.0.1:18080/app
```

### `/demo`：只读答辩演示台

`/demo` 用于答辩时快速展示系统总览。页面从 `/api/demo/dashboard` 读取 MySQL 演示库中的聚合数据，展示：

- 演示账号和角色。
- 拍品、拍卖、出价、订单和成交金额摘要。
- 已成交、竞拍中、已流拍、待开始等拍卖状态。
- 订单支付、评价、通知、统计和运维数据证据。
- 并发出价、支付幂等、缓存降级、通知失败等高风险链路说明。

可直接验证演示数据接口：

```bash
curl http://127.0.0.1:18080/api/demo/dashboard
```

### `/app`：真实业务工作台

`/app` 是真实业务前端，不使用 `/api/demo/dashboard` 作为业务数据源，而是通过真实 `/api/...` 接口访问认证、拍品、审核、拍卖、出价、订单、支付、评价、通知、统计和运维模块。

演示账号如下：

| 用户名 | 密码 | 角色/用途 |
|---|---|---|
| `admin` | `Admin@123` | 管理员：审核、拍卖配置、统计报表、运维异常处理 |
| `support` | `Support@123` | 运维/客服：日志、通知、异常辅助处理 |
| `seller_demo` | `Seller@123` | 卖家：拍品发布、提交审核、履约、评价 |
| `buyer_demo` | `Buyer@123` | 买家：浏览拍卖、出价、支付、确认收货、评价 |
| `buyer_demo_2` | `BuyerB@123` | 竞争买家：竞争出价和被超越提醒展示 |

## 推荐答辩演示顺序

1. 执行 `./scripts/deploy/init_demo_env.sh` 初始化演示库。
2. 执行 `./scripts/deploy/run_demo_server.sh` 启动服务。
3. 访问 `http://127.0.0.1:18080/healthz` 展示服务健康状态。
4. 打开 `http://127.0.0.1:18080/demo`，点击“播放演示顺序”，讲解系统总览和业务闭环。
5. 打开 `http://127.0.0.1:18080/app`，使用不同演示账号登录，分别展示管理员、卖家、买家、运维/客服入口。
6. 展示测试证据和发布验证结果。

也可以查看文字版演示提纲：

```bash
./scripts/deploy/show_demo_walkthrough.sh
```

## 测试与验证

常用验证命令：

```bash
./build/bin/auction_app --check-config
ctest --test-dir build --output-on-failure
scripts/test.sh frontend
scripts/test.sh ui
scripts/test.sh http
scripts/test.sh risk
```

发布前一键验证：

```bash
scripts/deploy/verify_release.sh
```

该脚本会初始化演示环境，检查 `/healthz`、`/demo`、`/app`、两套静态资源和 `/api/demo/dashboard`，再执行 `frontend`、`ui`、`http`、`risk` 与全量 `ctest`。

## 常见问题

### 浏览器无法连接

确认服务已启动：

```bash
./scripts/deploy/run_demo_server.sh
```

再检查：

```bash
curl -i http://127.0.0.1:18080/healthz
```

### 页面打开但没有数据

重新初始化演示数据后重启服务：

```bash
./scripts/deploy/init_demo_env.sh
./scripts/deploy/run_demo_server.sh
```

### MySQL 数据目录被锁定

如果脚本提示 `build/test_mysql/data` 被占用，通常是之前残留的本项目用户态 `mysqld`。先检查：

```bash
pgrep -af '/home/ljh/project/soft_course_design/build/test_mysql'
```

只处理本项目残留进程，不要影响系统级 MySQL。

## 已知边界

- 当前支付为 mock 支付适配器与签名校验闭环，不连接真实第三方支付平台。
- Redis 是可替换旁路能力，不决定出价、结算或支付事实。
- `/demo` 是只读答辩演示台，真实业务操作请使用 `/app`。
- 当前课程设计交付以静态页面合同、真实 HTTP 回归、高风险专项和全量 CTest 作为验收证据。

## 更多文档

- `docs/需求规格说明书.md`
- `docs/系统概要设计报告.md`
- `docs/部署与答辩说明.md`
- `docs/答辩演示操作手册.md`
- `docs/前端演示模块说明.md`
- `docs/测试计划与用例说明.md`
