#include "modules/item/item_service.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <stdexcept>
#include <sstream>

#include "common/db/mysql_connection.h"
#include "common/errors/error_code.h"
#include "modules/auth/auth_exception.h"
#include "modules/auth/auth_types.h"
#include "modules/item/item_exception.h"
#include "repository/item_repository.h"

namespace auction::modules::item {

namespace {

std::string TrimWhitespace(std::string value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) {
        value.erase(value.begin());
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
        value.pop_back();
    }
    return value;
}

std::string FormatAmount(const double value) {
    std::ostringstream output;
    output << std::fixed << std::setprecision(2) << (std::round(value * 100.0) / 100.0);
    return output.str();
}

void ThrowItemError(const common::errors::ErrorCode code, const std::string_view message) {
    throw ItemException(code, std::string(message));
}

void EnsureOwner(
    const modules::auth::AuthContext& context,
    const ItemRecord& item
) {
    if (context.user_id != item.seller_id) {
        ThrowItemError(common::errors::ErrorCode::kItemOwnerMismatch, "item owner mismatch");
    }
}

void ValidateTitle(const std::string& title) {
    if (title.empty() || title.size() > 200) {
        throw std::invalid_argument("title must be 1-200 chars");
    }
}

void ValidateDescription(const std::string& description) {
    if (description.empty()) {
        throw std::invalid_argument("description must not be empty");
    }
}

void ValidateCoverImageUrl(const std::string& cover_image_url) {
    if (cover_image_url.size() > 255) {
        throw std::invalid_argument("cover image url must be at most 255 chars");
    }
}

void ValidateImageUrl(const std::string& image_url) {
    if (image_url.empty() || image_url.size() > 255) {
        throw std::invalid_argument("image url must be 1-255 chars");
    }
}

void ValidateCreateRequest(const CreateItemRequest& request) {
    if (request.category_id == 0) {
        throw std::invalid_argument("category_id must be positive");
    }
    if (request.start_price <= 0.0) {
        throw std::invalid_argument("start_price must be positive");
    }
    ValidateTitle(TrimWhitespace(request.title));
    ValidateDescription(TrimWhitespace(request.description));
    ValidateCoverImageUrl(TrimWhitespace(request.cover_image_url));
}

void ValidateUpdateRequest(const UpdateItemRequest& request) {
    const bool has_changes = request.title.has_value() || request.description.has_value() ||
                             request.category_id.has_value() || request.start_price.has_value() ||
                             request.cover_image_url.has_value();
    if (!has_changes) {
        throw std::invalid_argument("at least one field must be provided for update");
    }

    if (request.title.has_value()) {
        ValidateTitle(TrimWhitespace(*request.title));
    }
    if (request.description.has_value()) {
        ValidateDescription(TrimWhitespace(*request.description));
    }
    if (request.category_id.has_value() && *request.category_id == 0) {
        throw std::invalid_argument("category_id must be positive");
    }
    if (request.start_price.has_value() && *request.start_price <= 0.0) {
        throw std::invalid_argument("start_price must be positive");
    }
    if (request.cover_image_url.has_value()) {
        ValidateCoverImageUrl(TrimWhitespace(*request.cover_image_url));
    }
}

void ValidateAddImageRequest(const AddItemImageRequest& request) {
    ValidateImageUrl(TrimWhitespace(request.image_url));
    if (request.sort_no.has_value() && *request.sort_no <= 0) {
        throw std::invalid_argument("sort_no must be positive");
    }
}

void ValidateItemCompleteness(
    const ItemRecord& item,
    const std::uint64_t image_count
) {
    ValidateTitle(TrimWhitespace(item.title));
    ValidateDescription(TrimWhitespace(item.description));
    if (item.category_id == 0) {
        throw std::invalid_argument("item category is required");
    }
    if (item.start_price <= 0.0) {
        throw std::invalid_argument("item start_price must be positive");
    }
    if (image_count == 0) {
        throw std::invalid_argument("item must have at least one image before submit");
    }
}

ItemRecord LoadItemOrThrow(repository::ItemRepository& repository, const std::uint64_t item_id) {
    const auto item = repository.FindItemById(item_id);
    if (!item.has_value()) {
        ThrowItemError(common::errors::ErrorCode::kItemNotFound, "item not found");
    }
    return *item;
}

void EnsureEditableItem(const ItemRecord& item) {
    if (!IsEditableItemStatus(item.item_status)) {
        ThrowItemError(
            common::errors::ErrorCode::kItemEditStatusInvalid,
            "item status does not allow edit"
        );
    }
}

}  // namespace

ItemService::ItemService(
    const common::config::AppConfig& config,
    std::filesystem::path project_root,
    middleware::AuthMiddleware& auth_middleware
) : mysql_config_(config.mysql),
    project_root_(std::move(project_root)),
    auth_middleware_(&auth_middleware) {}

