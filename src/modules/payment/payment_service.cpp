#include "modules/payment/payment_service.h"

#include <chrono>
#include <cctype>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

#include <json/json.h>

#include "common/errors/error_code.h"
#include "common/logging/logger.h"
#include "modules/auction/auction_types.h"
#include "modules/auth/auth_types.h"
#include "modules/item/item_types.h"
#include "modules/order/order_types.h"
#include "modules/payment/payment_exception.h"
#include "modules/payment/payment_signature.h"
#include "repository/payment_repository.h"

namespace auction::modules::payment {

namespace {

constexpr double kAmountEpsilon = 0.0001;

std::string TrimWhitespace(std::string value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) {
        value.erase(value.begin());
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
        value.pop_back();
    }
    return value;
}

bool AmountEquals(const double left, const double right) {
    return std::fabs(left - right) < kAmountEpsilon;
}

std::string CurrentTimestampText() {
    const auto now = std::chrono::system_clock::now();
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

std::string BuildMockPayUrl(
    const repository::OrderPaymentSnapshot& order_snapshot,
    const PaymentRecord& payment
) {
    return "mockpay://checkout?merchantNo=" + std::string(kDefaultMerchantNo) +
           "&orderNo=" + order_snapshot.order.order_no + "&paymentNo=" + payment.payment_no +
           "&amount=" + FormatPaymentAmount(order_snapshot.order.final_amount);
}

std::string BuildCallbackPayload(const PaymentCallbackRequest& request) {
    Json::Value root(Json::objectValue);
    root["paymentNo"] = request.payment_no;
    root["orderNo"] = request.order_no;
    root["merchantNo"] = request.merchant_no;
    root["transactionNo"] = request.transaction_no;
    root["payStatus"] = request.pay_status;
    root["payAmount"] = FormatPaymentAmount(request.pay_amount);
    root["signature"] = request.signature;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, root);
}

void ThrowPaymentError(const common::errors::ErrorCode code, const std::string_view message) {
    throw PaymentException(code, std::string(message));
}

repository::PaymentRepository MakeRepository(common::db::MysqlConnection& connection) {
    return repository::PaymentRepository(connection);
}

void InsertOperationLogSafely(
    repository::PaymentRepository& repository,
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
            "payment station notice create failed: " + std::string(exception.what())
        );
    }
}

void ValidateInitiatePaymentRequest(const InitiatePaymentRequest& request) {
    const auto channel = TrimWhitespace(request.pay_channel);
    if (!IsSupportedPaymentChannel(channel)) {
        ThrowPaymentError(
            common::errors::ErrorCode::kPaymentChannelInvalid,
            "payment channel is invalid"
        );
    }
}

void ValidateCallbackRequest(const PaymentCallbackRequest& request) {
    if (TrimWhitespace(request.payment_no).empty() || request.payment_no.size() > 64) {
        throw std::invalid_argument("payment_no must be between 1 and 64 characters");
    }
    if (TrimWhitespace(request.order_no).empty() || request.order_no.size() > 64) {
        throw std::invalid_argument("order_no must be between 1 and 64 characters");
    }
    if (TrimWhitespace(request.merchant_no).empty() || request.merchant_no.size() > 64) {
        throw std::invalid_argument("merchant_no must be between 1 and 64 characters");
    }
    if (TrimWhitespace(request.transaction_no).empty() || request.transaction_no.size() > 64) {
        throw std::invalid_argument("transaction_no must be between 1 and 64 characters");
    }
    if (!IsSupportedCallbackResult(request.pay_status)) {
        ThrowPaymentError(
            common::errors::ErrorCode::kPaymentCallbackInvalid,
            "payment callback status is invalid"
        );
    }
    if (!std::isfinite(request.pay_amount) || request.pay_amount <= 0.0) {
        ThrowPaymentError(
            common::errors::ErrorCode::kPaymentAmountMismatch,
            "payment callback amount must be positive"
        );
    }
}

