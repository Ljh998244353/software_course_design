#!/usr/bin/env bash
set -euo pipefail

cat <<'EOF'
在线拍卖平台 S29 演示脚本

1. 初始化演示环境
   scripts/deploy/init_demo_env.sh

2. 启动后端服务
   scripts/deploy/run_demo_server.sh

3. 浏览器演示入口
   http://127.0.0.1:18080/demo
   http://127.0.0.1:18080/app

4. 健康检查
   curl http://127.0.0.1:18080/healthz

5. 演示数据接口
   curl http://127.0.0.1:18080/api/demo/dashboard

6. 演示账号
   admin / Admin@123
   support / Support@123
   seller_demo / Seller@123
   buyer_demo / Buyer@123
   buyer_demo_2 / BuyerB@123

7. 答辩展示顺序
   需求边界: 校园或小型社区在线拍卖平台
   架构方案: C++20 + Drogon + MySQL + 模块化单体
   只读演示: 打开 /demo，点击“播放演示顺序”
   真实业务前端: 打开 /app，使用演示账号登录
   管理员: admin / Admin@123，展示审核、拍卖配置、统计和运维入口
   卖家: seller_demo / Seller@123，展示拍品管理和订单履约入口
   买家: buyer_demo / Buyer@123，展示拍卖大厅、出价、订单和评价入口
   运维/客服: support / Support@123，展示日志和异常看板入口
   主流程: 拍品发布、审核、创建拍卖、出价、结束、订单、支付、履约、评价
   高风险: 并发出价、拍卖结束竞争、支付回调幂等、通知失败不回滚
   可观测: operation_log、task_log、notification、statistics_daily
   验证结果: scripts/test.sh frontend、ui、http、risk 和全量 ctest 均已通过

8. 前端与发布验证
   scripts/test.sh frontend
   scripts/test.sh ui
   scripts/test.sh http
   scripts/deploy/verify_release.sh
EOF
