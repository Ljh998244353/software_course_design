#include <algorithm>
#include <cassert>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

#include "common/config/config_loader.h"
#include "common/db/mysql_connection.h"
#include "common/errors/error_code.h"
#include "jobs/auction_scheduler.h"
#include "jobs/order_scheduler.h"
#include "middleware/auth_middleware.h"
#include "modules/audit/item_audit_service.h"
#include "modules/auction/auction_service.h"
#include "modules/auth/auth_service.h"
#include "modules/auth/auth_session_store.h"
#include "modules/bid/bid_cache_store.h"
#include "modules/bid/bid_service.h"
#include "modules/item/item_service.h"
#include "modules/notification/notification_service.h"
#include "modules/order/order_service.h"
#include "modules/payment/payment_exception.h"
#include "modules/payment/payment_service.h"
#include "modules/payment/payment_signature.h"
#include "ws/auction_event_gateway.h"

namespace {

using auction::common::errors::ErrorCode;
using auction::modules::payment::PaymentException;

std::string BuildUniqueSuffix() {
    return std::to_string(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        )
            .count()
    );
}

std::string FormatNowOffsetSeconds(const int offset_seconds) {
    const auto target_time =
        std::chrono::system_clock::now() + std::chrono::seconds(offset_seconds);
    const auto time_value = std::chrono::system_clock::to_time_t(target_time);
    std::tm local_tm{};
    localtime_r(&time_value, &local_tm);

    char buffer[32]{};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &local_tm);
    return buffer;
}

std::string BuildPhoneNumber(const std::string& suffix, const int index) {
    std::string digits;
    digits.reserve(suffix.size() + 4);
    for (const char character : suffix) {
        if (std::isdigit(static_cast<unsigned char>(character)) != 0) {
            digits.push_back(character);
        }
    }
    digits += std::to_string(index);
    if (digits.size() < 9) {
        digits.insert(digits.begin(), 9 - digits.size(), '0');
    }
    digits = digits.substr(digits.size() - 9);
    return "13" + digits;
}

template <typename T>
bool Contains(const std::vector<T>& values, const T& expected) {
    return std::find(values.begin(), values.end(), expected) != values.end();
}

template <typename Fn>
void ExpectPaymentError(const ErrorCode expected_code, Fn&& function) {
    try {
        function();
        assert(false && "expected payment exception");
    } catch (const PaymentException& exception) {
        assert(exception.code() == expected_code);
    }
}

auction::common::db::MysqlConnection CreateConnection(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root
) {
    return auction::common::db::MysqlConnection(config.mysql, project_root);
}

std::uint64_t QueryUint64(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::string& sql
) {
    auto connection = CreateConnection(config, project_root);
    return connection.QueryCount(sql);
}

std::string QueryString(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::string& sql
) {
    auto connection = CreateConnection(config, project_root);
    return connection.QueryString(sql);
}

std::uint64_t QueryOrderIdByAuction(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t auction_id
) {
    return QueryUint64(
        config,
        project_root,
        "SELECT COALESCE(MAX(order_id), 0) FROM order_info WHERE auction_id = " +
            std::to_string(auction_id)
    );
}

std::string QueryOrderNoByOrderId(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t order_id
) {
    return QueryString(
        config,
        project_root,
        "SELECT order_no FROM order_info WHERE order_id = " + std::to_string(order_id)
    );
}

std::string QueryOrderStatus(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t order_id
) {
    return QueryString(
        config,
        project_root,
        "SELECT order_status FROM order_info WHERE order_id = " + std::to_string(order_id)
    );
}

std::string QueryPaymentStatusByOrder(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t order_id
) {
    return QueryString(
        config,
        project_root,
        "SELECT pay_status FROM payment_record WHERE order_id = " + std::to_string(order_id) +
            " ORDER BY payment_id DESC LIMIT 1"
    );
}

std::uint64_t CountPaymentsByOrder(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t order_id
) {
    return QueryUint64(
        config,
        project_root,
        "SELECT COUNT(*) FROM payment_record WHERE order_id = " + std::to_string(order_id)
    );
}

std::uint64_t CountCallbackLogsByOrder(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t order_id
) {
    return QueryUint64(
        config,
        project_root,
        "SELECT COUNT(*) FROM payment_callback_log WHERE order_id = " + std::to_string(order_id)
    );
}

std::uint64_t CountRejectedCallbackLogsByOrder(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t order_id
) {
    return QueryUint64(
        config,
        project_root,
        "SELECT COUNT(*) FROM payment_callback_log WHERE order_id = " + std::to_string(order_id) +
            " AND callback_status = 'REJECTED'"
    );
}

