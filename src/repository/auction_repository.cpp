#include "repository/auction_repository.h"

#include <cstring>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>

#include <mysql.h>

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

std::optional<std::uint64_t> ReadRowOptionalUint64(MYSQL_ROW row, const unsigned int index) {
    if (row[index] == nullptr) {
        return std::nullopt;
    }
    return std::stoull(row[index]);
}

double ReadRowDouble(MYSQL_ROW row, const unsigned int index) {
    return row[index] == nullptr ? 0.0 : std::stod(row[index]);
}

int ReadRowInt(MYSQL_ROW row, const unsigned int index) {
    return row[index] == nullptr ? 0 : std::stoi(row[index]);
}

modules::auction::AuctionItemSnapshot BuildItemSnapshot(MYSQL_ROW row) {
    return modules::auction::AuctionItemSnapshot{
        .item_id = ReadRowUint64(row, 0),
        .seller_id = ReadRowUint64(row, 1),
        .title = ReadRowString(row, 2),
        .cover_image_url = ReadRowString(row, 3),
        .item_status = ReadRowString(row, 4),
    };
}

modules::auction::AuctionRecord BuildAuctionRecord(MYSQL_ROW row) {
    return modules::auction::AuctionRecord{
        .auction_id = ReadRowUint64(row, 0),
        .item_id = ReadRowUint64(row, 1),
        .seller_id = ReadRowUint64(row, 2),
        .start_time = ReadRowString(row, 3),
        .end_time = ReadRowString(row, 4),
        .start_price = ReadRowDouble(row, 5),
        .current_price = ReadRowDouble(row, 6),
        .bid_step = ReadRowDouble(row, 7),
        .highest_bidder_id = ReadRowOptionalUint64(row, 8),
        .status = ReadRowString(row, 9),
        .anti_sniping_window_seconds = ReadRowInt(row, 10),
        .extend_seconds = ReadRowInt(row, 11),
        .version = ReadRowUint64(row, 12),
        .created_at = ReadRowString(row, 13),
        .updated_at = ReadRowString(row, 14),
    };
}

modules::auction::AdminAuctionSummary BuildAdminAuctionSummary(MYSQL_ROW row) {
    return modules::auction::AdminAuctionSummary{
        .auction_id = ReadRowUint64(row, 0),
        .item_id = ReadRowUint64(row, 1),
        .seller_id = ReadRowUint64(row, 2),
        .title = ReadRowString(row, 3),
        .cover_image_url = ReadRowString(row, 4),
        .status = ReadRowString(row, 5),
        .start_price = ReadRowDouble(row, 6),
        .current_price = ReadRowDouble(row, 7),
        .bid_step = ReadRowDouble(row, 8),
        .highest_bidder_id = ReadRowOptionalUint64(row, 9),
        .start_time = ReadRowString(row, 10),
        .end_time = ReadRowString(row, 11),
        .updated_at = ReadRowString(row, 12),
    };
}

