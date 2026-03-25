#pragma once

#include "Item.h"
#include "MySQLPool.h"
#include <optional>
#include <string>
#include <vector>
#include <memory>

namespace auction {

class ItemService {
public:
    static ItemService& instance();

    std::optional<Item> createItem(int64_t seller_id,
                                  const std::string& title,
                                  const std::string& description,
                                  int category_id,
                                  double start_price,
                                  double reserve_price,
                                  double bid_step,
                                  const std::vector<std::string>& images);

    std::optional<Item> getItemById(int64_t item_id);

    bool updateItemStatus(int64_t item_id, ItemStatus status);
    bool submitForReview(int64_t item_id);
    bool approveItem(int64_t item_id);
    bool rejectItem(int64_t item_id);

    std::vector<Item> getItemsBySeller(int64_t seller_id, ItemStatus status,
                                      int page = 1, int page_size = 20);
    std::vector<Item> getItemsByCategory(int category_id, ItemStatus status,
                                        int page = 1, int page_size = 20);

private:
    ItemService() = default;

    Item rowToItem(std::shared_ptr<MySQLPool::ResultSet>& result);
    static ItemStatus stringToStatus(const std::string& status);
    static std::string statusToString(ItemStatus status);
    static std::string join(const std::vector<std::string>& vec, const std::string& delim);
};

} // namespace auction
