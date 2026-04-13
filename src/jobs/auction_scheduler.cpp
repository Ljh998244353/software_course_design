#include "jobs/auction_scheduler.h"

namespace auction::jobs {

AuctionScheduler::AuctionScheduler(modules::auction::AuctionService& auction_service)
    : auction_service_(&auction_service) {}

modules::auction::AuctionScheduleResult AuctionScheduler::RunStartCycle() {
    return auction_service_->StartDueAuctions();
}

modules::auction::AuctionScheduleResult AuctionScheduler::RunFinishCycle() {
    return auction_service_->FinishDueAuctions();
}

}  // namespace auction::jobs
