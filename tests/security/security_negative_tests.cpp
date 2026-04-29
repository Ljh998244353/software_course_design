#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cctype>
#include <filesystem>
#include <functional>
#include <optional>
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
#include "modules/auth/auth_exception.h"
#include "modules/auth/auth_service.h"
#include "modules/auth/auth_session_store.h"
#include "modules/bid/bid_cache_store.h"
#include "modules/bid/bid_exception.h"
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
using auction::modules::auth::AuthException;
using auction::modules::bid::BidException;
using auction::modules::payment::PaymentException;

struct RegisteredUser {
    std::uint64_t user_id{0};
    std::string username;
    std::string token;
    std::string header;
};

struct ServiceBundle {
    auction::modules::auth::AuthService& auth_service;
    auction::modules::item::ItemService& item_service;
    auction::modules::audit::ItemAuditService& item_audit_service;
    auction::modules::auction::AuctionService& auction_service;
    auction::modules::bid::BidService& bid_service;
    auction::modules::order::OrderService& order_service;
    auction::modules::payment::PaymentService& payment_service;
    auction::jobs::AuctionScheduler& auction_scheduler;
    auction::jobs::OrderScheduler& order_scheduler;
    const auction::common::config::AppConfig& config;
    const std::filesystem::path& project_root;
};

struct SettledOrderContext {
    std::uint64_t auction_id{0};
    std::uint64_t order_id{0};
    std::string order_no;
    RegisteredUser seller;
    RegisteredUser buyer;
};

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

std::string CompactToken(const std::string& value) {
    return std::to_string(std::hash<std::string>{}(value));
}

