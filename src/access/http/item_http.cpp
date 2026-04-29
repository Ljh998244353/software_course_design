#include "access/http/item_http.h"

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>

#include "common/http/http_utils.h"
#include "modules/item/item_types.h"

#if AUCTION_HAS_DROGON
#include <drogon/drogon.h>
#endif

namespace auction::access::http {

namespace {

#if AUCTION_HAS_DROGON

std::uint64_t ParsePositiveId(const std::string& raw_value, const std::string& field_name) {
    std::size_t parsed_length = 0;
    const auto value = std::stoull(raw_value, &parsed_length);
    if (parsed_length != raw_value.size() || value == 0) {
        throw std::invalid_argument(field_name + " must be a positive integer");
    }
    return value;
}

std::uint64_t ReadRequiredUInt64(const Json::Value& json, const std::string_view field_name) {
    const std::string field(field_name);
    if (!json.isMember(field) || json[field].isNull() || !json[field].isIntegral()) {
        throw std::invalid_argument(field + " must be an integer");
    }
    if (!json[field].isUInt64() && json[field].asInt64() <= 0) {
        throw std::invalid_argument(field + " must be positive");
    }

    const auto value = json[field].isUInt64()
                           ? json[field].asUInt64()
                           : static_cast<Json::UInt64>(json[field].asInt64());
    if (value == 0) {
        throw std::invalid_argument(field + " must be positive");
    }
    return static_cast<std::uint64_t>(value);
}

std::optional<std::uint64_t> ReadOptionalUInt64(
    const Json::Value& json,
    const std::string_view field_name
) {
    const std::string field(field_name);
    if (!json.isMember(field) || json[field].isNull()) {
        return std::nullopt;
    }
    if (!json[field].isIntegral()) {
        throw std::invalid_argument(field + " must be an integer");
    }
    if (!json[field].isUInt64() && json[field].asInt64() <= 0) {
        throw std::invalid_argument(field + " must be positive");
    }
    const auto value = json[field].isUInt64()
                           ? json[field].asUInt64()
                           : static_cast<Json::UInt64>(json[field].asInt64());
    if (value == 0) {
        throw std::invalid_argument(field + " must be positive");
    }
    return static_cast<std::uint64_t>(value);
}

double ReadRequiredDouble(const Json::Value& json, const std::string_view field_name) {
    const std::string field(field_name);
    if (!json.isMember(field) || json[field].isNull() || !json[field].isNumeric()) {
        throw std::invalid_argument(field + " must be a number");
    }
    return json[field].asDouble();
}

std::optional<double> ReadOptionalDouble(
    const Json::Value& json,
    const std::string_view field_name
) {
    const std::string field(field_name);
    if (!json.isMember(field) || json[field].isNull()) {
        return std::nullopt;
    }
    if (!json[field].isNumeric()) {
        throw std::invalid_argument(field + " must be a number");
    }
    return json[field].asDouble();
}

std::optional<int> ReadOptionalInt(const Json::Value& json, const std::string_view field_name) {
    const std::string field(field_name);
    if (!json.isMember(field) || json[field].isNull()) {
        return std::nullopt;
    }
    if (!json[field].isInt()) {
        throw std::invalid_argument(field + " must be an integer");
    }
    return json[field].asInt();
}

bool ReadOptionalBool(
    const Json::Value& json,
    const std::string_view field_name,
    const bool default_value
) {
    const std::string field(field_name);
    if (!json.isMember(field) || json[field].isNull()) {
        return default_value;
    }
    if (!json[field].isBool()) {
        throw std::invalid_argument(field + " must be a boolean");
    }
    return json[field].asBool();
}

Json::Value ToItemSummaryJson(const modules::item::ItemSummary& item) {
    Json::Value json(Json::objectValue);
    json["itemId"] = static_cast<Json::UInt64>(item.item_id);
    json["sellerId"] = static_cast<Json::UInt64>(item.seller_id);
    json["categoryId"] = static_cast<Json::UInt64>(item.category_id);
    json["title"] = item.title;
    json["startPrice"] = item.start_price;
    json["itemStatus"] = item.item_status;
    json["rejectReason"] = item.reject_reason;
    json["coverImageUrl"] = item.cover_image_url;
    json["createdAt"] = item.created_at;
    json["updatedAt"] = item.updated_at;
    return json;
}

Json::Value ToItemRecordJson(const modules::item::ItemRecord& item) {
    Json::Value json(Json::objectValue);
    json["itemId"] = static_cast<Json::UInt64>(item.item_id);
    json["sellerId"] = static_cast<Json::UInt64>(item.seller_id);
    json["categoryId"] = static_cast<Json::UInt64>(item.category_id);
    json["title"] = item.title;
    json["description"] = item.description;
    json["startPrice"] = item.start_price;
    json["itemStatus"] = item.item_status;
    json["rejectReason"] = item.reject_reason;
    json["coverImageUrl"] = item.cover_image_url;
    json["createdAt"] = item.created_at;
    json["updatedAt"] = item.updated_at;
    return json;
}

Json::Value ToItemImageJson(const modules::item::ItemImageRecord& image) {
    Json::Value json(Json::objectValue);
    json["imageId"] = static_cast<Json::UInt64>(image.image_id);
    json["itemId"] = static_cast<Json::UInt64>(image.item_id);
    json["imageUrl"] = image.image_url;
    json["sortNo"] = image.sort_no;
    json["isCover"] = image.is_cover;
    json["createdAt"] = image.created_at;
    return json;
}

Json::Value ToAuditLogJson(const modules::item::ItemAuditLogRecord& log) {
    Json::Value json(Json::objectValue);
    json["auditLogId"] = static_cast<Json::UInt64>(log.audit_log_id);
    json["itemId"] = static_cast<Json::UInt64>(log.item_id);
    json["adminId"] = static_cast<Json::UInt64>(log.admin_id);
    json["adminUsername"] = log.admin_username;
    json["auditResult"] = log.audit_result;
    json["auditComment"] = log.audit_comment;
    json["createdAt"] = log.created_at;
    return json;
}

Json::Value ToCreateItemResultJson(const modules::item::CreateItemResult& result) {
    Json::Value json(Json::objectValue);
    json["itemId"] = static_cast<Json::UInt64>(result.item_id);
    json["itemStatus"] = result.item_status;
    json["createdAt"] = result.created_at;
    return json;
}

Json::Value ToUpdateItemResultJson(const modules::item::UpdateItemResult& result) {
    Json::Value json(Json::objectValue);
    json["itemId"] = static_cast<Json::UInt64>(result.item_id);
    json["itemStatus"] = result.item_status;
    json["updatedAt"] = result.updated_at;
    return json;
}

Json::Value ToSubmitAuditResultJson(const modules::item::SubmitAuditResult& result) {
    Json::Value json(Json::objectValue);
    json["itemId"] = static_cast<Json::UInt64>(result.item_id);
    json["itemStatus"] = result.item_status;
    json["submittedAt"] = result.submitted_at;
    return json;
}

Json::Value ToDeleteImageResultJson(const modules::item::DeleteItemImageResult& result) {
    Json::Value json(Json::objectValue);
    json["itemId"] = static_cast<Json::UInt64>(result.item_id);
    json["imageId"] = static_cast<Json::UInt64>(result.image_id);
    json["itemStatus"] = result.item_status;
    json["coverImageUrl"] = result.cover_image_url;
    return json;
}

Json::Value ToItemDetailJson(const modules::item::ItemDetail& detail) {
    Json::Value json(Json::objectValue);
    json["item"] = ToItemRecordJson(detail.item);

    Json::Value images(Json::arrayValue);
    for (const auto& image : detail.images) {
        images.append(ToItemImageJson(image));
    }
    json["images"] = std::move(images);

    if (detail.latest_audit_log.has_value()) {
        json["latestAuditLog"] = ToAuditLogJson(*detail.latest_audit_log);
    } else {
        json["latestAuditLog"] = Json::Value(Json::nullValue);
    }
    return json;
}

Json::Value ToItemListJson(const std::vector<modules::item::ItemSummary>& items) {
    Json::Value data(Json::objectValue);
    Json::Value list(Json::arrayValue);
    for (const auto& item : items) {
        list.append(ToItemSummaryJson(item));
    }
    data["list"] = std::move(list);
    data["total"] = static_cast<Json::UInt64>(items.size());
    data["pageNo"] = 1;
    data["pageSize"] = static_cast<Json::UInt64>(items.size());
    return data;
}

modules::item::CreateItemRequest BuildCreateItemRequest(const Json::Value& body) {
    return modules::item::CreateItemRequest{
        .title = common::http::ReadRequiredString(body, "title"),
        .description = common::http::ReadRequiredString(body, "description"),
        .category_id = ReadRequiredUInt64(body, "categoryId"),
        .start_price = ReadRequiredDouble(body, "startPrice"),
        .cover_image_url =
            common::http::ReadOptionalString(body, "coverImageUrl").value_or(""),
    };
}

modules::item::UpdateItemRequest BuildUpdateItemRequest(const Json::Value& body) {
    return modules::item::UpdateItemRequest{
        .title = common::http::ReadOptionalString(body, "title"),
        .description = common::http::ReadOptionalString(body, "description"),
        .category_id = ReadOptionalUInt64(body, "categoryId"),
        .start_price = ReadOptionalDouble(body, "startPrice"),
        .cover_image_url = common::http::ReadOptionalString(body, "coverImageUrl"),
    };
}

modules::item::AddItemImageRequest BuildAddImageRequest(const Json::Value& body) {
    return modules::item::AddItemImageRequest{
        .image_url = common::http::ReadRequiredString(body, "imageUrl"),
        .sort_no = ReadOptionalInt(body, "sortNo"),
        .is_cover = ReadOptionalBool(body, "isCover", false),
    };
}

#endif

}  // namespace

void RegisterItemHttpRoutes(const std::shared_ptr<common::http::HttpServiceContext> services) {
#if AUCTION_HAS_DROGON
    drogon::app().registerHandler(
        "/api/items",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            callback(common::http::ExecuteApi([&]() {
                const auto body = common::http::RequireJsonBody(request);
                const auto result = services->item_service().CreateDraftItem(
                    request->getHeader("authorization"),
                    BuildCreateItemRequest(body)
                );
                return common::http::ApiResponse::Success(ToCreateItemResultJson(result));
            }));
        },
        {drogon::Post}
    );

