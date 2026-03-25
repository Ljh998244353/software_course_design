# 基于 C++ 高性能架构的在线拍卖系统

## 封面

**系统名称**：基于 C++ 高性能架构的在线拍卖系统

**设计人员**：学号 - 姓名

**指导教师**：李威 / 殷成凤

**版本**：V1.0

**拟稿日期**：2026-03-11

------

# 目录

1. 概述
2. 总体要求
3. 功能性需求
4. 非功能性需求
5. 主要业务流程与用例
6. 系统概要设计
7. 数据库设计
8. 接口设计（API）
9. 支付与结算设计
10. 拍卖统计与报表
11. 用户评价与反馈
12. 安全性设计
13. 系统测试计划
14. 部署与运维
15. 术语与参考资料

# 第一部分：引言 (Introduction)

## 1.1 项目背景

随着互联网经济从"静态展示"向"动态竞价"演进，在线拍卖系统已成为二手交易、大宗商品分配及限量收藏品流通的核心基础设施。然而，传统基于 Python、PHP 或早期 Java 框架构建的拍卖平台，在面对"秒杀级"瞬时高并发竞价时，常面临**请求排队严重、响应延迟高、数据最终一致性难以保证**等技术瓶颈。特别是在拍卖结束前的"最后几秒"，成千上万次的并发出价对服务器的 I/O 模型和内存并发控制提出了极高的要求。

本项目的提出，正是为了探索如何利用 **C++20** 及其现代特性（如协程 Coroutines、RAII 内存管理、强类型安全）构建一个**硬核高性能**的拍卖引擎。相比于解释型语言，C++ 能够通过直接操控内存与系统级 API，实现极低的出价撮合延迟。综上所述，本项目旨在通过 C++ 开发一套具备**微秒级撮合、强一致性事务处理、及动态延时防刷机制**的在线拍卖平台。这不仅满足了课程设计对软件工程全生命周期的要求，更是对高性能系统编程、网络通信协议及复杂状态机设计的一次深度实践。

## 1.2 编写目标

### 1.2 编写目的

本需求规格说明书旨在为"基于 C++ 的高性能在线拍卖平台"的设计、开发、测试及部署提供权威且详尽的技术准则。与传统的 Web 应用文档不同，本手册核心聚焦于**底层并发控制**、**低延迟撮合算法**以及**高可靠状态机**的实现细节，具体目标包括：

#### 1.2.1 规范核心拍卖引擎的功能逻辑

- **状态一致性定义：** 明确拍品从"待审核"到"竞价中"再到"成交/流拍"的 C++ 有限状态机（FSM）转换规则，消除竞态条件（Race Conditions）。
- **业务闭环覆盖：** 详细描述物品管理、实时竞价、异步结算、用户评价等 6 大核心模块的功能边界，确保 C++ 后端与前端通过 RESTful 及 WebSocket 协议实现高效解耦。

#### 1.2.2 确立高性能非功能约束基准

- **高并发处理：** 定义系统在 Linux 环境下利用 `epoll` 或 `io_uring` 模型处理 $10,000+$ 并发连接的能力指标。
- **确定性延迟（Deterministic Latency）：** 设定出价请求在核心引擎中的处理延迟上限（例如 $\le 10ms$），并规定如何利用 C++ 的非阻塞特性规避 I/O 阻塞。
- **资源利用率监控：** 规定内存管理策略（如使用自定义内存池或 `std::pmr`），确保在长时间高负载运行下，系统内存抖动与 CPU 开销保持在极低水平。

#### 1.2.3 提供可落地的开发与工程指导

- **技术契约：** 为 C++ 开发团队提供接口协议（API Spec）与数据结构定义，降低模块间耦合。
- **测试与审计标准：** 定义基于 Google Test 的单元测试覆盖率要求，以及针对支付、结算等敏感操作的幂等性校验逻辑。
- **维护与演进：** 为后续引入分布式架构（如基于 C++ 的 gRPC 微服务化）留出扩展性接口与设计预留。

### 1.3 核心技术栈 (Core Technology Stack)

本项目采用现代 C++ 开发生态，通过零开销抽象与异步 I/O 架构，确保系统在高并发场景下的极致表现：

| 层次 | 技术选型 | 说明 |
|------|---------|------|
| **开发语言** | C++20 | Concepts、Coroutines、RAII、智能指针 |
| **后端框架** | Drogon / Workflow | 非阻塞 I/O，epoll/io_uring |
| **数据库** | MySQL 8.0 | 持久化存储，事务支持 |
| **缓存** | Redis 7.0+ | 竞价热点数据，LUA 脚本原子操作 |
| **构建工具** | CMake 3.20+ | FetchContent 依赖管理 |
| **前端** | Vue3 / React | SPA 单页应用，WebSocket 通信 |
| **容器化** | Docker | 开发/生产环境一致性 |

- **开发语言：Modern C++ (C++20)**
  - 利用 **Concepts** 对模板参数进行约束，提升泛型代码的可读性与编译期安全。
  - 采用 **Coroutines (协程)** 简化异步回调逻辑，在保持非阻塞 I/O 高性能的同时，实现同步风格的代码编写。
  - 全面应用 **RAII (资源获取即初始化)** 与智能指针，根除内存泄漏与野指针问题。
- **后端框架：Drogon / Workflow**
  - **Drogon:** 基于非阻塞 I/O 模型的高性能 HTTP 框架，在 TechEmpower 评测中位居前列，提供原生数据库连接池与 JSON 处理支持。
  - **Workflow:** 搜狗开源的一款企业级编程范式，其任务流调度模型（Task-based）非常适合处理拍卖中复杂的"出价-校验-回调-推送"异步逻辑。
- **数据存储方案：**
  - **Redis (In-Memory Data Structure Store):** 作为竞价热点缓存。利用 Redis 的单线程原子性操作（如 `WATCH/MULTI` 或 Lua 脚本）存储活跃拍卖的当前最高价，支撑微秒级实时响应。
  - **MySQL 8.0:** 作为持久化存储层，利用其事务特性保证用户信息、财务结算、历史订单的强一致性。
- **构建与工具链：**
  - **CMake 3.20+:** 实现模块化构建与依赖管理（如 FetchContent 引入第三方库）。
  - **GDB / Valgrind:** 用于底层调试与内存指纹分析，确保生产环境的鲁棒性。

### 1.4 术语定义 (Definitions & Acronyms)

为确保项目开发过程中语义的一致性，特定义以下核心专业术语：

