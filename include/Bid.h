#pragma once

#include <string>
#include <cstdint>
#include <ctime>

namespace auction {

class Bid {
public:
    int64_t id;
    int64_t auction_item_id;
    int64_t bidder_id;
    double amount;
    time_t created_at;
    std::string idempotent_token;

    Bid() : id(0), auction_item_id(0), bidder_id(0), amount(0.0), created_at(0) {}
};

} // namespace auction
