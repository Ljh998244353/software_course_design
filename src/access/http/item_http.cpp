#include "access/http/item_http.h"

#include <stdexcept>

#include "common/errors/error_code.h"
#include "common/http/api_response.h"
#include "common/logging/logger.h"
#include "modules/auth/auth_exception.h"
#include "modules/item/item_exception.h"

#if AUCTION_HAS_DROGON
#include <drogon/drogon.h>
#endif

namespace auction::access::http {

#if AUCTION_HAS_DROGON

namespace {

void AddCorsHeaders(const drogon::HttpResponsePtr& response) {
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
    response->addHeader("Access-Control-Max-Age", "86400");
}

drogon::HttpResponsePtr MakeOk(Json::Value data) {
    auto response = drogon::HttpResponse::newHttpJsonResponse(
        common::http::ApiResponse::Success(std::move(data)).ToJsonValue()
    );
    response->setStatusCode(drogon::k200OK);
    AddCorsHeaders(response);
    return response;
}

drogon::HttpResponsePtr MakeError(
    common::errors::ErrorCode code,
    const std::string& message,
    drogon::HttpStatusCode http_status = drogon::k400BadRequest
) {
    auto response = drogon::HttpResponse::newHttpJsonResponse(
        common::http::ApiResponse::Failure(code, message).ToJsonValue()
    );
    response->setStatusCode(http_status);
    AddCorsHeaders(response);
    return response;
}

void RegisterCorsPreflight(const std::string& path, const std::string& methods) {
    drogon::app().registerHandler(
        path,
        [methods](const drogon::HttpRequestPtr&,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            auto response = drogon::HttpResponse::newHttpResponse();
            response->setStatusCode(drogon::k204NoContent);
            response->addHeader("Access-Control-Allow-Origin", "*");
            response->addHeader("Access-Control-Allow-Methods", methods);
            response->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
            response->addHeader("Access-Control-Max-Age", "86400");
            callback(response);
        },
        {drogon::Options}
    );
}

std::uint64_t SafeParseUint64(const std::string& value, const std::string& param_name) {
    try {
        std::size_t parsed = 0;
        const auto result = static_cast<std::uint64_t>(std::stoull(value, &parsed));
        if (parsed != value.size()) {
            throw std::invalid_argument(param_name + " is not a valid number");
        }
        return result;
    } catch (const std::out_of_range&) {
        throw std::invalid_argument(param_name + " out of range");
    } catch (const std::invalid_argument&) {
        throw std::invalid_argument(param_name + " is not a valid number");
    }
}

Json::Value CreateItemResultToJson(const modules::item::CreateItemResult& r) {
    Json::Value json(Json::objectValue);
    json["item_id"] = static_cast<Json::UInt64>(r.item_id);
    json["item_status"] = r.item_status;
    json["created_at"] = r.created_at;
    return json;
}

Json::Value UpdateItemResultToJson(const modules::item::UpdateItemResult& r) {
    Json::Value json(Json::objectValue);
    json["item_id"] = static_cast<Json::UInt64>(r.item_id);
    json["item_status"] = r.item_status;
    json["updated_at"] = r.updated_at;
    return json;
}

Json::Value SubmitAuditResultToJson(const modules::item::SubmitAuditResult& r) {
    Json::Value json(Json::objectValue);
    json["item_id"] = static_cast<Json::UInt64>(r.item_id);
    json["item_status"] = r.item_status;
    json["submitted_at"] = r.submitted_at;
    return json;
}

Json::Value ItemImageRecordToJson(const modules::item::ItemImageRecord& r) {
    Json::Value json(Json::objectValue);
    json["image_id"] = static_cast<Json::UInt64>(r.image_id);
    json["item_id"] = static_cast<Json::UInt64>(r.item_id);
    json["image_url"] = r.image_url;
    json["sort_no"] = r.sort_no;
    json["is_cover"] = r.is_cover;
    json["created_at"] = r.created_at;
    return json;
}

Json::Value ItemRecordToJson(const modules::item::ItemRecord& r) {
    Json::Value json(Json::objectValue);
    json["item_id"] = static_cast<Json::UInt64>(r.item_id);
    json["seller_id"] = static_cast<Json::UInt64>(r.seller_id);
    json["category_id"] = static_cast<Json::UInt64>(r.category_id);
    json["title"] = r.title;
    json["description"] = r.description;
    json["start_price"] = r.start_price;
    json["trade_mode"] = r.trade_mode;
    json["location"] = r.location;
    json["tags_json"] = r.tags_json;
    json["suggested_bid_step"] = r.suggested_bid_step;
    json["suggested_anti_sniping_window_seconds"] =
        r.suggested_anti_sniping_window_seconds;
    json["suggested_extend_seconds"] = r.suggested_extend_seconds;
    json["item_status"] = r.item_status;
    json["reject_reason"] = r.reject_reason;
    json["cover_image_url"] = r.cover_image_url;
    json["created_at"] = r.created_at;
    json["updated_at"] = r.updated_at;
    return json;
}

Json::Value ItemSummaryToJson(const modules::item::ItemSummary& r) {
    Json::Value json(Json::objectValue);
    json["item_id"] = static_cast<Json::UInt64>(r.item_id);
    json["seller_id"] = static_cast<Json::UInt64>(r.seller_id);
    json["category_id"] = static_cast<Json::UInt64>(r.category_id);
    json["title"] = r.title;
    json["start_price"] = r.start_price;
    json["trade_mode"] = r.trade_mode;
    json["location"] = r.location;
    json["tags_json"] = r.tags_json;
    json["suggested_bid_step"] = r.suggested_bid_step;
    json["suggested_anti_sniping_window_seconds"] =
        r.suggested_anti_sniping_window_seconds;
    json["suggested_extend_seconds"] = r.suggested_extend_seconds;
    json["item_status"] = r.item_status;
    json["reject_reason"] = r.reject_reason;
    json["cover_image_url"] = r.cover_image_url;
    json["created_at"] = r.created_at;
    json["updated_at"] = r.updated_at;
    return json;
}

Json::Value AuditLogRecordToJson(const modules::item::ItemAuditLogRecord& r) {
    Json::Value json(Json::objectValue);
    json["audit_log_id"] = static_cast<Json::UInt64>(r.audit_log_id);
    json["item_id"] = static_cast<Json::UInt64>(r.item_id);
    json["admin_id"] = static_cast<Json::UInt64>(r.admin_id);
    json["admin_username"] = r.admin_username;
    json["audit_result"] = r.audit_result;
    json["audit_comment"] = r.audit_comment;
    json["created_at"] = r.created_at;
    return json;
}

Json::Value ItemDetailToJson(const modules::item::ItemDetail& detail) {
    Json::Value json = ItemRecordToJson(detail.item);
    Json::Value images(Json::arrayValue);
    for (const auto& image : detail.images) {
        images.append(ItemImageRecordToJson(image));
    }
    json["images"] = std::move(images);
    if (detail.latest_audit_log.has_value()) {
        json["latest_audit_log"] = AuditLogRecordToJson(*detail.latest_audit_log);
    } else {
        json["latest_audit_log"] = Json::nullValue;
    }
    return json;
}

void MapItemHttpStatus(
    const common::errors::ErrorCode code,
    drogon::HttpStatusCode& out_status
) {
    switch (code) {
        case common::errors::ErrorCode::kItemNotFound:
            out_status = drogon::k404NotFound;
            break;
        case common::errors::ErrorCode::kItemOwnerMismatch:
        case common::errors::ErrorCode::kItemEditStatusInvalid:
        case common::errors::ErrorCode::kItemSubmitStatusInvalid:
        case common::errors::ErrorCode::kItemAuditStatusInvalid:
        case common::errors::ErrorCode::kItemImageInvalid:
            out_status = drogon::k409Conflict;
            break;
        case common::errors::ErrorCode::kAuthTokenMissing:
        case common::errors::ErrorCode::kAuthTokenExpired:
        case common::errors::ErrorCode::kAuthSessionInvalid:
            out_status = drogon::k401Unauthorized;
            break;
        case common::errors::ErrorCode::kAuthUserFrozen:
        case common::errors::ErrorCode::kAuthUserDisabled:
        case common::errors::ErrorCode::kAuthPermissionDenied:
            out_status = drogon::k403Forbidden;
            break;
        default:
            out_status = drogon::k400BadRequest;
            break;
    }
}

}  // namespace

#endif

void RegisterItemHttpRoutes(
    modules::item::ItemService& item_service
) {
#if AUCTION_HAS_DROGON

    // POST /api/items - create draft item (requires auth)
    RegisterCorsPreflight("/api/items", "POST, OPTIONS");
    drogon::app().registerHandler(
        "/api/items",
        [&item_service](const drogon::HttpRequestPtr& request,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            try {
                const auto auth_header = request->getHeader("authorization");

                const auto body = request->getJsonObject();
                if (!body) {
                    callback(MakeError(
                        common::errors::ErrorCode::kInvalidArgument,
                        "request body must be valid JSON"
                    ));
                    return;
                }

                modules::item::CreateItemRequest req;
                req.title = body->get("title", "").asString();
                req.description = body->get("description", "").asString();
                req.category_id = body->get("category_id", 0).asUInt64();
                req.start_price = body->get("start_price", 0.0).asDouble();
                req.trade_mode = body->get("trade_mode", "MEETUP").asString();
                req.location = body->get("location", "线上").asString();
                req.tags_json = body->isMember("tags_json") ? Json::writeString(Json::StreamWriterBuilder{}, (*body)["tags_json"]) : "[]";
                req.suggested_bid_step = body->get("suggested_bid_step", 0.0).asDouble();
                req.suggested_anti_sniping_window_seconds =
                    body->get("suggested_anti_sniping_window_seconds", 0).asInt();
                req.suggested_extend_seconds = body->get("suggested_extend_seconds", 0).asInt();
                req.cover_image_url = body->get("cover_image_url", "").asString();

                const auto result = item_service.CreateDraftItem(auth_header, req);

                callback(MakeOk(CreateItemResultToJson(result)));
            } catch (const modules::item::ItemException& e) {
                drogon::HttpStatusCode http_status = drogon::k400BadRequest;
                MapItemHttpStatus(e.code(), http_status);
                callback(MakeError(e.code(), e.what(), http_status));
            } catch (const modules::auth::AuthException& e) {
                drogon::HttpStatusCode http_status = drogon::k401Unauthorized;
                MapItemHttpStatus(e.code(), http_status);
                callback(MakeError(e.code(), e.what(), http_status));
            } catch (const std::invalid_argument& e) {
                callback(MakeError(common::errors::ErrorCode::kInvalidArgument, e.what()));
            } catch (...) {
                common::logging::Logger::Instance().Error(
                    "unhandled exception in POST /api/items"
                );
                callback(MakeError(
                    common::errors::ErrorCode::kInternalError,
                    "internal server error",
                    drogon::k500InternalServerError
                ));
            }
        },
        {drogon::Post}
    );

    // GET /api/items/mine - list current seller's items (requires auth)
    RegisterCorsPreflight("/api/items/mine", "GET, OPTIONS");
    drogon::app().registerHandler(
        "/api/items/mine",
        [&item_service](const drogon::HttpRequestPtr& request,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            try {
                const auto auth_header = request->getHeader("authorization");
                std::optional<std::string> item_status;
                const auto status_filter = request->getParameter("item_status").empty()
                    ? request->getParameter("itemStatus")
                    : request->getParameter("item_status");
                if (!status_filter.empty()) {
                    item_status = status_filter;
                }

                const auto items = item_service.ListMyItems(auth_header, item_status);
                Json::Value data(Json::objectValue);
                Json::Value rows(Json::arrayValue);
                for (const auto& item : items) {
                    rows.append(ItemSummaryToJson(item));
                }
                data["items"] = std::move(rows);
                data["total"] = static_cast<Json::UInt64>(items.size());
                callback(MakeOk(data));
            } catch (const modules::item::ItemException& e) {
                drogon::HttpStatusCode http_status = drogon::k400BadRequest;
                MapItemHttpStatus(e.code(), http_status);
                callback(MakeError(e.code(), e.what(), http_status));
            } catch (const modules::auth::AuthException& e) {
                drogon::HttpStatusCode http_status = drogon::k401Unauthorized;
                MapItemHttpStatus(e.code(), http_status);
                callback(MakeError(e.code(), e.what(), http_status));
            } catch (const std::invalid_argument& e) {
                callback(MakeError(common::errors::ErrorCode::kInvalidArgument, e.what()));
            } catch (...) {
                common::logging::Logger::Instance().Error(
                    "unhandled exception in GET /api/items/mine"
                );
                callback(MakeError(
                    common::errors::ErrorCode::kInternalError,
                    "internal server error",
                    drogon::k500InternalServerError
                ));
            }
        },
        {drogon::Get}
    );

    // GET /api/items/{id} - item detail with images and latest audit log (requires auth)
    drogon::app().registerHandler(
        "/api/items/{id}",
        [&item_service](const drogon::HttpRequestPtr& request,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                        const std::string& id_str) {
            try {
                if (id_str.empty()) {
                    callback(MakeError(
                        common::errors::ErrorCode::kInvalidArgument,
                        "item id is required"
                    ));
                    return;
                }

                const auto item_id = SafeParseUint64(id_str, "item id");
                const auto auth_header = request->getHeader("authorization");
                const auto detail = item_service.GetItemDetail(auth_header, item_id);
                callback(MakeOk(ItemDetailToJson(detail)));
            } catch (const modules::item::ItemException& e) {
                drogon::HttpStatusCode http_status = drogon::k400BadRequest;
                MapItemHttpStatus(e.code(), http_status);
                callback(MakeError(e.code(), e.what(), http_status));
            } catch (const modules::auth::AuthException& e) {
                drogon::HttpStatusCode http_status = drogon::k401Unauthorized;
                MapItemHttpStatus(e.code(), http_status);
                callback(MakeError(e.code(), e.what(), http_status));
            } catch (const std::invalid_argument& e) {
                callback(MakeError(common::errors::ErrorCode::kInvalidArgument, e.what()));
            } catch (...) {
                common::logging::Logger::Instance().Error(
                    "unhandled exception in GET /api/items/{id}"
                );
                callback(MakeError(
                    common::errors::ErrorCode::kInternalError,
                    "internal server error",
                    drogon::k500InternalServerError
                ));
            }
        },
        {drogon::Get}
    );

    // PUT /api/items/{id} - update item (requires auth)
    RegisterCorsPreflight("/api/items/{id}", "PUT, OPTIONS");
    drogon::app().registerHandler(
        "/api/items/{id}",
        [&item_service](const drogon::HttpRequestPtr& request,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                        const std::string& id_str) {
            try {
                if (id_str.empty()) {
                    callback(MakeError(
                        common::errors::ErrorCode::kInvalidArgument,
                        "item id is required"
                    ));
                    return;
                }

                const auto item_id = SafeParseUint64(id_str, "item id");
                const auto auth_header = request->getHeader("authorization");

                const auto body = request->getJsonObject();
                if (!body) {
                    callback(MakeError(
                        common::errors::ErrorCode::kInvalidArgument,
                        "request body must be valid JSON"
                    ));
                    return;
                }

                modules::item::UpdateItemRequest req;
                if (body->isMember("title")) req.title = (*body)["title"].asString();
                if (body->isMember("description")) req.description = (*body)["description"].asString();
                if (body->isMember("category_id")) req.category_id = (*body)["category_id"].asUInt64();
                if (body->isMember("start_price")) req.start_price = (*body)["start_price"].asDouble();
                if (body->isMember("trade_mode")) req.trade_mode = (*body)["trade_mode"].asString();
                if (body->isMember("location")) req.location = (*body)["location"].asString();
                if (body->isMember("tags_json")) {
                    Json::StreamWriterBuilder builder;
                    builder["indentation"] = "";
                    req.tags_json = Json::writeString(builder, (*body)["tags_json"]);
                }
                if (body->isMember("suggested_bid_step")) {
                    req.suggested_bid_step = (*body)["suggested_bid_step"].asDouble();
                }
                if (body->isMember("suggested_anti_sniping_window_seconds")) {
                    req.suggested_anti_sniping_window_seconds =
                        (*body)["suggested_anti_sniping_window_seconds"].asInt();
                }
                if (body->isMember("suggested_extend_seconds")) {
                    req.suggested_extend_seconds = (*body)["suggested_extend_seconds"].asInt();
                }
                if (body->isMember("cover_image_url")) req.cover_image_url = (*body)["cover_image_url"].asString();

                const auto result = item_service.UpdateItem(auth_header, item_id, req);

                callback(MakeOk(UpdateItemResultToJson(result)));
            } catch (const modules::item::ItemException& e) {
                drogon::HttpStatusCode http_status = drogon::k400BadRequest;
                MapItemHttpStatus(e.code(), http_status);
                callback(MakeError(e.code(), e.what(), http_status));
            } catch (const modules::auth::AuthException& e) {
                drogon::HttpStatusCode http_status = drogon::k401Unauthorized;
                MapItemHttpStatus(e.code(), http_status);
                callback(MakeError(e.code(), e.what(), http_status));
            } catch (const std::invalid_argument& e) {
                callback(MakeError(common::errors::ErrorCode::kInvalidArgument, e.what()));
            } catch (...) {
                common::logging::Logger::Instance().Error(
                    "unhandled exception in PUT /api/items/{id}"
                );
                callback(MakeError(
                    common::errors::ErrorCode::kInternalError,
                    "internal server error",
                    drogon::k500InternalServerError
                ));
            }
        },
        {drogon::Put}
    );

    // POST /api/items/{id}/images - add image metadata (requires auth)
    RegisterCorsPreflight("/api/items/{id}/images", "POST, OPTIONS");
    drogon::app().registerHandler(
        "/api/items/{id}/images",
        [&item_service](const drogon::HttpRequestPtr& request,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                        const std::string& id_str) {
            try {
                if (id_str.empty()) {
                    callback(MakeError(
                        common::errors::ErrorCode::kInvalidArgument,
                        "item id is required"
                    ));
                    return;
                }

                const auto item_id = SafeParseUint64(id_str, "item id");
                const auto auth_header = request->getHeader("authorization");

                const auto body = request->getJsonObject();
                if (!body) {
                    callback(MakeError(
                        common::errors::ErrorCode::kInvalidArgument,
                        "request body must be valid JSON"
                    ));
                    return;
                }

                modules::item::AddItemImageRequest req;
                req.image_url = body->get("image_url", "").asString();
                if (body->isMember("sort_no")) req.sort_no = (*body)["sort_no"].asInt();
                req.is_cover = body->get("is_cover", false).asBool();

                const auto result = item_service.AddItemImage(auth_header, item_id, req);

                callback(MakeOk(ItemImageRecordToJson(result)));
            } catch (const modules::item::ItemException& e) {
                drogon::HttpStatusCode http_status = drogon::k400BadRequest;
                MapItemHttpStatus(e.code(), http_status);
                callback(MakeError(e.code(), e.what(), http_status));
            } catch (const modules::auth::AuthException& e) {
                drogon::HttpStatusCode http_status = drogon::k401Unauthorized;
                MapItemHttpStatus(e.code(), http_status);
                callback(MakeError(e.code(), e.what(), http_status));
            } catch (const std::invalid_argument& e) {
                callback(MakeError(common::errors::ErrorCode::kInvalidArgument, e.what()));
            } catch (...) {
                common::logging::Logger::Instance().Error(
                    "unhandled exception in POST /api/items/{id}/images"
                );
                callback(MakeError(
                    common::errors::ErrorCode::kInternalError,
                    "internal server error",
                    drogon::k500InternalServerError
                ));
            }
        },
        {drogon::Post}
    );

    // DELETE /api/items/{id}/images/{image_id} - delete image metadata (requires auth)
    RegisterCorsPreflight("/api/items/{id}/images/{image_id}", "DELETE, OPTIONS");
    drogon::app().registerHandler(
        "/api/items/{id}/images/{image_id}",
        [&item_service](const drogon::HttpRequestPtr& request,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                        const std::string& id_str,
                        const std::string& image_id_str) {
            try {
                if (id_str.empty()) {
                    callback(MakeError(
                        common::errors::ErrorCode::kInvalidArgument,
                        "item id is required"
                    ));
                    return;
                }

                const auto item_id = SafeParseUint64(id_str, "item id");
                const auto image_id = SafeParseUint64(image_id_str, "image id");
                const auto auth_header = request->getHeader("authorization");

                const auto result = item_service.DeleteItemImage(auth_header, item_id, image_id);

                Json::Value json;
                json["image_id"] = static_cast<Json::UInt64>(result.image_id);
                json["item_id"] = static_cast<Json::UInt64>(result.item_id);
                json["item_status"] = result.item_status;
                json["cover_image_url"] = result.cover_image_url;
                callback(MakeOk(json));
            } catch (const modules::item::ItemException& e) {
                drogon::HttpStatusCode http_status = drogon::k400BadRequest;
                MapItemHttpStatus(e.code(), http_status);
                callback(MakeError(e.code(), e.what(), http_status));
            } catch (const modules::auth::AuthException& e) {
                drogon::HttpStatusCode http_status = drogon::k401Unauthorized;
                MapItemHttpStatus(e.code(), http_status);
                callback(MakeError(e.code(), e.what(), http_status));
            } catch (const std::invalid_argument& e) {
                callback(MakeError(common::errors::ErrorCode::kInvalidArgument, e.what()));
            } catch (...) {
                common::logging::Logger::Instance().Error(
                    "unhandled exception in DELETE /api/items/{id}/images/{image_id}"
                );
                callback(MakeError(
                    common::errors::ErrorCode::kInternalError,
                    "internal server error",
                    drogon::k500InternalServerError
                ));
            }
        },
        {drogon::Delete}
    );

    // POST /api/items/{id}/submit-review - submit item for audit (requires auth)
    RegisterCorsPreflight("/api/items/{id}/submit-review", "POST, OPTIONS");
    drogon::app().registerHandler(
        "/api/items/{id}/submit-review",
        [&item_service](const drogon::HttpRequestPtr& request,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                        const std::string& id_str) {
            try {
                if (id_str.empty()) {
                    callback(MakeError(
                        common::errors::ErrorCode::kInvalidArgument,
                        "item id is required"
                    ));
                    return;
                }

                const auto item_id = SafeParseUint64(id_str, "item id");
                const auto auth_header = request->getHeader("authorization");

                const auto result = item_service.SubmitForAudit(auth_header, item_id);

                callback(MakeOk(SubmitAuditResultToJson(result)));
            } catch (const modules::item::ItemException& e) {
                drogon::HttpStatusCode http_status = drogon::k400BadRequest;
                MapItemHttpStatus(e.code(), http_status);
                callback(MakeError(e.code(), e.what(), http_status));
            } catch (const modules::auth::AuthException& e) {
                drogon::HttpStatusCode http_status = drogon::k401Unauthorized;
                MapItemHttpStatus(e.code(), http_status);
                callback(MakeError(e.code(), e.what(), http_status));
            } catch (const std::invalid_argument& e) {
                callback(MakeError(common::errors::ErrorCode::kInvalidArgument, e.what()));
            } catch (...) {
                common::logging::Logger::Instance().Error(
                    "unhandled exception in POST /api/items/{id}/submit-review"
                );
                callback(MakeError(
                    common::errors::ErrorCode::kInternalError,
                    "internal server error",
                    drogon::k500InternalServerError
                ));
            }
        },
        {drogon::Post}
    );

    // POST /api/items/{id}/offline - seller offlines an item not in active transaction flow
    RegisterCorsPreflight("/api/items/{id}/offline", "POST, OPTIONS");
    drogon::app().registerHandler(
        "/api/items/{id}/offline",
        [&item_service](const drogon::HttpRequestPtr& request,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                        const std::string& id_str) {
            try {
                if (id_str.empty()) {
                    callback(MakeError(
                        common::errors::ErrorCode::kInvalidArgument,
                        "item id is required"
                    ));
                    return;
                }

                const auto item_id = SafeParseUint64(id_str, "item id");
                const auto auth_header = request->getHeader("authorization");
                const auto result = item_service.OfflineItem(auth_header, item_id);
                callback(MakeOk(UpdateItemResultToJson(result)));
            } catch (const modules::item::ItemException& e) {
                drogon::HttpStatusCode http_status = drogon::k400BadRequest;
                MapItemHttpStatus(e.code(), http_status);
                callback(MakeError(e.code(), e.what(), http_status));
            } catch (const modules::auth::AuthException& e) {
                drogon::HttpStatusCode http_status = drogon::k401Unauthorized;
                MapItemHttpStatus(e.code(), http_status);
                callback(MakeError(e.code(), e.what(), http_status));
            } catch (const std::invalid_argument& e) {
                callback(MakeError(common::errors::ErrorCode::kInvalidArgument, e.what()));
            } catch (...) {
                common::logging::Logger::Instance().Error(
                    "unhandled exception in POST /api/items/{id}/offline"
                );
                callback(MakeError(
                    common::errors::ErrorCode::kInternalError,
                    "internal server error",
                    drogon::k500InternalServerError
                ));
            }
        },
        {drogon::Post}
    );

#else
    static_cast<void>(item_service);
#endif
}

}  // namespace auction::access::http