- **竞价撮合 (Bid Matching / Auction Engine):**

  系统核心逻辑模块，负责在收到出价请求后，根据当前价格、最小加价幅度（Bid Step）及有效时间窗口进行验证，并瞬间更新最高出价者的逻辑过程。

- **延迟修正 (Latency Compensation):**

  一种处理网络抖动的策略。系统通过逻辑时钟（Logical Clock）或服务器端到达时间戳，判定在拍卖结束瞬间（秒杀时刻）由于网络延迟导致的并发请求先后顺序，确保公平性。

- **原子出价 (Atomic Bidding):**

  利用 C++ 原子变量（`std::atomic`）或数据库行级锁实现的一种机制，确保在极短时间内（如 1ms）多个用户对同一拍品出价时，系统能且仅能成功接受一个请求，防止出现"双赢"或价格跳变异常。

- **时间轮调度 (Time Wheel Scheduling):**

  一种高效处理大规模定时任务的数据结构。在系统中用于管理成千上万个拍品的倒计时，避免为每个拍卖开启独立硬件定时器导致的上下文切换开销。

- **幂等性回调 (Idempotent Callback):**

  支付系统在通知拍卖引擎成交结果时，无论通知发送多少次，系统内部的状态变更（如标记已支付）仅生效一次，防止重复结算。

- **延时触发逻辑 (Anti-Sniping Mechanism):**

  俗称"延时保护"。若在拍卖预定结束前 $N$ 秒内出现有效加价，系统自动将结束时间顺延 $M$ 秒，以对抗恶意秒杀脚本，鼓励真实买家博弈。

# 2. 总体要求

## 2.1 现状及痛点

传统线下拍卖周期长、参与门槛高；现有在线拍卖平台功能复杂且面向大流量场景，针对校园/小型社区的轻量级可定制平台缺乏。本项目目标是实现一个轻量、稳定、功能齐全的拍卖系统。

## 2.2 系统目标

- 提供物品发布与图片展示功能，并支持管理员审核流程。
- 支持管理员创建与配置拍卖活动（时间、规则、起拍价、加价幅度、延时机制等）。
- 实时记录出价并确定最高出价者；支持并发竞价与竞价防刷机制。
- 支付与结算流程完整、可对接第三方支付（模拟或真实）。
- 完成拍卖统计、报表导出与交易评价功能。

## 2.3 用户及角色分析

| 角色 | 权限范围 | 典型操作 |
|------|---------|---------|
| **普通用户** | 个人中心、我要买、我要卖 | 注册登录、浏览拍卖、出价、支付、评价 |
| **卖家** | 发布物品、管理我的商品 | 发布物品、设置拍卖参数、发货 |
| **管理员** | 全系统管理 | 审核物品、配置拍卖活动、处理投诉、导出报表 |

## 2.4 系统边界及上下文

- **前端：** Vue3/React SPA 应用，通过 HTTP REST API 与后端通信，WebSocket 接收实时竞价推送
- **第三方支付服务：** 支付接口模拟（沙箱环境）或接入支付宝/微信支付
- **图片存储：** 本地文件系统（开发阶段）或 S3-compatible 对象存储（生产）
- **邮件/短信通知服务：** 可选集成，用于订单状态通知

# 第三部分：功能性需求 (Functional Requirements)

## 3.1 账户与安全模块

本模块负责系统用户身份的生命周期管理及访问控制。

- **身份验证：** 支持基于用户名/邮箱/手机号的注册与登录；登录成功后系统分发加密令牌（JWT），作为后续所有受限请求的凭证。
- **权限分级：** 实现基于角色的访问控制（RBAC），区分普通用户（买家/卖家）、管理员及系统审计员，严格限制各角色对敏感接口的调用权限。
- **安全加固：** 后端需对所有入参进行合法性校验，敏感操作（如资金变动、出价）必须在服务端进行二次鉴权。

### JWT Token 规范

```
Header: HS256 算法
Payload:
  - sub: user_id (BIGINT)
  - role: "user" | "seller" | "admin"
  - exp: 过期时间 (Unix timestamp)
  - iat: 签发时间
Token 有效期：24 小时
```

## 3.2 商品与审核模块

负责管理拍卖标的物的完整生命周期，确保上架商品的合规性。

- **物品发布：** 卖家可上传拍品基本信息（标题、描述、分类、图片集）并设定拍卖参数（起拍价、加价步长、预计开始/结束时间）。
- **状态流转管理：** 系统通过状态机管理拍品状态。
- **审核流程：** 管理员可批量处理待审核拍品，支持通过、驳回并附带意见，审核结果需实时通知卖家。

### 拍品状态机 (Item State Machine)

```
                    ┌─────────────┐
                    │   DRAFT     │  草稿（卖家编辑中）
                    └──────┬──────┘
                           │ submit
                           ▼
                    ┌─────────────┐
                    │  PENDING    │  待审核
                    └──────┬──────┘
                           │ approve
              ┌────────────┼────────────┐
              ▼            ▼            ▼
       ┌──────────┐  ┌──────────┐  ┌──────────┐
       │ UPCOMING │  │ ACTIVE   │  │ REJECTED │  驳回
       └────┬─────┘  └────┬─────┘  └──────────┘
            │             │
            │ start_time  │ auction.end_time
            │ reached     │ reached
            ▼             │
       ┌──────────┐        │
       │ ACTIVE  │◄───────┘
       └────┬─────┘
            │
     ┌──────┴──────┐
     ▼             ▼
┌────────┐    ┌────────┐
│  SOLD  │    │UNSOLD  │  流拍
└────────┘    └────────┘
```

### 拍卖活动状态机 (Auction State Machine)

```
PLANNED ──► RUNNING ──► ENDED
    │           │
    └───────────┴────► CANCELLED
```

## 3.3 核心拍卖引擎 (Core Engine)

本模块是系统的计算核心，负责处理高频竞价逻辑。

- **实时出价撮合：**
  - **合法性校验：** 系统需验证出价时间是否有效、金额是否满足当前最高价加最小步长要求、买家是否有足够余额/资质。
  - **原子更新：** 确保在极短时间内、并发出价场景下，仅有一个合法出价被确认为"当前最高价"。
- **动态延时机制 (Anti-Sniping)：** 实现防秒杀逻辑。若在拍卖预定结束前 $N$ 秒内产生有效出价，系统自动顺延结束时间 $M$ 秒，循环触发直至不再有新出价。
- **精准倒计时：** 基于服务端统一高精度时钟（微秒级）进行倒计时管理，消除客户端本地时钟偏差导致的竞价争议。