modules::auction::PublicAuctionSummary BuildPublicAuctionSummary(MYSQL_ROW row) {
    return modules::auction::PublicAuctionSummary{
        .auction_id = ReadRowUint64(row, 0),
        .item_id = ReadRowUint64(row, 1),
        .title = ReadRowString(row, 2),
        .cover_image_url = ReadRowString(row, 3),
        .status = ReadRowString(row, 4),
        .current_price = ReadRowDouble(row, 5),
        .start_time = ReadRowString(row, 6),
        .end_time = ReadRowString(row, 7),
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

std::string BuildItemSelectSql(const std::string& where_clause) {
    return "SELECT item_id, seller_id, title, COALESCE(cover_image_url, ''), item_status "
           "FROM item " +
           where_clause;
}

std::string BuildAuctionSelectSql(const std::string& where_clause) {
    return "SELECT auction_id, item_id, seller_id, CAST(start_time AS CHAR), "
           "CAST(end_time AS CHAR), CAST(start_price AS CHAR), CAST(current_price AS CHAR), "
           "CAST(bid_step AS CHAR), highest_bidder_id, status, anti_sniping_window_seconds, "
           "extend_seconds, version, CAST(created_at AS CHAR), CAST(updated_at AS CHAR) "
           "FROM auction " +
           where_clause;
}

std::optional<modules::auction::AuctionItemSnapshot> FindSingleItemBySql(
    common::db::MysqlConnection& connection,
    const std::string& sql
) {
    const auto result = ExecuteQuery(connection, sql);
    if (MYSQL_ROW row = mysql_fetch_row(result.get()); row != nullptr) {
        return BuildItemSnapshot(row);
    }
    return std::nullopt;
}

std::optional<modules::auction::AuctionRecord> FindSingleAuctionBySql(
    common::db::MysqlConnection& connection,
    const std::string& sql
) {
    const auto result = ExecuteQuery(connection, sql);
    if (MYSQL_ROW row = mysql_fetch_row(result.get()); row != nullptr) {
        return BuildAuctionRecord(row);
    }
    return std::nullopt;
}

std::string BuildAdminAuctionListSql(const modules::auction::AdminAuctionQuery& query, AuctionRepository& repository) {
    std::string sql =
        "SELECT a.auction_id, a.item_id, a.seller_id, i.title, COALESCE(i.cover_image_url, ''), "
        "a.status, CAST(a.start_price AS CHAR), CAST(a.current_price AS CHAR), "
        "CAST(a.bid_step AS CHAR), a.highest_bidder_id, CAST(a.start_time AS CHAR), "
        "CAST(a.end_time AS CHAR), CAST(a.updated_at AS CHAR) "
        "FROM auction a INNER JOIN item i ON i.item_id = a.item_id WHERE 1 = 1";

    if (query.status.has_value()) {
        sql += " AND a.status = '" + repository.EscapeString(*query.status) + "'";
    }
    if (query.item_id.has_value()) {
        sql += " AND a.item_id = " + std::to_string(*query.item_id);
    }
    if (query.seller_id.has_value()) {
        sql += " AND a.seller_id = " + std::to_string(*query.seller_id);
    }

    const int offset = (query.page_no - 1) * query.page_size;
    sql += " ORDER BY a.created_at DESC LIMIT " + std::to_string(query.page_size) + " OFFSET " +
           std::to_string(offset);
    return sql;
}

std::string BuildPublicAuctionListSql(const modules::auction::PublicAuctionQuery& query, AuctionRepository& repository) {
    std::string sql =
        "SELECT a.auction_id, a.item_id, i.title, COALESCE(i.cover_image_url, ''), a.status, "
        "CAST(a.current_price AS CHAR), CAST(a.start_time AS CHAR), CAST(a.end_time AS CHAR) "
        "FROM auction a INNER JOIN item i ON i.item_id = a.item_id "
        "WHERE a.status <> 'CANCELLED'";

    if (query.status.has_value()) {
        sql += " AND a.status = '" + repository.EscapeString(*query.status) + "'";
    }
    if (query.keyword.has_value() && !query.keyword->empty()) {
        sql += " AND i.title LIKE '%" + repository.EscapeString(*query.keyword) + "%'";
    }

    const int offset = (query.page_no - 1) * query.page_size;
    sql += " ORDER BY a.created_at DESC LIMIT " + std::to_string(query.page_size) + " OFFSET " +
           std::to_string(offset);
    return sql;
}

}  // namespace

std::optional<modules::auction::AuctionItemSnapshot> AuctionRepository::FindItemSnapshotById(
    const std::uint64_t item_id
) const {
    return FindSingleItemBySql(
        connection(),
        BuildItemSelectSql("WHERE item_id = " + std::to_string(item_id) + " LIMIT 1")
    );
}

std::optional<modules::auction::AuctionItemSnapshot> AuctionRepository::FindItemSnapshotByIdForUpdate(
    const std::uint64_t item_id
) const {
    return FindSingleItemBySql(
        connection(),
        BuildItemSelectSql(
            "WHERE item_id = " + std::to_string(item_id) + " LIMIT 1 FOR UPDATE"
        )
    );
}

bool AuctionRepository::HasNonTerminalAuction(const std::uint64_t item_id) const {
    return QueryCount(
               "SELECT COUNT(*) FROM auction WHERE item_id = " + std::to_string(item_id) +
               " AND status IN ('PENDING_START', 'RUNNING', 'SETTLING')"
    ) > 0;
}

std::optional<modules::auction::AuctionRecord> AuctionRepository::FindAuctionById(
    const std::uint64_t auction_id
) const {
    return FindSingleAuctionBySql(
        connection(),
        BuildAuctionSelectSql("WHERE auction_id = " + std::to_string(auction_id) + " LIMIT 1")
    );
}

std::optional<modules::auction::AuctionRecord> AuctionRepository::FindAuctionByIdForUpdate(
    const std::uint64_t auction_id
) const {
    return FindSingleAuctionBySql(
        connection(),
        BuildAuctionSelectSql(
            "WHERE auction_id = " + std::to_string(auction_id) + " LIMIT 1 FOR UPDATE"
        )
    );
}

modules::auction::AuctionRecord AuctionRepository::CreateAuction(const CreateAuctionParams& params) const {
    PreparedStatement statement(
        connection().native_handle(),
        "INSERT INTO auction (item_id, seller_id, start_time, end_time, start_price, current_price, "
        "bid_step, highest_bidder_id, status, anti_sniping_window_seconds, extend_seconds, version) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, NULL, 'PENDING_START', ?, ?, 0)"
    );

    auto item_id = params.item_id;
    auto seller_id = params.seller_id;
    auto anti_sniping_window_seconds = params.anti_sniping_window_seconds;
    auto extend_seconds = params.extend_seconds;
    unsigned long start_time_length = 0;
    unsigned long end_time_length = 0;
    unsigned long start_price_length = 0;
    unsigned long current_price_length = 0;
    unsigned long bid_step_length = 0;
    bool item_id_is_null = false;
    bool seller_id_is_null = false;
    bool start_time_is_null = false;
    bool end_time_is_null = false;
    bool start_price_is_null = false;
    bool current_price_is_null = false;
    bool bid_step_is_null = false;
    bool anti_sniping_window_is_null = false;
    bool extend_seconds_is_null = false;
    MYSQL_BIND params_bind[9]{};
    BindUint64Param(params_bind[0], item_id, item_id_is_null);
    BindUint64Param(params_bind[1], seller_id, seller_id_is_null);
    BindStringParam(params_bind[2], params.start_time, start_time_length, start_time_is_null);
    BindStringParam(params_bind[3], params.end_time, end_time_length, end_time_is_null);
    BindStringParam(params_bind[4], params.start_price, start_price_length, start_price_is_null);
    BindStringParam(
        params_bind[5],
        params.current_price,
        current_price_length,
        current_price_is_null
    );
    BindStringParam(params_bind[6], params.bid_step, bid_step_length, bid_step_is_null);
    BindIntParam(
        params_bind[7],
        anti_sniping_window_seconds,
        anti_sniping_window_is_null
    );
    BindIntParam(params_bind[8], extend_seconds, extend_seconds_is_null);
    statement.BindParams(params_bind);
    statement.Execute();

    const auto auction = FindAuctionById(statement.InsertId());
    if (!auction.has_value()) {
        throw std::runtime_error("created auction could not be loaded");
    }
    return *auction;
}

modules::auction::AuctionRecord AuctionRepository::UpdatePendingAuction(
    const UpdateAuctionParams& params
) const {
    PreparedStatement statement(
        connection().native_handle(),
        "UPDATE auction SET start_time = ?, end_time = ?, start_price = ?, current_price = ?, "
        "bid_step = ?, anti_sniping_window_seconds = ?, extend_seconds = ?, "
        "version = version + 1, updated_at = CURRENT_TIMESTAMP(3) WHERE auction_id = ?"
    );

    auto auction_id = params.auction_id;
    auto anti_sniping_window_seconds = params.anti_sniping_window_seconds;
    auto extend_seconds = params.extend_seconds;
    unsigned long start_time_length = 0;
    unsigned long end_time_length = 0;
    unsigned long start_price_length = 0;
    unsigned long current_price_length = 0;
    unsigned long bid_step_length = 0;
    bool start_time_is_null = false;
    bool end_time_is_null = false;
    bool start_price_is_null = false;
    bool current_price_is_null = false;
    bool bid_step_is_null = false;
    bool anti_sniping_window_is_null = false;
    bool extend_seconds_is_null = false;
    bool auction_id_is_null = false;
    MYSQL_BIND params_bind[8]{};
    BindStringParam(params_bind[0], params.start_time, start_time_length, start_time_is_null);
    BindStringParam(params_bind[1], params.end_time, end_time_length, end_time_is_null);
    BindStringParam(params_bind[2], params.start_price, start_price_length, start_price_is_null);
    BindStringParam(
        params_bind[3],
        params.start_price,
        current_price_length,
        current_price_is_null
    );
    BindStringParam(params_bind[4], params.bid_step, bid_step_length, bid_step_is_null);
    BindIntParam(
        params_bind[5],
        anti_sniping_window_seconds,
        anti_sniping_window_is_null
    );
    BindIntParam(params_bind[6], extend_seconds, extend_seconds_is_null);
    BindUint64Param(params_bind[7], auction_id, auction_id_is_null);
    statement.BindParams(params_bind);
    statement.Execute();

    const auto auction = FindAuctionById(params.auction_id);
    if (!auction.has_value()) {
        throw std::runtime_error("updated auction could not be loaded");
    }
    return *auction;
}

void AuctionRepository::UpdateAuctionStatus(
    const std::uint64_t auction_id,
    const std::string& status
) const {
    PreparedStatement statement(
        connection().native_handle(),
        "UPDATE auction SET status = ?, version = version + 1, updated_at = CURRENT_TIMESTAMP(3) "
        "WHERE auction_id = ?"
    );

    auto mutable_auction_id = auction_id;
    unsigned long status_length = 0;
    bool status_is_null = false;
    bool auction_id_is_null = false;
    MYSQL_BIND params[2]{};
    BindStringParam(params[0], status, status_length, status_is_null);
    BindUint64Param(params[1], mutable_auction_id, auction_id_is_null);
    statement.BindParams(params);
    statement.Execute();
}

void AuctionRepository::UpdateItemStatus(
    const std::uint64_t item_id,
    const std::string& item_status
) const {
    PreparedStatement statement(
        connection().native_handle(),
        "UPDATE item SET item_status = ?, updated_at = CURRENT_TIMESTAMP(3) WHERE item_id = ?"
    );

    auto mutable_item_id = item_id;
    unsigned long item_status_length = 0;
    bool item_status_is_null = false;
    bool item_id_is_null = false;
    MYSQL_BIND params[2]{};
    BindStringParam(params[0], item_status, item_status_length, item_status_is_null);
    BindUint64Param(params[1], mutable_item_id, item_id_is_null);
    statement.BindParams(params);
    statement.Execute();
}

bool AuctionRepository::IsStartDue(const std::uint64_t auction_id) const {
    return QueryCount(
               "SELECT COUNT(*) FROM auction WHERE auction_id = " + std::to_string(auction_id) +
               " AND start_time <= CURRENT_TIMESTAMP(3)"
    ) > 0;
}

bool AuctionRepository::IsFinishDue(const std::uint64_t auction_id) const {
    return QueryCount(
               "SELECT COUNT(*) FROM auction WHERE auction_id = " + std::to_string(auction_id) +
               " AND end_time <= CURRENT_TIMESTAMP(3)"
    ) > 0;
}

std::vector<std::uint64_t> AuctionRepository::ListDueStartAuctionIds() const {
    const auto result = ExecuteQuery(
        connection(),
        "SELECT auction_id FROM auction WHERE status = 'PENDING_START' "
        "AND start_time <= CURRENT_TIMESTAMP(3) ORDER BY auction_id ASC"
    );

    std::vector<std::uint64_t> auction_ids;
    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(result.get())) != nullptr) {
        auction_ids.push_back(ReadRowUint64(row, 0));
    }
    return auction_ids;
}