std::string QueryAuctionStatus(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t auction_id
) {
    return QueryString(
        config,
        project_root,
        "SELECT status FROM auction WHERE auction_id = " + std::to_string(auction_id)
    );
}

std::string QueryItemStatusByAuction(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t auction_id
) {
    return QueryString(
        config,
        project_root,
        "SELECT i.item_status FROM item i INNER JOIN auction a ON a.item_id = i.item_id "
        "WHERE a.auction_id = " +
            std::to_string(auction_id)
    );
}

void UpdateAuctionWindow(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t auction_id,
    const int start_offset_seconds,
    const int end_offset_seconds
) {
    auto connection = CreateConnection(config, project_root);
    connection.Execute(
        "UPDATE auction SET start_time = DATE_ADD(CURRENT_TIMESTAMP(3), INTERVAL " +
        std::to_string(start_offset_seconds) + " SECOND), end_time = DATE_ADD(CURRENT_TIMESTAMP(3), INTERVAL " +
        std::to_string(end_offset_seconds) + " SECOND) WHERE auction_id = " +
        std::to_string(auction_id)
    );
}

void UpdateAuctionEndToPast(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t auction_id
) {
    auto connection = CreateConnection(config, project_root);
    connection.Execute(
        "UPDATE auction SET end_time = DATE_SUB(CURRENT_TIMESTAMP(3), INTERVAL 5 SECOND) "
        "WHERE auction_id = " +
        std::to_string(auction_id)
    );
}

std::uint64_t CreateApprovedItem(
    auction::modules::item::ItemService& item_service,
    auction::modules::audit::ItemAuditService& item_audit_service,
    const std::string& seller_header,
    const std::string& admin_header,
    const std::string& unique_suffix
) {
    const auto created = item_service.CreateDraftItem(
        seller_header,
        {
            .title = "Payment test item " + unique_suffix,
            .description = "used for S08 payment tests",
            .category_id = 1,
            .start_price = 100.0,
            .cover_image_url = "",
        }
    );
    const auto image = item_service.AddItemImage(
        seller_header,
        created.item_id,
        {
            .image_url = "/images/payment/" + unique_suffix + ".jpg",
            .sort_no = std::nullopt,
            .is_cover = true,
        }
    );
    assert(image.image_id > 0);
    const auto submitted = item_service.SubmitForAudit(seller_header, created.item_id);
    assert(submitted.item_status == "PENDING_AUDIT");
    const auto approved = item_audit_service.AuditItem(
        admin_header,
        created.item_id,
        {
            .audit_result = "APPROVED",
            .reason = "approved for payment tests",
        }
    );
    assert(approved.new_status == "READY_FOR_AUCTION");
    return created.item_id;
}

struct RegisteredUser {
    std::uint64_t user_id{0};
    std::string username;
    std::string header;
};

RegisteredUser RegisterAndLoginUser(
    auction::modules::auth::AuthService& auth_service,
    const std::string& unique_suffix,
    const std::string& username_prefix,
    const int index
) {
    RegisteredUser result;
    result.username = username_prefix + "_" + unique_suffix;
    const auto password = "Password@1";
    const auto registered = auth_service.RegisterUser({
        .username = result.username,
        .password = password,
        .phone = BuildPhoneNumber(unique_suffix, index),
        .email = result.username + "@auction.local",
        .nickname = username_prefix,
    });
    const auto login = auth_service.Login({
        .principal = result.username,
        .password = password,
    });
    result.user_id = registered.user_id;
    result.header = "Bearer " + login.token;
    return result;
}

std::uint64_t CreateRunningAuction(
    auction::modules::item::ItemService& item_service,
    auction::modules::audit::ItemAuditService& item_audit_service,
    auction::modules::auction::AuctionService& auction_service,
    auction::jobs::AuctionScheduler& auction_scheduler,
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::string& seller_header,
    const std::string& admin_header,
    const std::string& unique_suffix
) {
    const auto item_id = CreateApprovedItem(
        item_service,
        item_audit_service,
        seller_header,
        admin_header,
        unique_suffix
    );
    const auto created = auction_service.CreateAuction(
        admin_header,
        {
            .item_id = item_id,
            .start_time = FormatNowOffsetSeconds(600),
            .end_time = FormatNowOffsetSeconds(1200),
            .start_price = 100.0,
            .bid_step = 10.0,
            .anti_sniping_window_seconds = 0,
            .extend_seconds = 0,
        }
    );
    UpdateAuctionWindow(config, project_root, created.auction_id, -5, 300);
    const auto started = auction_scheduler.RunStartCycle();
    assert(Contains(started.affected_auction_ids, created.auction_id));
    return created.auction_id;
}

