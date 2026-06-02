# 在线拍卖平台

本项目是一个面向校园或小型社区场景的在线拍卖平台，覆盖卖家发布、管理员审核、拍卖创建、实时竞价、订单支付、履约评价、统计报表和运维异常处理等完整业务闭环。

系统后端采用 `C++20 + Drogon`，MySQL 作为拍卖、出价、订单和支付等交易事实的唯一来源。前端采用 `Next.js 16 + React 19`，通过真实 `/api/...` 与 `/ws/auction/{id}` 接口接入后端。

## 功能概览

- 用户与权限：注册、登录、登出、会话恢复、RBAC、用户状态管理
- 物品与审核：草稿拍品、图片元数据、提交审核、管理员审核
- 拍卖管理：创建、修改、取消拍卖，按状态机推进拍卖状态
- 实时竞价：公开拍卖大厅、详情、出价历史、WebSocket 价格推送、延时保护
- 订单与支付：订单详情、测试支付通道、支付回调幂等、发货、确认收货
- 评价与信用：订单互评、评价查询、信用摘要
- 统计与运维：日报统计、CSV 导出、操作日志、任务日志、异常标记、补偿入口

## 技术栈

- 后端：C++20、Drogon、MySQL 8、OpenSSL
- 前端：Next.js 16、React 19、TypeScript、Tailwind CSS、React Query、Framer Motion
- 测试：CTest、Shell 回归脚本、前端类型检查与生产构建

## 目录结构

```text
.
├── config/              # 应用和 Nginx 配置模板
├── docs/                # 需求、设计、测试、发布和进度文档
├── frontend/            # Next.js 前端工程
├── scripts/             # 构建、测试、数据库和发布脚本
├── sql/                 # schema 与 seed 数据
├── src/                 # C++ 后端源码
├── tests/               # 单元、模块、集成、风险与安全测试
├── CMakeLists.txt       # CMake 工程入口
└── README.md
```

## 环境要求

- WSL2 Ubuntu 24.04
- CMake 3.28+
- 支持 C++20 的编译器
- MySQL 8 客户端与 `mysqld`
- Node.js 20.9+
- `ctest`、`curl`、`openssl`

当前已知环境限制：

- 若系统缺少 `crypt` 库，`cmake -S . -B build` 会失败，需要在具备该系统库的环境完成后端构建。

## 快速开始

### 1. 构建后端

```bash
./scripts/bootstrap.sh
```

或手动执行：

```bash
cmake -S . -B build
cmake --build build
```

### 2. 运行前端校验

```bash
cd frontend
npm install
npm run release-check
```

### 3. 启动后端服务

```bash
./build/bin/auction_app
```

默认健康检查：

```bash
curl http://127.0.0.1:18080/healthz
```

### 4. 启动前端开发环境

```bash
cd frontend
npm run dev
```

默认前端地址：

```text
http://127.0.0.1:3000
```

## 核心接口

- HTTP：`/api/...`
- WebSocket：`/ws/auction/{id}`

前端环境变量示例见 [frontend/.env.example](/home/ljh/project/soft_course_design/frontend/.env.example)。

## 测试与发布验证

常用命令：

```bash
./build/bin/auction_app --check-config
./build/bin/auction_app --check-db
scripts/test.sh frontend
scripts/test.sh http
scripts/test.sh risk
scripts/test.sh security
ctest --test-dir build --output-on-failure
```

发布前一键验证：

```bash
scripts/deploy/verify_release.sh
```

该脚本会执行前端类型检查与生产构建，启动真实后端服务并校验 `/healthz` 与真实业务接口，然后串联 `frontend`、`http`、`risk` 与全量 `ctest`。

## 支付说明

当前支付链路使用内部测试支付通道，不连接真实第三方支付网关；但支付发起、回调验签、金额校验、幂等处理和审计日志均按正式业务链路实现。
