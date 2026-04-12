#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace auction::modules::item {

inline constexpr std::string_view kItemStatusDraft = "DRAFT";
inline constexpr std::string_view kItemStatusPendingAudit = "PENDING_AUDIT";
inline constexpr std::string_view kItemStatusRejected = "REJECTED";
inline constexpr std::string_view kItemStatusReadyForAuction = "READY_FOR_AUCTION";
inline constexpr std::string_view kItemStatusInAuction = "IN_AUCTION";
inline constexpr std::string_view kItemStatusSold = "SOLD";
inline constexpr std::string_view kItemStatusUnsold = "UNSOLD";
inline constexpr std::string_view kItemStatusOffline = "OFFLINE";

inline constexpr std::string_view kAuditResultApproved = "APPROVED";
inline constexpr std::string_view kAuditResultRejected = "REJECTED";

inline bool IsEditableItemStatus(const std::string_view item_status) {
    return item_status == kItemStatusDraft || item_status == kItemStatusRejected;
}

inline bool IsValidSubmitAuditStatus(const std::string_view item_status) {
    return item_status == kItemStatusDraft;
}

inline bool IsValidAuditItemStatus(const std::string_view item_status) {
    return item_status == kItemStatusPendingAudit;
}

inline bool IsValidAuditResult(const std::string_view audit_result) {
    return audit_result == kAuditResultApproved || audit_result == kAuditResultRejected;
}

inline bool IsKnownItemStatus(const std::string_view item_status) {
    return item_status == kItemStatusDraft || item_status == kItemStatusPendingAudit ||
           item_status == kItemStatusRejected || item_status == kItemStatusReadyForAuction ||
           item_status == kItemStatusInAuction || item_status == kItemStatusSold ||
           item_status == kItemStatusUnsold || item_status == kItemStatusOffline;
}

struct ItemRecord {
    std::uint64_t item_id{0};
    std::uint64_t seller_id{0};
    std::uint64_t category_id{0};
    std::string title;
    std::string description;
    double start_price{0.0};
    std::string item_status;
    std::string reject_reason;
    std::string cover_image_url;
    std::string created_at;
    std::string updated_at;
};

struct ItemImageRecord {
    std::uint64_t image_id{0};
    std::uint64_t item_id{0};
    std::string image_url;
    int sort_no{0};
    bool is_cover{false};
    std::string created_at;
};

struct ItemAuditLogRecord {
    std::uint64_t audit_log_id{0};
    std::uint64_t item_id{0};
    std::uint64_t admin_id{0};
    std::string admin_username;
    std::string audit_result;
    std::string audit_comment;
    std::string created_at;
};

struct CreateItemRequest {
    std::string title;
    std::string description;
    std::uint64_t category_id{0};
    double start_price{0.0};
    std::string cover_image_url;
};

struct UpdateItemRequest {
    std::optional<std::string> title;
    std::optional<std::string> description;
    std::optional<std::uint64_t> category_id;
    std::optional<double> start_price;
    std::optional<std::string> cover_image_url;
};

struct AddItemImageRequest {
    std::string image_url;
    std::optional<int> sort_no;
    bool is_cover{false};
};

struct CreateItemResult {
    std::uint64_t item_id{0};
    std::string item_status;
    std::string created_at;
};

struct UpdateItemResult {
    std::uint64_t item_id{0};
    std::string item_status;
    std::string updated_at;
};

struct SubmitAuditResult {
    std::uint64_t item_id{0};
    std::string item_status;
    std::string submitted_at;
};

struct DeleteItemImageResult {
    std::uint64_t item_id{0};
    std::uint64_t image_id{0};
    std::string item_status;
    std::string cover_image_url;
};

struct ItemSummary {
    std::uint64_t item_id{0};
    std::uint64_t seller_id{0};
    std::uint64_t category_id{0};
    std::string title;
    double start_price{0.0};
    std::string item_status;
    std::string reject_reason;
    std::string cover_image_url;
    std::string created_at;
    std::string updated_at;
};

struct ItemDetail {
    ItemRecord item;
    std::vector<ItemImageRecord> images;
    std::optional<ItemAuditLogRecord> latest_audit_log;
};

struct PendingAuditItemSummary {
    std::uint64_t item_id{0};
    std::uint64_t seller_id{0};
    std::string seller_username;
    std::string seller_nickname;
    std::uint64_t category_id{0};
    std::string title;
    double start_price{0.0};
    std::string created_at;
};

struct AuditItemRequest {
    std::string audit_result;
    std::string reason;
};

struct AuditItemResult {
    std::uint64_t item_id{0};
    std::string old_status;
    std::string new_status;
    std::string audit_result;
    std::string audited_at;
};

}  // namespace auction::modules::item
