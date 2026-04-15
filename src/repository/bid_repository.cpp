#include "repository/bid_repository.h"

#include <cmath>
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

std::string MaskUsername(const std::string& username) {
    if (username.empty()) {
        return {};
    }
    if (username.size() <= 2) {
        return username.substr(0, 1) + "***";
    }
    return username.substr(0, 1) + "***" + username.substr(username.size() - 1);
}

modules::bid::AuctionBidSnapshot BuildAuctionSnapshot(MYSQL_ROW row) {
    return modules::bid::AuctionBidSnapshot{
        .auction_id = ReadRowUint64(row, 0),
        .item_id = ReadRowUint64(row, 1),
        .seller_id = ReadRowUint64(row, 2),
        .status = ReadRowString(row, 3),
        .start_time = ReadRowString(row, 4),
        .end_time = ReadRowString(row, 5),
        .start_price = ReadRowDouble(row, 6),
        .current_price = ReadRowDouble(row, 7),
        .bid_step = ReadRowDouble(row, 8),
        .highest_bidder_id = ReadRowOptionalUint64(row, 9),
        .highest_bidder_username = ReadRowString(row, 10),
        .anti_sniping_window_seconds = ReadRowInt(row, 11),
        .extend_seconds = ReadRowInt(row, 12),
        .version = ReadRowUint64(row, 13),
    };
}

modules::bid::BidRecord BuildBidRecord(MYSQL_ROW row) {
    return modules::bid::BidRecord{
        .bid_id = ReadRowUint64(row, 0),
        .auction_id = ReadRowUint64(row, 1),
        .bidder_id = ReadRowUint64(row, 2),
        .bidder_username = ReadRowString(row, 3),
        .request_id = ReadRowString(row, 4),
        .bid_amount = ReadRowDouble(row, 5),
        .bid_status = ReadRowString(row, 6),
        .bid_time = ReadRowString(row, 7),
        .created_at = ReadRowString(row, 8),
    };
}

