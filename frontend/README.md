# 在线拍卖平台 - Next.js 前端

本目录是 `/home/ljh/project/soft_course_design` 的前端工程，基于 Next.js 16 App Router 构建。

## 技术栈

- Next.js 16 (App Router)
- React 19
- TypeScript
- Tailwind CSS 3.4+
- Framer Motion
- @tanstack/react-query
- lucide-react

运行环境要求：Node.js >= 20.9.0。

## 快速开始

```bash
# 安装依赖
npm install

# 类型检查
npm run typecheck

# 构建
npm run build

# 本地开发
npm run dev
```

开发服务器默认运行在 `http://localhost:3000`。

## API 模式

通过环境变量 `NEXT_PUBLIC_API_MODE` 控制后端接入方式：

| 模式 | 值 | 说明 |
|---|---|---|
| Mock | `mock`（默认） | 所有接口调用 `lib/api/mock-data.ts` 中的 typed mock 数据，不依赖后端 |
| Live | `live` | 调用真实后端接口；后端未就绪时显示 "API route not ready" |

### 环境变量

在 `frontend/` 下创建 `.env.local`：

```env
# mock 或 live
NEXT_PUBLIC_API_MODE=mock

# live 模式下的后端地址
NEXT_PUBLIC_API_BASE_URL=http://127.0.0.1:18080
NEXT_PUBLIC_WS_BASE_URL=ws://127.0.0.1:18080
```

## 页面路由

| 页面 | 路由 | 说明 |
|---|---|---|
| 首页 | `/` | 公共门户、搜索、分类、拍品网格 |
| 登录 | `/auth/login` | 登录/注册一体化页面 |
| 拍卖大厅 | `/auction/hall` | 登录后主阵地、筛选、实时拍品矩阵 |
| 竞价详情 | `/auction/detail/[id]` | 核心实时竞价页、乐观出价和回滚 |
| 拍品发布 | `/auction/publish` | 卖家分步发布向导 |
| 订单支付 | `/checkout/[orderId]` | 支付方式选择、交易确认 |
| 管理大盘 | `/admin/dashboard` | 管理员审核、KPI、运维终端 |

## 目录结构

```
frontend/
  app/               # Next.js App Router 页面
  components/        # 可复用组件 (layout/ ui/ auction/)
  lib/api/           # API client 和 mock 数据
  types/             # TypeScript 类型定义
  tailwind.config.ts # Tailwind 设计系统配置
```

## Mock 模式说明

Mock 模式下部分交互有模拟行为：

- 出价金额能被 13 整除时，模拟 `409 Conflict`（价格被抢先）
- 出价金额能被 17 整除时，模拟 `429 Too Many Requests`（限流）
- 其他金额正常返回成功

## 构建与部署

```bash
# 生产构建
npm run build

# 生产运行
npm run start
```

`npm run build` 当前显式使用 `next build --webpack`。Next.js 16 默认 Turbopack 在当前 WSL2/Codex 沙箱中会因进程/端口限制失败，webpack 构建路径已完成验证。

当前阶段仅用于本地开发和演示验证，未配置生产部署。
