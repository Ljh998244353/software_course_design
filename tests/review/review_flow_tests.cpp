#include <algorithm>
#include <cassert>
#include <cctype>
#include <chrono>
#include <ctime>
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
#include "modules/auth/auth_exception.h"
#include "modules/auth/auth_service.h"
#include "modules/auth/auth_session_store.h"
#include "modules/bid/bid_cache_store.h"
#include "modules/bid/bid_service.h"
#include "modules/item/item_service.h"
#include "modules/notification/notification_service.h"
#include "modules/order/order_service.h"
#include "modules/payment/payment_service.h"
#include "modules/payment/payment_signature.h"
#include "modules/review/review_exception.h"
#include "modules/review/review_service.h"
#include "ws/auction_event_gateway.h"

namespace {

using auction::common::errors::ErrorCode;
using auction::modules::review::ReviewException;

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
void ExpectReviewError(const ErrorCode expected_code, Fn&& function) {
    try {
        function();
        assert(false && "expected review exception");
    } catch (const ReviewException& exception) {
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

std::uint64_t CountReviewsByOrder(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t order_id
) {
    return QueryUint64(
        config,
        project_root,
        "SELECT COUNT(*) FROM review WHERE order_id = " + std::to_string(order_id)
    );
}

std::uint64_t CountNotificationsByOrder(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t order_id
) {
    return QueryUint64(
        config,
        project_root,
        "SELECT COUNT(*) FROM notification WHERE biz_type = 'ORDER' AND biz_id = " +
            std::to_string(order_id) + " AND notice_type = 'ORDER_REVIEW_RECEIVED'"
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

void PromoteOrderToCompleted(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t order_id
) {
    auto connection = CreateConnection(config, project_root);
    connection.Execute(
        "UPDATE order_info SET order_status = 'COMPLETED', completed_at = CURRENT_TIMESTAMP(3), "
        "updated_at = CURRENT_TIMESTAMP(3) WHERE order_id = " +
        std::to_string(order_id)
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
            .title = "Review test item " + unique_suffix,
            .description = "used for S09 review tests",
            .category_id = 1,
            .start_price = 100.0,
            .cover_image_url = "",
        }
    );
    const auto image = item_service.AddItemImage(
        seller_header,
        created.item_id,
        {
            .image_url = "/images/review/" + unique_suffix + ".jpg",
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
            .reason = "approved for review tests",
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

struct CompletedOrderContext {
    std::uint64_t auction_id{0};
    std::uint64_t order_id{0};
    std::string order_no;
    RegisteredUser seller;
    RegisteredUser buyer;
};

CompletedOrderContext CreateCompletedOrder(
    auction::modules::auth::AuthService& auth_service,
    auction::modules::item::ItemService& item_service,
    auction::modules::audit::ItemAuditService& item_audit_service,
    auction::modules::auction::AuctionService& auction_service,
    auction::modules::bid::BidService& bid_service,
    auction::jobs::AuctionScheduler& auction_scheduler,
    auction::jobs::OrderScheduler& order_scheduler,
    auction::modules::payment::PaymentService& payment_service,
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::string& admin_header,
    const std::string& unique_suffix,
    const int start_index
) {
    CompletedOrderContext context;
    context.seller = RegisterAndLoginUser(
        auth_service,
        unique_suffix,
        "review_seller",
        start_index
    );
    context.buyer = RegisterAndLoginUser(
        auth_service,
        unique_suffix,
        "review_buyer",
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
            .request_id = "review-bid-" + unique_suffix,
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

    const auto initiated = payment_service.InitiatePayment(
        context.buyer.header,
        context.order_id,
        {
            .pay_channel = "MOCK_ALIPAY",
        }
    );

    auction::modules::payment::PaymentCallbackRequest callback{
        .payment_no = initiated.payment_no,
        .order_no = initiated.order_no,
        .merchant_no = initiated.merchant_no,
        .transaction_no = "TXN_REVIEW_" + unique_suffix,
        .pay_status = "SUCCESS",
        .pay_amount = initiated.pay_amount,
        .signature = "",
    };
    callback.signature =
        auction::modules::payment::SignMockPaymentCallback(config.auth.token_secret, callback);
    const auto callback_result = payment_service.HandlePaymentCallback(callback);
    assert(callback_result.order_status == "PAID");

    PromoteOrderToCompleted(config, project_root, context.order_id);
    assert(QueryOrderStatus(config, project_root, context.order_id) == "COMPLETED");
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
    auction::modules::review::ReviewService review_service(
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

    const auto mutual_context = CreateCompletedOrder(
        auth_service,
        item_service,
        item_audit_service,
        auction_service,
        bid_service,
        auction_scheduler,
        order_scheduler,
        payment_service,
        config,
        project_root,
        admin_header,
        unique_suffix + "_mutual",
        10
    );

    const auto buyer_review = review_service.SubmitReview(
        mutual_context.buyer.header,
        mutual_context.order_id,
        {
            .rating = 5,
            .content = "Smooth transaction and fast delivery.",
        }
    );
    assert(buyer_review.review.review_type == "BUYER_TO_SELLER");
    assert(!buyer_review.order_marked_reviewed);
    assert(buyer_review.order_status_after == "COMPLETED");
    assert(QueryOrderStatus(config, project_root, mutual_context.order_id) == "COMPLETED");

    const auto seller_review = review_service.SubmitReview(
        mutual_context.seller.header,
        mutual_context.order_id,
        {
            .rating = 4,
            .content = "Buyer completed payment on time.",
        }
    );
    assert(seller_review.review.review_type == "SELLER_TO_BUYER");
    assert(seller_review.order_marked_reviewed);
    assert(seller_review.order_status_after == "REVIEWED");
    assert(QueryOrderStatus(config, project_root, mutual_context.order_id) == "REVIEWED");
    assert(CountReviewsByOrder(config, project_root, mutual_context.order_id) == 2);
    assert(CountNotificationsByOrder(config, project_root, mutual_context.order_id) == 2);

    const auto order_reviews = review_service.ListOrderReviews(mutual_context.order_id);
    assert(order_reviews.order_status == "REVIEWED");
    assert(order_reviews.reviews.size() == 2);
    assert(order_reviews.reviews.front().reviewer_id == mutual_context.buyer.user_id);
    assert(order_reviews.reviews.back().reviewer_id == mutual_context.seller.user_id);

    const auto seller_summary =
        review_service.GetUserReviewSummary(mutual_context.seller.user_id);
    assert(seller_summary.user_id == mutual_context.seller.user_id);
    assert(seller_summary.total_reviews == 1);
    assert(seller_summary.average_rating == 5.0);
    assert(seller_summary.positive_reviews == 1);
    assert(seller_summary.neutral_reviews == 0);
    assert(seller_summary.negative_reviews == 0);

    const auto buyer_summary =
        review_service.GetUserReviewSummary(mutual_context.buyer.user_id);
    assert(buyer_summary.user_id == mutual_context.buyer.user_id);
    assert(buyer_summary.total_reviews == 1);
    assert(buyer_summary.average_rating == 4.0);
    assert(buyer_summary.positive_reviews == 1);
    assert(buyer_summary.neutral_reviews == 0);
    assert(buyer_summary.negative_reviews == 0);

    const auto duplicate_context = CreateCompletedOrder(
        auth_service,
        item_service,
        item_audit_service,
        auction_service,
        bid_service,
        auction_scheduler,
        order_scheduler,
        payment_service,
        config,
        project_root,
        admin_header,
        unique_suffix + "_duplicate",
        30
    );
    const auto first_duplicate_review = review_service.SubmitReview(
        duplicate_context.buyer.header,
        duplicate_context.order_id,
        {
            .rating = 5,
            .content = "first review",
        }
    );
    assert(first_duplicate_review.review.review_id > 0);
    ExpectReviewError(
        ErrorCode::kReviewDuplicateSubmitted,
        [&]() {
            (void)review_service.SubmitReview(
                duplicate_context.buyer.header,
                duplicate_context.order_id,
                {
                    .rating = 4,
                    .content = "duplicate review",
                }
            );
        }
    );
    assert(CountReviewsByOrder(config, project_root, duplicate_context.order_id) == 1);

    const auto status_context = CreateCompletedOrder(
        auth_service,
        item_service,
        item_audit_service,
        auction_service,
        bid_service,
        auction_scheduler,
        order_scheduler,
        payment_service,
        config,
        project_root,
        admin_header,
        unique_suffix + "_status",
        50
    );
    PromoteOrderToCompleted(config, project_root, status_context.order_id);
    auto connection = CreateConnection(config, project_root);
    connection.Execute(
        "UPDATE order_info SET order_status = 'PAID', completed_at = NULL, updated_at = CURRENT_TIMESTAMP(3) "
        "WHERE order_id = " +
        std::to_string(status_context.order_id)
    );
    ExpectReviewError(
        ErrorCode::kReviewOrderStatusInvalid,
        [&]() {
            (void)review_service.SubmitReview(
                status_context.buyer.header,
                status_context.order_id,
                {
                    .rating = 5,
                    .content = "should fail on paid order",
                }
            );
        }
    );
    assert(CountReviewsByOrder(config, project_root, status_context.order_id) == 0);

    const auto outsider_context = CreateCompletedOrder(
        auth_service,
        item_service,
        item_audit_service,
        auction_service,
        bid_service,
        auction_scheduler,
        order_scheduler,
        payment_service,
        config,
        project_root,
        admin_header,
        unique_suffix + "_outsider",
        70
    );
    const auto outsider = RegisterAndLoginUser(
        auth_service,
        unique_suffix + "_outsider_user",
        "review_outsider",
        90
    );
    ExpectReviewError(
        ErrorCode::kReviewPermissionDenied,
        [&]() {
            (void)review_service.SubmitReview(
                outsider.header,
                outsider_context.order_id,
                {
                    .rating = 5,
                    .content = "should not be allowed",
                }
            );
        }
    );
    assert(CountReviewsByOrder(config, project_root, outsider_context.order_id) == 0);

    return 0;
}