### 竞价防刷参数配置

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `anti_snipe_N` | 60 秒 | 拍卖结束前 N 秒内出新价触发延时 |
| `anti_snipe_M` | 120 秒 | 每次延时 M 秒 |
| `max_extensions` | 10 次 | 最大延时次数上限（防止无限延时） |

### 竞价处理流程

```
1. 客户端发送出价请求 POST /api/bids
   └─► 请求参数: { auction_item_id, amount, idempotent_token }

2. 服务端接收请求 (WebSocket/HTTP)
   │
   ├─► 基础校验: 参数完整性、用户认证、余额检查
   │
   ├─► 时间校验: 拍卖是否在竞价中
   │
   ├─► Redis Lua 原子操作:
   │   ├─ WATCH current_price, highest_bidder_id, end_time
   │   ├─ 校验 amount >= current_price + bid_step
   │   ├─ 校验 now < end_time 或 anti_snipe 规则
   │   ├─ MULTI
   │   │   ├─ SET current_price = amount
   │   │   ├─ SET highest_bidder_id = bidder_id
   │   │   └─ ZADD bid_history {timestamp, amount}
   │   └─ EXEC
   │
   ├─► 若 anti_snipe 触发: ZREMRANGEBYRANK + ZREMRANGEBYRANK end_time += M
   │
   ├─► 发布 BidUpdate 消息到 Redis Pub/Sub
   │
   └─► 返回: { success, current_price, highest_bidder, next_end_time }
```

## 3.4 交易与支付模拟模块

负责拍卖结束后的清算与资金流向管理。

- **订单自动化：** 拍卖成交瞬间，系统自动生成包含买卖双方、成交价及佣金等信息的订单，并冻结/扣除相应信用额度或资金。
- **幂等支付处理：** 支付接口必须实现幂等性设计，确保在网络波动或重复回调场景下，订单状态更新与资金结算仅执行一次。
- **异常处理：** 支持超时未支付自动关单、违规判定及保证金扣除等逻辑。

### 订单状态流转

```
PENDING_PAYMENT ──► PAID ──► SHIPPED ──► COMPLETED
       │                           │
       └──────► CANCELLED ◄────────┘
       │
       └──────► TIMEOUT (48h 未支付)
```

## 3.5 消息推送模块

确保拍卖现场信息的瞬时同步。

- **实时广播：** 当拍品最高价发生变动、拍卖状态变更或倒计时进入关键时刻时，系统需主动推送消息至所有关注该拍品的在线客户端。
- **连接管理：** 支持大规模长连接维持与心跳监测，确保推送链路的稳定性与低延迟。

### WebSocket 消息格式

```json
// 竞价更新消息
{
  "type": "bid_update",
  "data": {
    "auction_item_id": 12345,
    "current_price": 1500.00,
    "highest_bidder_id": 789,
    "bid_count": 42,
    "next_end_time": "2026-03-25T15:30:00Z"
  }
}

// 拍卖结束消息
{
  "type": "auction_ended",
  "data": {
    "auction_item_id": 12345,
    "final_price": 1500.00,
    "winner_id": 789,
    "status": "sold"
  }
}

// 倒计时提醒（最后 60 秒）
{
  "type": "countdown_tick",
  "data": {
    "auction_item_id": 12345,
    "remaining_seconds": 60,
    "is_extended": false
  }
}
```

## 3.6 报表与统计

为平台运营提供数据支持。

- **拍卖追踪：** 完整记录每一件拍品的所有出价历史（时间、用户、金额），支持交易溯源与审计。
- **多维聚合分析：** 定期生成平台运营报表，包括成交总额（GMV）、成交率、用户活跃度及出价频率分布等。
- **异步导出：** 支持管理员导出 CSV 或 Excel 格式的原始交易报表，导出过程不得阻塞核心竞价业务。

# 第四部分：非功能性需求 (Non-functional Requirements)

## 4.1 高并发性能 (High Performance)

| 指标 | 目标值 | 说明 |
|------|--------|------|
| **竞价 QPS** | 10,000+ | 核心竞价接口每秒处理请求数 |
| **P99 延迟** | < 10ms | 单次出价处理耗时（P99 分位） |
| **WebSocket 连接数** | 50,000+ | 同时维持的实时推送连接 |
| **撮合延迟** | < 1ms | Redis Lua 脚本执行时间 |

## 4.2 数据一致性 (Data Consistency)

- **原子性保障：** 竞价操作必须满足 ACID 特性。利用 Redis 原子操作（如 Lua 脚本）进行内存预减/校验，结合数据库事务确保持久化数据与缓存数据的一致性。
- **并发冲突处理：** 面对毫秒级并发出价，系统需采用悲观锁或带有版本号的乐观锁机制，确保最高价更新的唯一性。
- **Redis 与 MySQL 一致性：** 采用 CACHE-ASIDE 模式，读取优先 Redis，写入双写并通过后台任务补偿最终一致。

## 4.3 内存安全性 (Memory Safety)

- **资源生命周期管理：** 强制采用 RAII 模式管理系统资源（如内存、文件句柄、数据库连接），严禁显式的资源申请与手动释放。
- **自动化检测：** 开发阶段必须通过静态分析工具及动态监测工具（如 ASan）扫描，确保系统在高负载压力下不存在内存泄漏、越界访问及野指针风险。
- **智能指针策略：** 使用 `std::unique_ptr` 管理独占资源，`std::shared_ptr` 管理共享资源，避免循环引用（搭配 `std::weak_ptr`）。

## 4.4 可扩展性与可维护性 (Scalability)

- **模块化解耦：** 核心引擎、用户中心、支付结算、推送服务应实现逻辑解耦，通过标准接口通讯。
- **演进路径：** 系统设计需预留分布式扩展能力，支持未来将核心引擎抽离为独立服务，并可通过负载均衡器进行横向扩容。
- **可观测性：** 系统需提供详细的运行日志、错误堆栈跟踪及性能指标指标（Metrics），便于运维调优。

# 第五部分：系统概要设计 (System Design)

## 5.1 逻辑架构设计 (Logical Architecture)

系统采用分层解耦架构，确保各组件在高性能 C++ 环境下协同工作：

