#!/usr/bin/env bash
set -euo pipefail

cat <<'EOF'
在线拍卖平台 S15 演示脚本

1. 初始化演示环境
   scripts/deploy/init_demo_env.sh

2. 启动后端服务
   scripts/deploy/run_demo_server.sh

3. 健康检查
   curl http://127.0.0.1:18080/healthz

4. 演示账号
   admin / Admin@123
   support / Support@123
   seller_demo / Seller@123
   buyer_demo / Buyer@123
   buyer_demo_2 / BuyerB@123

5. 答辩展示顺序
   需求边界: 校园或小型社区在线拍卖平台
   架构方案: C++20 + Drogon + MySQL + 模块化单体
   主流程: 拍品发布、审核、创建拍卖、出价、结束、订单、支付、履约、评价
   高风险: 并发出价、拍卖结束竞争、支付回调幂等、通知失败不回滚
   可观测: operation_log、task_log、notification、statistics_daily
   验证结果: scripts/test.sh risk 和全量 ctest 均已通过
EOF