std::vector<std::uint64_t> AuctionRepository::ListDueFinishAuctionIds() const {
    const auto result = ExecuteQuery(
        connection(),
        "SELECT auction_id FROM auction WHERE status = 'RUNNING' "
        "AND end_time <= CURRENT_TIMESTAMP(3) ORDER BY auction_id ASC"
    );

    std::vector<std::uint64_t> auction_ids;
    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(result.get())) != nullptr) {
        auction_ids.push_back(ReadRowUint64(row, 0));
    }
    return auction_ids;
}

std::vector<modules::auction::AdminAuctionSummary> AuctionRepository::ListAdminAuctions(
    const modules::auction::AdminAuctionQuery& query
) const {
    auto* mutable_this = const_cast<AuctionRepository*>(this);
    const auto result = ExecuteQuery(connection(), BuildAdminAuctionListSql(query, *mutable_this));

    std::vector<modules::auction::AdminAuctionSummary> auctions;
    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(result.get())) != nullptr) {
        auctions.push_back(BuildAdminAuctionSummary(row));
    }
    return auctions;
}

std::optional<modules::auction::AdminAuctionDetail> AuctionRepository::FindAdminAuctionDetail(
    const std::uint64_t auction_id
) const {
    const auto result = ExecuteQuery(
        connection(),
        "SELECT a.auction_id, a.item_id, a.seller_id, CAST(a.start_time AS CHAR), "
        "CAST(a.end_time AS CHAR), CAST(a.start_price AS CHAR), CAST(a.current_price AS CHAR), "
        "CAST(a.bid_step AS CHAR), a.highest_bidder_id, a.status, a.anti_sniping_window_seconds, "
        "a.extend_seconds, a.version, CAST(a.created_at AS CHAR), CAST(a.updated_at AS CHAR), "
        "i.item_id, i.seller_id, i.title, COALESCE(i.cover_image_url, ''), i.item_status, "
        "COALESCE(seller.username, ''), COALESCE(highest.username, '') "
        "FROM auction a "
        "INNER JOIN item i ON i.item_id = a.item_id "
        "INNER JOIN user_account seller ON seller.user_id = a.seller_id "
        "LEFT JOIN user_account highest ON highest.user_id = a.highest_bidder_id "
        "WHERE a.auction_id = " +
            std::to_string(auction_id) + " LIMIT 1"
    );

    if (MYSQL_ROW row = mysql_fetch_row(result.get()); row != nullptr) {
        return modules::auction::AdminAuctionDetail{
            .auction =
                modules::auction::AuctionRecord{
                    .auction_id = ReadRowUint64(row, 0),
                    .item_id = ReadRowUint64(row, 1),
                    .seller_id = ReadRowUint64(row, 2),
                    .start_time = ReadRowString(row, 3),
                    .end_time = ReadRowString(row, 4),
                    .start_price = ReadRowDouble(row, 5),
                    .current_price = ReadRowDouble(row, 6),
                    .bid_step = ReadRowDouble(row, 7),
                    .highest_bidder_id = ReadRowOptionalUint64(row, 8),
                    .status = ReadRowString(row, 9),
                    .anti_sniping_window_seconds = ReadRowInt(row, 10),
                    .extend_seconds = ReadRowInt(row, 11),
                    .version = ReadRowUint64(row, 12),
                    .created_at = ReadRowString(row, 13),
                    .updated_at = ReadRowString(row, 14),
                },
            .item =
                modules::auction::AuctionItemSnapshot{
                    .item_id = ReadRowUint64(row, 15),
                    .seller_id = ReadRowUint64(row, 16),
                    .title = ReadRowString(row, 17),
                    .cover_image_url = ReadRowString(row, 18),
                    .item_status = ReadRowString(row, 19),
                },
            .seller_username = ReadRowString(row, 20),
            .highest_bidder_username = ReadRowString(row, 21),
        };
    }

    return std::nullopt;
}