CreateItemResult ItemService::CreateDraftItem(
    const std::string_view authorization_header,
    const CreateItemRequest& request
) {
    const auto seller_context = RequireSeller(authorization_header);
    ValidateCreateRequest(request);

    auto connection = CreateConnection();
    repository::ItemRepository repository(connection);
    if (!repository.CategoryExistsAndActive(request.category_id)) {
        ThrowItemError(common::errors::ErrorCode::kItemCategoryInvalid, "item category invalid");
    }

    const auto created = repository.CreateItem(repository::CreateItemParams{
        .seller_id = seller_context.user_id,
        .category_id = request.category_id,
        .title = TrimWhitespace(request.title),
        .description = TrimWhitespace(request.description),
        .start_price = FormatAmount(request.start_price),
        .cover_image_url = TrimWhitespace(request.cover_image_url),
    });

    return CreateItemResult{
        .item_id = created.item_id,
        .item_status = created.item_status,
        .created_at = created.created_at,
    };
}

UpdateItemResult ItemService::UpdateItem(
    const std::string_view authorization_header,
    const std::uint64_t item_id,
    const UpdateItemRequest& request
) {
    const auto seller_context = RequireSeller(authorization_header);
    ValidateUpdateRequest(request);

    auto connection = CreateConnection();
    repository::ItemRepository repository(connection);
    const auto item = LoadItemOrThrow(repository, item_id);
    EnsureOwner(seller_context, item);
    EnsureEditableItem(item);

    const auto next_category_id = request.category_id.value_or(item.category_id);
    if (!repository.CategoryExistsAndActive(next_category_id)) {
        ThrowItemError(common::errors::ErrorCode::kItemCategoryInvalid, "item category invalid");
    }

    const auto updated = repository.UpdateItem(repository::UpdateItemParams{
        .item_id = item_id,
        .category_id = next_category_id,
        .title = request.title.has_value() ? TrimWhitespace(*request.title) : item.title,
        .description = request.description.has_value() ? TrimWhitespace(*request.description)
                                                      : item.description,
        .start_price = request.start_price.has_value() ? FormatAmount(*request.start_price)
                                                       : FormatAmount(item.start_price),
        .cover_image_url = request.cover_image_url.has_value() ? TrimWhitespace(*request.cover_image_url)
                                                               : item.cover_image_url,
        .item_status = item.item_status == kItemStatusRejected ? std::string(kItemStatusDraft)
                                                               : item.item_status,
        .reject_reason = item.item_status == kItemStatusRejected ? "" : item.reject_reason,
    });

    return UpdateItemResult{
        .item_id = updated.item_id,
        .item_status = updated.item_status,
        .updated_at = updated.updated_at,
    };
}

ItemImageRecord ItemService::AddItemImage(
    const std::string_view authorization_header,
    const std::uint64_t item_id,
    const AddItemImageRequest& request
) {
    const auto seller_context = RequireSeller(authorization_header);
    ValidateAddImageRequest(request);

    auto connection = CreateConnection();
    repository::ItemRepository repository(connection);
    const auto item = LoadItemOrThrow(repository, item_id);
    EnsureOwner(seller_context, item);
    EnsureEditableItem(item);

    const auto image_url = TrimWhitespace(request.image_url);
    const auto image_count = repository.CountItemImages(item_id);
    const auto sort_no = request.sort_no.value_or(repository.NextImageSortNo(item_id));
    const bool should_make_cover =
        request.is_cover || image_count == 0 || item.cover_image_url.empty();

    connection.BeginTransaction();
    try {
        if (item.item_status == kItemStatusRejected) {
            repository.UpdateItemStatus(item_id, std::string(kItemStatusDraft), "");
        }
        if (should_make_cover) {
            repository.ClearItemImageCoverFlags(item_id);
        }

        const auto image = repository.CreateItemImage(repository::CreateItemImageParams{
            .item_id = item_id,
            .image_url = image_url,
            .sort_no = sort_no,
            .is_cover = should_make_cover,
        });

        if (should_make_cover) {
            repository.UpdateItemCoverImage(item_id, image.image_url);
        }

        connection.Commit();
        return image;
    } catch (...) {
        connection.Rollback();
        throw;
    }
}