    drogon::app().registerHandler(
        "/api/items/mine",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            callback(common::http::ExecuteApi([&]() {
                const auto items = services->item_service().ListMyItems(
                    request->getHeader("authorization"),
                    request->getOptionalParameter<std::string>("itemStatus")
                );
                return common::http::ApiResponse::Success(ToItemListJson(items));
            }));
        },
        {drogon::Get}
    );

    drogon::app().registerHandler(
        "/api/items/{1}/images",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                   const std::string& raw_item_id) {
            callback(common::http::ExecuteApi([&]() {
                const auto body = common::http::RequireJsonBody(request);
                const auto image = services->item_service().AddItemImage(
                    request->getHeader("authorization"),
                    ParsePositiveId(raw_item_id, "item id"),
                    BuildAddImageRequest(body)
                );
                return common::http::ApiResponse::Success(ToItemImageJson(image));
            }));
        },
        {drogon::Post}
    );

    drogon::app().registerHandler(
        "/api/items/{1}/images/{2}",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                   const std::string& raw_item_id,
                   const std::string& raw_image_id) {
            callback(common::http::ExecuteApi([&]() {
                const auto result = services->item_service().DeleteItemImage(
                    request->getHeader("authorization"),
                    ParsePositiveId(raw_item_id, "item id"),
                    ParsePositiveId(raw_image_id, "image id")
                );
                return common::http::ApiResponse::Success(ToDeleteImageResultJson(result));
            }));
        },
        {drogon::Delete}
    );

    drogon::app().registerHandler(
        "/api/items/{1}/submit-audit",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                   const std::string& raw_item_id) {
            callback(common::http::ExecuteApi([&]() {
                const auto result = services->item_service().SubmitForAudit(
                    request->getHeader("authorization"),
                    ParsePositiveId(raw_item_id, "item id")
                );
                return common::http::ApiResponse::Success(ToSubmitAuditResultJson(result));
            }));
        },
        {drogon::Post}
    );

    drogon::app().registerHandler(
        "/api/items/{1}",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                   const std::string& raw_item_id) {
            callback(common::http::ExecuteApi([&]() {
                const auto body = common::http::RequireJsonBody(request);
                const auto result = services->item_service().UpdateItem(
                    request->getHeader("authorization"),
                    ParsePositiveId(raw_item_id, "item id"),
                    BuildUpdateItemRequest(body)
                );
                return common::http::ApiResponse::Success(ToUpdateItemResultJson(result));
            }));
        },
        {drogon::Put}
    );

    drogon::app().registerHandler(
        "/api/items/{1}",
        [services](const drogon::HttpRequestPtr& request,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                   const std::string& raw_item_id) {
            callback(common::http::ExecuteApi([&]() {
                const auto detail = services->item_service().GetItemDetail(
                    request->getHeader("authorization"),
                    ParsePositiveId(raw_item_id, "item id")
                );
                return common::http::ApiResponse::Success(ToItemDetailJson(detail));
            }));
        },
        {drogon::Get}
    );
#else
    static_cast<void>(services);
#endif
}

}  // namespace auction::access::http
