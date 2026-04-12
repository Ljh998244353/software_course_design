#include "repository/item_repository.h"

#include <array>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string_view>

#include <mysql.h>

#include "common/errors/error_code.h"
#include "modules/item/item_exception.h"

namespace auction::repository {

namespace {

using MysqlResultPtr = std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)>;

class PreparedStatement {
public:
    PreparedStatement(MYSQL* handle, const std::string_view sql) : handle_(mysql_stmt_init(handle)) {
        if (handle_ == nullptr) {
            throw std::runtime_error("failed to initialize mysql statement");
        }

        if (mysql_stmt_prepare(handle_, sql.data(), sql.size()) != 0) {
            const auto error_message =
                std::string("failed to prepare statement: ") + mysql_stmt_error(handle_);
            mysql_stmt_close(handle_);
            handle_ = nullptr;
            throw std::runtime_error(error_message);
        }
    }

    ~PreparedStatement() {
        if (handle_ != nullptr) {
            mysql_stmt_close(handle_);
        }
    }

    PreparedStatement(const PreparedStatement&) = delete;
    PreparedStatement& operator=(const PreparedStatement&) = delete;

    void BindParams(MYSQL_BIND* binds) {
        if (mysql_stmt_bind_param(handle_, binds) != 0) {
            throw std::runtime_error(std::string("failed to bind params: ") + mysql_stmt_error(handle_));
        }
    }

    void Execute() {
        if (mysql_stmt_execute(handle_) != 0) {
            throw std::runtime_error(std::string("failed to execute statement: ") +
                                     mysql_stmt_error(handle_));
        }
    }

    [[nodiscard]] std::uint64_t InsertId() const {
        return mysql_stmt_insert_id(handle_);
    }

    [[nodiscard]] std::uint64_t AffectedRows() const {
        return mysql_stmt_affected_rows(handle_);
    }

