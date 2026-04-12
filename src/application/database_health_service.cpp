#include "application/database_health_service.h"

#include <exception>

#include <json/json.h>

#include "common/db/mysql_connection.h"
#include "common/errors/error_code.h"
#include "repository/database_meta_repository.h"

namespace auction::application {

DatabaseHealthReport CollectDatabaseHealth(
    const common::config::AppConfig& config,
    const std::filesystem::path& project_root
) {
    common::db::MysqlConnection connection(config.mysql, project_root);
    repository::DatabaseMetaRepository repository(connection);
    const auto snapshot = repository.LoadSnapshot(config.mysql.database);

    DatabaseHealthReport report;
    report.database = config.mysql.database;
    report.charset = config.mysql.charset;
    report.endpoint = connection.endpoint();
    report.server_version = snapshot.server_version;
    report.table_count = snapshot.table_count;
    report.required_table_count = snapshot.required_table_count;
    report.expected_table_count = snapshot.expected_table_count;
    report.admin_user_count = snapshot.admin_user_count;
    report.category_count = snapshot.category_count;
    report.schema_ready = report.required_table_count == report.expected_table_count;
    report.seed_ready = report.admin_user_count >= 1 && report.category_count >= 4;
    return report;
}

common::http::ApiResponse BuildDatabaseCheckResponse(
    const common::config::AppConfig& config,
    const std::filesystem::path& project_root,
    const std::filesystem::path& config_path
) {
    Json::Value payload(Json::objectValue);
    payload["service"] = "auction_app";
    payload["configPath"] = config_path.string();
    payload["projectRoot"] = project_root.string();
    payload["database"]["name"] = config.mysql.database;
    payload["database"]["charset"] = config.mysql.charset;
    payload["database"]["endpoint"] = config.mysql.socket.empty()
        ? config.mysql.host + ":" + std::to_string(config.mysql.port)
        : config.mysql.ResolveSocketPath(project_root).string();

    try {
        const auto report = CollectDatabaseHealth(config, project_root);
        payload["status"] = report.schema_ready && report.seed_ready ? "ok" : "degraded";
        payload["database"]["endpoint"] = report.endpoint;
        payload["database"]["serverVersion"] = report.server_version;
        payload["database"]["tableCount"] = Json::UInt64(report.table_count);
        payload["database"]["requiredTableCount"] = Json::UInt64(report.required_table_count);
        payload["database"]["expectedTableCount"] = Json::UInt64(report.expected_table_count);
        payload["database"]["adminUserCount"] = Json::UInt64(report.admin_user_count);
        payload["database"]["categoryCount"] = Json::UInt64(report.category_count);
        payload["database"]["schemaReady"] = report.schema_ready;
        payload["database"]["seedReady"] = report.seed_ready;

        if (report.schema_ready && report.seed_ready) {
            return common::http::ApiResponse::Success(payload);
        }

        return common::http::ApiResponse::Failure(
            common::errors::ErrorCode::kDatabaseQueryFailed,
            "database schema or seed is not ready",
            payload
        );
    } catch (const std::exception& exception) {
        payload["status"] = "down";
        return common::http::ApiResponse::Failure(
            common::errors::ErrorCode::kDatabaseUnavailable,
            exception.what(),
            payload
        );
    }
}

}  // namespace auction::application
