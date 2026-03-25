#include "ItemService.h"
#include "MySQLPool.h"
#include "Logger.h"

namespace auction {

ItemService& ItemService::instance() {
    static ItemService instance;
    return instance;
}

std::optional<Item> ItemService::createItem(int64_t seller_id,
                                           const std::string& title,
                                           const std::string& description,
                                           int category_id,
                                           double start_price,
                                           double reserve_price,
                                           double bid_step,
                                           const std::vector<std::string>& images) {
    (void)seller_id;
    (void)title;
    (void)description;
    (void)category_id;
    (void)start_price;
    (void)reserve_price;
    (void)bid_step;
    (void)images;

    Item item;
    item.title = title;
    item.description = description;
    item.category_id = category_id;
    item.start_price = start_price;
    item.reserve_price = reserve_price;
    item.bid_step = bid_step;
    item.images = images;
    item.status = ItemStatus::DRAFT;
    return item;
}

std::optional<Item> ItemService::getItemById(int64_t item_id) {
    (void)item_id;
    Item item;
    item.id = item_id;
    return item;
}

bool ItemService::updateItemStatus(int64_t item_id, ItemStatus status) {
    (void)item_id;
    (void)status;
    return true;
}

bool ItemService::submitForReview(int64_t item_id) {
    return updateItemStatus(item_id, ItemStatus::PENDING_REVIEW);
}

bool ItemService::approveItem(int64_t item_id) {
    return updateItemStatus(item_id, ItemStatus::ACTIVE);
}

bool ItemService::rejectItem(int64_t item_id) {
    return updateItemStatus(item_id, ItemStatus::REJECTED);
}

std::vector<Item> ItemService::getItemsBySeller(int64_t seller_id,
                                               ItemStatus status,
                                               int page,
                                               int page_size) {
    (void)seller_id;
    (void)status;
    (void)page;
    (void)page_size;
    return {};
}

std::vector<Item> ItemService::getItemsByCategory(int category_id,
                                                  ItemStatus status,
                                                  int page,
                                                  int page_size) {
    (void)category_id;
    (void)status;
    (void)page;
    (void)page_size;
    return {};
}

Item ItemService::rowToItem(std::shared_ptr<MySQLPool::ResultSet>& result) {
    (void)result;
    Item item;
    return item;
}

ItemStatus ItemService::stringToStatus(const std::string& status) {
    if (status == "draft") return ItemStatus::DRAFT;
    if (status == "pending_review") return ItemStatus::PENDING_REVIEW;
    if (status == "active") return ItemStatus::ACTIVE;
    if (status == "closed") return ItemStatus::CLOSED;
    if (status == "rejected") return ItemStatus::REJECTED;
    return ItemStatus::DRAFT;
}

std::string ItemService::statusToString(ItemStatus status) {
    switch (status) {
        case ItemStatus::DRAFT: return "draft";
        case ItemStatus::PENDING_REVIEW: return "pending_review";
        case ItemStatus::ACTIVE: return "active";
        case ItemStatus::CLOSED: return "closed";
        case ItemStatus::REJECTED: return "rejected";
        default: return "draft";
    }
}

std::string ItemService::join(const std::vector<std::string>& vec, const std::string& delim) {
    std::string result;
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i > 0) result += delim;
        result += vec[i];
    }
    return result;
}

} // namespace auction
