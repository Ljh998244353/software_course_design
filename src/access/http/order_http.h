#pragma once

#include "modules/order/order_service.h"
#include "modules/payment/payment_service.h"

namespace auction::access::http {

void RegisterOrderHttpRoutes(
    modules::order::OrderService& order_service,
    modules::payment::PaymentService& payment_service
);

}  // namespace auction::access::http