    [[nodiscard]] unsigned int ErrorCode() const {
        return mysql_stmt_errno(handle_);
    }

private:
    MYSQL_STMT* handle_{nullptr};
};

MysqlResultPtr ExecuteQuery(common::db::MysqlConnection& connection, const std::string& sql) {
    MYSQL* handle = connection.native_handle();
    if (mysql_query(handle, sql.c_str()) != 0) {
        throw std::runtime_error(std::string("mysql query failed: ") + mysql_error(handle));
    }

    MYSQL_RES* result = mysql_store_result(handle);
    if (result == nullptr) {
        throw std::runtime_error(std::string("mysql read result failed: ") + mysql_error(handle));
    }

    return MysqlResultPtr(result, &mysql_free_result);
}

std::string ReadRowString(MYSQL_ROW row, const unsigned int index) {
    return row[index] == nullptr ? "" : std::string(row[index]);
}

std::uint64_t ReadRowUint64(MYSQL_ROW row, const unsigned int index) {
    return row[index] == nullptr ? 0ULL : std::stoull(row[index]);
}

int ReadRowInt(MYSQL_ROW row, const unsigned int index) {
    return row[index] == nullptr ? 0 : std::stoi(row[index]);
}

double ReadRowDouble(MYSQL_ROW row, const unsigned int index) {
    return row[index] == nullptr ? 0.0 : std::stod(row[index]);
}

bool ReadRowBool(MYSQL_ROW row, const unsigned int index) {
    return row[index] != nullptr && std::string_view(row[index]) != "0";
}

modules::item::ItemRecord BuildItemRecord(MYSQL_ROW row) {
    return modules::item::ItemRecord{
        .item_id = ReadRowUint64(row, 0),
        .seller_id = ReadRowUint64(row, 1),
        .category_id = ReadRowUint64(row, 2),
        .title = ReadRowString(row, 3),
        .description = ReadRowString(row, 4),
        .start_price = ReadRowDouble(row, 5),
        .item_status = ReadRowString(row, 6),
        .reject_reason = ReadRowString(row, 7),
        .cover_image_url = ReadRowString(row, 8),
        .created_at = ReadRowString(row, 9),
        .updated_at = ReadRowString(row, 10),
    };
}

modules::item::ItemSummary BuildItemSummary(MYSQL_ROW row) {
    return modules::item::ItemSummary{
        .item_id = ReadRowUint64(row, 0),
        .seller_id = ReadRowUint64(row, 1),
        .category_id = ReadRowUint64(row, 2),
        .title = ReadRowString(row, 3),
        .start_price = ReadRowDouble(row, 4),
        .item_status = ReadRowString(row, 5),
        .reject_reason = ReadRowString(row, 6),
        .cover_image_url = ReadRowString(row, 7),
        .created_at = ReadRowString(row, 8),
        .updated_at = ReadRowString(row, 9),
    };
}

modules::item::PendingAuditItemSummary BuildPendingAuditItemSummary(MYSQL_ROW row) {
    return modules::item::PendingAuditItemSummary{
        .item_id = ReadRowUint64(row, 0),
        .seller_id = ReadRowUint64(row, 1),
        .seller_username = ReadRowString(row, 2),
        .seller_nickname = ReadRowString(row, 3),
        .category_id = ReadRowUint64(row, 4),
        .title = ReadRowString(row, 5),
        .start_price = ReadRowDouble(row, 6),
        .created_at = ReadRowString(row, 7),
    };
}

modules::item::ItemImageRecord BuildImageRecord(MYSQL_ROW row) {
    return modules::item::ItemImageRecord{
        .image_id = ReadRowUint64(row, 0),
        .item_id = ReadRowUint64(row, 1),
        .image_url = ReadRowString(row, 2),
        .sort_no = ReadRowInt(row, 3),
        .is_cover = ReadRowBool(row, 4),
        .created_at = ReadRowString(row, 5),
    };
}

modules::item::ItemAuditLogRecord BuildAuditLogRecord(MYSQL_ROW row) {
    return modules::item::ItemAuditLogRecord{
        .audit_log_id = ReadRowUint64(row, 0),
        .item_id = ReadRowUint64(row, 1),
        .admin_id = ReadRowUint64(row, 2),
        .admin_username = ReadRowString(row, 3),
        .audit_result = ReadRowString(row, 4),
        .audit_comment = ReadRowString(row, 5),
        .created_at = ReadRowString(row, 6),
    };
}

void BindStringParam(
    MYSQL_BIND& bind,
    const std::string& value,
    unsigned long& length,
    bool& is_null
) {
    std::memset(&bind, 0, sizeof(bind));
    length = static_cast<unsigned long>(value.size());
    is_null = false;
    bind.buffer_type = MYSQL_TYPE_STRING;
    bind.buffer = const_cast<char*>(value.data());
    bind.buffer_length = length;
    bind.length = &length;
    bind.is_null = &is_null;
}

void BindNullableStringParam(
    MYSQL_BIND& bind,
    const std::string& value,
    unsigned long& length,
    bool& is_null
) {
    std::memset(&bind, 0, sizeof(bind));
    length = static_cast<unsigned long>(value.size());
    is_null = value.empty();
    bind.buffer_type = MYSQL_TYPE_STRING;
    bind.buffer = is_null ? nullptr : const_cast<char*>(value.data());
    bind.buffer_length = length;
    bind.length = &length;
    bind.is_null = &is_null;
}

void BindUint64Param(MYSQL_BIND& bind, std::uint64_t& value, bool& is_null) {
    std::memset(&bind, 0, sizeof(bind));
    is_null = false;
    bind.buffer_type = MYSQL_TYPE_LONGLONG;
    bind.buffer = &value;
    bind.is_unsigned = true;
    bind.is_null = &is_null;
}

void BindIntParam(MYSQL_BIND& bind, int& value, bool& is_null) {
    std::memset(&bind, 0, sizeof(bind));
    is_null = false;
    bind.buffer_type = MYSQL_TYPE_LONG;
    bind.buffer = &value;
    bind.is_null = &is_null;
}

void BindTinyIntParam(MYSQL_BIND& bind, std::int8_t& value, bool& is_null) {
    std::memset(&bind, 0, sizeof(bind));
    is_null = false;
    bind.buffer_type = MYSQL_TYPE_TINY;
    bind.buffer = &value;
    bind.is_null = &is_null;
}

std::optional<modules::item::ItemRecord> FindSingleItemBySql(
    common::db::MysqlConnection& connection,
    const std::string& sql
) {
    const auto result = ExecuteQuery(connection, sql);
    if (MYSQL_ROW row = mysql_fetch_row(result.get()); row != nullptr) {
        return BuildItemRecord(row);
    }
    return std::nullopt;
}

std::optional<modules::item::ItemImageRecord> FindSingleImageBySql(
    common::db::MysqlConnection& connection,
    const std::string& sql
) {
    const auto result = ExecuteQuery(connection, sql);
    if (MYSQL_ROW row = mysql_fetch_row(result.get()); row != nullptr) {
        return BuildImageRecord(row);
    }
    return std::nullopt;
}

std::optional<modules::item::ItemAuditLogRecord> FindSingleAuditLogBySql(
    common::db::MysqlConnection& connection,
    const std::string& sql
) {
    const auto result = ExecuteQuery(connection, sql);
    if (MYSQL_ROW row = mysql_fetch_row(result.get()); row != nullptr) {
        return BuildAuditLogRecord(row);
    }
    return std::nullopt;
}

std::string BuildItemSelectSql(const std::string& where_clause) {
    return "SELECT item_id, seller_id, COALESCE(category_id, 0), title, description, "
           "CAST(start_price AS CHAR), item_status, COALESCE(reject_reason, ''), "
           "COALESCE(cover_image_url, ''), CAST(created_at AS CHAR), CAST(updated_at AS CHAR) "
           "FROM item " +
           where_clause;
}

}  // namespace