struct SettledOrderContext {
    std::uint64_t auction_id{0};
    std::uint64_t order_id{0};
    std::string order_no;
    double final_amount{0.0};
    RegisteredUser seller;
    RegisteredUser buyer;
};

SettledOrderContext CreateSettledOrder(
    auction::modules::auth::AuthService& auth_service,
    auction::modules::item::ItemService& item_service,
    auction::modules::audit::ItemAuditService& item_audit_service,
    auction::modules::auction::AuctionService& auction_service,
    auction::modules::bid::BidService& bid_service,
    auction::jobs::AuctionScheduler& auction_scheduler,
    auction::jobs::OrderScheduler& order_scheduler,
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::string& admin_header,
    const std::string& unique_suffix,
    const int start_index
) {
    SettledOrderContext context;
    context.seller = RegisterAndLoginUser(
        auth_service,
        unique_suffix,
        "payment_seller",
        start_index
    );
    context.buyer = RegisterAndLoginUser(
        auth_service,
        unique_suffix,
        "payment_buyer",
        start_index + 1
    );

    context.auction_id = CreateRunningAuction(
        item_service,
        item_audit_service,
        auction_service,
        auction_scheduler,
        config,
        project_root,
        context.seller.header,
        admin_header,
        unique_suffix
    );
    const auto bid = bid_service.PlaceBid(
        context.buyer.header,
        context.auction_id,
        {
            .request_id = "payment-bid-" + unique_suffix,
            .bid_amount = 150.0,
        }
    );
    assert(bid.bid_status == "WINNING");

    UpdateAuctionEndToPast(config, project_root, context.auction_id);
    const auto finished = auction_scheduler.RunFinishCycle();
    assert(Contains(finished.affected_auction_ids, context.auction_id));
    const auto settled = order_scheduler.RunSettlementCycle();
    assert(settled.succeeded >= 1);

    context.order_id = QueryOrderIdByAuction(config, project_root, context.auction_id);
    context.order_no = QueryOrderNoByOrderId(config, project_root, context.order_id);
    context.final_amount =
        std::stod(QueryString(
            config,
            project_root,
            "SELECT CAST(final_amount AS CHAR) FROM order_info WHERE order_id = " +
                std::to_string(context.order_id)
        ));
    return context;
}

}  // namespace