std::string BuildPhoneNumber(const std::string& suffix, const int index) {
    std::string digits;
    digits.reserve(suffix.size() + 24);
    for (const char character : suffix) {
        if (std::isdigit(static_cast<unsigned char>(character)) != 0) {
            digits.push_back(character);
        }
    }
    digits += CompactToken(suffix + ":" + std::to_string(index));
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
void ExpectAuthError(const ErrorCode expected_code, Fn&& function) {
    try {
        function();
        assert(false && "expected auth exception");
    } catch (const AuthException& exception) {
        assert(exception.code() == expected_code);
    }
}

template <typename Fn>
void ExpectBidError(const ErrorCode expected_code, Fn&& function) {
    try {
        function();
        assert(false && "expected bid exception");
    } catch (const BidException& exception) {
        assert(exception.code() == expected_code);
    }
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

void ExecuteSql(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::string& sql
) {
    auto connection = CreateConnection(config, project_root);
    connection.Execute(sql);
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

void UpdateAuctionWindow(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t auction_id,
    const int start_offset_seconds,
    const int end_offset_seconds
) {
    ExecuteSql(
        config,
        project_root,
        "UPDATE auction SET start_time = DATE_ADD(CURRENT_TIMESTAMP(3), INTERVAL " +
            std::to_string(start_offset_seconds) +
            " SECOND), end_time = DATE_ADD(CURRENT_TIMESTAMP(3), INTERVAL " +
            std::to_string(end_offset_seconds) + " SECOND) WHERE auction_id = " +
            std::to_string(auction_id)
    );
}

void UpdateAuctionEndToPast(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t auction_id
) {
    ExecuteSql(
        config,
        project_root,
        "UPDATE auction SET end_time = DATE_SUB(CURRENT_TIMESTAMP(3), INTERVAL 5 SECOND) "
        "WHERE auction_id = " +
            std::to_string(auction_id)
    );
}

std::uint64_t CountBidRecords(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t auction_id
) {
    return QueryUint64(
        config,
        project_root,
        "SELECT COUNT(*) FROM bid_record WHERE auction_id = " + std::to_string(auction_id)
    );
}

std::uint64_t CountPaymentRecordsByOrder(
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

RegisteredUser RegisterAndLoginUser(
    auction::modules::auth::AuthService& auth_service,
    const std::string& unique_suffix,
    const std::string& username_prefix,
    const int index
) {
    RegisteredUser result;
    const auto compact_suffix = CompactToken(unique_suffix + ":" + std::to_string(index));
    const auto max_prefix_length = static_cast<std::size_t>(50 - 1 - compact_suffix.size());
    const auto compact_prefix =
        username_prefix.size() > max_prefix_length ? username_prefix.substr(0, max_prefix_length)
                                                   : username_prefix;
    const auto password = "Password@1";
    result.username = compact_prefix + "_" + compact_suffix;
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
    result.token = login.token;
    result.header = "Bearer " + login.token;
    return result;
}

std::uint64_t CreateApprovedItem(
    ServiceBundle& services,
    const std::string& seller_header,
    const std::string& admin_header,
    const std::string& unique_suffix,
    const std::string& title_prefix
) {
    const auto created = services.item_service.CreateDraftItem(
        seller_header,
        {
            .title = title_prefix + " " + unique_suffix,
            .description = "used for S14 security negative tests",
            .category_id = 1,
            .start_price = 100.0,
            .cover_image_url = "",
        }
    );
    const auto image = services.item_service.AddItemImage(
        seller_header,
        created.item_id,
        {
            .image_url = "/images/security/" + unique_suffix + "/" + title_prefix + ".jpg",
            .sort_no = std::nullopt,
            .is_cover = true,
        }
    );
    assert(image.image_id > 0);
    const auto submitted = services.item_service.SubmitForAudit(seller_header, created.item_id);
    assert(submitted.item_status == "PENDING_AUDIT");
    const auto approved = services.item_audit_service.AuditItem(
        admin_header,
        created.item_id,
        {
            .audit_result = "APPROVED",
            .reason = "approved for security negative tests",
        }
    );
    assert(approved.new_status == "READY_FOR_AUCTION");
    return created.item_id;
}

std::uint64_t CreateRunningAuction(
    ServiceBundle& services,
    const std::string& seller_header,
    const std::string& admin_header,
    const std::string& unique_suffix,
    const std::string& title_prefix
) {
    const auto item_id = CreateApprovedItem(
        services,
        seller_header,
        admin_header,
        unique_suffix,
        title_prefix
    );
    const auto created = services.auction_service.CreateAuction(
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
    UpdateAuctionWindow(
        services.config,
        services.project_root,
        created.auction_id,
        -5,
        300
    );
    const auto started = services.auction_scheduler.RunStartCycle();
    assert(Contains(started.affected_auction_ids, created.auction_id));
    return created.auction_id;
}

SettledOrderContext CreateSettledOrder(
    ServiceBundle& services,
    const std::string& admin_header,
    const std::string& unique_suffix,
    const int start_index
) {
    SettledOrderContext context;
    context.seller = RegisterAndLoginUser(
        services.auth_service,
        unique_suffix,
        "sec_payment_seller",
        start_index
    );
    context.buyer = RegisterAndLoginUser(
        services.auth_service,
        unique_suffix,
        "sec_payment_buyer",
        start_index + 1
    );
    context.auction_id = CreateRunningAuction(
        services,
        context.seller.header,
        admin_header,
        unique_suffix,
        "security-payment-item"
    );
    const auto bid = services.bid_service.PlaceBid(
        context.buyer.header,
        context.auction_id,
        {
            .request_id = "security-payment-bid-" + unique_suffix,
            .bid_amount = 150.0,
        }
    );
    assert(bid.bid_status == "WINNING");
    UpdateAuctionEndToPast(services.config, services.project_root, context.auction_id);
    const auto finished = services.auction_scheduler.RunFinishCycle();
    assert(Contains(finished.affected_auction_ids, context.auction_id));
    const auto settled = services.order_scheduler.RunSettlementCycle();
    assert(settled.succeeded >= 1);
    context.order_id =
        QueryOrderIdByAuction(services.config, services.project_root, context.auction_id);
    context.order_no =
        QueryOrderNoByOrderId(services.config, services.project_root, context.order_id);
    return context;
}

void RunAuthAndRbacNegativeScenario(
    ServiceBundle& services,
    const std::string& admin_header,
    const RegisteredUser& seller,
    const RegisteredUser& buyer,
    const std::string& unique_suffix
) {
    ExpectAuthError(ErrorCode::kAuthTokenMissing, [&] {
        const auto ignored = services.bid_service.PlaceBid(
            "",
            0,
            {
                .request_id = "missing-auth-bid",
                .bid_amount = 100.0,
            }
        );
        (void)ignored;
    });

    const auto item_id = CreateApprovedItem(
        services,
        seller.header,
        admin_header,
        unique_suffix + "_rbac",
        "security-rbac-item"
    );
    ExpectAuthError(ErrorCode::kAuthPermissionDenied, [&] {
        const auto ignored = services.auction_service.CreateAuction(
            seller.header,
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
        (void)ignored;
    });

    const auto frozen = services.auth_service.ChangeUserStatus(
        admin_header.substr(std::string("Bearer ").size()),
        buyer.user_id,
        "FROZEN",
        "S14 security negative"
    );
    assert(frozen.new_status == "FROZEN");
    ExpectAuthError(ErrorCode::kAuthSessionInvalid, [&] {
        const auto ignored = services.bid_service.PlaceBid(
            buyer.header,
            0,
            {
                .request_id = "frozen-token-bid",
                .bid_amount = 100.0,
            }
        );
        (void)ignored;
    });
    ExpectAuthError(ErrorCode::kAuthUserFrozen, [&] {
        const auto ignored = services.auth_service.Login({
            .principal = buyer.username,
            .password = "Password@1",
        });
        (void)ignored;
    });
}

void RunBidNegativeScenario(
    ServiceBundle& services,
    const std::string& seller_header,
    const std::string& admin_header,
    const RegisteredUser& buyer_1,
    const RegisteredUser& buyer_2,
    const std::string& unique_suffix
) {
    const auto auction_id = CreateRunningAuction(
        services,
        seller_header,
        admin_header,
        unique_suffix + "_bid",
        "security-bid-item"
    );

    ExpectBidError(ErrorCode::kBidAmountInvalid, [&] {
        const auto ignored = services.bid_service.PlaceBid(
            buyer_1.header,
            auction_id,
            {
                .request_id = "security-invalid-amount",
                .bid_amount = 100.001,
            }
        );
        (void)ignored;
    });

    const auto first_bid = services.bid_service.PlaceBid(
        buyer_1.header,
        auction_id,
        {
            .request_id = "security-duplicate-request",
            .bid_amount = 100.0,
        }
    );
    assert(first_bid.bid_status == "WINNING");

    ExpectBidError(ErrorCode::kBidIdempotencyConflict, [&] {
        const auto ignored = services.bid_service.PlaceBid(
            buyer_1.header,
            auction_id,
            {
                .request_id = "security-duplicate-request",
                .bid_amount = 110.0,
            }
        );
        (void)ignored;
    });

    UpdateAuctionEndToPast(services.config, services.project_root, auction_id);
    ExpectBidError(ErrorCode::kBidAuctionClosed, [&] {
        const auto ignored = services.bid_service.PlaceBid(
            buyer_2.header,
            auction_id,
            {
                .request_id = "security-late-bid",
                .bid_amount = 110.0,
            }
        );
        (void)ignored;
    });
    assert(CountBidRecords(services.config, services.project_root, auction_id) == 1);
}

void RunPaymentNegativeScenario(
    ServiceBundle& services,
    const std::string& admin_header,
    const RegisteredUser& intruder,
    const std::string& unique_suffix
) {
    const auto context = CreateSettledOrder(
        services,
        admin_header,
        unique_suffix + "_payment",
        100
    );

    ExpectPaymentError(ErrorCode::kOrderOwnerMismatch, [&] {
        const auto ignored = services.payment_service.InitiatePayment(
            intruder.header,
            context.order_id,
            {
                .pay_channel = "MOCK_ALIPAY",
            }
        );
        (void)ignored;
    });
    assert(CountPaymentRecordsByOrder(services.config, services.project_root, context.order_id) == 0);

    const auto initiated = services.payment_service.InitiatePayment(
        context.buyer.header,
        context.order_id,
        {
            .pay_channel = "MOCK_ALIPAY",
        }
    );
    assert(initiated.pay_status == "WAITING_CALLBACK");

    auction::modules::payment::PaymentCallbackRequest invalid_merchant{
        .payment_no = initiated.payment_no,
        .order_no = initiated.order_no,
        .merchant_no = "EVIL_MERCHANT",
        .transaction_no = "TXN_SEC_MERCHANT_" + unique_suffix,
        .pay_status = "SUCCESS",
        .pay_amount = initiated.pay_amount,
        .signature = "",
    };
    invalid_merchant.signature = auction::modules::payment::SignMockPaymentCallback(
        services.config.auth.token_secret,
        invalid_merchant
    );
    ExpectPaymentError(ErrorCode::kPaymentCallbackInvalid, [&] {
        (void)services.payment_service.HandlePaymentCallback(invalid_merchant);
    });

    auction::modules::payment::PaymentCallbackRequest amount_mismatch{
        .payment_no = initiated.payment_no,
        .order_no = initiated.order_no,
        .merchant_no = initiated.merchant_no,
        .transaction_no = "TXN_SEC_AMOUNT_" + unique_suffix,
        .pay_status = "SUCCESS",
        .pay_amount = initiated.pay_amount + 10.0,
        .signature = "",
    };
    amount_mismatch.signature = auction::modules::payment::SignMockPaymentCallback(
        services.config.auth.token_secret,
        amount_mismatch
    );
    ExpectPaymentError(ErrorCode::kPaymentAmountMismatch, [&] {
        (void)services.payment_service.HandlePaymentCallback(amount_mismatch);
    });

    auction::modules::payment::PaymentCallbackRequest bad_signature{
        .payment_no = initiated.payment_no,
        .order_no = initiated.order_no,
        .merchant_no = initiated.merchant_no,
        .transaction_no = "TXN_SEC_SIGNATURE_" + unique_suffix,
        .pay_status = "SUCCESS",
        .pay_amount = initiated.pay_amount,
        .signature = "invalid_signature",
    };
    ExpectPaymentError(ErrorCode::kPaymentSignatureInvalid, [&] {
        (void)services.payment_service.HandlePaymentCallback(bad_signature);
    });

    assert(QueryOrderStatus(services.config, services.project_root, context.order_id) ==
           "PENDING_PAYMENT");
    assert(QueryPaymentStatusByOrder(services.config, services.project_root, context.order_id) ==
           "WAITING_CALLBACK");
    assert(CountRejectedCallbackLogsByOrder(services.config, services.project_root, context.order_id) == 3);

    auction::modules::payment::PaymentCallbackRequest success{
        .payment_no = initiated.payment_no,
        .order_no = initiated.order_no,
        .merchant_no = initiated.merchant_no,
        .transaction_no = "TXN_SEC_SUCCESS_" + unique_suffix,
        .pay_status = "SUCCESS",
        .pay_amount = initiated.pay_amount,
        .signature = "",
    };
    success.signature = auction::modules::payment::SignMockPaymentCallback(
        services.config.auth.token_secret,
        success
    );
    const auto success_result = services.payment_service.HandlePaymentCallback(success);
    assert(!success_result.idempotent);
    assert(QueryOrderStatus(services.config, services.project_root, context.order_id) == "PAID");

    auto transaction_conflict = success;
    transaction_conflict.transaction_no = "TXN_SEC_CONFLICT_" + unique_suffix;
    transaction_conflict.signature = auction::modules::payment::SignMockPaymentCallback(
        services.config.auth.token_secret,
        transaction_conflict
    );
    ExpectPaymentError(ErrorCode::kPaymentStatusInvalid, [&] {
        (void)services.payment_service.HandlePaymentCallback(transaction_conflict);
    });
    assert(QueryOrderStatus(services.config, services.project_root, context.order_id) == "PAID");
    assert(CountRejectedCallbackLogsByOrder(services.config, services.project_root, context.order_id) == 4);
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
    ServiceBundle services{
        .auth_service = auth_service,
        .item_service = item_service,
        .item_audit_service = item_audit_service,
        .auction_service = auction_service,
        .bid_service = bid_service,
        .order_service = order_service,
        .payment_service = payment_service,
        .auction_scheduler = auction_scheduler,
        .order_scheduler = order_scheduler,
        .config = config,
        .project_root = project_root,
    };

    const auto admin_login = auth_service.Login({
        .principal = "admin",
        .password = "Admin@123",
    });
    const auto admin_header = "Bearer " + admin_login.token;
    const auto unique_suffix = BuildUniqueSuffix();
    const auto seller = RegisterAndLoginUser(auth_service, unique_suffix, "sec_seller", 1);
    const auto buyer = RegisterAndLoginUser(auth_service, unique_suffix, "sec_buyer", 2);
    const auto intruder = RegisterAndLoginUser(auth_service, unique_suffix, "sec_intruder", 3);

    RunAuthAndRbacNegativeScenario(services, admin_header, seller, buyer, unique_suffix);
    const auto active_buyer = RegisterAndLoginUser(auth_service, unique_suffix, "sec_active_buyer", 4);
    RunBidNegativeScenario(
        services,
        seller.header,
        admin_header,
        active_buyer,
        intruder,
        unique_suffix
    );
    RunPaymentNegativeScenario(services, admin_header, intruder, unique_suffix);

    return 0;
}