bool ItemRepository::CategoryExistsAndActive(const std::uint64_t category_id) const {
    return QueryCount(
               "SELECT COUNT(*) FROM item_category WHERE category_id = " + std::to_string(category_id) +
               " AND status = 'ACTIVE'"
    ) > 0;
}

std::optional<modules::item::ItemRecord> ItemRepository::FindItemById(const std::uint64_t item_id) const {
    return FindSingleItemBySql(
        connection(),
        BuildItemSelectSql("WHERE item_id = " + std::to_string(item_id) + " LIMIT 1")
    );
}

modules::item::ItemRecord ItemRepository::CreateItem(const CreateItemParams& params) const {
    PreparedStatement statement(
        connection().native_handle(),
        "INSERT INTO item (seller_id, category_id, title, description, start_price, item_status, "
        "cover_image_url) VALUES (?, ?, ?, ?, ?, 'DRAFT', ?)"
    );

    auto seller_id = params.seller_id;
    auto category_id = params.category_id;
    auto start_price = params.start_price;
    unsigned long title_length = 0;
    unsigned long description_length = 0;
    unsigned long start_price_length = 0;
    unsigned long cover_image_url_length = 0;
    bool seller_id_is_null = false;
    bool category_id_is_null = false;
    bool title_is_null = false;
    bool description_is_null = false;
    bool start_price_is_null = false;
    bool cover_image_url_is_null = false;
    MYSQL_BIND params_bind[6]{};
    BindUint64Param(params_bind[0], seller_id, seller_id_is_null);
    BindUint64Param(params_bind[1], category_id, category_id_is_null);
    BindStringParam(params_bind[2], params.title, title_length, title_is_null);
    BindStringParam(params_bind[3], params.description, description_length, description_is_null);
    BindStringParam(params_bind[4], start_price, start_price_length, start_price_is_null);
    BindNullableStringParam(
        params_bind[5],
        params.cover_image_url,
        cover_image_url_length,
        cover_image_url_is_null
    );
    statement.BindParams(params_bind);
    statement.Execute();

    const auto item = FindItemById(statement.InsertId());
    if (!item.has_value()) {
        throw std::runtime_error("created item could not be loaded");
    }
    return *item;
}