DeleteItemImageResult ItemService::DeleteItemImage(
    const std::string_view authorization_header,
    const std::uint64_t item_id,
    const std::uint64_t image_id
) {
    const auto seller_context = RequireSeller(authorization_header);

    auto connection = CreateConnection();
    repository::ItemRepository repository(connection);
    const auto item = LoadItemOrThrow(repository, item_id);
    EnsureOwner(seller_context, item);
    EnsureEditableItem(item);

    const auto image = repository.FindImageById(item_id, image_id);
    if (!image.has_value()) {
        ThrowItemError(
            common::errors::ErrorCode::kItemImageInvalid,
            "item image does not exist"
        );
    }

    std::string cover_image_url = item.cover_image_url;
    std::string next_item_status =
        item.item_status == kItemStatusRejected ? std::string(kItemStatusDraft) : item.item_status;

    connection.BeginTransaction();
    try {
        if (item.item_status == kItemStatusRejected) {
            repository.UpdateItemStatus(item_id, next_item_status, "");
        }

        repository.DeleteItemImage(item_id, image_id);

        if (image->is_cover || item.cover_image_url == image->image_url) {
            repository.ClearItemImageCoverFlags(item_id);
            const auto first_image = repository.FindFirstImageBySort(item_id);
            if (first_image.has_value()) {
                repository.UpdateImageCoverFlag(first_image->image_id, true);
                repository.UpdateItemCoverImage(item_id, first_image->image_url);
                cover_image_url = first_image->image_url;
            } else {
                repository.UpdateItemCoverImage(item_id, "");
                cover_image_url.clear();
            }
        }

        if (repository.CountItemImages(item_id) == 0) {
            repository.UpdateItemCoverImage(item_id, "");
            cover_image_url.clear();
        }

        connection.Commit();
    } catch (...) {
        connection.Rollback();
        throw;
    }

    return DeleteItemImageResult{
        .item_id = item_id,
        .image_id = image_id,
        .item_status = std::move(next_item_status),
        .cover_image_url = std::move(cover_image_url),
    };
}

SubmitAuditResult ItemService::SubmitForAudit(
    const std::string_view authorization_header,
    const std::uint64_t item_id
) {
    const auto seller_context = RequireSeller(authorization_header);

    auto connection = CreateConnection();
    repository::ItemRepository repository(connection);
    const auto item = LoadItemOrThrow(repository, item_id);
    EnsureOwner(seller_context, item);

    if (!IsValidSubmitAuditStatus(item.item_status)) {
        ThrowItemError(
            common::errors::ErrorCode::kItemSubmitStatusInvalid,
            "item status does not allow submit audit"
        );
    }
    if (!repository.CategoryExistsAndActive(item.category_id)) {
        ThrowItemError(common::errors::ErrorCode::kItemCategoryInvalid, "item category invalid");
    }

    const auto image_count = repository.CountItemImages(item_id);
    ValidateItemCompleteness(item, image_count);

    connection.BeginTransaction();
    try {
        if (item.cover_image_url.empty()) {
            const auto first_image = repository.FindFirstImageBySort(item_id);
            if (!first_image.has_value()) {
                throw std::invalid_argument("item must have at least one image before submit");
            }
            repository.ClearItemImageCoverFlags(item_id);
            repository.UpdateImageCoverFlag(first_image->image_id, true);
            repository.UpdateItemCoverImage(item_id, first_image->image_url);
        }

        repository.UpdateItemStatus(item_id, std::string(kItemStatusPendingAudit), "");
        connection.Commit();
    } catch (...) {
        connection.Rollback();
        throw;
    }

    const auto submitted = LoadItemOrThrow(repository, item_id);
    return SubmitAuditResult{
        .item_id = submitted.item_id,
        .item_status = submitted.item_status,
        .submitted_at = submitted.updated_at,
    };
}

std::vector<ItemSummary> ItemService::ListMyItems(
    const std::string_view authorization_header,
    const std::optional<std::string>& item_status
) {
    const auto seller_context = RequireSeller(authorization_header);
    if (item_status.has_value() && !IsKnownItemStatus(*item_status)) {
        throw std::invalid_argument("item_status filter is invalid");
    }

    auto connection = CreateConnection();
    repository::ItemRepository repository(connection);
    return repository.ListItemsBySeller(seller_context.user_id, item_status);
}

ItemDetail ItemService::GetItemDetail(
    const std::string_view authorization_header,
    const std::uint64_t item_id
) {
    const auto context = auth_middleware_->RequireAuthenticated(authorization_header);

    auto connection = CreateConnection();
    repository::ItemRepository repository(connection);
    const auto item = LoadItemOrThrow(repository, item_id);

    if (context.role_code == modules::auth::kRoleUser) {
        EnsureOwner(context, item);
    } else if (context.role_code != modules::auth::kRoleAdmin &&
               context.role_code != modules::auth::kRoleSupport) {
        throw modules::auth::AuthException(
            common::errors::ErrorCode::kAuthPermissionDenied,
            "permission denied"
        );
    }

    return ItemDetail{
        .item = item,
        .images = repository.ListItemImages(item_id),
        .latest_audit_log = repository.FindLatestAuditLog(item_id),
    };
}

common::db::MysqlConnection ItemService::CreateConnection() const {
    return common::db::MysqlConnection(mysql_config_, project_root_);
}

modules::auth::AuthContext ItemService::RequireSeller(
    const std::string_view authorization_header
) const {
    return auth_middleware_->RequireAnyRole(authorization_header, {modules::auth::kRoleUser});
}

}  // namespace auction::modules::item