int main() {
    namespace fs = std::filesystem;

    const fs::path project_root(AUCTION_PROJECT_ROOT);
    const fs::path config_path =
        auction::common::config::ConfigLoader::ResolveConfigPath(project_root);
    const auto config = auction::common::config::ConfigLoader::LoadFromFile(config_path);

    auction::modules::auth::InMemoryAuthSessionStore session_store;
    auction::modules::auth::AuthService auth_service(config, project_root, session_store);
    auction::middleware::AuthMiddleware auth_middleware(auth_service);
    auction::modules::item::ItemService item_service(config, project_root, auth_middleware);
    auction::modules::audit::ItemAuditService item_audit_service(
        config,
        project_root,
        auth_middleware
    );
    auction::modules::auction::AuctionService auction_service(
        config,
        project_root,
        auth_middleware
    );
    auction::ws::InMemoryAuctionEventGateway event_gateway;
    auction::modules::notification::NotificationService notification_service(
        config,
        project_root,
        event_gateway
    );
    auction::modules::bid::InMemoryBidCacheStore cache_store;
    auction::modules::bid::BidService bid_service(
        config,
        project_root,
        auth_middleware,
        notification_service,
        cache_store
    );
    auction::modules::order::OrderService order_service(
        config,
        project_root,
        auth_middleware,
        notification_service
    );
    auction::modules::payment::PaymentService payment_service(
        config,
        project_root,
        auth_middleware,
        notification_service
    );
    auction::jobs::AuctionScheduler auction_scheduler(auction_service);
    auction::jobs::OrderScheduler order_scheduler(order_service);

    const auto admin_login = auth_service.Login({
        .principal = "admin",
        .password = "Admin@123",
    });
    const auto admin_header = "Bearer " + admin_login.token;
    const auto unique_suffix = BuildUniqueSuffix();

    const auto success_context = CreateSettledOrder(
        auth_service,
        item_service,
        item_audit_service,
        auction_service,
        bid_service,
        auction_scheduler,
        order_scheduler,
        config,
        project_root,
        admin_header,
        unique_suffix + "_success",
        10
    );

    const auto initiated = payment_service.InitiatePayment(
        success_context.buyer.header,
        success_context.order_id,
        {
            .pay_channel = "MOCK_ALIPAY",
        }
    );
    assert(initiated.pay_status == "WAITING_CALLBACK");
    assert(!initiated.pay_url.empty());
    const auto initiated_again = payment_service.InitiatePayment(
        success_context.buyer.header,
        success_context.order_id,
        {
            .pay_channel = "MOCK_ALIPAY",
        }
    );
    assert(initiated_again.reused_existing);
    assert(initiated_again.payment_no == initiated.payment_no);
    assert(CountPaymentsByOrder(config, project_root, success_context.order_id) == 1);

    auction::modules::payment::PaymentCallbackRequest success_callback{
        .payment_no = initiated.payment_no,
        .order_no = initiated.order_no,
        .merchant_no = initiated.merchant_no,
        .transaction_no = "TXN_SUCCESS_" + unique_suffix,
        .pay_status = "SUCCESS",
        .pay_amount = initiated.pay_amount,
        .signature = "",
    };
    success_callback.signature =
        auction::modules::payment::SignMockPaymentCallback(config.auth.token_secret, success_callback);

    const auto callback_result = payment_service.HandlePaymentCallback(success_callback);
    assert(callback_result.order_status == "PAID");
    assert(callback_result.pay_status == "SUCCESS");
    assert(!callback_result.idempotent);

    const auto duplicate_result = payment_service.HandlePaymentCallback(success_callback);
    assert(duplicate_result.idempotent);
    assert(QueryOrderStatus(config, project_root, success_context.order_id) == "PAID");
    assert(QueryPaymentStatusByOrder(config, project_root, success_context.order_id) == "SUCCESS");
    assert(QueryAuctionStatus(config, project_root, success_context.auction_id) == "SOLD");
    assert(QueryItemStatusByAuction(config, project_root, success_context.auction_id) == "SOLD");
    assert(CountCallbackLogsByOrder(config, project_root, success_context.order_id) == 2);

    const auto mismatch_context = CreateSettledOrder(
        auth_service,
        item_service,
        item_audit_service,
        auction_service,
        bid_service,
        auction_scheduler,
        order_scheduler,
        config,
        project_root,
        admin_header,
        unique_suffix + "_mismatch",
        20
    );
    const auto mismatch_payment = payment_service.InitiatePayment(
        mismatch_context.buyer.header,
        mismatch_context.order_id,
        {
            .pay_channel = "MOCK_WECHAT",
        }
    );

    auction::modules::payment::PaymentCallbackRequest mismatch_callback{
        .payment_no = mismatch_payment.payment_no,
        .order_no = mismatch_payment.order_no,
        .merchant_no = mismatch_payment.merchant_no,
        .transaction_no = "TXN_MISMATCH_" + unique_suffix,
        .pay_status = "SUCCESS",
        .pay_amount = mismatch_payment.pay_amount + 10.0,
        .signature = "",
    };
    mismatch_callback.signature =
        auction::modules::payment::SignMockPaymentCallback(config.auth.token_secret, mismatch_callback);

    ExpectPaymentError(
        ErrorCode::kPaymentAmountMismatch,
        [&]() {
            (void)payment_service.HandlePaymentCallback(mismatch_callback);
        }
    );
    assert(QueryOrderStatus(config, project_root, mismatch_context.order_id) == "PENDING_PAYMENT");
    assert(QueryPaymentStatusByOrder(config, project_root, mismatch_context.order_id) ==
           "WAITING_CALLBACK");
    assert(CountRejectedCallbackLogsByOrder(config, project_root, mismatch_context.order_id) == 1);

    const auto signature_context = CreateSettledOrder(
        auth_service,
        item_service,
        item_audit_service,
        auction_service,
        bid_service,
        auction_scheduler,
        order_scheduler,
        config,
        project_root,
        admin_header,
        unique_suffix + "_signature",
        30
    );
    const auto signature_payment = payment_service.InitiatePayment(
        signature_context.buyer.header,
        signature_context.order_id,
        {
            .pay_channel = "MOCK_ALIPAY",
        }
    );

    auction::modules::payment::PaymentCallbackRequest invalid_signature_callback{
        .payment_no = signature_payment.payment_no,
        .order_no = signature_payment.order_no,
        .merchant_no = signature_payment.merchant_no,
        .transaction_no = "TXN_SIG_" + unique_suffix,
        .pay_status = "SUCCESS",
        .pay_amount = signature_payment.pay_amount,
        .signature = "invalid_signature",
    };

    ExpectPaymentError(
        ErrorCode::kPaymentSignatureInvalid,
        [&]() {
            (void)payment_service.HandlePaymentCallback(invalid_signature_callback);
        }
    );
    assert(QueryOrderStatus(config, project_root, signature_context.order_id) == "PENDING_PAYMENT");
    assert(QueryPaymentStatusByOrder(config, project_root, signature_context.order_id) ==
           "WAITING_CALLBACK");
    assert(CountRejectedCallbackLogsByOrder(config, project_root, signature_context.order_id) == 1);

    return 0;
}
