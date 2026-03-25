#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <ctime>

namespace auction {

enum class ItemStatus {
    DRAFT,
    PENDING_REVIEW,
    ACTIVE,
    CLOSED,
    REJECTED
};

class Item {
public:
    int64_t id;
    int64_t seller_id;
    int category_id;
    std::string title;
    std::string description;
    double start_price;
    double reserve_price;
    double bid_step;
    std::vector<std::string> images;
    ItemStatus status;
    int version;
    time_t created_at;
    time_t updated_at;

    Item() : id(0), seller_id(0), category_id(0),
             start_price(0.0), reserve_price(0.0), bid_step(1.0),
             status(ItemStatus::DRAFT), version(0),
             created_at(0), updated_at(0) {}

    bool canBeReviewed() const { return status == ItemStatus::PENDING_REVIEW; }
    bool isActive() const { return status == ItemStatus::ACTIVE; }
    bool isOwnedBy(int64_t user_id) const { return seller_id == user_id; }
};

} // namespace auction