bool HasTransactionConflict(
    const PaymentRecord& payment,
    const std::string& transaction_no
) {
    return !payment.transaction_no.empty() && payment.transaction_no != transaction_no;
}

}  // namespace

PaymentService::PaymentService(
    const common::config::AppConfig& config,
    std::filesystem::path project_root,
    middleware::AuthMiddleware& auth_middleware,
    notification::NotificationService& notification_service
) : mysql_config_(config.mysql),
    project_root_(std::move(project_root)),
    callback_secret_(config.auth.token_secret),
    auth_middleware_(&auth_middleware),
    notification_service_(&notification_service) {}

InitiatePaymentResult PaymentService::InitiatePayment(
    const std::string_view authorization_header,
    const std::uint64_t order_id,
    const InitiatePaymentRequest& request
) {
    const auto auth_context = auth_middleware_->RequireAnyRole(
        authorization_header,
        {modules::auth::kRoleUser}
    );
    ValidateInitiatePaymentRequest(request);

    auto connection = CreateConnection();
    auto repository = MakeRepository(connection);
    bool transaction_started = false;
    try {
        connection.BeginTransaction();
        transaction_started = true;

        const auto order_snapshot = repository.FindOrderPaymentSnapshotByIdForUpdate(order_id);
        if (!order_snapshot.has_value()) {
            ThrowPaymentError(common::errors::ErrorCode::kOrderNotFound, "order not found");
        }
        if (order_snapshot->order.buyer_id != auth_context.user_id) {
            ThrowPaymentError(
                common::errors::ErrorCode::kOrderOwnerMismatch,
                "user does not own this order"
            );
        }
        if (order_snapshot->order.order_status != modules::order::kOrderStatusPendingPayment) {
            ThrowPaymentError(
                common::errors::ErrorCode::kOrderStatusInvalid,
                "order status does not allow payment"
            );
        }
        if (repository.IsOrderPaymentDeadlineExpired(order_id)) {
            ThrowPaymentError(
                common::errors::ErrorCode::kOrderPaymentDeadlineExpired,
                "order payment deadline has expired"
            );
        }

        if (const auto active_payment = repository.FindLatestActivePaymentByOrderId(order_id);
            active_payment.has_value()) {
            if (active_payment->pay_channel != request.pay_channel) {
                ThrowPaymentError(
                    common::errors::ErrorCode::kPaymentStatusInvalid,
                    "another active payment already exists for the order"
                );
            }

            connection.Rollback();
            transaction_started = false;
            return InitiatePaymentResult{
                .payment_id = active_payment->payment_id,
                .order_id = order_snapshot->order.order_id,
                .order_no = order_snapshot->order.order_no,
                .payment_no = active_payment->payment_no,
                .pay_channel = active_payment->pay_channel,
                .pay_amount = active_payment->pay_amount,
                .pay_status = active_payment->pay_status,
                .merchant_no = std::string(kDefaultMerchantNo),
                .pay_url = BuildMockPayUrl(*order_snapshot, *active_payment),
                .expire_at = order_snapshot->order.pay_deadline_at,
                .reused_existing = true,
            };
        }

        const auto created_payment = repository.CreatePayment(repository::CreatePaymentParams{
            .payment_no = GenerateBusinessNo("PAY"),
            .order_id = order_id,
            .pay_channel = request.pay_channel,
            .pay_amount = FormatPaymentAmount(order_snapshot->order.final_amount),
            .pay_status = std::string(kPaymentStatusWaitingCallback),
        });
        repository.InsertOperationLog(
            auth_context.user_id,
            auth_context.role_code,
            "initiate_payment",
            std::to_string(order_id),
            "SUCCESS",
            "payment_no=" + created_payment.payment_no
        );
        connection.Commit();
        transaction_started = false;

        return InitiatePaymentResult{
            .payment_id = created_payment.payment_id,
            .order_id = order_snapshot->order.order_id,
            .order_no = order_snapshot->order.order_no,
            .payment_no = created_payment.payment_no,
            .pay_channel = created_payment.pay_channel,
            .pay_amount = created_payment.pay_amount,
            .pay_status = created_payment.pay_status,
            .merchant_no = std::string(kDefaultMerchantNo),
            .pay_url = BuildMockPayUrl(*order_snapshot, created_payment),
            .expire_at = order_snapshot->order.pay_deadline_at,
            .reused_existing = false,
        };
    } catch (...) {
        if (transaction_started) {
            connection.Rollback();
        }
        throw;
    }
}

