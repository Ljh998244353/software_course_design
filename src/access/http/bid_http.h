#pragma once

#include "modules/bid/bid_service.h"

namespace auction::access::http {

void RegisterBidHttpRoutes(
    modules::bid::BidService& bid_service
);

}  // namespace auction::access::http
