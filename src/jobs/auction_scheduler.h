#pragma once

#include "modules/auction/auction_service.h"

namespace auction::jobs {

class AuctionScheduler {
public:
    explicit AuctionScheduler(modules::auction::AuctionService& auction_service);

    [[nodiscard]] modules::auction::AuctionScheduleResult RunStartCycle();
    [[nodiscard]] modules::auction::AuctionScheduleResult RunFinishCycle();

private:
    modules::auction::AuctionService* auction_service_;
};

}  // namespace auction::jobs