PaymentCallbackResult PaymentService::HandlePaymentCallback(
    const PaymentCallbackRequest& request
) {
    ValidateCallbackRequest(request);

    const auto raw_payload = BuildCallbackPayload(request);
    const auto payload_hash = Sha256Hex(raw_payload);

    auto connection = CreateConnection();
    auto repository = MakeRepository(connection);
    bool transaction_started = false;
    try {
        connection.BeginTransaction();
        transaction_started = true;

        const auto aggregate = repository.FindPaymentAggregateByPaymentNoForUpdate(request.payment_no);
        const auto callback_log_id = repository.CreatePaymentCallbackLog(
            repository::CreatePaymentCallbackLogParams{
                .payment_id = aggregate.has_value()
                                  ? std::optional<std::uint64_t>(aggregate->payment.payment_id)
                                  : std::nullopt,
                .order_id = aggregate.has_value()
                                ? std::optional<std::uint64_t>(aggregate->order.order_id)
                                : std::nullopt,
                .transaction_no = request.transaction_no,
                .callback_status = std::string(kCallbackStatusReceived),
                .raw_payload = raw_payload,
            }
        );

        const auto reject_callback = [&](const common::errors::ErrorCode code,
                                         const std::string& reason) -> void {
            repository.UpdatePaymentCallbackLog(
                callback_log_id,
                std::string(kCallbackStatusRejected),
                reason
            );
            InsertOperationLogSafely(
                repository,
                std::nullopt,
                "SYSTEM",
                "payment_callback",
                aggregate.has_value() ? std::to_string(aggregate->order.order_id)
                                      : request.payment_no,
                "FAILED",
                reason
            );
            connection.Commit();
            transaction_started = false;
            ThrowPaymentError(code, reason);
        };

        if (!aggregate.has_value()) {
            reject_callback(common::errors::ErrorCode::kPaymentNotFound, "payment not found");
        }

        const auto& payment = aggregate->payment;
        const auto& order = aggregate->order;

        if (request.order_no != order.order_no) {
            reject_callback(
                common::errors::ErrorCode::kPaymentCallbackInvalid,
                "order_no does not match payment order"
            );
        }
        if (request.merchant_no != kDefaultMerchantNo) {
            reject_callback(
                common::errors::ErrorCode::kPaymentCallbackInvalid,
                "merchant_no does not match configured merchant"
            );
        }
        if (!AmountEquals(request.pay_amount, payment.pay_amount) ||
            !AmountEquals(request.pay_amount, order.final_amount)) {
            reject_callback(
                common::errors::ErrorCode::kPaymentAmountMismatch,
                "payment callback amount does not match order amount"
            );
        }
        if (HasTransactionConflict(payment, request.transaction_no)) {
            reject_callback(
                common::errors::ErrorCode::kPaymentStatusInvalid,
                "transaction_no conflicts with processed payment record"
            );
        }
        if (!VerifyMockPaymentCallbackSignature(callback_secret_, request)) {
            reject_callback(
                common::errors::ErrorCode::kPaymentSignatureInvalid,
                "payment callback signature is invalid"
            );
        }

        repository.UpdatePaymentCallbackLog(
            callback_log_id,
            std::string(kCallbackStatusVerified),
            "callback signature and business fields verified"
        );

        const auto processed_at = CurrentTimestampText();
        if (request.pay_status == kPaymentStatusSuccess) {
            if (payment.pay_status == kPaymentStatusSuccess ||
                order.order_status == modules::order::kOrderStatusPaid ||
                order.order_status == modules::order::kOrderStatusWaitingDelivery ||
                order.order_status == modules::order::kOrderStatusShipped ||
                order.order_status == modules::order::kOrderStatusCompleted ||
                order.order_status == modules::order::kOrderStatusReviewed) {
                repository.UpdatePaymentCallbackLog(
                    callback_log_id,
                    std::string(kCallbackStatusProcessed),
                    "duplicate success callback ignored"
                );
                InsertOperationLogSafely(
                    repository,
                    std::nullopt,
                    "SYSTEM",
                    "payment_callback",
                    std::to_string(order.order_id),
                    "SUCCESS",
                    "duplicate success callback"
                );
                connection.Commit();
                transaction_started = false;
                return PaymentCallbackResult{
                    .payment_id = payment.payment_id,
                    .order_id = order.order_id,
                    .order_status = order.order_status,
                    .pay_status = payment.pay_status,
                    .idempotent = true,
                    .processed_at = processed_at,
                };
            }

            if (order.order_status == modules::order::kOrderStatusClosed ||
                payment.pay_status == kPaymentStatusClosed) {
                reject_callback(
                    common::errors::ErrorCode::kPaymentStatusInvalid,
                    "closed order can no longer accept successful payment callback"
                );
            }
            if (order.order_status != modules::order::kOrderStatusPendingPayment) {
                reject_callback(
                    common::errors::ErrorCode::kOrderStatusInvalid,
                    "order status does not allow successful payment callback"
                );
            }
            if (aggregate->auction_status != modules::auction::kAuctionStatusSettling) {
                reject_callback(
                    common::errors::ErrorCode::kOrderSettlementConflict,
                    "auction status does not allow payment success settlement"
                );
            }
            if (aggregate->item_status != modules::item::kItemStatusInAuction) {
                reject_callback(
                    common::errors::ErrorCode::kOrderSettlementConflict,
                    "item status does not allow payment success settlement"
                );
            }

            repository.MarkPaymentSuccess(payment.payment_id, request.transaction_no, payload_hash);
            repository.UpdateOrderStatusToPaid(order.order_id);
            repository.UpdateAuctionStatus(
                aggregate->auction_id,
                std::string(modules::auction::kAuctionStatusSold)
            );
            repository.UpdateItemStatus(
                aggregate->item_id,
                std::string(modules::item::kItemStatusSold)
            );
            repository.UpdatePaymentCallbackLog(
                callback_log_id,
                std::string(kCallbackStatusProcessed),
                "payment success applied"
            );
            repository.InsertOperationLog(
                std::nullopt,
                "SYSTEM",
                "payment_callback",
                std::to_string(order.order_id),
                "SUCCESS",
                "payment success applied"
            );
            connection.Commit();
            transaction_started = false;

            CreateStationNoticeSafely(
                *notification_service_,
                notification::StationNoticeRequest{
                    .user_id = order.buyer_id,
                    .notice_type = "PAYMENT_SUCCESS",
                    .title = "Payment completed",
                    .content = "Order " + order.order_no + " has been paid successfully.",
                    .biz_type = "ORDER",
                    .biz_id = order.order_id,
                }
            );
            CreateStationNoticeSafely(
                *notification_service_,
                notification::StationNoticeRequest{
                    .user_id = order.seller_id,
                    .notice_type = "ORDER_PAID",
                    .title = "Buyer completed payment",
                    .content = "Order " + order.order_no + " has been paid. Please prepare delivery.",
                    .biz_type = "ORDER",
                    .biz_id = order.order_id,
                }
            );

            return PaymentCallbackResult{
                .payment_id = payment.payment_id,
                .order_id = order.order_id,
                .order_status = std::string(modules::order::kOrderStatusPaid),
                .pay_status = std::string(kPaymentStatusSuccess),
                .idempotent = false,
                .processed_at = processed_at,
            };
        }

        if (payment.pay_status == kPaymentStatusSuccess ||
            order.order_status == modules::order::kOrderStatusPaid ||
            order.order_status == modules::order::kOrderStatusWaitingDelivery ||
            order.order_status == modules::order::kOrderStatusShipped ||
            order.order_status == modules::order::kOrderStatusCompleted ||
            order.order_status == modules::order::kOrderStatusReviewed) {
            repository.UpdatePaymentCallbackLog(
                callback_log_id,
                std::string(kCallbackStatusProcessed),
                "ignored failed callback after successful payment"
            );
            InsertOperationLogSafely(
                repository,
                std::nullopt,
                "SYSTEM",
                "payment_callback",
                std::to_string(order.order_id),
                "SUCCESS",
                "ignored failed callback after payment success"
            );
            connection.Commit();
            transaction_started = false;
            return PaymentCallbackResult{
                .payment_id = payment.payment_id,
                .order_id = order.order_id,
                .order_status = order.order_status,
                .pay_status = payment.pay_status,
                .idempotent = true,
                .processed_at = processed_at,
            };
        }

        if (order.order_status == modules::order::kOrderStatusClosed ||
            payment.pay_status == kPaymentStatusClosed) {
            repository.UpdatePaymentCallbackLog(
                callback_log_id,
                std::string(kCallbackStatusProcessed),
                "ignored failed callback after order close"
            );
            InsertOperationLogSafely(
                repository,
                std::nullopt,
                "SYSTEM",
                "payment_callback",
                std::to_string(order.order_id),
                "SUCCESS",
                "ignored failed callback after order close"
            );
            connection.Commit();
            transaction_started = false;
            return PaymentCallbackResult{
                .payment_id = payment.payment_id,
                .order_id = order.order_id,
                .order_status = order.order_status,
                .pay_status = payment.pay_status,
                .idempotent = true,
                .processed_at = processed_at,
            };
        }

        if (payment.pay_status == kPaymentStatusFailed) {
            repository.UpdatePaymentCallbackLog(
                callback_log_id,
                std::string(kCallbackStatusProcessed),
                "duplicate failed callback ignored"
            );
            InsertOperationLogSafely(
                repository,
                std::nullopt,
                "SYSTEM",
                "payment_callback",
                std::to_string(order.order_id),
                "SUCCESS",
                "duplicate failed callback"
            );
            connection.Commit();
            transaction_started = false;
            return PaymentCallbackResult{
                .payment_id = payment.payment_id,
                .order_id = order.order_id,
                .order_status = order.order_status,
                .pay_status = payment.pay_status,
                .idempotent = true,
                .processed_at = processed_at,
            };
        }

        if (order.order_status != modules::order::kOrderStatusPendingPayment) {
            reject_callback(
                common::errors::ErrorCode::kOrderStatusInvalid,
                "order status does not allow failed payment callback"
            );
        }

        repository.MarkPaymentFailed(payment.payment_id, request.transaction_no, payload_hash);
        repository.UpdatePaymentCallbackLog(
            callback_log_id,
            std::string(kCallbackStatusProcessed),
            "payment failure recorded"
        );
        repository.InsertOperationLog(
            std::nullopt,
            "SYSTEM",
            "payment_callback",
            std::to_string(order.order_id),
            "SUCCESS",
            "payment failure recorded"
        );
        connection.Commit();
        transaction_started = false;

        CreateStationNoticeSafely(
            *notification_service_,
            notification::StationNoticeRequest{
                .user_id = order.buyer_id,
                .notice_type = "PAYMENT_FAILED",
                .title = "Payment failed",
                .content = "Order " + order.order_no +
                           " payment failed. You can retry before the payment deadline.",
                .biz_type = "ORDER",
                .biz_id = order.order_id,
            }
        );

        return PaymentCallbackResult{
            .payment_id = payment.payment_id,
            .order_id = order.order_id,
            .order_status = order.order_status,
            .pay_status = std::string(kPaymentStatusFailed),
            .idempotent = false,
            .processed_at = processed_at,
        };
    } catch (...) {
        if (transaction_started) {
            connection.Rollback();
        }
        throw;
    }
}

common::db::MysqlConnection PaymentService::CreateConnection() const {
    return common::db::MysqlConnection(mysql_config_, project_root_);
}

}  // namespace auction::modules::payment