std::vector<modules::auction::PublicAuctionSummary> AuctionRepository::ListPublicAuctions(
    const modules::auction::PublicAuctionQuery& query
) const {
    auto* mutable_this = const_cast<AuctionRepository*>(this);
    const auto result = ExecuteQuery(connection(), BuildPublicAuctionListSql(query, *mutable_this));

    std::vector<modules::auction::PublicAuctionSummary> auctions;
    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(result.get())) != nullptr) {
        auctions.push_back(BuildPublicAuctionSummary(row));
    }
    return auctions;
}

std::optional<modules::auction::PublicAuctionDetail> AuctionRepository::FindPublicAuctionDetail(
    const std::uint64_t auction_id
) const {
    const auto result = ExecuteQuery(
        connection(),
        "SELECT a.auction_id, a.item_id, i.title, COALESCE(i.cover_image_url, ''), a.status, "
        "CAST(a.start_price AS CHAR), CAST(a.current_price AS CHAR), CAST(a.bid_step AS CHAR), "
        "CAST(a.start_time AS CHAR), CAST(a.end_time AS CHAR), a.anti_sniping_window_seconds, "
        "a.extend_seconds, COALESCE(highest.username, '') "
        "FROM auction a "
        "INNER JOIN item i ON i.item_id = a.item_id "
        "LEFT JOIN user_account highest ON highest.user_id = a.highest_bidder_id "
        "WHERE a.auction_id = " +
            std::to_string(auction_id) + " AND a.status <> 'CANCELLED' LIMIT 1"
    );

    if (MYSQL_ROW row = mysql_fetch_row(result.get()); row != nullptr) {
        return modules::auction::PublicAuctionDetail{
            .auction_id = ReadRowUint64(row, 0),
            .item_id = ReadRowUint64(row, 1),
            .title = ReadRowString(row, 2),
            .cover_image_url = ReadRowString(row, 3),
            .status = ReadRowString(row, 4),
            .start_price = ReadRowDouble(row, 5),
            .current_price = ReadRowDouble(row, 6),
            .bid_step = ReadRowDouble(row, 7),
            .start_time = ReadRowString(row, 8),
            .end_time = ReadRowString(row, 9),
            .anti_sniping_window_seconds = ReadRowInt(row, 10),
            .extend_seconds = ReadRowInt(row, 11),
            .highest_bidder_masked = ReadRowString(row, 12),
        };
    }

    return std::nullopt;
}

