#include "access/http/admin_http.h"

#include <stdexcept>

#include "common/errors/error_code.h"
#include "common/http/api_response.h"
#include "common/logging/logger.h"
#include "modules/auth/auth_exception.h"
#include "modules/item/item_exception.h"
#include "modules/statistics/statistics_exception.h"

#if AUCTION_HAS_DROGON
#include <drogon/drogon.h>
#endif

namespace auction::access::http {

#if AUCTION_HAS_DROGON

namespace {

void AddCorsHeaders(const drogon::HttpResponsePtr& response) {
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
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

Json::Value PendingAuditItemToJson(const modules::item::PendingAuditItemSummary& item) {
    Json::Value json(Json::objectValue);
    json["item_id"] = static_cast<Json::UInt64>(item.item_id);
    json["seller_id"] = static_cast<Json::UInt64>(item.seller_id);
    json["seller_username"] = item.seller_username;
    json["seller_nickname"] = item.seller_nickname;
    json["category_id"] = static_cast<Json::UInt64>(item.category_id);
    json["title"] = item.title;
    json["start_price"] = item.start_price;
    json["cover_image_url"] = item.cover_image_url;
    json["created_at"] = item.created_at;
    return json;
}

Json::Value AuditItemResultToJson(const modules::item::AuditItemResult& r) {
    Json::Value json(Json::objectValue);
    json["item_id"] = static_cast<Json::UInt64>(r.item_id);
    json["old_status"] = r.old_status;
    json["new_status"] = r.new_status;
    json["audit_result"] = r.audit_result;
    json["audited_at"] = r.audited_at;
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

Json::Value DailyStatisticsToJson(const modules::statistics::DailyStatisticsRecord& r) {
    Json::Value json(Json::objectValue);
    json["stat_id"] = static_cast<Json::UInt64>(r.stat_id);
    json["stat_date"] = r.stat_date;
    json["auction_count"] = r.auction_count;
    json["sold_count"] = r.sold_count;
    json["unsold_count"] = r.unsold_count;
    json["bid_count"] = r.bid_count;
    json["gmv_amount"] = r.gmv_amount;
    json["created_at"] = r.created_at;
    return json;
}

void MapAdminHttpStatus(
    const common::errors::ErrorCode code,
    drogon::HttpStatusCode& out_status
) {
    switch (code) {
        case common::errors::ErrorCode::kItemNotFound:
            out_status = drogon::k404NotFound;
            break;
        case common::errors::ErrorCode::kItemAuditStatusInvalid:
        case common::errors::ErrorCode::kItemAuditResultInvalid:
        case common::errors::ErrorCode::kItemAuditReasonRequired:
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
        case common::errors::ErrorCode::kStatisticsDateInvalid:
        case common::errors::ErrorCode::kStatisticsRangeInvalid:
            out_status = drogon::k400BadRequest;
            break;
        default:
            out_status = drogon::k400BadRequest;
            break;
    }
}

}  // namespace

#endif

void RegisterAdminHttpRoutes(
    modules::audit::ItemAuditService& audit_service,
    modules::statistics::StatisticsService& statistics_service
) {
#if AUCTION_HAS_DROGON

    // GET /api/admin/items/pending - list pending audit items (requires admin)
    RegisterCorsPreflight("/api/admin/items/pending", "GET, OPTIONS");
    drogon::app().registerHandler(
        "/api/admin/items/pending",
        [&audit_service](const drogon::HttpRequestPtr& request,
                         std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            try {
                const auto auth_header = request->getHeader("authorization");
                const auto items = audit_service.ListPendingItems(auth_header);

                Json::Value arr(Json::arrayValue);
                for (const auto& item : items) {
                    arr.append(PendingAuditItemToJson(item));
                }

                callback(MakeOk(arr));
            } catch (const modules::auth::AuthException& e) {
                drogon::HttpStatusCode http_status = drogon::k401Unauthorized;
                MapAdminHttpStatus(e.code(), http_status);
                callback(MakeError(e.code(), e.what(), http_status));
            } catch (...) {
                common::logging::Logger::Instance().Error(
                    "unhandled exception in GET /api/admin/items/pending"
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

    // GET /api/admin/items/{id}/audit-logs - list item audit logs (requires admin/support)
    RegisterCorsPreflight("/api/admin/items/{id}/audit-logs", "GET, OPTIONS");
    drogon::app().registerHandler(
        "/api/admin/items/{id}/audit-logs",
        [&audit_service](const drogon::HttpRequestPtr& request,
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
                const auto logs = audit_service.GetAuditLogs(auth_header, item_id);

                Json::Value arr(Json::arrayValue);
                for (const auto& log : logs) {
                    arr.append(AuditLogRecordToJson(log));
                }
                callback(MakeOk(arr));
            } catch (const modules::item::ItemException& e) {
                drogon::HttpStatusCode http_status = drogon::k400BadRequest;
                MapAdminHttpStatus(e.code(), http_status);
                callback(MakeError(e.code(), e.what(), http_status));
            } catch (const modules::auth::AuthException& e) {
                drogon::HttpStatusCode http_status = drogon::k401Unauthorized;
                MapAdminHttpStatus(e.code(), http_status);
                callback(MakeError(e.code(), e.what(), http_status));
            } catch (const std::invalid_argument& e) {
                callback(MakeError(common::errors::ErrorCode::kInvalidArgument, e.what()));
            } catch (...) {
                common::logging::Logger::Instance().Error(
                    "unhandled exception in GET /api/admin/items/{id}/audit-logs"
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

    // POST /api/admin/items/{id}/approve - approve item (requires admin)
    RegisterCorsPreflight("/api/admin/items/{id}/approve", "POST, OPTIONS");
    drogon::app().registerHandler(
        "/api/admin/items/{id}/approve",
        [&audit_service](const drogon::HttpRequestPtr& request,
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

                modules::item::AuditItemRequest req;
                req.audit_result = "APPROVED";

                const auto result = audit_service.AuditItem(auth_header, item_id, req);

                callback(MakeOk(AuditItemResultToJson(result)));
            } catch (const modules::item::ItemException& e) {
                drogon::HttpStatusCode http_status = drogon::k400BadRequest;
                MapAdminHttpStatus(e.code(), http_status);
                callback(MakeError(e.code(), e.what(), http_status));
            } catch (const modules::auth::AuthException& e) {
                drogon::HttpStatusCode http_status = drogon::k401Unauthorized;
                MapAdminHttpStatus(e.code(), http_status);
                callback(MakeError(e.code(), e.what(), http_status));
            } catch (const std::invalid_argument& e) {
                callback(MakeError(common::errors::ErrorCode::kInvalidArgument, e.what()));
            } catch (...) {
                common::logging::Logger::Instance().Error(
                    "unhandled exception in POST /api/admin/items/{id}/approve"
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

    // POST /api/admin/items/{id}/reject - reject item (requires admin)
    RegisterCorsPreflight("/api/admin/items/{id}/reject", "POST, OPTIONS");
    drogon::app().registerHandler(
        "/api/admin/items/{id}/reject",
        [&audit_service](const drogon::HttpRequestPtr& request,
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

                modules::item::AuditItemRequest req;
                req.audit_result = "REJECTED";
                req.reason = body->get("reason", "").asString();

                const auto result = audit_service.AuditItem(auth_header, item_id, req);

                callback(MakeOk(AuditItemResultToJson(result)));
            } catch (const modules::item::ItemException& e) {
                drogon::HttpStatusCode http_status = drogon::k400BadRequest;
                MapAdminHttpStatus(e.code(), http_status);
                callback(MakeError(e.code(), e.what(), http_status));
            } catch (const modules::auth::AuthException& e) {
                drogon::HttpStatusCode http_status = drogon::k401Unauthorized;
                MapAdminHttpStatus(e.code(), http_status);
                callback(MakeError(e.code(), e.what(), http_status));
            } catch (const std::invalid_argument& e) {
                callback(MakeError(common::errors::ErrorCode::kInvalidArgument, e.what()));
            } catch (...) {
                common::logging::Logger::Instance().Error(
                    "unhandled exception in POST /api/admin/items/{id}/reject"
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

    // GET /api/admin/statistics/daily - daily statistics (requires admin)
    RegisterCorsPreflight("/api/admin/statistics/daily", "GET, OPTIONS");
    drogon::app().registerHandler(
        "/api/admin/statistics/daily",
        [&statistics_service](const drogon::HttpRequestPtr& request,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            try {
                const auto auth_header = request->getHeader("authorization");

                modules::statistics::DailyStatisticsQuery query;
                query.start_date = request->getParameter("start_date");
                query.end_date = request->getParameter("end_date");

                const auto result = statistics_service.ListDailyStatistics(auth_header, query);

                Json::Value arr(Json::arrayValue);
                for (const auto& record : result.records) {
                    arr.append(DailyStatisticsToJson(record));
                }

                callback(MakeOk(arr));
            } catch (const modules::auth::AuthException& e) {
                drogon::HttpStatusCode http_status = drogon::k401Unauthorized;
                MapAdminHttpStatus(e.code(), http_status);
                callback(MakeError(e.code(), e.what(), http_status));
            } catch (const modules::statistics::StatisticsException& e) {
                drogon::HttpStatusCode http_status = drogon::k400BadRequest;
                MapAdminHttpStatus(e.code(), http_status);
                callback(MakeError(e.code(), e.what(), http_status));
            } catch (...) {
                common::logging::Logger::Instance().Error(
                    "unhandled exception in GET /api/admin/statistics/daily"
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

#else
    static_cast<void>(audit_service);
    static_cast<void>(statistics_service);
#endif
}

}  // namespace auction::access::http
