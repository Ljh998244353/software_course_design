#pragma once

#include "Order.h"
#include "MySQLPool.h"
#include <optional>
#include <string>
#include <vector>
#include <memory>

namespace auction {

class OrderService {
public:
    static OrderService& instance();

    std::optional<Order> createOrder(int64_t auction_item_id,
                                   int64_t buyer_id,
                                   int64_t seller_id,
                                   double final_price);

    std::optional<Order> getOrderById(int64_t order_id);

    std::vector<Order> getOrdersByUser(int64_t user_id,
                                      const std::string& role,
                                      OrderStatus status = OrderStatus::PENDING_PAYMENT,
                                      int page = 1,
                                      int page_size = 20);

    bool updateOrderStatus(int64_t order_id, OrderStatus status);

    bool markAsPaid(int64_t order_id, const std::string& payment_method);
    bool markAsShipped(int64_t order_id,
                      const std::string& tracking_no,
                      const std::string& carrier);
    bool markAsCompleted(int64_t order_id);
    bool cancelOrder(int64_t order_id);

private:
    OrderService() = default;

    std::string generateOrderNo();
    Order rowToOrder(std::shared_ptr<MySQLPool::ResultSet>& result);
    static OrderStatus stringToStatus(const std::string& status);
    static std::string statusToString(OrderStatus status);
};

} // namespace auction