```
┌─────────────────────────────────────────────────────────────────┐
│                         客户端层 (Client)                        │
│              Vue3/React SPA + WebSocket 长连接                    │
└─────────────────────────────────────────────────────────────────┘
                                  │
                                  ▼
┌─────────────────────────────────────────────────────────────────┐
│                         接入层 (Access Layer)                     │
│              Drogon HTTP Server + WebSocket Server               │
│              (SSL 卸载、请求分发、协议校验、Session 管理)           │
└─────────────────────────────────────────────────────────────────┘
                                  │
        ┌─────────────────────────┼─────────────────────────┐
        ▼                         ▼                         ▼
┌───────────────┐         ┌───────────────┐         ┌───────────────┐
│  用户模块      │         │  拍卖引擎      │         │  支付模块      │
│  (Auth/Users) │         │ (Bid Engine)  │         │  (Payments)   │
└───────────────┘         └───────────────┘         └───────────────┘
        │                         │                         │
        └─────────────────────────┼─────────────────────────┘
                                  ▼
┌─────────────────────────────────────────────────────────────────┐
│                       持久化层 (Persistence)                      │
│   ┌─────────────────┐              ┌─────────────────┐         │
│   │   Redis Cluster  │              │   MySQL Server  │         │
│   │  (热点数据/缓存)  │              │   (持久化存储)   │         │
│   └─────────────────┘              └─────────────────┘         │
└─────────────────────────────────────────────────────────────────┘
```

## 5.2 并发模型设计 (Concurrency Model)

为了最大化多核 CPU 利用率并降低上下文切换开销，系统采用 **多线程 Reactor 模型**：

- **Main Reactor (Acceptor)：** 运行在主线程，负责监听端口并接受新连接，通过 Round-Robin 算法将连接分发给不同的 Sub-Reactor。
- **Sub-Reactors (Event Loops)：** 运行在独立的 IO 线程池中（通常数量等于 CPU 核心数）。每个 Event Loop 负责其管辖连接的所有数据读写及协议解析，确保 IO 操作非阻塞。
- **Worker Thread Pool：** 专门处理计算密集型任务（如支付加密、复杂统计报表生成），防止因业务逻辑耗时过长而导致 IO 响应延迟。

## 5.3 关键算法与数据结构 (Core Algorithms & Data Structures)

### 5.3.1 分级时间轮定时器 (Hierarchical Timing Wheel)

传统的定时器在处理万级并发倒计时时存在 $O(\log N)$ 的插入开销。

- **设计：** 系统实现一个**分级时间轮 (Hierarchical Timing Wheel)**。
- **原理：** 将时间划分为槽位（Slots），每个槽位挂载该时刻到期的拍卖事件。指针每秒跳动一次，触发对应槽位的所有任务。
- **优势：** 插入、删除和触发操作均为 $O(1)$ 复杂度，极大地降低了高并发下的定时开销。

```
层级结构示例（3 层时间轮）：

层级 1 (秒轮): 60 槽，每槽 1 秒，范围 1-60 秒
层级 2 (分轮): 60 槽，每槽 1 分钟，范围 1-60 分钟
层级 3 (时轮): 24 槽，每槽 1 小时，范围 1-24 小时

插入 45 秒后到期: 放入层级1 槽位 45
插入 90 秒后到期: 放入层级1 槽位 30 (第二轮) + 降级到层级2
插入 90 分钟后到期: 放入层级2 槽位 30 (第二轮) + 降级到层级3
```

### 5.3.2 异步日志与持久化双缓冲 (Double Buffering)

为了解决"高频出价"与"磁盘写入慢"之间的矛盾：

- **设计：** 引入**双缓冲无锁队列 (Lock-free Double Buffer)**。
- **流程：** 出价引擎将成交记录快速写入"前端缓冲区"后立即返回；后台持久化线程定期交换（Swap）缓冲区，并将"后端缓冲区"的数据批量写入 MySQL。
- **优势：** 避免了磁盘 IO 直接阻塞竞价核心路径。

### 5.3.3 Redis Lua 脚本实现原子竞价

```lua
-- atomic_bid.lua
-- 键: auction:{item_id}  字段: price, bidder, end_time, step
-- 参数: KEYS[1]=item_key, ARGV[1]=amount, ARGV[2]=bidder_id, ARGV[3]=timestamp

local current_price = tonumber(redis.call('HGET', KEYS[1], 'price'))
local bid_step = tonumber(redis.call('HGET', KEYS[1], 'step'))
local end_time = tonumber(redis.call('HGET', KEYS[1], 'end_time'))
local current_bidder = redis.call('HGET', KEYS[1], 'bidder')
local anti_snipe_N = tonumber(ARGV[4])
local anti_snipe_M = tonumber(ARGV[5])

-- 校验出价金额
if tonumber(ARGV[1]) < current_price + bid_step then
    return {err = 'BID_TOO_LOW', current_price}
end

-- 校验拍卖是否结束
if tonumber(ARGV[3]) > end_time then
    return {err = 'AUCTION_ENDED'}
end

-- 更新最高价
redis.call('HSET', KEYS[1], 'price', ARGV[1])
redis.call('HSET', KEYS[1], 'bidder', ARGV[2])

-- 检查是否触发 anti-snipe (结束前 N 秒内)
local remaining = end_time - tonumber(ARGV[3])
if remaining < anti_snipe_N then
    local new_end_time = end_time + anti_snipe_M
    redis.call('HSET', KEYS[1], 'end_time', new_end_time)
    return {ok = 'BID_ACCEPTED_EXTENDED', price = ARGV[1], new_end_time}
end

return {ok = 'BID_ACCEPTED', price = ARGV[1]}
```

## 5.4 Redis Key 设计

| Key Pattern | 类型 | 说明 | TTL |
|-------------|------|------|-----|
| `auction:item:{id}` | Hash | 拍品实时数据 (price, bidder, end_time, step) | 与拍卖共存 |
| `auction:history:{id}` | Sorted Set | 出价历史，按 timestamp 排序 | 永久 |
| `user:session:{token}` | String | JWT 黑名单/会话数据 | 24h |
| `user:balance:{id}` | String | 用户余额快照 | 实时更新 |
| `auction:watchers:{id}` | Set | 关注某拍品的用户 ID | 与拍卖共存 |

## 5.5 项目目录结构

