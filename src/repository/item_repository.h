#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "modules/item/item_types.h"
#include "repository/base_repository.h"

namespace auction::repository {

struct CreateItemParams {
    std::uint64_t seller_id{0};
    std::uint64_t category_id{0};
    std::string title;
    std::string description;
    std::string start_price;
    std::string cover_image_url;
};

struct UpdateItemParams {
    std::uint64_t item_id{0};
    std::uint64_t category_id{0};
    std::string title;
    std::string description;
    std::string start_price;
    std::string cover_image_url;
    std::string item_status;
    std::string reject_reason;
};

struct CreateItemImageParams {
    std::uint64_t item_id{0};
    std::string image_url;
    int sort_no{0};
    bool is_cover{false};
};

struct CreateItemAuditLogParams {
    std::uint64_t item_id{0};
    std::uint64_t admin_id{0};
    std::string audit_result;
    std::string audit_comment;
};

class ItemRepository : public BaseRepository {
public:
    using BaseRepository::BaseRepository;

    [[nodiscard]] bool CategoryExistsAndActive(std::uint64_t category_id) const;
    [[nodiscard]] std::optional<modules::item::ItemRecord> FindItemById(std::uint64_t item_id) const;
    [[nodiscard]] modules::item::ItemRecord CreateItem(const CreateItemParams& params) const;
    [[nodiscard]] modules::item::ItemRecord UpdateItem(const UpdateItemParams& params) const;
    [[nodiscard]] std::vector<modules::item::ItemSummary> ListItemsBySeller(
        std::uint64_t seller_id,
        const std::optional<std::string>& item_status
    ) const;
    [[nodiscard]] std::vector<modules::item::PendingAuditItemSummary> ListPendingAuditItems() const;

    [[nodiscard]] std::optional<modules::item::ItemImageRecord> FindImageById(
        std::uint64_t item_id,
        std::uint64_t image_id
    ) const;
    [[nodiscard]] std::optional<modules::item::ItemImageRecord> FindFirstImageBySort(
        std::uint64_t item_id
    ) const;
    [[nodiscard]] std::vector<modules::item::ItemImageRecord> ListItemImages(
        std::uint64_t item_id
    ) const;
    [[nodiscard]] std::uint64_t CountItemImages(std::uint64_t item_id) const;
    [[nodiscard]] int NextImageSortNo(std::uint64_t item_id) const;
    [[nodiscard]] modules::item::ItemImageRecord CreateItemImage(
        const CreateItemImageParams& params
    ) const;
    void DeleteItemImage(std::uint64_t item_id, std::uint64_t image_id) const;
    void ClearItemImageCoverFlags(std::uint64_t item_id) const;
    void UpdateImageCoverFlag(std::uint64_t image_id, bool is_cover) const;
    void UpdateItemCoverImage(std::uint64_t item_id, const std::string& cover_image_url) const;

    void UpdateItemStatus(
        std::uint64_t item_id,
        const std::string& item_status,
        const std::string& reject_reason
    ) const;
    void InsertAuditLog(const CreateItemAuditLogParams& params) const;
    [[nodiscard]] std::optional<modules::item::ItemAuditLogRecord> FindLatestAuditLog(
        std::uint64_t item_id
    ) const;
    [[nodiscard]] std::vector<modules::item::ItemAuditLogRecord> ListAuditLogs(
        std::uint64_t item_id
    ) const;
};

}  // namespace auction::repository