modules::item::ItemRecord ItemRepository::UpdateItem(const UpdateItemParams& params) const {
    PreparedStatement statement(
        connection().native_handle(),
        "UPDATE item SET category_id = ?, title = ?, description = ?, start_price = ?, "
        "cover_image_url = ?, item_status = ?, reject_reason = ?, "
        "updated_at = CURRENT_TIMESTAMP(3) WHERE item_id = ?"
    );

    auto category_id = params.category_id;
    auto item_id = params.item_id;
    auto start_price = params.start_price;
    unsigned long title_length = 0;
    unsigned long description_length = 0;
    unsigned long start_price_length = 0;
    unsigned long cover_image_url_length = 0;
    unsigned long item_status_length = 0;
    unsigned long reject_reason_length = 0;
    bool category_id_is_null = false;
    bool title_is_null = false;
    bool description_is_null = false;
    bool start_price_is_null = false;
    bool cover_image_url_is_null = false;
    bool item_status_is_null = false;
    bool reject_reason_is_null = false;
    bool item_id_is_null = false;
    MYSQL_BIND params_bind[8]{};
    BindUint64Param(params_bind[0], category_id, category_id_is_null);
    BindStringParam(params_bind[1], params.title, title_length, title_is_null);
    BindStringParam(params_bind[2], params.description, description_length, description_is_null);
    BindStringParam(params_bind[3], start_price, start_price_length, start_price_is_null);
    BindNullableStringParam(
        params_bind[4],
        params.cover_image_url,
        cover_image_url_length,
        cover_image_url_is_null
    );
    BindStringParam(params_bind[5], params.item_status, item_status_length, item_status_is_null);
    BindNullableStringParam(
        params_bind[6],
        params.reject_reason,
        reject_reason_length,
        reject_reason_is_null
    );
    BindUint64Param(params_bind[7], item_id, item_id_is_null);
    statement.BindParams(params_bind);
    statement.Execute();

    const auto item = FindItemById(item_id);
    if (!item.has_value()) {
        throw std::runtime_error("updated item could not be loaded");
    }
    return *item;
}

std::vector<modules::item::ItemSummary> ItemRepository::ListItemsBySeller(
    const std::uint64_t seller_id,
    const std::optional<std::string>& item_status
) const {
    std::string sql =
        "SELECT item_id, seller_id, COALESCE(category_id, 0), title, CAST(start_price AS CHAR), "
        "item_status, COALESCE(reject_reason, ''), COALESCE(cover_image_url, ''), "
        "CAST(created_at AS CHAR), CAST(updated_at AS CHAR) "
        "FROM item WHERE seller_id = " +
        std::to_string(seller_id);
    if (item_status.has_value()) {
        sql += " AND item_status = '" + EscapeString(*item_status) + "'";
    }
    sql += " ORDER BY created_at DESC, item_id DESC";

    const auto result = ExecuteQuery(connection(), sql);
    std::vector<modules::item::ItemSummary> items;
    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(result.get())) != nullptr) {
        items.push_back(BuildItemSummary(row));
    }
    return items;
}

std::vector<modules::item::PendingAuditItemSummary> ItemRepository::ListPendingAuditItems() const {
    const auto result = ExecuteQuery(
        connection(),
        "SELECT i.item_id, i.seller_id, u.username, COALESCE(u.nickname, ''), "
        "COALESCE(i.category_id, 0), i.title, CAST(i.start_price AS CHAR), "
        "CAST(i.created_at AS CHAR) "
        "FROM item i "
        "JOIN user_account u ON u.user_id = i.seller_id "
        "WHERE i.item_status = 'PENDING_AUDIT' "
        "ORDER BY i.created_at ASC, i.item_id ASC"
    );

    std::vector<modules::item::PendingAuditItemSummary> items;
    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(result.get())) != nullptr) {
        items.push_back(BuildPendingAuditItemSummary(row));
    }
    return items;
}

std::optional<modules::item::ItemImageRecord> ItemRepository::FindImageById(
    const std::uint64_t item_id,
    const std::uint64_t image_id
) const {
    return FindSingleImageBySql(
        connection(),
        "SELECT image_id, item_id, image_url, sort_no, is_cover, CAST(created_at AS CHAR) "
        "FROM item_image WHERE item_id = " +
            std::to_string(item_id) + " AND image_id = " + std::to_string(image_id) + " LIMIT 1"
    );
}

std::optional<modules::item::ItemImageRecord> ItemRepository::FindFirstImageBySort(
    const std::uint64_t item_id
) const {
    return FindSingleImageBySql(
        connection(),
        "SELECT image_id, item_id, image_url, sort_no, is_cover, CAST(created_at AS CHAR) "
        "FROM item_image WHERE item_id = " +
            std::to_string(item_id) + " ORDER BY sort_no ASC, image_id ASC LIMIT 1"
    );
}

std::vector<modules::item::ItemImageRecord> ItemRepository::ListItemImages(
    const std::uint64_t item_id
) const {
    const auto result = ExecuteQuery(
        connection(),
        "SELECT image_id, item_id, image_url, sort_no, is_cover, CAST(created_at AS CHAR) "
        "FROM item_image WHERE item_id = " +
            std::to_string(item_id) + " ORDER BY sort_no ASC, image_id ASC"
    );

    std::vector<modules::item::ItemImageRecord> images;
    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(result.get())) != nullptr) {
        images.push_back(BuildImageRecord(row));
    }
    return images;
}