```
auction_system/
├── CMakeLists.txt              # 根构建配置
├── cmake/
│   └── FetchContent_*.cmake    # 第三方依赖引入
├── src/
│   ├── main.cpp                # 程序入口
│   ├── config/
│   │   └── ConfigManager.cpp   # 配置管理
│   ├── network/
│   │   ├── HttpServer.cpp      # HTTP 服务器
│   │   └── WebSocketServer.cpp # WebSocket 服务器
│   ├── engine/
│   │   ├── BidEngine.cpp       # 竞价引擎核心
│   │   ├── TimingWheel.cpp     # 时间轮定时器
│   │   └── AntiSnipe.cpp       # 防秒杀逻辑
│   ├── service/
│   │   ├── UserService.cpp     # 用户服务
│   │   ├── ItemService.cpp     # 物品服务
│   │   ├── AuctionService.cpp  # 拍卖服务
│   │   ├── OrderService.cpp    # 订单服务
│   │   └── PaymentService.cpp  # 支付服务
│   ├── db/
│   │   ├── MySQLPool.cpp       # MySQL 连接池
│   │   └── RedisPool.cpp       # Redis 连接池
│   ├── cache/
│   │   └── DoubleBuffer.cpp    # 双缓冲持久化
│   ├── model/
│   │   ├── User.cpp/h          # 用户模型
│   │   ├── Item.cpp/h          # 物品模型
│   │   ├── Bid.cpp/h           # 出价记录模型
│   │   └── Order.cpp/h         # 订单模型
│   └── util/
│       ├── Logger.cpp          # 日志工具
│       └── JwtHelper.cpp       # JWT 工具
├── include/                    # 公共头文件
├── scripts/
│   ├── init_db.sql             # 数据库初始化脚本
│   └── atomic_bid.lua          # Redis Lua 脚本
├── tests/
│   ├── unit/
│   │   ├── BidEngineTest.cpp   # 竞价引擎单元测试
│   │   └── TimingWheelTest.cpp # 时间轮单元测试
│   └── integration/
│       └── ApiFlowTest.cpp     # API 集成测试
├── docker/
│   └── Dockerfile
└── docs/
    └── api_spec.md             # API 详细规格说明
```

## 5.6 部署视图 (Deployment View)

- **容器化部署：** 利用 Docker 封装 C++ 运行时环境，确保跨平台（Dev/Ops）一致性。
- **水平扩展：** 接入层支持负载均衡器（LB）；核心引擎支持按拍品 ID 取模（Sharding）进行分布式部署，以应对超大规模拍卖活动。

```
                    ┌─────────────┐
                    │   Nginx     │
                    │  (LB/Proxy) │
                    └──────┬──────┘
                           │
           ┌───────────────┼───────────────┐
           ▼               ▼               ▼
    ┌──────────┐    ┌──────────┐    ┌──────────┐
    │  Drogon  │    │  Drogon  │    │  Drogon  │
    │ Server 1 │    │ Server 2 │    │ Server N │
    └────┬─────┘    └────┬─────┘    └────┬─────┘
         │               │               │
    ┌────┴───────────────┴───────────────┴────┐
    │                                          │
    ▼                                          ▼
┌──────────┐                            ┌──────────┐
│MySQL主库  │◄─────主从复制─────►         │MySQL从库  │
└──────────┘                            └──────────┘
                                              │
                                              ▼
                                       ┌──────────┐
                                       │  Redis   │
                                       │ Cluster  │
                                       └──────────┘
```

# 6. 系统概要设计

## 6.1 系统架构概览

采用典型的三层架构（前端—后端—数据库），并辅以消息/通知组件：

- 前端：Vue3/React 单页应用，WebSocket 实时通信
- 后端：C++ HTTP Server (Drogon)，处理 REST API 与 WebSocket
- 数据库：MySQL 8.0 核心数据，Redis 7.0+ 缓存/竞价/定时器
- 文件存储：本地文件系统（开发）或 S3-compatible 对象存储
- 消息通知：WebSocket 实时推送

## 6.2 模块划分

| 模块 | 职责 | 主要类/组件 |
|------|------|-----------|
| **用户模块** | 注册/登录/JWT/权限 | UserService, AuthController |
| **商品模块** | 物品 CRUD/审核/状态机 | ItemService, ItemController |
| **拍卖模块** | 活动管理/时间调度/竞价 | AuctionService, BidEngine, TimingWheel |
| **支付模块** | 订单/支付回调/幂等 | OrderService, PaymentService |
| **报表模块** | 统计/导出 | ReportService |
| **评价模块** | 买卖互评 | CommentService |

## 6.3 部署视图

- **开发/测试：** 单机部署，MySQL + Redis + Auction Server 同机
- **生产（建议）：** 多实例后端 + Nginx 负载均衡 + MySQL 主从 + Redis Cluster + 对象存储

# 7. 数据库设计

## 7.1 概念 ER 描述（文字说明）

主要实体：User、Item、Auction、Bid、Order、Payment、Comment。

关系说明：
- User(1) ──► (N) Item：卖家发布物品
- Auction(1) ──► (N) AuctionItem：活动包含多个拍品
- AuctionItem(1) ──► (N) Bid：每个拍品有多条出价记录
- AuctionItem(1) ──► (1) Order：成交后生成订单
- Order(1) ──► (N) Payment：订单对应支付记录
- Order(1) ──► (N) Comment：订单对应评价

## 7.2 主要表结构（逻辑设计与示例 SQL）

### users

