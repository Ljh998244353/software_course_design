#include <algorithm>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

#include "common/config/config_loader.h"
#include "common/db/mysql_connection.h"
#include "common/errors/error_code.h"
#include "jobs/auction_scheduler.h"
#include "middleware/auth_middleware.h"
#include "modules/audit/item_audit_service.h"
#include "modules/auction/auction_exception.h"
#include "modules/auction/auction_service.h"
#include "modules/auth/auth_service.h"
#include "modules/auth/auth_session_store.h"
#include "modules/item/item_service.h"

namespace {

using auction::common::errors::ErrorCode;
using auction::modules::auction::AuctionException;

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

template <typename Fn>
void ExpectAuctionError(const ErrorCode expected_code, Fn&& function) {
    try {
        function();
        assert(false && "expected auction exception");
    } catch (const AuctionException& exception) {
        assert(exception.code() == expected_code);
    }
}

template <typename T>
bool Contains(const std::vector<T>& values, const T& expected) {
    return std::find(values.begin(), values.end(), expected) != values.end();
}

auction::common::db::MysqlConnection CreateConnection(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root
) {
    return auction::common::db::MysqlConnection(config.mysql, project_root);
}

void UpdateAuctionToStartNow(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t auction_id
) {
    auto connection = CreateConnection(config, project_root);
    connection.Execute(
        "UPDATE auction SET start_time = DATE_SUB(CURRENT_TIMESTAMP(3), INTERVAL 5 SECOND), "
        "end_time = DATE_ADD(CURRENT_TIMESTAMP(3), INTERVAL 5 MINUTE) WHERE auction_id = " +
        std::to_string(auction_id)
    );
}

void UpdateAuctionToFinishNow(
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

void ExtendAuctionEndTime(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t auction_id
) {
    auto connection = CreateConnection(config, project_root);
    connection.Execute(
        "UPDATE auction SET end_time = DATE_ADD(CURRENT_TIMESTAMP(3), INTERVAL 10 MINUTE) "
        "WHERE auction_id = " +
        std::to_string(auction_id)
    );
}

void SetWinningBidSnapshot(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::uint64_t auction_id,
    const std::uint64_t highest_bidder_id,
    const std::string& current_price
) {
    auto connection = CreateConnection(config, project_root);
    connection.Execute(
        "UPDATE auction SET highest_bidder_id = " + std::to_string(highest_bidder_id) +
        ", current_price = " + current_price + " WHERE auction_id = " +
        std::to_string(auction_id)
    );
}

std::uint64_t QueryTaskLogCount(
    const auction::common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::string& task_type,
    const std::uint64_t auction_id
) {
    auto connection = CreateConnection(config, project_root);
    return connection.QueryCount(
        "SELECT COUNT(*) FROM task_log WHERE task_type = '" + task_type + "' AND biz_key = '" +
        std::to_string(auction_id) + "'"
    );
}

std::uint64_t CreateApprovedItem(
    auction::modules::item::ItemService& item_service,
    auction::modules::audit::ItemAuditService& item_audit_service,
    const std::string& seller_header,
    const std::string& admin_header,
    const std::string& unique_suffix,
    const std::string& title_prefix
) {
    const auto created = item_service.CreateDraftItem(
        seller_header,
        {
            .title = title_prefix + " " + unique_suffix,
            .description = "用于 S06 自动化测试",
            .category_id = 1,
            .start_price = 99.0,
            .cover_image_url = "",
        }
    );
    const auto image = item_service.AddItemImage(
        seller_header,
        created.item_id,
        {
            .image_url = "/images/" + unique_suffix + "/" + title_prefix + ".jpg",
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
            .reason = "审核通过",
        }
    );
    assert(approved.new_status == "READY_FOR_AUCTION");
    return created.item_id;
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
    auction::jobs::AuctionScheduler auction_scheduler(auction_service);

    const auto unique_suffix = BuildUniqueSuffix();
    const auto seller_username = "auction_seller_" + unique_suffix;
    const auto buyer_username = "auction_buyer_" + unique_suffix;
    const auto seller_password = "SellerPass1";
    const auto buyer_password = "BuyerPass1";

    const auto seller_registered = auth_service.RegisterUser({
        .username = seller_username,
        .password = seller_password,
        .phone = "135" + unique_suffix.substr(unique_suffix.size() - 8),
        .email = seller_username + "@auction.local",
        .nickname = "seller",
    });
    const auto buyer_registered = auth_service.RegisterUser({
        .username = buyer_username,
        .password = buyer_password,
        .phone = "139" + unique_suffix.substr(unique_suffix.size() - 8),
        .email = buyer_username + "@auction.local",
        .nickname = "buyer",
    });
    assert(seller_registered.user_id > 0);
    assert(buyer_registered.user_id > 0);

    const auto seller_login = auth_service.Login({
        .principal = seller_username,
        .password = seller_password,
    });
    const auto admin_login = auth_service.Login({
        .principal = "admin",
        .password = "Admin@123",
    });
    const auto support_login = auth_service.Login({
        .principal = "support",
        .password = "Support@123",
    });

    const auto seller_header = "Bearer " + seller_login.token;
    const auto admin_header = "Bearer " + admin_login.token;
    const auto support_header = "Bearer " + support_login.token;

    const auto item_id_1 = CreateApprovedItem(
        item_service,
        item_audit_service,
        seller_header,
        admin_header,
        unique_suffix + "_1",
        "拍卖物品一"
    );
    const auto created_auction_1 = auction_service.CreateAuction(
        admin_header,
        {
            .item_id = item_id_1,
            .start_time = FormatNowOffsetSeconds(600),
            .end_time = FormatNowOffsetSeconds(1200),
            .start_price = 120.0,
            .bid_step = 10.0,
            .anti_sniping_window_seconds = 0,
            .extend_seconds = 0,
        }
    );
    assert(created_auction_1.auction_id > 0);
    assert(created_auction_1.status == "PENDING_START");
    assert(created_auction_1.current_price == 120.0);

    const auto admin_auctions = auction_service.ListAdminAuctions(
        support_header,
        {
            .status = std::optional<std::string>("PENDING_START"),
            .item_id = std::nullopt,
            .seller_id = std::optional<std::uint64_t>(seller_registered.user_id),
            .page_no = 1,
            .page_size = 20,
        }
    );
    assert(!admin_auctions.empty());
    assert(std::any_of(admin_auctions.begin(), admin_auctions.end(), [&](const auto& auction) {
        return auction.auction_id == created_auction_1.auction_id;
    }));

    const auto admin_detail_1 =
        auction_service.GetAdminAuctionDetail(support_header, created_auction_1.auction_id);
    assert(admin_detail_1.auction.status == "PENDING_START");
    assert(admin_detail_1.item.item_id == item_id_1);
    assert(admin_detail_1.seller_username == seller_username);

    const auto public_auctions = auction_service.ListPublicAuctions({
        .keyword = std::optional<std::string>(unique_suffix),
        .status = std::nullopt,
        .page_no = 1,
        .page_size = 20,
    });
    assert(std::any_of(public_auctions.begin(), public_auctions.end(), [&](const auto& auction) {
        return auction.auction_id == created_auction_1.auction_id;
    }));

    const auto public_detail_1 = auction_service.GetPublicAuctionDetail(created_auction_1.auction_id);
    assert(public_detail_1.highest_bidder_masked.empty());

    ExpectAuctionError(ErrorCode::kAuctionDuplicateActiveByItem, [&] {
        const auto ignored = auction_service.CreateAuction(
            admin_header,
            {
                .item_id = item_id_1,
                .start_time = FormatNowOffsetSeconds(700),
                .end_time = FormatNowOffsetSeconds(1300),
                .start_price = 130.0,
                .bid_step = 5.0,
                .anti_sniping_window_seconds = 0,
                .extend_seconds = 0,
            }
        );
        (void)ignored;
    });

    const auto draft_item = item_service.CreateDraftItem(
        seller_header,
        {
            .title = "未审核拍品 " + unique_suffix,
            .description = "未审核",
            .category_id = 1,
            .start_price = 66.0,
            .cover_image_url = "",
        }
    );
    const auto draft_image = item_service.AddItemImage(
        seller_header,
        draft_item.item_id,
        {
            .image_url = "/images/" + unique_suffix + "/draft.jpg",
            .sort_no = std::nullopt,
            .is_cover = true,
        }
    );
    assert(draft_image.image_id > 0);
    ExpectAuctionError(ErrorCode::kAuctionItemStatusInvalid, [&] {
        const auto ignored = auction_service.CreateAuction(
            admin_header,
            {
                .item_id = draft_item.item_id,
                .start_time = FormatNowOffsetSeconds(600),
                .end_time = FormatNowOffsetSeconds(1200),
                .start_price = 100.0,
                .bid_step = 5.0,
                .anti_sniping_window_seconds = 0,
                .extend_seconds = 0,
            }
        );
        (void)ignored;
    });

    ExpectAuctionError(ErrorCode::kAuctionEndTimeInvalid, [&] {
        const auto ignored = auction_service.CreateAuction(
            admin_header,
            {
                .item_id = item_id_1,
                .start_time = FormatNowOffsetSeconds(1200),
                .end_time = FormatNowOffsetSeconds(600),
                .start_price = 100.0,
                .bid_step = 5.0,
                .anti_sniping_window_seconds = 0,
                .extend_seconds = 0,
            }
        );
        (void)ignored;
    });

    ExpectAuctionError(ErrorCode::kAuctionAntiSnipingInvalid, [&] {
        const auto ignored = auction_service.CreateAuction(
            admin_header,
            {
                .item_id = item_id_1,
                .start_time = FormatNowOffsetSeconds(1200),
                .end_time = FormatNowOffsetSeconds(1800),
                .start_price = 100.0,
                .bid_step = 5.0,
                .anti_sniping_window_seconds = 60,
                .extend_seconds = 0,
            }
        );
        (void)ignored;
    });

    const auto updated_auction_1 = auction_service.UpdatePendingAuction(
        admin_header,
        created_auction_1.auction_id,
        {
            .start_time = std::optional<std::string>(FormatNowOffsetSeconds(900)),
            .end_time = std::optional<std::string>(FormatNowOffsetSeconds(1500)),
            .start_price = std::optional<double>(150.0),
            .bid_step = std::optional<double>(15.0),
            .anti_sniping_window_seconds = std::optional<int>(30),
            .extend_seconds = std::optional<int>(30),
        }
    );
    assert(updated_auction_1.status == "PENDING_START");
    const auto updated_detail_1 =
        auction_service.GetAdminAuctionDetail(admin_header, created_auction_1.auction_id);
    assert(updated_detail_1.auction.start_price == 150.0);
    assert(updated_detail_1.auction.current_price == 150.0);
    assert(updated_detail_1.auction.bid_step == 15.0);
    assert(updated_detail_1.auction.anti_sniping_window_seconds == 30);
    assert(updated_detail_1.auction.extend_seconds == 30);

    const auto cancelled_auction_1 = auction_service.CancelPendingAuction(
        admin_header,
        created_auction_1.auction_id,
        {
            .reason = "测试取消",
        }
    );
    assert(cancelled_auction_1.old_status == "PENDING_START");
    assert(cancelled_auction_1.new_status == "CANCELLED");
    const auto item_detail_after_cancel = item_service.GetItemDetail(admin_header, item_id_1);
    assert(item_detail_after_cancel.item.item_status == "READY_FOR_AUCTION");
    const auto public_after_cancel = auction_service.ListPublicAuctions({
        .keyword = std::optional<std::string>(unique_suffix + "_1"),
        .status = std::nullopt,
        .page_no = 1,
        .page_size = 20,
    });
    assert(std::none_of(public_after_cancel.begin(), public_after_cancel.end(), [&](const auto& auction) {
        return auction.auction_id == created_auction_1.auction_id;
    }));

    const auto recreated_auction = auction_service.CreateAuction(
        admin_header,
        {
            .item_id = item_id_1,
            .start_time = FormatNowOffsetSeconds(600),
            .end_time = FormatNowOffsetSeconds(1200),
            .start_price = 180.0,
            .bid_step = 10.0,
            .anti_sniping_window_seconds = 0,
            .extend_seconds = 0,
        }
    );
    assert(recreated_auction.auction_id > created_auction_1.auction_id);

    const auto item_id_2 = CreateApprovedItem(
        item_service,
        item_audit_service,
        seller_header,
        admin_header,
        unique_suffix + "_2",
        "拍卖物品二"
    );
    const auto created_auction_2 = auction_service.CreateAuction(
        admin_header,
        {
            .item_id = item_id_2,
            .start_time = FormatNowOffsetSeconds(600),
            .end_time = FormatNowOffsetSeconds(1200),
            .start_price = 200.0,
            .bid_step = 20.0,
            .anti_sniping_window_seconds = 0,
            .extend_seconds = 0,
        }
    );
    UpdateAuctionToStartNow(config, project_root, created_auction_2.auction_id);
    const auto start_result_1 = auction_scheduler.RunStartCycle();
    assert(Contains(start_result_1.affected_auction_ids, created_auction_2.auction_id));
    const auto running_detail =
        auction_service.GetAdminAuctionDetail(admin_header, created_auction_2.auction_id);
    assert(running_detail.auction.status == "RUNNING");
    const auto item_detail_running = item_service.GetItemDetail(admin_header, item_id_2);
    assert(item_detail_running.item.item_status == "IN_AUCTION");
    assert(QueryTaskLogCount(config, project_root, "AUCTION_START", created_auction_2.auction_id) >= 1);

    const auto start_result_repeat = auction_scheduler.RunStartCycle();
    assert(!Contains(start_result_repeat.affected_auction_ids, created_auction_2.auction_id));

    ExpectAuctionError(ErrorCode::kAuctionUpdateStatusInvalid, [&] {
        const auto ignored = auction_service.UpdatePendingAuction(
            admin_header,
            created_auction_2.auction_id,
            {
                .start_time = std::optional<std::string>(FormatNowOffsetSeconds(700)),
                .end_time = std::nullopt,
                .start_price = std::nullopt,
                .bid_step = std::nullopt,
                .anti_sniping_window_seconds = std::nullopt,
                .extend_seconds = std::nullopt,
            }
        );
        (void)ignored;
    });
    ExpectAuctionError(ErrorCode::kAuctionCancelStatusInvalid, [&] {
        const auto ignored = auction_service.CancelPendingAuction(
            admin_header,
            created_auction_2.auction_id,
            {
                .reason = "不允许取消",
            }
        );
        (void)ignored;
    });

    UpdateAuctionToFinishNow(config, project_root, created_auction_2.auction_id);
    const auto finish_result_unsold = auction_scheduler.RunFinishCycle();
    assert(Contains(finish_result_unsold.affected_auction_ids, created_auction_2.auction_id));
    const auto finished_unsold_detail =
        auction_service.GetAdminAuctionDetail(admin_header, created_auction_2.auction_id);
    assert(finished_unsold_detail.auction.status == "UNSOLD");
    const auto item_detail_unsold = item_service.GetItemDetail(admin_header, item_id_2);
    assert(item_detail_unsold.item.item_status == "UNSOLD");
    assert(QueryTaskLogCount(config, project_root, "AUCTION_FINISH", created_auction_2.auction_id) >= 1);

    const auto item_id_3 = CreateApprovedItem(
        item_service,
        item_audit_service,
        seller_header,
        admin_header,
        unique_suffix + "_3",
        "拍卖物品三"
    );
    const auto created_auction_3 = auction_service.CreateAuction(
        admin_header,
        {
            .item_id = item_id_3,
            .start_time = FormatNowOffsetSeconds(600),
            .end_time = FormatNowOffsetSeconds(1200),
            .start_price = 300.0,
            .bid_step = 30.0,
            .anti_sniping_window_seconds = 0,
            .extend_seconds = 0,
        }
    );
    UpdateAuctionToStartNow(config, project_root, created_auction_3.auction_id);
    const auto start_result_3 = auction_scheduler.RunStartCycle();
    assert(Contains(start_result_3.affected_auction_ids, created_auction_3.auction_id));
    SetWinningBidSnapshot(
        config,
        project_root,
        created_auction_3.auction_id,
        buyer_registered.user_id,
        "399.00"
    );
    UpdateAuctionToFinishNow(config, project_root, created_auction_3.auction_id);
    const auto finish_result_settling = auction_scheduler.RunFinishCycle();
    assert(Contains(finish_result_settling.affected_auction_ids, created_auction_3.auction_id));
    const auto settling_detail =
        auction_service.GetAdminAuctionDetail(admin_header, created_auction_3.auction_id);
    assert(settling_detail.auction.status == "SETTLING");
    assert(settling_detail.auction.current_price == 399.0);
    const auto item_detail_settling = item_service.GetItemDetail(admin_header, item_id_3);
    assert(item_detail_settling.item.item_status == "IN_AUCTION");
    const auto public_detail_3 = auction_service.GetPublicAuctionDetail(created_auction_3.auction_id);
    assert(!public_detail_3.highest_bidder_masked.empty());

    const auto item_id_4 = CreateApprovedItem(
        item_service,
        item_audit_service,
        seller_header,
        admin_header,
        unique_suffix + "_4",
        "拍卖物品四"
    );
    const auto created_auction_4 = auction_service.CreateAuction(
        admin_header,
        {
            .item_id = item_id_4,
            .start_time = FormatNowOffsetSeconds(600),
            .end_time = FormatNowOffsetSeconds(1200),
            .start_price = 400.0,
            .bid_step = 40.0,
            .anti_sniping_window_seconds = 60,
            .extend_seconds = 30,
        }
    );
    UpdateAuctionToStartNow(config, project_root, created_auction_4.auction_id);
    const auto start_result_4 = auction_scheduler.RunStartCycle();
    assert(Contains(start_result_4.affected_auction_ids, created_auction_4.auction_id));
    ExtendAuctionEndTime(config, project_root, created_auction_4.auction_id);
    const auto finish_result_delayed = auction_scheduler.RunFinishCycle();
    assert(!Contains(finish_result_delayed.affected_auction_ids, created_auction_4.auction_id));
    const auto delayed_detail =
        auction_service.GetAdminAuctionDetail(admin_header, created_auction_4.auction_id);
    assert(delayed_detail.auction.status == "RUNNING");

    return 0;
}