std::uint64_t ItemRepository::CountItemImages(const std::uint64_t item_id) const {
    return QueryCount(
        "SELECT COUNT(*) FROM item_image WHERE item_id = " + std::to_string(item_id)
    );
}

int ItemRepository::NextImageSortNo(const std::uint64_t item_id) const {
    const auto value = QueryString(
        "SELECT COALESCE(MAX(sort_no), 0) + 1 FROM item_image WHERE item_id = " +
        std::to_string(item_id)
    );
    return value.empty() ? 1 : std::stoi(value);
}

modules::item::ItemImageRecord ItemRepository::CreateItemImage(const CreateItemImageParams& params) const {
    PreparedStatement statement(
        connection().native_handle(),
        "INSERT INTO item_image (item_id, image_url, sort_no, is_cover) VALUES (?, ?, ?, ?)"
    );

    auto item_id = params.item_id;
    auto sort_no = params.sort_no;
    auto image_url = params.image_url;
    std::int8_t is_cover = params.is_cover ? 1 : 0;
    unsigned long image_url_length = 0;
    bool item_id_is_null = false;
    bool image_url_is_null = false;
    bool sort_no_is_null = false;
    bool is_cover_is_null = false;
    MYSQL_BIND params_bind[4]{};
    BindUint64Param(params_bind[0], item_id, item_id_is_null);
    BindStringParam(params_bind[1], image_url, image_url_length, image_url_is_null);
    BindIntParam(params_bind[2], sort_no, sort_no_is_null);
    BindTinyIntParam(params_bind[3], is_cover, is_cover_is_null);
    statement.BindParams(params_bind);

    try {
        statement.Execute();
    } catch (const std::runtime_error&) {
        if (statement.ErrorCode() == 1062) {
            throw modules::item::ItemException(
                common::errors::ErrorCode::kItemImageInvalid,
                "image sort number already exists"
            );
        }
        throw;
    }

    const auto image = FindImageById(item_id, statement.InsertId());
    if (!image.has_value()) {
        throw std::runtime_error("created image could not be loaded");
    }
    return *image;
}

void ItemRepository::DeleteItemImage(const std::uint64_t item_id, const std::uint64_t image_id) const {
    PreparedStatement statement(
        connection().native_handle(),
        "DELETE FROM item_image WHERE item_id = ? AND image_id = ?"
    );

    auto mutable_item_id = item_id;
    auto mutable_image_id = image_id;
    bool item_id_is_null = false;
    bool image_id_is_null = false;
    MYSQL_BIND params_bind[2]{};
    BindUint64Param(params_bind[0], mutable_item_id, item_id_is_null);
    BindUint64Param(params_bind[1], mutable_image_id, image_id_is_null);
    statement.BindParams(params_bind);
    statement.Execute();
}

void ItemRepository::ClearItemImageCoverFlags(const std::uint64_t item_id) const {
    PreparedStatement statement(
        connection().native_handle(),
        "UPDATE item_image SET is_cover = 0 WHERE item_id = ?"
    );

    auto mutable_item_id = item_id;
    bool item_id_is_null = false;
    MYSQL_BIND params_bind[1]{};
    BindUint64Param(params_bind[0], mutable_item_id, item_id_is_null);
    statement.BindParams(params_bind);
    statement.Execute();
}

void ItemRepository::UpdateImageCoverFlag(const std::uint64_t image_id, const bool is_cover) const {
    PreparedStatement statement(
        connection().native_handle(),
        "UPDATE item_image SET is_cover = ? WHERE image_id = ?"
    );

    std::int8_t mutable_is_cover = is_cover ? 1 : 0;
    auto mutable_image_id = image_id;
    bool is_cover_is_null = false;
    bool image_id_is_null = false;
    MYSQL_BIND params_bind[2]{};
    BindTinyIntParam(params_bind[0], mutable_is_cover, is_cover_is_null);
    BindUint64Param(params_bind[1], mutable_image_id, image_id_is_null);
    statement.BindParams(params_bind);
    statement.Execute();
}