```sql
CREATE TABLE users (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  username VARCHAR(64) UNIQUE NOT NULL,
  email VARCHAR(128),
  phone VARCHAR(32),
  password_hash VARCHAR(255) NOT NULL,
  role ENUM('user','seller','admin') DEFAULT 'user',
  real_name VARCHAR(128),
  avatar_url VARCHAR(512),
  verified BOOLEAN DEFAULT FALSE,
  balance DECIMAL(12,2) DEFAULT 0.00,
  frozen_balance DECIMAL(12,2) DEFAULT 0.00,
  credit_score DECIMAL(3,1) DEFAULT 5.0,
  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
  updated_at DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  INDEX idx_username (username),
  INDEX idx_email (email),
  INDEX idx_role (role)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

### items

```sql
CREATE TABLE items (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  seller_id BIGINT NOT NULL,
  category_id INT,
  title VARCHAR(200) NOT NULL,
  description TEXT,
  start_price DECIMAL(12,2) NOT NULL,
  reserve_price DECIMAL(12,2),
  bid_step DECIMAL(12,2) NOT NULL DEFAULT 1.00,
  images JSON,
  status ENUM('draft','pending_review','active','closed','rejected') DEFAULT 'draft',
  version INT DEFAULT 0,
  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
  updated_at DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  FOREIGN KEY (seller_id) REFERENCES users(id),
  INDEX idx_seller (seller_id),
  INDEX idx_status (status),
  INDEX idx_category (category_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

### categories

```sql
CREATE TABLE categories (
  id INT PRIMARY KEY AUTO_INCREMENT,
  name VARCHAR(64) NOT NULL,
  parent_id INT DEFAULT NULL,
  FOREIGN KEY (parent_id) REFERENCES categories(id),
  INDEX idx_parent (parent_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

### auctions

```sql
CREATE TABLE auctions (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  name VARCHAR(200) NOT NULL,
  description TEXT,
  start_time DATETIME NOT NULL,
  end_time DATETIME NOT NULL,
  anti_snipe_N INT DEFAULT 60,
  anti_snipe_M INT DEFAULT 120,
  max_extensions INT DEFAULT 10,
  rule JSON,
  status ENUM('planned','running','ended','cancelled') DEFAULT 'planned',
  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_status (status),
  INDEX idx_time (start_time, end_time)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

### auction_items

```sql
CREATE TABLE auction_items (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  auction_id BIGINT NOT NULL,
  item_id BIGINT NOT NULL UNIQUE,
  current_price DECIMAL(12,2) NOT NULL,
  highest_bidder_id BIGINT,
  bid_count INT DEFAULT 0,
  extension_count INT DEFAULT 0,
  status ENUM('waiting','on_auction','sold','unsold') DEFAULT 'waiting',
  version INT DEFAULT 0,
  FOREIGN KEY (auction_id) REFERENCES auctions(id),
  FOREIGN KEY (item_id) REFERENCES items(id),
  INDEX idx_auction (auction_id),
  INDEX idx_status (status),
  INDEX idx_highest_bidder (highest_bidder_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

### bids

```sql
CREATE TABLE bids (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  auction_item_id BIGINT NOT NULL,
  bidder_id BIGINT NOT NULL,
  amount DECIMAL(12,2) NOT NULL,
  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
  idempotent_token VARCHAR(64),
  FOREIGN KEY (auction_item_id) REFERENCES auction_items(id),
  FOREIGN KEY (bidder_id) REFERENCES users(id),
  UNIQUE INDEX idx_idempotent (idempotent_token),
  INDEX idx_item_bidder (auction_item_id, bidder_id),
  INDEX idx_item_amount (auction_item_id, amount),
  INDEX idx_created (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

### orders

```sql
CREATE TABLE orders (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  order_no VARCHAR(32) UNIQUE NOT NULL,
  auction_item_id BIGINT NOT NULL,
  buyer_id BIGINT NOT NULL,
  seller_id BIGINT NOT NULL,
  final_price DECIMAL(12,2) NOT NULL,
  commission DECIMAL(12,2) DEFAULT 0.00,
  status ENUM('pending_payment','paid','shipped','completed','cancelled','timeout') DEFAULT 'pending_payment',
  payment_deadline DATETIME,
  paid_at DATETIME,
  shipped_at DATETIME,
  completed_at DATETIME,
  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
  updated_at DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  FOREIGN KEY (auction_item_id) REFERENCES auction_items(id),
  FOREIGN KEY (buyer_id) REFERENCES users(id),
  FOREIGN KEY (seller_id) REFERENCES users(id),
  INDEX idx_buyer (buyer_id),
  INDEX idx_seller (seller_id),
  INDEX idx_status (status),
  INDEX idx_payment_deadline (payment_deadline)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

### payments

```sql
CREATE TABLE payments (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  order_id BIGINT NOT NULL,
  amount DECIMAL(12,2) NOT NULL,
  method ENUM('alipay','wechat','card') DEFAULT 'alipay',
  status ENUM('initiated','success','failed','refunded') DEFAULT 'initiated',
  txn_id VARCHAR(128),
  idempotent_token VARCHAR(64) UNIQUE,
  callback_data JSON,
  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
  updated_at DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  FOREIGN KEY (order_id) REFERENCES orders(id),
  INDEX idx_order (order_id),
  INDEX idx_status (status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

### comments

```sql
CREATE TABLE comments (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  order_id BIGINT NOT NULL,
  from_user_id BIGINT NOT NULL,
  to_user_id BIGINT NOT NULL,
  rating TINYINT NOT NULL CHECK (rating >= 1 AND rating <= 5),
  content TEXT,
  is_hidden BOOLEAN DEFAULT FALSE,
  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
  FOREIGN KEY (order_id) REFERENCES orders(id),
  FOREIGN KEY (from_user_id) REFERENCES users(id),
  FOREIGN KEY (to_user_id) REFERENCES users(id),
  INDEX idx_to_user (to_user_id),
  INDEX idx_order (order_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

## 7.3 索引与事务设计

- 在 bids 表对 `(auction_item_id, amount, created_at)` 建复合索引以快速检索当前最高价。
- 出价操作采用事务或乐观锁（例如在 auction_items 表中使用 current_price 与版本号）保证并发一致性。
- 使用 `SELECT ... FOR UPDATE` 对 auction_items 行锁，防止并发出价导致的价格不一致。

# 8. 接口设计（RESTful API 详细规格）

> 所有 API 返回格式统一，认证通过 Header `Authorization: Bearer <token>` 传递 JWT。

## 8.1 统一响应格式

```json
// 成功响应
{
  "code": 0,
  "message": "success",
  "data": { ... }
}

// 错误响应
{
  "code": 10001,
  "message": "错误描述",
  "data": null
}
```

### 错误码定义

| 错误码 | 说明 |
|--------|------|
| 0 | 成功 |
| 10001 | 参数错误 |
| 10002 | 未认证 |
| 10003 | 权限不足 |
| 10004 | 资源不存在 |
| 20001 | 出价金额过低 |
| 20002 | 拍卖已结束 |
| 20003 | 不允许自卖自买 |
| 20004 | 余额不足 |
| 30001 | 订单不存在 |
| 30002 | 支付失败 |
| 30003 | 订单已支付 |
| 40001 | 物品不存在 |
| 40002 | 物品状态不允许此操作 |

## 8.2 用户相关 API

### POST /api/auth/register
注册用户

**请求体：**
```json
{
  "username": "string (3-32字符)",
  "password": "string (6-32字符)",
  "email": "string (邮箱格式)",
  "phone": "string (可选)"
}
```

**响应：**
```json
{
  "code": 0,
  "data": {
    "user_id": 123,
    "username": "testuser"
  }
}
```

### POST /api/auth/login
用户登录

**请求体：**
```json
{
  "username": "string",
  "password": "string"
}
```

**响应：**
```json
{
  "code": 0,
  "data": {
    "token": "eyJhbGciOiJIUzI1NiIs...",
    "expires_in": 86400,
    "user": {
      "id": 123,
      "username": "testuser",
      "role": "user"
    }
  }
}
```

### GET /api/users/me
获取当前用户信息

**响应：**
```json
{
  "code": 0,
  "data": {
    "id": 123,
    "username": "testuser",
    "email": "test@example.com",
    "balance": "1000.00",
    "credit_score": "4.8"
  }
}
```

## 8.3 物品相关 API

### POST /api/items
发布物品（需登录）

**请求体：**
```json
{
  "title": "iPhone 15 Pro",
  "description": "99新无划痕",
  "category_id": 1,
  "start_price": 5000.00,
  "reserve_price": 4500.00,
  "bid_step": 50.00,
  "images": ["https://...", "https://..."]
}
```

### GET /api/items/{id}
获取物品详情

**响应：**
```json
{
  "code": 0,
  "data": {
    "id": 1,
    "title": "iPhone 15 Pro",
    "description": "99新无划痕",
    "category": {"id": 1, "name": "数码"},
    "start_price": "5000.00",
    "current_price": "5500.00",
    "bid_step": "50.00",
    "seller": {"id": 10, "username": "seller1"},
    "status": "active",
    "images": ["..."],
    "created_at": "2026-03-20T10:00:00Z"
  }
}
```

### GET /api/items
分页查询物品列表

**Query 参数：**
- `status`: 物品状态 (draft/pending_review/active/closed/rejected)
- `category_id`: 分类 ID
- `page`: 页码 (默认 1)
- `page_size`: 每页数量 (默认 20, 最大 100)
- `keyword`: 搜索关键词

### POST /api/admin/items/{id}/review
管理员审核物品

**请求体：**
```json
{
  "action": "approve",
  "reason": "可选的审核意见"
}
```
`action` 可选: `approve`, `reject`

## 8.4 拍卖活动 API

### POST /api/auctions
创建拍卖活动（管理员）

**请求体：**
```json
{
  "name": "春季数码专场",
  "description": "全场数码产品低价起拍",
  "start_time": "2026-04-01T00:00:00Z",
  "end_time": "2026-04-07T23:59:59Z",
  "anti_snipe_N": 60,
  "anti_snipe_M": 120,
  "max_extensions": 10,
  "rule": {
    "min_bid_count": 1,
    "auto_relist": false
  }
}
```

### POST /api/auctions/{id}/items
将物品加入拍卖活动

**请求体：**
```json
{
  "item_id": 123
}
```

### GET /api/auctions
获取拍卖活动列表

**Query 参数：**
- `status`: planned/running/ended/cancelled
- `page`: 页码
- `page_size`: 每页数量

### GET /api/auctions/{id}
获取拍卖活动详情

## 8.5 竞价 API

### POST /api/bids
提交出价

**请求体：**
```json
{
  "auction_item_id": 123,
  "amount": 5500.00,
  "idempotent_token": "uuid-v4"
}
```

**响应（成功）：**
```json
{
  "code": 0,
  "data": {
    "bid_id": 456,
    "current_price": "5500.00",
    "highest_bidder_id": 789,
    "bid_count": 42,
    "next_end_time": "2026-03-25T16:00:00Z",
    "is_extended": false
  }
}
```

**响应（触发防秒杀）：**
```json
{
  "code": 0,
  "data": {
    "bid_id": 456,
    "current_price": "5500.00",
    "highest_bidder_id": 789,
    "bid_count": 43,
    "next_end_time": "2026-03-25T16:02:00Z",
    "is_extended": true,
    "extension_count": 2
  }
}
```

### GET /api/auction-items/{id}/bids
获取出价历史

**Query 参数：**
- `page`: 页码
- `page_size`: 每页数量

## 8.6 订单与支付 API

### GET /api/orders
获取当前用户的订单列表

**Query 参数：**
- `status`: 订单状态
- `role`: buyer/seller
- `page`: 页码

### POST /api/orders/{id}/pay
发起支付

**请求体：**
```json
{
  "method": "alipay",
  "idempotent_token": "uuid-v4"
}
```

### POST /api/payments/callback
支付回调（第三方支付回调）

**请求体：**
```json
{
  "order_no": "ORD202603250001",
  "txn_id": "第三方交易号",
  "status": "success",
  "sign": "MD5签名"
}
```

### POST /api/orders/{id}/ship
卖家发货

**请求体：**
```json
{
  "tracking_no": "快递单号",
  "carrier": "顺丰速运"
}
```

### POST /api/orders/{id}/confirm
买家确认收货

## 8.7 评价 API

### POST /api/orders/{id}/comment
提交评价

**请求体：**
```json
{
  "rating": 5,
  "content": "物品很好，卖家发货快！"
}
```

## 8.8 WebSocket 连接

### 连接地址
```
wss://api.example.com/ws?token=<jwt_token>
```

### 心跳
客户端每 30 秒发送：
```json
{"type": "ping"}
```
服务端回复：
```json
{"type": "pong"}
```

### 订阅拍品
```json
{"type": "subscribe", "auction_item_ids": [123, 456]}
```

# 9. 支付与结算设计

## 9.1 支付流程

```
┌──────────┐     ┌──────────┐     ┌──────────┐     ┌──────────┐
│  买家     │     │  系统     │     │  支付网关  │     │  系统     │
│  发起支付  │────►│  创建订单  │────►│  生成支付  │────►│  等待回调  │
└──────────┘     │  冻结余额  │     │  请求      │     │           │
                 └──────────┘     └──────────┘     └───────┬───┘
                                                            │
                         ┌──────────┐     ┌──────────┐     │
                         │  支付成功  │◄────│  回调通知  │◄────┘
                         └────┬─────┘     └──────────┘
                              │
                              ▼
                         ┌──────────┐
                         │  更新订单  │
                         │  解冻余额  │
                         │  通知卖家  │
                         └──────────┘
```

## 9.2 手续费计算

| 场景 | 费率 |
|------|------|
| 平台服务费 | 成交价的 5% |
| 支付通道费 | 买家承担 |
| 提现手续费 | 1% (最低 1 元) |

## 9.3 超时与违约处理

| 超时场景 | 处理方式 |
|----------|----------|
| 48h 未支付 | 订单自动取消，释放冻结余额，拍品可重新上架 |
| 卖家 72h 未发货 | 买家可申请退款，卖家扣罚信用分 |
| 买家 7d 未确认收货 | 系统自动确认，货款结算给卖家 |

# 10. 拍卖统计与报表

## 10.1 核心指标

| 指标 | 计算公式 | 更新频率 |
|------|----------|----------|
| **GMV** | SUM(final_price) | 实时 |
| **成交率** | sold_count / auction_item_count | 每小时 |
| **平均出价次数** | total_bids / sold_count | 每小时 |
| **用户活跃数** | COUNT(DISTINCT bidder_id) | 每日 |

## 10.2 报表类型

1. **实时大屏：** 展示当前进行中的拍卖数量、实时 GMV、出价次数
2. **活动报表：** 单场拍卖的成交明细、竞价排行、流拍列表
3. **用户报表：** 买卖家的成交记录、信用评分变化
4. **财务报表：** 收支明细、手续费收入、提现记录

## 10.3 导出功能

- **格式：** CSV、Excel (.xlsx)
- **异步导出：** 大数据量导出通过后台任务完成，完成后通知下载
- **保留时间：** 导出文件保留 7 天

# 11. 用户评价与反馈

## 11.1 评价规则

- 订单完成后 30 天内可评价
- 买卖双方互评，好评/中评/差评
- 评价内容可选，评分必填（1-5 星）
- 差评超过 3 条屏蔽商品展示
- 恶意差评可向管理员申诉

## 11.2 信用评分计算

```
信用评分 = SUM(评分) / COUNT(评价数)

初始值: 5.0 分
最低值: 1.0 分
最高值: 5.0 分
```

# 12. 安全性设计

## 12.1 认证与授权

- 所有 API 强制 HTTPS
- JWT Token 有效期 24 小时，支持 Refresh Token 续期
- 敏感操作（支付、发货）需要二次验证
- 管理员操作需记录审计日志

## 12.2 输入安全

- 参数校验：长度、格式、范围
- SQL 注入防护：参数化查询
- XSS 防护：输出编码
- CSRF 防护：Token 验证

## 12.3 并发安全

- 竞价：Redis Lua 原子脚本 + 乐观锁
- 库存：预扣减 + 最终一致性
- 幂等：唯一 Token 索引防止重复

## 12.4 限流策略

| 接口 | 限流阈值 |
|------|----------|
| 出价接口 | 10 次/秒/用户 |
| WebSocket 连接 | 1000 连接/IP |
| 通用 API | 100 次/分钟/用户 |

# 13. 系统测试计划

## 13.1 测试目的

验证功能正确性、并发性能和关键业务流程的稳定性。

## 13.2 测试范围

- **单元测试：** 竞价规则、价格校验、订单处理、时间轮逻辑
- **集成测试：** API 流程（发布->审核->加入拍卖->出价->支付->评价）
- **性能测试：** 并发出价 100~1000 并发，WebSocket 推送延迟
- **安全测试：** SQL 注入、XSS、越权访问、幂等性

## 13.3 典型测试用例

| 用例 ID | 描述 | 预期结果 |
|---------|------|----------|
| TC-01 | 用户发布物品，管理员审核通过 | 物品状态变为 active |
| TC-02 | 100 并发对同一拍品出价 | 最终只有 1 人成功，状态正确 |
| TC-03 | 拍卖结束前 50 秒出价 | 触发 anti-snipe，延时 120 秒 |
| TC-04 | 支付回调重复通知 | 订单状态仅更新一次，金额不重复扣减 |
| TC-05 | 用户余额不足时出价 | 返回错误码 20004 |
| TC-06 | 订单 48h 未支付 | 订单自动取消，冻结余额解冻 |

# 14. 部署与运维

## 14.1 环境需求

- **操作系统：** Ubuntu 20.04+ / CentOS 8+
- **编译器：** GCC 11+ / Clang 15+
- **CMake：** 3.20+
- **数据库：** MySQL 8.0+ / Redis 7.0+
- **Docker：** 20.10+ (可选)

## 14.2 编译构建

```bash
# 创建构建目录
mkdir build && cd build

# 配置 CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译
make -j$(nproc)

# 运行
./auction_server --config ../config/server.yaml
```

## 14.3 配置示例 (config/server.yaml)

```yaml
server:
  host: "0.0.0.0"
  port: 8080
  worker_threads: 4

database:
  host: "localhost"
  port: 3306
  username: "auction_user"
  password: "password"
  database: "auction_db"
  pool_size: 16

redis:
  host: "localhost"
  port: 6379
  pool_size: 32

jwt:
  secret: "your-256-bit-secret"
  expiry_hours: 24

auction:
  anti_snipe_N: 60
  anti_snipe_M: 120
  max_extensions: 10
  bid_timeout_seconds: 30
```

## 14.4 Docker 部署（可选）

```bash
# 构建镜像
docker build -t auction-server:latest .

# 运行容器
docker run -d -p 8080:8080 \
  -v /data/auction/logs:/var/log/auction \
  --link mysql --link redis \
  auction-server:latest
```

## 14.5 监控与告警

| 指标 | 告警阈值 | 告警方式 |
|------|----------|----------|
| CPU 使用率 | > 80% | 邮件 |
| 内存使用率 | > 85% | 邮件 |
| 竞价 QPS | < 1000 | 短信 |
| 支付失败率 | > 5% | 短信 |
| WebSocket 连接数 | < 1000 | 邮件 |

# 15. 术语与参考资料

| 术语 | 英文 | 定义 |
|------|------|------|
| 竞价撮合 | Bid Matching | 核心引擎处理出价并确定最高价 |
| 原子出价 | Atomic Bidding | 保证并发出价时仅一个成功 |
| 防秒杀 | Anti-Sniping | 延时机制防止最后时刻秒杀 |
| 时间轮 | Timing Wheel | 高效定时任务数据结构 |
| 双缓冲 | Double Buffering | 异步持久化避免阻塞 |
| 幂等性 | Idempotency | 重复请求结果一致 |

**参考资料：**
- Drogon 框架文档：https://github.com/an-tao/drogon
- Workflow 框架文档：https://github.com/sogou/workflow
- Redis Lua 脚本：https://redis.io/commands/eval
- Google Test：https://github.com/google/googletest

------

*本规格说明书为课程设计项目文档，随着开发进展可能持续更新。*
