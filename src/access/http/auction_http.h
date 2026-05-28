#pragma once

#include "modules/auction/auction_service.h"

namespace auction::access::http {

void RegisterAuctionHttpRoutes(
    modules::auction::AuctionService& auction_service
);

}  // namespace auction::access::http