void ItemRepository::UpdateItemCoverImage(
    const std::uint64_t item_id,
    const std::string& cover_image_url
) const {
    PreparedStatement statement(
        connection().native_handle(),
        "UPDATE item SET cover_image_url = ?, updated_at = CURRENT_TIMESTAMP(3) WHERE item_id = ?"
    );

    auto mutable_item_id = item_id;
    unsigned long cover_image_url_length = 0;
    bool cover_image_url_is_null = false;
    bool item_id_is_null = false;
    MYSQL_BIND params_bind[2]{};
    BindNullableStringParam(
        params_bind[0],
        cover_image_url,
        cover_image_url_length,
        cover_image_url_is_null
    );
    BindUint64Param(params_bind[1], mutable_item_id, item_id_is_null);
    statement.BindParams(params_bind);
    statement.Execute();
}

void ItemRepository::UpdateItemStatus(
    const std::uint64_t item_id,
    const std::string& item_status,
    const std::string& reject_reason
) const {
    PreparedStatement statement(
        connection().native_handle(),
        "UPDATE item SET item_status = ?, reject_reason = ?, "
        "updated_at = CURRENT_TIMESTAMP(3) WHERE item_id = ?"
    );

    auto mutable_item_id = item_id;
    unsigned long item_status_length = 0;
    unsigned long reject_reason_length = 0;
    bool item_status_is_null = false;
    bool reject_reason_is_null = false;
    bool item_id_is_null = false;
    MYSQL_BIND params_bind[3]{};
    BindStringParam(params_bind[0], item_status, item_status_length, item_status_is_null);
    BindNullableStringParam(
        params_bind[1],
        reject_reason,
        reject_reason_length,
        reject_reason_is_null
    );
    BindUint64Param(params_bind[2], mutable_item_id, item_id_is_null);
    statement.BindParams(params_bind);
    statement.Execute();
}

void ItemRepository::InsertAuditLog(const CreateItemAuditLogParams& params) const {
    PreparedStatement statement(
        connection().native_handle(),
        "INSERT INTO item_audit_log (item_id, admin_id, audit_result, audit_comment) "
        "VALUES (?, ?, ?, ?)"
    );

    auto item_id = params.item_id;
    auto admin_id = params.admin_id;
    unsigned long audit_result_length = 0;
    unsigned long audit_comment_length = 0;
    bool item_id_is_null = false;
    bool admin_id_is_null = false;
    bool audit_result_is_null = false;
    bool audit_comment_is_null = false;
    MYSQL_BIND params_bind[4]{};
    BindUint64Param(params_bind[0], item_id, item_id_is_null);
    BindUint64Param(params_bind[1], admin_id, admin_id_is_null);
    BindStringParam(params_bind[2], params.audit_result, audit_result_length, audit_result_is_null);
    BindNullableStringParam(
        params_bind[3],
        params.audit_comment,
        audit_comment_length,
        audit_comment_is_null
    );
    statement.BindParams(params_bind);
    statement.Execute();
}

std::optional<modules::item::ItemAuditLogRecord> ItemRepository::FindLatestAuditLog(
    const std::uint64_t item_id
) const {
    return FindSingleAuditLogBySql(
        connection(),
        "SELECT l.audit_log_id, l.item_id, l.admin_id, u.username, l.audit_result, "
        "COALESCE(l.audit_comment, ''), CAST(l.created_at AS CHAR) "
        "FROM item_audit_log l "
        "JOIN user_account u ON u.user_id = l.admin_id "
        "WHERE l.item_id = " +
            std::to_string(item_id) +
            " ORDER BY l.created_at DESC, l.audit_log_id DESC LIMIT 1"
    );
}

std::vector<modules::item::ItemAuditLogRecord> ItemRepository::ListAuditLogs(
    const std::uint64_t item_id
) const {
    const auto result = ExecuteQuery(
        connection(),
        "SELECT l.audit_log_id, l.item_id, l.admin_id, u.username, l.audit_result, "
        "COALESCE(l.audit_comment, ''), CAST(l.created_at AS CHAR) "
        "FROM item_audit_log l "
        "JOIN user_account u ON u.user_id = l.admin_id "
        "WHERE l.item_id = " +
            std::to_string(item_id) +
            " ORDER BY l.created_at ASC, l.audit_log_id ASC"
    );

    std::vector<modules::item::ItemAuditLogRecord> logs;
    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(result.get())) != nullptr) {
        logs.push_back(BuildAuditLogRecord(row));
    }
    return logs;
}

}  // namespace auction::repository
