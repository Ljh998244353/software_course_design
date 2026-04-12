#include <algorithm>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>

#include "common/config/config_loader.h"
#include "common/errors/error_code.h"
#include "middleware/auth_middleware.h"
#include "modules/audit/item_audit_service.h"
#include "modules/auth/auth_exception.h"
#include "modules/auth/auth_service.h"
#include "modules/auth/auth_session_store.h"
#include "modules/item/item_exception.h"
#include "modules/item/item_service.h"

namespace {

using auction::common::errors::ErrorCode;
using auction::modules::auth::AuthException;
using auction::modules::item::ItemException;

std::string BuildUniqueSuffix() {
    return std::to_string(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        )
            .count()
    );
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
void ExpectItemError(const ErrorCode expected_code, Fn&& function) {
    try {
        function();
        assert(false && "expected item exception");
    } catch (const ItemException& exception) {
        assert(exception.code() == expected_code);
    }
}

template <typename Fn>
void ExpectInvalidArgument(Fn&& function) {
    try {
        function();
        assert(false && "expected invalid_argument");
    } catch (const std::invalid_argument&) {
    }
}

template <typename T, typename Pred>
const T& FindOrFail(const std::vector<T>& values, Pred&& predicate) {
    const auto iterator = std::find_if(values.begin(), values.end(), predicate);
    assert(iterator != values.end());
    return *iterator;
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

    const auto unique_suffix = BuildUniqueSuffix();
    const auto seller_username = "seller_" + unique_suffix;
    const auto other_username = "seller_other_" + unique_suffix;
    const auto seller_password = "SellerPass1";
    const auto other_password = "SellerPass2";

    const auto seller_registered = auth_service.RegisterUser({
        .username = seller_username,
        .password = seller_password,
        .phone = "137" + unique_suffix.substr(unique_suffix.size() - 8),
        .email = seller_username + "@auction.local",
        .nickname = "seller",
    });
    assert(seller_registered.user_id > 0);

    const auto other_registered = auth_service.RegisterUser({
        .username = other_username,
        .password = other_password,
        .phone = "136" + unique_suffix.substr(unique_suffix.size() - 8),
        .email = other_username + "@auction.local",
        .nickname = "other",
    });
    assert(other_registered.user_id > 0);

    const auto seller_login = auth_service.Login({
        .principal = seller_username,
        .password = seller_password,
    });
    const auto other_login = auth_service.Login({
        .principal = other_username,
        .password = other_password,
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
    const auto other_header = "Bearer " + other_login.token;
    const auto admin_header = "Bearer " + admin_login.token;
    const auto support_header = "Bearer " + support_login.token;

    const auto created = item_service.CreateDraftItem(
        seller_header,
        {
            .title = "课程设计相机 " + unique_suffix,
            .description = "成色较新，含原装配件",
            .category_id = 1,
            .start_price = 88.0,
            .cover_image_url = "",
        }
    );
    assert(created.item_id > 0);
    assert(created.item_status == "DRAFT");

    ExpectItemError(ErrorCode::kItemOwnerMismatch, [&] {
        const auto ignored = item_service.GetItemDetail(other_header, created.item_id);
        (void)ignored;
    });

    const auto image_1 = item_service.AddItemImage(
        seller_header,
        created.item_id,
        {
            .image_url = "/images/" + unique_suffix + "/front.jpg",
            .sort_no = std::nullopt,
            .is_cover = false,
        }
    );
    assert(image_1.image_id > 0);
    assert(image_1.sort_no == 1);
    assert(image_1.is_cover);

    const auto image_2 = item_service.AddItemImage(
        seller_header,
        created.item_id,
        {
            .image_url = "/images/" + unique_suffix + "/back.jpg",
            .sort_no = std::nullopt,
            .is_cover = false,
        }
    );
    assert(image_2.sort_no == 2);
    assert(!image_2.is_cover);

    const auto detail_after_add = item_service.GetItemDetail(seller_header, created.item_id);
    assert(detail_after_add.images.size() == 2);
    assert(detail_after_add.item.cover_image_url == image_1.image_url);

    const auto deleted = item_service.DeleteItemImage(
        seller_header,
        created.item_id,
        image_1.image_id
    );
    assert(deleted.cover_image_url == image_2.image_url);
    assert(deleted.item_status == "DRAFT");

    const auto detail_after_delete = item_service.GetItemDetail(seller_header, created.item_id);
    assert(detail_after_delete.images.size() == 1);
    assert(detail_after_delete.images.front().image_id == image_2.image_id);
    assert(detail_after_delete.images.front().is_cover);

    const auto mine_draft = item_service.ListMyItems(seller_header, std::string("DRAFT"));
    const auto& created_draft = FindOrFail(mine_draft, [&](const auto& row) {
        return row.item_id == created.item_id;
    });
    assert(created_draft.item_status == "DRAFT");

    const auto submitted = item_service.SubmitForAudit(seller_header, created.item_id);
    assert(submitted.item_status == "PENDING_AUDIT");

    ExpectItemError(ErrorCode::kItemEditStatusInvalid, [&] {
        const auto ignored = item_service.UpdateItem(
            seller_header,
            created.item_id,
            {
                .title = std::string("不允许编辑"),
                .description = std::nullopt,
                .category_id = std::nullopt,
                .start_price = std::nullopt,
                .cover_image_url = std::nullopt,
            }
        );
        (void)ignored;
    });

    ExpectAuthError(ErrorCode::kAuthPermissionDenied, [&] {
        const auto ignored = item_audit_service.ListPendingItems(seller_header);
        (void)ignored;
    });

    const auto pending_items = item_audit_service.ListPendingItems(admin_header);
    const auto& pending_item = FindOrFail(pending_items, [&](const auto& row) {
        return row.item_id == created.item_id;
    });
    assert(pending_item.seller_id == seller_registered.user_id);
    assert(pending_item.title.find(unique_suffix) != std::string::npos);

    ExpectItemError(ErrorCode::kItemAuditReasonRequired, [&] {
        const auto ignored = item_audit_service.AuditItem(
            admin_header,
            created.item_id,
            {
                .audit_result = "REJECTED",
                .reason = "",
            }
        );
        (void)ignored;
    });

    const auto rejected = item_audit_service.AuditItem(
        admin_header,
        created.item_id,
        {
            .audit_result = "REJECTED",
            .reason = "图片说明不足",
        }
    );
    assert(rejected.old_status == "PENDING_AUDIT");
    assert(rejected.new_status == "REJECTED");

    const auto detail_after_reject = item_service.GetItemDetail(admin_header, created.item_id);
    assert(detail_after_reject.item.item_status == "REJECTED");
    assert(detail_after_reject.item.reject_reason == "图片说明不足");
    assert(detail_after_reject.latest_audit_log.has_value());
    assert(detail_after_reject.latest_audit_log->audit_result == "REJECTED");

    const auto support_logs = item_audit_service.GetAuditLogs(support_header, created.item_id);
    assert(support_logs.size() == 1);
    assert(support_logs.front().audit_comment == "图片说明不足");

    const auto updated = item_service.UpdateItem(
        seller_header,
        created.item_id,
        {
            .title = std::string("课程设计相机二次修改 ") + unique_suffix,
            .description = std::nullopt,
            .category_id = std::nullopt,
            .start_price = std::nullopt,
            .cover_image_url = std::nullopt,
        }
    );
    assert(updated.item_status == "DRAFT");

    const auto image_3 = item_service.AddItemImage(
        seller_header,
        created.item_id,
        {
            .image_url = "/images/" + unique_suffix + "/detail.jpg",
            .sort_no = 3,
            .is_cover = true,
        }
    );
    assert(image_3.is_cover);

    ExpectItemError(ErrorCode::kItemOwnerMismatch, [&] {
        const auto ignored = item_service.UpdateItem(
            other_header,
            created.item_id,
            {
                .title = std::string("非法修改"),
                .description = std::nullopt,
                .category_id = std::nullopt,
                .start_price = std::nullopt,
                .cover_image_url = std::nullopt,
            }
        );
        (void)ignored;
    });

    const auto detail_after_fix = item_service.GetItemDetail(seller_header, created.item_id);
    assert(detail_after_fix.item.item_status == "DRAFT");
    assert(detail_after_fix.item.reject_reason.empty());
    assert(detail_after_fix.images.size() == 2);
    assert(detail_after_fix.item.cover_image_url == image_3.image_url);

    const auto resubmitted = item_service.SubmitForAudit(seller_header, created.item_id);
    assert(resubmitted.item_status == "PENDING_AUDIT");

    const auto approved = item_audit_service.AuditItem(
        admin_header,
        created.item_id,
        {
            .audit_result = "APPROVED",
            .reason = "审核通过",
        }
    );
    assert(approved.old_status == "PENDING_AUDIT");
    assert(approved.new_status == "READY_FOR_AUCTION");

    const auto final_detail = item_service.GetItemDetail(seller_header, created.item_id);
    assert(final_detail.item.item_status == "READY_FOR_AUCTION");
    assert(final_detail.item.reject_reason.empty());
    assert(final_detail.latest_audit_log.has_value());
    assert(final_detail.latest_audit_log->audit_result == "APPROVED");

    const auto admin_logs = item_audit_service.GetAuditLogs(admin_header, created.item_id);
    assert(admin_logs.size() == 2);
    assert(admin_logs.front().audit_result == "REJECTED");
    assert(admin_logs.back().audit_result == "APPROVED");

    const auto mine_ready =
        item_service.ListMyItems(seller_header, std::string("READY_FOR_AUCTION"));
    const auto& ready_item = FindOrFail(mine_ready, [&](const auto& row) {
        return row.item_id == created.item_id;
    });
    assert(ready_item.item_status == "READY_FOR_AUCTION");

    ExpectInvalidArgument([&] {
        const auto another_created = item_service.CreateDraftItem(
            seller_header,
            {
                .title = "未上传图片拍品 " + unique_suffix,
                .description = "缺少图片",
                .category_id = 1,
                .start_price = 30.0,
                .cover_image_url = "",
            }
        );
        const auto ignored = item_service.SubmitForAudit(seller_header, another_created.item_id);
        (void)ignored;
    });

    std::cout << "auction_item_flow_tests passed\n";
    return 0;
}
