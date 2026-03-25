#include "OrderService.h"
#include "MySQLPool.h"
#include "UserService.h"
#include "Logger.h"
#include <sstream>
#include <chrono>

namespace auction {

OrderService& OrderService::instance() {
    static OrderService instance;
    return instance;
}

std::optional<Order> OrderService::createOrder(int64_t auction_item_id,
                                              int64_t buyer_id,
                                              int64_t seller_id,
                                              double final_price) {
    (void)auction_item_id;
    (void)buyer_id;
    (void)seller_id;
    (void)final_price;

    std::string order_no = generateOrderNo();
    double commission = final_price * 0.05;

    Order order;
    order.order_no = order_no;
    order.auction_item_id = auction_item_id;
    order.buyer_id = buyer_id;
    order.seller_id = seller_id;
    order.final_price = final_price;
    order.commission = commission;
    order.status = OrderStatus::PENDING_PAYMENT;
    return order;
}

std::optional<Order> OrderService::getOrderById(int64_t order_id) {
    (void)order_id;
    Order order;
    order.id = order_id;
    return order;
}

std::vector<Order> OrderService::getOrdersByUser(int64_t user_id,
                                                const std::string& role,
                                                OrderStatus status,
                                                int page,
                                                int page_size) {
    (void)user_id;
    (void)role;
    (void)status;
    (void)page;
    (void)page_size;
    return {};
}

bool OrderService::updateOrderStatus(int64_t order_id, OrderStatus status) {
    (void)order_id;
    (void)status;
    return true;
}

bool OrderService::markAsPaid(int64_t order_id, const std::string& payment_method) {
    auto order = getOrderById(order_id);
    if (!order || !order->isPendingPayment()) {
        return false;
    }

    if (updateOrderStatus(order_id, OrderStatus::PAID)) {
        AUCTION_LOG_INFO("Order %lld paid", (long long)order_id);
        return true;
    }
    return false;
}

bool OrderService::markAsShipped(int64_t order_id,
                                const std::string& tracking_no,
                                const std::string& carrier) {
    auto order = getOrderById(order_id);
    if (!order || order->status != OrderStatus::PAID) {
        return false;
    }

    if (updateOrderStatus(order_id, OrderStatus::SHIPPED)) {
        AUCTION_LOG_INFO("Order %lld shipped", (long long)order_id);
        return true;
    }
    return false;
}

bool OrderService::markAsCompleted(int64_t order_id) {
    auto order = getOrderById(order_id);
    if (!order || order->status != OrderStatus::SHIPPED) {
        return false;
    }

    if (updateOrderStatus(order_id, OrderStatus::COMPLETED)) {
        AUCTION_LOG_INFO("Order %lld completed", (long long)order_id);
        return true;
    }
    return false;
}

bool OrderService::cancelOrder(int64_t order_id) {
    auto order = getOrderById(order_id);
    if (!order || !order->canBeCancelled()) {
        return false;
    }

    if (updateOrderStatus(order_id, OrderStatus::CANCELLED)) {
        AUCTION_LOG_INFO("Order %lld cancelled", (long long)order_id);
        return true;
    }
    return false;
}

std::string OrderService::generateOrderNo() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    std::ostringstream oss;
    oss << "ORD" << time_t << (rand() % 10000);
    return oss.str();
}

Order OrderService::rowToOrder(std::shared_ptr<MySQLPool::ResultSet>& result) {
    (void)result;
    Order order;
    return order;
}

OrderStatus OrderService::stringToStatus(const std::string& status) {
    if (status == "pending_payment") return OrderStatus::PENDING_PAYMENT;
    if (status == "paid") return OrderStatus::PAID;
    if (status == "shipped") return OrderStatus::SHIPPED;
    if (status == "completed") return OrderStatus::COMPLETED;
    if (status == "cancelled") return OrderStatus::CANCELLED;
    if (status == "timeout") return OrderStatus::TIMEOUT;
    return OrderStatus::PENDING_PAYMENT;
}

std::string OrderService::statusToString(OrderStatus status) {
    switch (status) {
        case OrderStatus::PENDING_PAYMENT: return "pending_payment";
        case OrderStatus::PAID: return "paid";
        case OrderStatus::SHIPPED: return "shipped";
        case OrderStatus::COMPLETED: return "completed";
        case OrderStatus::CANCELLED: return "cancelled";
        case OrderStatus::TIMEOUT: return "timeout";
        default: return "pending_payment";
    }
}

} // namespace auction
