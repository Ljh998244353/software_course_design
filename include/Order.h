#pragma once

#include <string>
#include <cstdint>
#include <ctime>

namespace auction {

enum class OrderStatus {
    PENDING_PAYMENT,
    PAID,
    SHIPPED,
    COMPLETED,
    CANCELLED,
    TIMEOUT
};

class Order {
public:
    int64_t id;
    std::string order_no;
    int64_t auction_item_id;
    int64_t buyer_id;
    int64_t seller_id;
    double final_price;
    double commission;
    OrderStatus status;
    time_t payment_deadline;
    time_t paid_at;
    time_t shipped_at;
    time_t completed_at;
    time_t created_at;
    time_t updated_at;

    Order() : id(0), auction_item_id(0), buyer_id(0), seller_id(0),
              final_price(0.0), commission(0.0), status(OrderStatus::PENDING_PAYMENT),
              payment_deadline(0), paid_at(0), shipped_at(0), completed_at(0),
              created_at(0), updated_at(0) {}

    bool isPendingPayment() const { return status == OrderStatus::PENDING_PAYMENT; }
    bool canBeCancelled() const { return status == OrderStatus::PENDING_PAYMENT; }
    bool isBuyer(int64_t user_id) const { return buyer_id == user_id; }
    bool isSeller(int64_t user_id) const { return seller_id == user_id; }
};

} // namespace auction
