#include "modules/item/item_service.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <stdexcept>
#include <sstream>
#include <json/json.h>

#include "common/db/mysql_connection.h"
#include "common/errors/error_code.h"
#include "common/logging/logger.h"
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
    if (request.suggested_bid_step < 0.0) {
        throw std::invalid_argument("suggested_bid_step must be non-negative");
    }
    if (request.suggested_anti_sniping_window_seconds < 0 ||
        request.suggested_extend_seconds < 0) {
        throw std::invalid_argument("suggested anti-sniping config must be non-negative");
    }
    ValidateTitle(TrimWhitespace(request.title));
    ValidateDescription(TrimWhitespace(request.description));
    if (TrimWhitespace(request.trade_mode).empty()) {
        throw std::invalid_argument("trade_mode must not be empty");
    }
    if (TrimWhitespace(request.location).empty()) {
        throw std::invalid_argument("location must not be empty");
    }
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
    if (request.trade_mode.has_value() && TrimWhitespace(*request.trade_mode).empty()) {
        throw std::invalid_argument("trade_mode must not be empty");
    }
    if (request.location.has_value() && TrimWhitespace(*request.location).empty()) {
        throw std::invalid_argument("location must not be empty");
    }
    if (request.suggested_bid_step.has_value() && *request.suggested_bid_step < 0.0) {
        throw std::invalid_argument("suggested_bid_step must be non-negative");
    }
    if (request.suggested_anti_sniping_window_seconds.has_value() &&
        *request.suggested_anti_sniping_window_seconds < 0) {
        throw std::invalid_argument("suggested_anti_sniping_window_seconds must be non-negative");
    }
    if (request.suggested_extend_seconds.has_value() &&
        *request.suggested_extend_seconds < 0) {
        throw std::invalid_argument("suggested_extend_seconds must be non-negative");
    }
    if (request.cover_image_url.has_value()) {
        ValidateCoverImageUrl(TrimWhitespace(*request.cover_image_url));
    }
}

std::string NormalizeTagsJson(const std::string& tags_json) {
    const auto trimmed = TrimWhitespace(tags_json);
    if (trimmed.empty()) {
        return "[]";
    }

    Json::CharReaderBuilder builder;
    Json::Value root;
    std::string errors;
    std::istringstream stream(trimmed);
    if (!Json::parseFromStream(builder, stream, &root, &errors) || !root.isArray()) {
        throw std::invalid_argument("tags_json must be a JSON array");
    }

    Json::Value normalized(Json::arrayValue);
    for (const auto& entry : root) {
        if (!entry.isString()) {
            throw std::invalid_argument("tags_json entries must be strings");
        }
        const auto tag = TrimWhitespace(entry.asString());
        if (!tag.empty()) {
            normalized.append(tag);
        }
    }

    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    return Json::writeString(writer, normalized);
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

void CreateStationNoticeSafely(
    modules::notification::NotificationService* notification_service,
    const modules::notification::StationNoticeRequest& request
) {
    if (notification_service == nullptr) {
        return;
    }
    try {
        notification_service->CreateStationNotice(request);
    } catch (const std::exception& exception) {
        common::logging::Logger::Instance().Warn(
            "item station notice create failed: " + std::string(exception.what())
        );
    }
}

}  // namespace

ItemService::ItemService(
    const common::config::AppConfig& config,
    std::filesystem::path project_root,
    middleware::AuthMiddleware& auth_middleware,
    modules::notification::NotificationService* notification_service
) : mysql_config_(config.mysql),
    project_root_(std::move(project_root)),
    auth_middleware_(&auth_middleware),
    notification_service_(notification_service) {}

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
        .trade_mode = TrimWhitespace(request.trade_mode),
        .location = TrimWhitespace(request.location),
        .tags_json = NormalizeTagsJson(request.tags_json),
        .suggested_bid_step = FormatAmount(request.suggested_bid_step),
        .suggested_anti_sniping_window_seconds = request.suggested_anti_sniping_window_seconds,
        .suggested_extend_seconds = request.suggested_extend_seconds,
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
        .trade_mode = request.trade_mode.has_value() ? TrimWhitespace(*request.trade_mode)
                                                     : item.trade_mode,
        .location = request.location.has_value() ? TrimWhitespace(*request.location)
                                                 : item.location,
        .tags_json = request.tags_json.has_value() ? NormalizeTagsJson(*request.tags_json)
                                                   : item.tags_json,
        .suggested_bid_step = request.suggested_bid_step.has_value()
                                  ? FormatAmount(*request.suggested_bid_step)
                                  : FormatAmount(item.suggested_bid_step),
        .suggested_anti_sniping_window_seconds =
            request.suggested_anti_sniping_window_seconds.value_or(
                item.suggested_anti_sniping_window_seconds
            ),
        .suggested_extend_seconds = request.suggested_extend_seconds.value_or(
            item.suggested_extend_seconds
        ),
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
    CreateStationNoticeSafely(
        notification_service_,
        modules::notification::StationNoticeRequest{
            .user_id = submitted.seller_id,
            .notice_type = "ITEM_SUBMITTED_FOR_REVIEW",
            .title = "Item submitted for review",
            .content = "Item \"" + submitted.title +
                       "\" has been submitted. Please wait for administrator review.",
            .biz_type = "ITEM",
            .biz_id = submitted.item_id,
        }
    );
    for (const auto admin_id : repository.ListActiveAdminUserIds()) {
        CreateStationNoticeSafely(
            notification_service_,
            modules::notification::StationNoticeRequest{
                .user_id = admin_id,
                .notice_type = "ITEM_REVIEW_REQUIRED",
                .title = "New item pending review",
                .content = "Seller submitted item \"" + submitted.title +
                           "\". Please review it in the admin dashboard.",
                .biz_type = "ITEM",
                .biz_id = submitted.item_id,
            }
        );
    }
    return SubmitAuditResult{
        .item_id = submitted.item_id,
        .item_status = submitted.item_status,
        .submitted_at = submitted.updated_at,
    };
}

UpdateItemResult ItemService::OfflineItem(
    const std::string_view authorization_header,
    const std::uint64_t item_id
) {
    const auto seller_context = RequireSeller(authorization_header);

    auto connection = CreateConnection();
    repository::ItemRepository repository(connection);
    const auto item = LoadItemOrThrow(repository, item_id);
    EnsureOwner(seller_context, item);

    if (!IsValidOfflineItemStatus(item.item_status)) {
        ThrowItemError(
            common::errors::ErrorCode::kItemEditStatusInvalid,
            "item status does not allow offline"
        );
    }

    connection.BeginTransaction();
    try {
        repository.UpdateItemStatus(item_id, std::string(kItemStatusOffline), "");
        connection.Commit();
    } catch (...) {
        connection.Rollback();
        throw;
    }

    const auto offlined = LoadItemOrThrow(repository, item_id);
    return UpdateItemResult{
        .item_id = offlined.item_id,
        .item_status = offlined.item_status,
        .updated_at = offlined.updated_at,
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