modules::bid::BidHistoryEntry BuildBidHistoryEntry(MYSQL_ROW row) {
    return modules::bid::BidHistoryEntry{
        .bid_id = ReadRowUint64(row, 0),
        .bid_amount = ReadRowDouble(row, 1),
        .bid_status = ReadRowString(row, 2),
        .bid_time = ReadRowString(row, 3),
        .bidder_masked = MaskUsername(ReadRowString(row, 4)),
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

std::string BuildAuctionSnapshotSql(const std::string& where_clause) {
    return "SELECT a.auction_id, a.item_id, a.seller_id, a.status, CAST(a.start_time AS CHAR), "
           "CAST(a.end_time AS CHAR), CAST(a.start_price AS CHAR), CAST(a.current_price AS CHAR), "
           "CAST(a.bid_step AS CHAR), a.highest_bidder_id, COALESCE(highest.username, ''), "
           "a.anti_sniping_window_seconds, a.extend_seconds, a.version "
           "FROM auction a LEFT JOIN user_account highest ON highest.user_id = a.highest_bidder_id " +
           where_clause;
}

std::string BuildBidSelectSql(const std::string& where_clause) {
    return "SELECT b.bid_id, b.auction_id, b.bidder_id, COALESCE(u.username, ''), "
           "b.request_id, CAST(b.bid_amount AS CHAR), b.bid_status, CAST(b.bid_time AS CHAR), "
           "CAST(b.created_at AS CHAR) "
           "FROM bid_record b INNER JOIN user_account u ON u.user_id = b.bidder_id " +
           where_clause;
}

std::optional<modules::bid::AuctionBidSnapshot> FindSingleAuctionSnapshotBySql(
    common::db::MysqlConnection& connection,
    const std::string& sql
) {
    const auto result = ExecuteQuery(connection, sql);
    if (MYSQL_ROW row = mysql_fetch_row(result.get()); row != nullptr) {
        return BuildAuctionSnapshot(row);
    }
    return std::nullopt;
}

std::optional<modules::bid::BidRecord> FindSingleBidBySql(
    common::db::MysqlConnection& connection,
    const std::string& sql
) {
    const auto result = ExecuteQuery(connection, sql);
    if (MYSQL_ROW row = mysql_fetch_row(result.get()); row != nullptr) {
        return BuildBidRecord(row);
    }
    return std::nullopt;
}

std::optional<modules::bid::BidRecord> FindBidById(
    common::db::MysqlConnection& connection,
    const std::uint64_t bid_id
) {
    return FindSingleBidBySql(
        connection,
        BuildBidSelectSql("WHERE b.bid_id = " + std::to_string(bid_id) + " LIMIT 1")
    );
}

}  // namespace

std::optional<modules::bid::AuctionBidSnapshot> BidRepository::FindAuctionSnapshotById(
    const std::uint64_t auction_id
) const {
    return FindSingleAuctionSnapshotBySql(
        connection(),
        BuildAuctionSnapshotSql("WHERE a.auction_id = " + std::to_string(auction_id) + " LIMIT 1")
    );
}

std::optional<modules::bid::AuctionBidSnapshot> BidRepository::FindAuctionSnapshotByIdForUpdate(
    const std::uint64_t auction_id
) const {
    return FindSingleAuctionSnapshotBySql(
        connection(),
        BuildAuctionSnapshotSql(
            "WHERE a.auction_id = " + std::to_string(auction_id) + " LIMIT 1 FOR UPDATE"
        )
    );
}

bool BidRepository::IsAuctionExpired(const std::uint64_t auction_id) const {
    return QueryCount(
               "SELECT COUNT(*) FROM auction WHERE auction_id = " + std::to_string(auction_id) +
               " AND end_time <= CURRENT_TIMESTAMP(3)"
    ) > 0;
}

bool BidRepository::IsWithinAntiSnipingWindow(
    const std::uint64_t auction_id,
    const int window_seconds
) const {
    const auto window_microseconds =
        std::to_string(static_cast<long long>(window_seconds) * 1000000LL);
    return QueryCount(
               "SELECT COUNT(*) FROM auction WHERE auction_id = " + std::to_string(auction_id) +
               " AND end_time > CURRENT_TIMESTAMP(3) "
               "AND TIMESTAMPDIFF(MICROSECOND, CURRENT_TIMESTAMP(3), end_time) <= " +
               window_microseconds
    ) > 0;
}

std::optional<modules::bid::BidRecord> BidRepository::FindBidByRequestId(
    const std::uint64_t auction_id,
    const std::uint64_t bidder_id,
    const std::string_view request_id
) const {
    return FindSingleBidBySql(
        connection(),
        BuildBidSelectSql(
            "WHERE b.auction_id = " + std::to_string(auction_id) + " AND b.bidder_id = " +
            std::to_string(bidder_id) + " AND b.request_id = '" + EscapeString(request_id) +
            "' LIMIT 1"
        )
    );
}

std::optional<modules::bid::BidRecord> BidRepository::FindCurrentWinningBid(
    const std::uint64_t auction_id
) const {
    return FindSingleBidBySql(
        connection(),
        BuildBidSelectSql(
            "WHERE b.auction_id = " + std::to_string(auction_id) +
            " AND b.bid_status = 'WINNING' ORDER BY b.bid_time DESC, b.bid_id DESC LIMIT 1"
        )
    );
}

std::optional<modules::bid::BidRecord> BidRepository::FindLatestBidByBidder(
    const std::uint64_t auction_id,
    const std::uint64_t bidder_id
) const {
    return FindSingleBidBySql(
        connection(),
        BuildBidSelectSql(
            "WHERE b.auction_id = " + std::to_string(auction_id) + " AND b.bidder_id = " +
            std::to_string(bidder_id) + " ORDER BY b.bid_time DESC, b.bid_id DESC LIMIT 1"
        )
    );
}

modules::bid::BidRecord BidRepository::CreateBidRecord(const CreateBidRecordParams& params) const {
    PreparedStatement statement(
        connection().native_handle(),
        "INSERT INTO bid_record (auction_id, bidder_id, request_id, bid_amount, bid_status, "
        "bid_time) VALUES (?, ?, ?, ?, ?, CURRENT_TIMESTAMP(3))"
    );

    auto auction_id = params.auction_id;
    auto bidder_id = params.bidder_id;
    unsigned long request_id_length = 0;
    unsigned long bid_amount_length = 0;
    unsigned long bid_status_length = 0;
    bool auction_id_is_null = false;
    bool bidder_id_is_null = false;
    bool request_id_is_null = false;
    bool bid_amount_is_null = false;
    bool bid_status_is_null = false;
    MYSQL_BIND params_bind[5]{};
    BindUint64Param(params_bind[0], auction_id, auction_id_is_null);
    BindUint64Param(params_bind[1], bidder_id, bidder_id_is_null);
    BindStringParam(params_bind[2], params.request_id, request_id_length, request_id_is_null);
    BindStringParam(params_bind[3], params.bid_amount, bid_amount_length, bid_amount_is_null);
    BindStringParam(params_bind[4], params.bid_status, bid_status_length, bid_status_is_null);
    statement.BindParams(params_bind);
    statement.Execute();

    const auto created = FindBidById(connection(), statement.InsertId());
    if (!created.has_value()) {
        throw std::runtime_error("created bid record could not be loaded");
    }
    return *created;
}

void BidRepository::MarkWinningBidOutbid(const std::uint64_t auction_id) const {
    PreparedStatement statement(
        connection().native_handle(),
        "UPDATE bid_record SET bid_status = 'OUTBID' WHERE auction_id = ? AND bid_status = 'WINNING'"
    );

    auto mutable_auction_id = auction_id;
    bool auction_id_is_null = false;
    MYSQL_BIND params_bind[1]{};
    BindUint64Param(params_bind[0], mutable_auction_id, auction_id_is_null);
    statement.BindParams(params_bind);
    statement.Execute();
}

void BidRepository::UpdateAuctionPrice(
    const std::uint64_t auction_id,
    const std::string& current_price,
    const std::uint64_t highest_bidder_id
) const {
    PreparedStatement statement(
        connection().native_handle(),
        "UPDATE auction SET current_price = ?, highest_bidder_id = ?, version = version + 1, "
        "updated_at = CURRENT_TIMESTAMP(3) WHERE auction_id = ?"
    );

    auto mutable_auction_id = auction_id;
    auto mutable_highest_bidder_id = highest_bidder_id;
    unsigned long current_price_length = 0;
    bool current_price_is_null = false;
    bool highest_bidder_id_is_null = false;
    bool auction_id_is_null = false;
    MYSQL_BIND params_bind[3]{};
    BindStringParam(params_bind[0], current_price, current_price_length, current_price_is_null);
    BindUint64Param(params_bind[1], mutable_highest_bidder_id, highest_bidder_id_is_null);
    BindUint64Param(params_bind[2], mutable_auction_id, auction_id_is_null);
    statement.BindParams(params_bind);
    statement.Execute();
}

void BidRepository::UpdateAuctionPriceAndExtendEndTime(
    const std::uint64_t auction_id,
    const std::string& current_price,
    const std::uint64_t highest_bidder_id,
    const int extend_seconds
) const {
    PreparedStatement statement(
        connection().native_handle(),
        "UPDATE auction SET current_price = ?, highest_bidder_id = ?, "
        "end_time = DATE_ADD(end_time, INTERVAL ? SECOND), version = version + 1, "
        "updated_at = CURRENT_TIMESTAMP(3) WHERE auction_id = ?"
    );

    auto mutable_auction_id = auction_id;
    auto mutable_highest_bidder_id = highest_bidder_id;
    auto mutable_extend_seconds = extend_seconds;
    unsigned long current_price_length = 0;
    bool current_price_is_null = false;
    bool highest_bidder_id_is_null = false;
    bool extend_seconds_is_null = false;
    bool auction_id_is_null = false;
    MYSQL_BIND params_bind[4]{};
    BindStringParam(params_bind[0], current_price, current_price_length, current_price_is_null);
    BindUint64Param(params_bind[1], mutable_highest_bidder_id, highest_bidder_id_is_null);
    BindIntParam(params_bind[2], mutable_extend_seconds, extend_seconds_is_null);
    BindUint64Param(params_bind[3], mutable_auction_id, auction_id_is_null);
    statement.BindParams(params_bind);
    statement.Execute();
}

std::vector<modules::bid::BidHistoryEntry> BidRepository::ListAuctionBidHistory(
    const std::uint64_t auction_id,
    const modules::bid::BidHistoryQuery& query
) const {
    const int offset = (query.page_no - 1) * query.page_size;
    const auto result = ExecuteQuery(
        connection(),
        "SELECT b.bid_id, CAST(b.bid_amount AS CHAR), b.bid_status, CAST(b.bid_time AS CHAR), "
        "COALESCE(u.username, '') FROM bid_record b "
        "INNER JOIN user_account u ON u.user_id = b.bidder_id "
        "WHERE b.auction_id = " +
            std::to_string(auction_id) +
            " ORDER BY b.bid_time DESC, b.bid_id DESC LIMIT " +
            std::to_string(query.page_size) + " OFFSET " + std::to_string(offset)
    );

    std::vector<modules::bid::BidHistoryEntry> entries;
    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(result.get())) != nullptr) {
        entries.push_back(BuildBidHistoryEntry(row));
    }
    return entries;
}

}  // namespace auction::repository
