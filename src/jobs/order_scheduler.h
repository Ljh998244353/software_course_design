#pragma once

#include "modules/order/order_service.h"

namespace auction::jobs {

class OrderScheduler {
public:
    explicit OrderScheduler(modules::order::OrderService& order_service);

    [[nodiscard]] modules::order::OrderScheduleResult RunSettlementCycle();
    [[nodiscard]] modules::order::OrderScheduleResult RunTimeoutCloseCycle();

private:
    modules::order::OrderService* order_service_;
};

}  // namespace auction::jobs
