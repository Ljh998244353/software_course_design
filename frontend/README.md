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

## 环境变量

在 `frontend/` 下创建 `.env.local`：

```env
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
  lib/api/           # API client
  types/             # TypeScript 类型定义
  tailwind.config.ts # Tailwind 设计系统配置
```

## 构建与部署

```bash
# 生产构建
npm run build

# 生产运行
npm run start
```

`npm run build` 当前显式使用 `next build --webpack`。Next.js 16 默认 Turbopack 在当前 WSL2/Codex 沙箱中会因进程/端口限制失败，webpack 构建路径已完成验证。

当前默认连接真实后端接口，详情页优先使用真实 WebSocket，连接失败时自动降级到轮询。
