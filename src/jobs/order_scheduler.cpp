#include "jobs/order_scheduler.h"

namespace auction::jobs {

OrderScheduler::OrderScheduler(modules::order::OrderService& order_service)
    : order_service_(&order_service) {}

modules::order::OrderScheduleResult OrderScheduler::RunSettlementCycle() {
    return order_service_->GenerateSettlementOrders();
}

modules::order::OrderScheduleResult OrderScheduler::RunTimeoutCloseCycle() {
    return order_service_->CloseExpiredOrders();
}

}  // namespace auction::jobs
