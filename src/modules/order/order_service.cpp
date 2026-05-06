#include "modules/order/order_service.h"

#include <chrono>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

#include "common/errors/error_code.h"
#include "common/logging/logger.h"
#include "modules/auction/auction_types.h"
#include "modules/item/item_types.h"
#include "modules/order/order_exception.h"
#include "repository/order_repository.h"

namespace auction::modules::order {

namespace {

std::string CurrentTimestampText() {
    const auto now = std::chrono::system_clock::now();
    const auto time_value = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm{};
    localtime_r(&time_value, &local_tm);

    std::ostringstream output;
    output << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
    return output.str();
}

std::string CurrentTimestampPlusMinutesText(const int minutes) {
    const auto now = std::chrono::system_clock::now() + std::chrono::minutes(minutes);
    const auto time_value = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm{};
    localtime_r(&time_value, &local_tm);

    std::ostringstream output;
    output << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
    return output.str();
}

std::string GenerateBusinessNo(const std::string_view prefix) {
    const auto suffix = std::chrono::duration_cast<std::chrono::microseconds>(
                            std::chrono::system_clock::now().time_since_epoch()
    )
                            .count();
    return std::string(prefix) + std::to_string(suffix);
}

std::string FormatAmount(const double value) {
    std::ostringstream output;
    output << std::fixed << std::setprecision(2) << (std::round(value * 100.0) / 100.0);
    return output.str();
}

void ThrowOrderError(const common::errors::ErrorCode code, const std::string_view message) {
    throw OrderException(code, std::string(message));
}

repository::OrderRepository MakeRepository(common::db::MysqlConnection& connection) {
    return repository::OrderRepository(connection);
}

UserOrderQuery NormalizeUserOrderQuery(const UserOrderQuery& query) {
    auto normalized = query;
    if (normalized.role == "all") {
        normalized.role.clear();
    }
    if (!normalized.role.empty() && normalized.role != "buyer" && normalized.role != "seller") {
        throw std::invalid_argument("role must be buyer, seller or all");
    }
    if (!normalized.order_status.empty() && !IsKnownOrderStatus(normalized.order_status)) {
        throw std::invalid_argument("order status is invalid");
    }
    if (normalized.page_no <= 0) {
        throw std::invalid_argument("pageNo must be a positive integer");
    }
    if (normalized.page_size <= 0 || normalized.page_size > 100) {
        throw std::invalid_argument("pageSize must be between 1 and 100");
    }
    return normalized;
}

void InsertTaskLogSafely(
    repository::OrderRepository& repository,
    const repository::OrderTaskLogParams& params
) {
    try {
        repository.InsertTaskLog(params);
    } catch (...) {
    }
}

void InsertOperationLogSafely(
    repository::OrderRepository& repository,
    const std::optional<std::uint64_t>& operator_id,
    const std::string& operator_role,
    const std::string& operation_name,
    const std::string& biz_key,
    const std::string& result,
    const std::string& detail
) {
    try {
        repository.InsertOperationLog(
            operator_id,
            operator_role,
            operation_name,
            biz_key,
            result,
            detail
        );
    } catch (...) {
    }
}

void CreateStationNoticeSafely(
    notification::NotificationService& notification_service,
    const notification::StationNoticeRequest& request
) {
    try {
        notification_service.CreateStationNotice(request);
    } catch (const std::exception& exception) {
        common::logging::Logger::Instance().Warn(
            "station notice create failed: " + std::string(exception.what())
        );
    }
}

std::string DescribeException(const std::exception& exception) {
    return exception.what();
}

}  // namespace

OrderService::OrderService(
    const common::config::AppConfig& config,
    std::filesystem::path project_root,
    middleware::AuthMiddleware& auth_middleware,
    notification::NotificationService& notification_service
) : mysql_config_(config.mysql),
    project_root_(std::move(project_root)),
    auth_middleware_(&auth_middleware),
    notification_service_(&notification_service) {}

UserOrderListResult OrderService::ListMyOrders(
    const std::string_view authorization_header,
    const UserOrderQuery& query
) {
    const auto auth_context = auth_middleware_->RequireAnyRole(
        authorization_header,
        {modules::auth::kRoleUser}
    );
    const auto normalized_query = NormalizeUserOrderQuery(query);

    auto connection = CreateConnection();
    auto repository = MakeRepository(connection);
    return UserOrderListResult{
        .records = repository.ListUserOrders(auth_context.user_id, normalized_query),
        .page_no = normalized_query.page_no,
        .page_size = normalized_query.page_size,
    };
}

UserOrderEntry OrderService::GetMyOrder(
    const std::string_view authorization_header,
    const std::uint64_t order_id
) {
    const auto auth_context = auth_middleware_->RequireAnyRole(
        authorization_header,
        {modules::auth::kRoleUser}
    );

    auto connection = CreateConnection();
    auto repository = MakeRepository(connection);
    const auto order = repository.FindUserOrderDetailById(order_id);
    if (!order.has_value()) {
        ThrowOrderError(common::errors::ErrorCode::kOrderNotFound, "order not found");
    }
    if (order->order.buyer_id != auth_context.user_id &&
        order->order.seller_id != auth_context.user_id) {
        ThrowOrderError(
            common::errors::ErrorCode::kOrderOwnerMismatch,
            "user does not own this order"
        );
    }
    return *order;
}

OrderScheduleResult OrderService::GenerateSettlementOrders() {
    auto connection = CreateConnection();
    auto repository = MakeRepository(connection);
    const auto auction_ids = repository.ListSettlingAuctionIds();

    OrderScheduleResult result;
    result.scanned = static_cast<int>(auction_ids.size());

    for (const auto auction_id : auction_ids) {
        bool transaction_started = false;
        std::string scheduled_at;
        std::optional<OrderRecord> created_order;
        std::optional<repository::SettlementAuctionAggregate> aggregate_snapshot;
        try {
            connection.BeginTransaction();
            transaction_started = true;

            const auto aggregate = repository.FindSettlementAuctionByIdForUpdate(auction_id);
            if (!aggregate.has_value()) {
                connection.Rollback();
                transaction_started = false;
                ++result.skipped;
                InsertTaskLogSafely(
                    repository,
                    repository::OrderTaskLogParams{
                        .task_type = std::string(kTaskTypeOrderSettlement),
                        .biz_key = std::to_string(auction_id),
                        .task_status = std::string(kTaskStatusSkipped),
                        .retry_count = 0,
                        .last_error = "settling auction not found",
                        .scheduled_at = "",
                    }
                );
                continue;
            }

            aggregate_snapshot = *aggregate;
            scheduled_at = aggregate->end_time;

            if (aggregate->status != modules::auction::kAuctionStatusSettling) {
                connection.Rollback();
                transaction_started = false;
                ++result.skipped;
                InsertTaskLogSafely(
                    repository,
                    repository::OrderTaskLogParams{
                        .task_type = std::string(kTaskTypeOrderSettlement),
                        .biz_key = std::to_string(auction_id),
                        .task_status = std::string(kTaskStatusSkipped),
                        .retry_count = 0,
                        .last_error = "auction status changed before order settlement",
                        .scheduled_at = scheduled_at,
                    }
                );
                continue;
            }

            if (const auto existing_order = repository.FindOrderByAuctionId(auction_id);
                existing_order.has_value()) {
                connection.Rollback();
                transaction_started = false;
                ++result.skipped;
                InsertTaskLogSafely(
                    repository,
                    repository::OrderTaskLogParams{
                        .task_type = std::string(kTaskTypeOrderSettlement),
                        .biz_key = std::to_string(auction_id),
                        .task_status = std::string(kTaskStatusSkipped),
                        .retry_count = 0,
                        .last_error = "order already exists for settling auction",
                        .scheduled_at = scheduled_at,
                    }
                );
                continue;
            }

            if (!aggregate->highest_bidder_id.has_value()) {
                repository.UpdateAuctionStatus(
                    auction_id,
                    std::string(modules::auction::kAuctionStatusUnsold)
                );
                repository.UpdateItemStatus(
                    aggregate->item_id,
                    std::string(modules::item::kItemStatusUnsold)
                );
                repository.InsertOperationLog(
                    std::nullopt,
                    "SYSTEM",
                    "generate_order",
                    std::to_string(auction_id),
                    "FAILED",
                    "settling auction has no highest bidder, fallback to unsold"
                );
                connection.Commit();
                transaction_started = false;

                ++result.succeeded;
                InsertTaskLogSafely(
                    repository,
                    repository::OrderTaskLogParams{
                        .task_type = std::string(kTaskTypeOrderSettlement),
                        .biz_key = std::to_string(auction_id),
                        .task_status = std::string(kTaskStatusSuccess),
                        .retry_count = 0,
                        .last_error = "",
                        .scheduled_at = scheduled_at,
                    }
                );
                continue;
            }

            if (aggregate->item_status != modules::item::kItemStatusInAuction) {
                ThrowOrderError(
                    common::errors::ErrorCode::kOrderSettlementConflict,
                    "item status does not allow settlement order generation"
                );
            }

            created_order = repository.CreateOrder(repository::CreateOrderParams{
                .order_no = GenerateBusinessNo("ORD"),
                .auction_id = auction_id,
                .buyer_id = *aggregate->highest_bidder_id,
                .seller_id = aggregate->seller_id,
                .final_amount = FormatAmount(aggregate->current_price),
                .order_status = std::string(kOrderStatusPendingPayment),
                .pay_deadline_at = CurrentTimestampPlusMinutesText(kDefaultPayTimeoutMinutes),
            });
            repository.InsertOperationLog(
                std::nullopt,
                "SYSTEM",
                "generate_order",
                std::to_string(created_order->order_id),
                "SUCCESS",
                "auction_id=" + std::to_string(auction_id)
            );
            connection.Commit();
            transaction_started = false;

            ++result.succeeded;
            result.affected_order_ids.push_back(created_order->order_id);
            InsertTaskLogSafely(
                repository,
                repository::OrderTaskLogParams{
                    .task_type = std::string(kTaskTypeOrderSettlement),
                    .biz_key = std::to_string(auction_id),
                    .task_status = std::string(kTaskStatusSuccess),
                    .retry_count = 0,
                    .last_error = "",
                    .scheduled_at = scheduled_at,
                }
            );
        } catch (const std::exception& exception) {
            if (transaction_started) {
                connection.Rollback();
            }
            created_order.reset();
            ++result.failed;
            InsertTaskLogSafely(
                repository,
                repository::OrderTaskLogParams{
                    .task_type = std::string(kTaskTypeOrderSettlement),
                    .biz_key = std::to_string(auction_id),
                    .task_status = std::string(kTaskStatusFailed),
                    .retry_count = 0,
                    .last_error = DescribeException(exception),
                    .scheduled_at = scheduled_at,
                }
            );
            InsertOperationLogSafely(
                repository,
                std::nullopt,
                "SYSTEM",
                "generate_order",
                std::to_string(auction_id),
                "FAILED",
                DescribeException(exception)
            );
        }

        if (!created_order.has_value() || !aggregate_snapshot.has_value()) {
            continue;
        }

        CreateStationNoticeSafely(
            *notification_service_,
            notification::StationNoticeRequest{
                .user_id = created_order->buyer_id,
                .notice_type = "ORDER_PENDING_PAYMENT",
                .title = "Auction won, payment required",
                .content = "Order " + created_order->order_no + " for item \"" +
                           aggregate_snapshot->item_title + "\" is waiting for payment before " +
                           created_order->pay_deadline_at + ".",
                .biz_type = "ORDER",
                .biz_id = created_order->order_id,
            }
        );
        CreateStationNoticeSafely(
            *notification_service_,
            notification::StationNoticeRequest{
                .user_id = created_order->seller_id,
                .notice_type = "ORDER_CREATED",
                .title = "Auction settled, waiting buyer payment",
                .content = "Order " + created_order->order_no + " has been generated for item \"" +
                           aggregate_snapshot->item_title + "\".",
                .biz_type = "ORDER",
                .biz_id = created_order->order_id,
            }
        );
    }

    return result;
}

OrderScheduleResult OrderService::CloseExpiredOrders() {
    auto connection = CreateConnection();
    auto repository = MakeRepository(connection);
    const auto order_ids = repository.ListExpiredPendingOrderIds();

    OrderScheduleResult result;
    result.scanned = static_cast<int>(order_ids.size());

    for (const auto order_id : order_ids) {
        bool transaction_started = false;
        bool closed_successfully = false;
        std::string scheduled_at;
        std::optional<repository::ExpiredOrderAggregate> aggregate_snapshot;
        try {
            connection.BeginTransaction();
            transaction_started = true;

            const auto aggregate = repository.FindExpiredOrderAggregateByIdForUpdate(order_id);
            if (!aggregate.has_value()) {
                connection.Rollback();
                transaction_started = false;
                ++result.skipped;
                InsertTaskLogSafely(
                    repository,
                    repository::OrderTaskLogParams{
                        .task_type = std::string(kTaskTypeOrderTimeoutClose),
                        .biz_key = std::to_string(order_id),
                        .task_status = std::string(kTaskStatusSkipped),
                        .retry_count = 0,
                        .last_error = "order not found for timeout close",
                        .scheduled_at = "",
                    }
                );
                continue;
            }

            aggregate_snapshot = *aggregate;
            scheduled_at = aggregate->order.pay_deadline_at;

            if (aggregate->order.order_status != kOrderStatusPendingPayment) {
                connection.Rollback();
                transaction_started = false;
                ++result.skipped;
                InsertTaskLogSafely(
                    repository,
                    repository::OrderTaskLogParams{
                        .task_type = std::string(kTaskTypeOrderTimeoutClose),
                        .biz_key = std::to_string(order_id),
                        .task_status = std::string(kTaskStatusSkipped),
                        .retry_count = 0,
                        .last_error = "order status changed before timeout close",
                        .scheduled_at = scheduled_at,
                    }
                );
                continue;
            }

            if (!repository.IsOrderPaymentDeadlineExpired(order_id)) {
                connection.Rollback();
                transaction_started = false;
                ++result.skipped;
                InsertTaskLogSafely(
                    repository,
                    repository::OrderTaskLogParams{
                        .task_type = std::string(kTaskTypeOrderTimeoutClose),
                        .biz_key = std::to_string(order_id),
                        .task_status = std::string(kTaskStatusSkipped),
                        .retry_count = 0,
                        .last_error = "payment deadline is no longer due",
                        .scheduled_at = scheduled_at,
                    }
                );
                continue;
            }

            if (aggregate->auction_status != modules::auction::kAuctionStatusSettling) {
                ThrowOrderError(
                    common::errors::ErrorCode::kOrderSettlementConflict,
                    "auction status does not allow timeout close"
                );
            }
            if (aggregate->item_status != modules::item::kItemStatusInAuction) {
                ThrowOrderError(
                    common::errors::ErrorCode::kOrderSettlementConflict,
                    "item status does not allow timeout close"
                );
            }

            repository.CloseOpenPaymentsByOrder(order_id);
            repository.UpdateOrderStatusToClosed(order_id);
            repository.UpdateAuctionStatus(
                aggregate->auction_id,
                std::string(modules::auction::kAuctionStatusUnsold)
            );
            repository.UpdateItemStatus(
                aggregate->item_id,
                std::string(modules::item::kItemStatusUnsold)
            );
            repository.InsertOperationLog(
                std::nullopt,
                "SYSTEM",
                "close_order_timeout",
                std::to_string(order_id),
                "SUCCESS",
                "auction_id=" + std::to_string(aggregate->auction_id)
            );
            connection.Commit();
            transaction_started = false;
            closed_successfully = true;

            ++result.succeeded;
            result.affected_order_ids.push_back(order_id);
            InsertTaskLogSafely(
                repository,
                repository::OrderTaskLogParams{
                    .task_type = std::string(kTaskTypeOrderTimeoutClose),
                    .biz_key = std::to_string(order_id),
                    .task_status = std::string(kTaskStatusSuccess),
                    .retry_count = 0,
                    .last_error = "",
                    .scheduled_at = scheduled_at,
                }
            );
        } catch (const std::exception& exception) {
            if (transaction_started) {
                connection.Rollback();
            }
            ++result.failed;
            InsertTaskLogSafely(
                repository,
                repository::OrderTaskLogParams{
                    .task_type = std::string(kTaskTypeOrderTimeoutClose),
                    .biz_key = std::to_string(order_id),
                    .task_status = std::string(kTaskStatusFailed),
                    .retry_count = 0,
                    .last_error = DescribeException(exception),
                    .scheduled_at = scheduled_at,
                }
            );
            InsertOperationLogSafely(
                repository,
                std::nullopt,
                "SYSTEM",
                "close_order_timeout",
                std::to_string(order_id),
                "FAILED",
                DescribeException(exception)
            );
        }

        if (!closed_successfully || !aggregate_snapshot.has_value()) {
            continue;
        }

        CreateStationNoticeSafely(
            *notification_service_,
            notification::StationNoticeRequest{
                .user_id = aggregate_snapshot->order.buyer_id,
                .notice_type = "ORDER_CLOSED",
                .title = "Order closed due to payment timeout",
                .content = "Order " + aggregate_snapshot->order.order_no + " for item \"" +
                           aggregate_snapshot->item_title +
                           "\" has been closed because payment timed out.",
                .biz_type = "ORDER",
                .biz_id = aggregate_snapshot->order.order_id,
            }
        );
        CreateStationNoticeSafely(
            *notification_service_,
            notification::StationNoticeRequest{
                .user_id = aggregate_snapshot->order.seller_id,
                .notice_type = "ORDER_TIMEOUT",
                .title = "Buyer payment timed out",
                .content = "Order " + aggregate_snapshot->order.order_no + " for item \"" +
                           aggregate_snapshot->item_title +
                           "\" has been closed because the buyer did not pay in time.",
                .biz_type = "ORDER",
                .biz_id = aggregate_snapshot->order.order_id,
            }
        );
    }

    return result;
}

OrderTransitionResult OrderService::ShipOrder(
    const std::string_view authorization_header,
    const std::uint64_t order_id
) {
    const auto auth_context = auth_middleware_->RequireAnyRole(
        authorization_header,
        {modules::auth::kRoleUser}
    );

    auto connection = CreateConnection();
    auto repository = MakeRepository(connection);
    bool transaction_started = false;
    try {
        connection.BeginTransaction();
        transaction_started = true;

        const auto order = repository.FindOrderByIdForUpdate(order_id);
        if (!order.has_value()) {
            ThrowOrderError(common::errors::ErrorCode::kOrderNotFound, "order not found");
        }
        if (order->seller_id != auth_context.user_id) {
            ThrowOrderError(
                common::errors::ErrorCode::kOrderOwnerMismatch,
                "user does not own this order as seller"
            );
        }
        if (order->order_status != kOrderStatusPaid) {
            ThrowOrderError(
                common::errors::ErrorCode::kOrderStatusInvalid,
                "order status does not allow shipment confirmation"
            );
        }

        repository.UpdateOrderStatusToShipped(order_id);
        repository.InsertOperationLog(
            auth_context.user_id,
            auth_context.role_code,
            "ship_order",
            std::to_string(order_id),
            "SUCCESS",
            "order shipped by seller"
        );
        connection.Commit();
        transaction_started = false;

        CreateStationNoticeSafely(
            *notification_service_,
            notification::StationNoticeRequest{
                .user_id = order->buyer_id,
                .notice_type = "ORDER_SHIPPED",
                .title = "Order shipped",
                .content = "Order " + order->order_no +
                           " has been shipped by the seller. Please confirm receipt after delivery.",
                .biz_type = "ORDER",
                .biz_id = order_id,
            }
        );

        return OrderTransitionResult{
            .order_id = order_id,
            .old_status = order->order_status,
            .new_status = std::string(kOrderStatusShipped),
        };
    } catch (...) {
        if (transaction_started) {
            connection.Rollback();
        }
        InsertOperationLogSafely(
            repository,
            auth_context.user_id,
            auth_context.role_code,
            "ship_order",
            std::to_string(order_id),
            "FAILED",
            "order shipment confirmation failed"
        );
        throw;
    }
}

OrderTransitionResult OrderService::ConfirmReceipt(
    const std::string_view authorization_header,
    const std::uint64_t order_id
) {
    const auto auth_context = auth_middleware_->RequireAnyRole(
        authorization_header,
        {modules::auth::kRoleUser}
    );

    auto connection = CreateConnection();
    auto repository = MakeRepository(connection);
    bool transaction_started = false;
    try {
        connection.BeginTransaction();
        transaction_started = true;

        const auto order = repository.FindOrderByIdForUpdate(order_id);
        if (!order.has_value()) {
            ThrowOrderError(common::errors::ErrorCode::kOrderNotFound, "order not found");
        }
        if (order->buyer_id != auth_context.user_id) {
            ThrowOrderError(
                common::errors::ErrorCode::kOrderOwnerMismatch,
                "user does not own this order as buyer"
            );
        }
        if (order->order_status != kOrderStatusShipped) {
            ThrowOrderError(
                common::errors::ErrorCode::kOrderStatusInvalid,
                "order status does not allow receipt confirmation"
            );
        }

        repository.UpdateOrderStatusToCompleted(order_id);
        repository.InsertOperationLog(
            auth_context.user_id,
            auth_context.role_code,
            "confirm_receipt",
            std::to_string(order_id),
            "SUCCESS",
            "order completed by buyer"
        );
        connection.Commit();
        transaction_started = false;

        CreateStationNoticeSafely(
            *notification_service_,
            notification::StationNoticeRequest{
                .user_id = order->seller_id,
                .notice_type = "ORDER_COMPLETED",
                .title = "Order completed",
                .content = "Order " + order->order_no +
                           " has been confirmed received by the buyer.",
                .biz_type = "ORDER",
                .biz_id = order_id,
            }
        );

        return OrderTransitionResult{
            .order_id = order_id,
            .old_status = order->order_status,
            .new_status = std::string(kOrderStatusCompleted),
        };
    } catch (...) {
        if (transaction_started) {
            connection.Rollback();
        }
        InsertOperationLogSafely(
            repository,
            auth_context.user_id,
            auth_context.role_code,
            "confirm_receipt",
            std::to_string(order_id),
            "FAILED",
            "order receipt confirmation failed"
        );
        throw;
    }
}

common::db::MysqlConnection OrderService::CreateConnection() const {
    return common::db::MysqlConnection(mysql_config_, project_root_);
}

}  // namespace auction::modules::order