void AuctionRepository::InsertTaskLog(const CreateTaskLogParams& params) const {
    PreparedStatement statement(
        connection().native_handle(),
        "INSERT INTO task_log (task_type, biz_key, task_status, retry_count, last_error, "
        "scheduled_at, started_at, finished_at) "
        "VALUES (?, ?, ?, ?, ?, ?, CURRENT_TIMESTAMP(3), CURRENT_TIMESTAMP(3))"
    );

    auto retry_count = params.retry_count;
    unsigned long task_type_length = 0;
    unsigned long biz_key_length = 0;
    unsigned long task_status_length = 0;
    unsigned long last_error_length = 0;
    unsigned long scheduled_at_length = 0;
    bool task_type_is_null = false;
    bool biz_key_is_null = false;
    bool task_status_is_null = false;
    bool retry_count_is_null = false;
    bool last_error_is_null = false;
    bool scheduled_at_is_null = false;
    MYSQL_BIND params_bind[6]{};
    BindStringParam(params_bind[0], params.task_type, task_type_length, task_type_is_null);
    BindStringParam(params_bind[1], params.biz_key, biz_key_length, biz_key_is_null);
    BindStringParam(
        params_bind[2],
        params.task_status,
        task_status_length,
        task_status_is_null
    );
    BindIntParam(params_bind[3], retry_count, retry_count_is_null);
    BindNullableStringParam(params_bind[4], params.last_error, last_error_length, last_error_is_null);
    BindNullableStringParam(
        params_bind[5],
        params.scheduled_at,
        scheduled_at_length,
        scheduled_at_is_null
    );
    statement.BindParams(params_bind);
    statement.Execute();
}

}  // namespace auction::repository
