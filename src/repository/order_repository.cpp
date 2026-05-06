#include "repository/order_repository.h"

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

modules::order::OrderRecord BuildOrderRecord(MYSQL_ROW row, const unsigned int offset) {
    return modules::order::OrderRecord{
        .order_id = ReadRowUint64(row, offset + 0),
        .order_no = ReadRowString(row, offset + 1),
        .auction_id = ReadRowUint64(row, offset + 2),
        .buyer_id = ReadRowUint64(row, offset + 3),
        .seller_id = ReadRowUint64(row, offset + 4),
        .final_amount = ReadRowDouble(row, offset + 5),
        .order_status = ReadRowString(row, offset + 6),
        .pay_deadline_at = ReadRowString(row, offset + 7),
        .paid_at = ReadRowString(row, offset + 8),
        .shipped_at = ReadRowString(row, offset + 9),
        .completed_at = ReadRowString(row, offset + 10),
        .closed_at = ReadRowString(row, offset + 11),
        .created_at = ReadRowString(row, offset + 12),
        .updated_at = ReadRowString(row, offset + 13),
    };
}

std::optional<modules::order::OrderPaymentSummary> BuildOrderPaymentSummary(
    MYSQL_ROW row,
    const unsigned int offset
) {
    if (row[offset] == nullptr) {
        return std::nullopt;
    }

    return modules::order::OrderPaymentSummary{
        .payment_id = ReadRowUint64(row, offset + 0),
        .payment_no = ReadRowString(row, offset + 1),
        .order_id = ReadRowUint64(row, offset + 2),
        .pay_channel = ReadRowString(row, offset + 3),
        .transaction_no = ReadRowString(row, offset + 4),
        .pay_amount = ReadRowDouble(row, offset + 5),
        .pay_status = ReadRowString(row, offset + 6),
        .paid_at = ReadRowString(row, offset + 7),
        .created_at = ReadRowString(row, offset + 8),
        .updated_at = ReadRowString(row, offset + 9),
    };
}

modules::order::UserOrderEntry BuildUserOrderEntry(MYSQL_ROW row) {
    return modules::order::UserOrderEntry{
        .order = BuildOrderRecord(row, 0),
        .item_id = ReadRowUint64(row, 14),
        .item_title = ReadRowString(row, 15),
        .cover_image_url = ReadRowString(row, 16),
        .auction_status = ReadRowString(row, 17),
        .buyer_username = ReadRowString(row, 18),
        .seller_username = ReadRowString(row, 19),
        .latest_payment = BuildOrderPaymentSummary(row, 20),
    };
}

std::string BuildUserOrderSelectSql() {
    return
        "SELECT o.order_id, o.order_no, o.auction_id, o.buyer_id, o.seller_id, "
        "CAST(o.final_amount AS CHAR), o.order_status, "
        "COALESCE(CAST(o.pay_deadline_at AS CHAR), ''), "
        "COALESCE(CAST(o.paid_at AS CHAR), ''), COALESCE(CAST(o.shipped_at AS CHAR), ''), "
        "COALESCE(CAST(o.completed_at AS CHAR), ''), COALESCE(CAST(o.closed_at AS CHAR), ''), "
        "CAST(o.created_at AS CHAR), CAST(o.updated_at AS CHAR), "
        "i.item_id, i.title, COALESCE(i.cover_image_url, ''), a.status, "
        "buyer.username, seller.username, "
        "p.payment_id, p.payment_no, p.order_id, p.pay_channel, COALESCE(p.transaction_no, ''), "
        "CAST(p.pay_amount AS CHAR), p.pay_status, COALESCE(CAST(p.paid_at AS CHAR), ''), "
        "CAST(p.created_at AS CHAR), CAST(p.updated_at AS CHAR) "
        "FROM order_info o "
        "INNER JOIN auction a ON a.auction_id = o.auction_id "
        "INNER JOIN item i ON i.item_id = a.item_id "
        "INNER JOIN user_account buyer ON buyer.user_id = o.buyer_id "
        "INNER JOIN user_account seller ON seller.user_id = o.seller_id "
        "LEFT JOIN payment_record p ON p.payment_id = ("
        "SELECT p2.payment_id FROM payment_record p2 "
        "WHERE p2.order_id = o.order_id ORDER BY p2.payment_id DESC LIMIT 1"
        ") ";
}

std::string BuildUserOrderScopeClause(const std::uint64_t user_id, const std::string& role) {
    if (role == "buyer") {
        return "o.buyer_id = " + std::to_string(user_id);
    }
    if (role == "seller") {
        return "o.seller_id = " + std::to_string(user_id);
    }
    return "(o.buyer_id = " + std::to_string(user_id) + " OR o.seller_id = " +
           std::to_string(user_id) + ")";
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

void BindNullableUint64Param(
    MYSQL_BIND& bind,
    std::uint64_t& value,
    bool& is_null,
    const bool present
) {
    std::memset(&bind, 0, sizeof(bind));
    is_null = !present;
    bind.buffer_type = MYSQL_TYPE_LONGLONG;
    bind.buffer = present ? &value : nullptr;
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

std::optional<modules::order::OrderRecord> FindOrderById(
    common::db::MysqlConnection& connection,
    const std::uint64_t order_id,
    const bool for_update
) {
    const auto result = ExecuteQuery(
        connection,
        "SELECT order_id, order_no, auction_id, buyer_id, seller_id, CAST(final_amount AS CHAR), "
        "order_status, COALESCE(CAST(pay_deadline_at AS CHAR), ''), "
        "COALESCE(CAST(paid_at AS CHAR), ''), COALESCE(CAST(shipped_at AS CHAR), ''), "
        "COALESCE(CAST(completed_at AS CHAR), ''), COALESCE(CAST(closed_at AS CHAR), ''), "
        "CAST(created_at AS CHAR), CAST(updated_at AS CHAR) "
        "FROM order_info WHERE order_id = " +
            std::to_string(order_id) + " LIMIT 1" + (for_update ? " FOR UPDATE" : "")
    );

    if (MYSQL_ROW row = mysql_fetch_row(result.get()); row != nullptr) {
        return BuildOrderRecord(row, 0);
    }
    return std::nullopt;
}

std::optional<modules::order::OrderRecord> LoadOrderByAuctionId(
    common::db::MysqlConnection& connection,
    const std::uint64_t auction_id
) {
    const auto result = ExecuteQuery(
        connection,
        "SELECT order_id, order_no, auction_id, buyer_id, seller_id, CAST(final_amount AS CHAR), "
        "order_status, COALESCE(CAST(pay_deadline_at AS CHAR), ''), "
        "COALESCE(CAST(paid_at AS CHAR), ''), COALESCE(CAST(shipped_at AS CHAR), ''), "
        "COALESCE(CAST(completed_at AS CHAR), ''), COALESCE(CAST(closed_at AS CHAR), ''), "
        "CAST(created_at AS CHAR), CAST(updated_at AS CHAR) "
        "FROM order_info WHERE auction_id = " +
            std::to_string(auction_id) + " LIMIT 1"
    );

    if (MYSQL_ROW row = mysql_fetch_row(result.get()); row != nullptr) {
        return BuildOrderRecord(row, 0);
    }
    return std::nullopt;
}

}  // namespace

std::vector<modules::order::UserOrderEntry> OrderRepository::ListUserOrders(
    const std::uint64_t user_id,
    const modules::order::UserOrderQuery& query
) const {
    const int page_no = query.page_no <= 0 ? 1 : query.page_no;
    const int page_size = query.page_size <= 0 ? 20 : query.page_size;
    const int offset = (page_no - 1) * page_size;

    std::string sql = BuildUserOrderSelectSql() + "WHERE " +
                      BuildUserOrderScopeClause(user_id, query.role);
    if (!query.order_status.empty()) {
        sql += " AND o.order_status = '" + EscapeString(query.order_status) + "'";
    }
    sql += " ORDER BY o.updated_at DESC, o.order_id DESC LIMIT " + std::to_string(page_size) +
           " OFFSET " + std::to_string(offset);

    const auto result = ExecuteQuery(connection(), sql);
    std::vector<modules::order::UserOrderEntry> orders;
    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(result.get())) != nullptr) {
        orders.push_back(BuildUserOrderEntry(row));
    }
    return orders;
}

std::optional<modules::order::UserOrderEntry> OrderRepository::FindUserOrderDetailById(
    const std::uint64_t order_id
) const {
    const auto result = ExecuteQuery(
        connection(),
        BuildUserOrderSelectSql() + "WHERE o.order_id = " + std::to_string(order_id) + " LIMIT 1"
    );

    if (MYSQL_ROW row = mysql_fetch_row(result.get()); row != nullptr) {
        return BuildUserOrderEntry(row);
    }
    return std::nullopt;
}

std::vector<std::uint64_t> OrderRepository::ListSettlingAuctionIds() const {
    const auto result = ExecuteQuery(
        connection(),
        "SELECT auction_id FROM auction WHERE status = 'SETTLING' ORDER BY auction_id ASC"
    );

    std::vector<std::uint64_t> auction_ids;
    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(result.get())) != nullptr) {
        auction_ids.push_back(ReadRowUint64(row, 0));
    }
    return auction_ids;
}

std::optional<SettlementAuctionAggregate> OrderRepository::FindSettlementAuctionById(
    const std::uint64_t auction_id
) const {
    const auto result = ExecuteQuery(
        connection(),
        "SELECT a.auction_id, a.item_id, a.seller_id, a.highest_bidder_id, "
        "CAST(a.current_price AS CHAR), a.status, CAST(a.end_time AS CHAR), "
        "i.item_status, i.title "
        "FROM auction a "
        "INNER JOIN item i ON i.item_id = a.item_id "
        "WHERE a.auction_id = " +
            std::to_string(auction_id) + " LIMIT 1"
    );

    if (MYSQL_ROW row = mysql_fetch_row(result.get()); row != nullptr) {
        return SettlementAuctionAggregate{
            .auction_id = ReadRowUint64(row, 0),
            .item_id = ReadRowUint64(row, 1),
            .seller_id = ReadRowUint64(row, 2),
            .highest_bidder_id = ReadRowOptionalUint64(row, 3),
            .current_price = ReadRowDouble(row, 4),
            .status = ReadRowString(row, 5),
            .end_time = ReadRowString(row, 6),
            .item_status = ReadRowString(row, 7),
            .item_title = ReadRowString(row, 8),
        };
    }

    return std::nullopt;
}

std::optional<SettlementAuctionAggregate> OrderRepository::FindSettlementAuctionByIdForUpdate(
    const std::uint64_t auction_id
) const {
    const auto result = ExecuteQuery(
        connection(),
        "SELECT a.auction_id, a.item_id, a.seller_id, a.highest_bidder_id, "
        "CAST(a.current_price AS CHAR), a.status, CAST(a.end_time AS CHAR), "
        "i.item_status, i.title "
        "FROM auction a "
        "INNER JOIN item i ON i.item_id = a.item_id "
        "WHERE a.auction_id = " +
            std::to_string(auction_id) + " LIMIT 1 FOR UPDATE"
    );

    if (MYSQL_ROW row = mysql_fetch_row(result.get()); row != nullptr) {
        return SettlementAuctionAggregate{
            .auction_id = ReadRowUint64(row, 0),
            .item_id = ReadRowUint64(row, 1),
            .seller_id = ReadRowUint64(row, 2),
            .highest_bidder_id = ReadRowOptionalUint64(row, 3),
            .current_price = ReadRowDouble(row, 4),
            .status = ReadRowString(row, 5),
            .end_time = ReadRowString(row, 6),
            .item_status = ReadRowString(row, 7),
            .item_title = ReadRowString(row, 8),
        };
    }

    return std::nullopt;
}

std::optional<modules::order::OrderRecord> OrderRepository::FindOrderByAuctionId(
    const std::uint64_t auction_id
) const {
    return LoadOrderByAuctionId(connection(), auction_id);
}

std::optional<modules::order::OrderRecord> OrderRepository::FindOrderById(
    const std::uint64_t order_id
) const {
    return ::auction::repository::FindOrderById(connection(), order_id, false);
}

std::optional<modules::order::OrderRecord> OrderRepository::FindOrderByIdForUpdate(
    const std::uint64_t order_id
) const {
    return ::auction::repository::FindOrderById(connection(), order_id, true);
}

modules::order::OrderRecord OrderRepository::CreateOrder(const CreateOrderParams& params) const {
    PreparedStatement statement(
        connection().native_handle(),
        "INSERT INTO order_info (order_no, auction_id, buyer_id, seller_id, final_amount, "
        "order_status, pay_deadline_at) VALUES (?, ?, ?, ?, ?, ?, ?)"
    );

    auto auction_id = params.auction_id;
    auto buyer_id = params.buyer_id;
    auto seller_id = params.seller_id;
    unsigned long order_no_length = 0;
    unsigned long final_amount_length = 0;
    unsigned long order_status_length = 0;
    unsigned long pay_deadline_length = 0;
    bool order_no_is_null = false;
    bool auction_id_is_null = false;
    bool buyer_id_is_null = false;
    bool seller_id_is_null = false;
    bool final_amount_is_null = false;
    bool order_status_is_null = false;
    bool pay_deadline_is_null = false;
    MYSQL_BIND params_bind[7]{};
    BindStringParam(params_bind[0], params.order_no, order_no_length, order_no_is_null);
    BindUint64Param(params_bind[1], auction_id, auction_id_is_null);
    BindUint64Param(params_bind[2], buyer_id, buyer_id_is_null);
    BindUint64Param(params_bind[3], seller_id, seller_id_is_null);
    BindStringParam(params_bind[4], params.final_amount, final_amount_length, final_amount_is_null);
    BindStringParam(params_bind[5], params.order_status, order_status_length, order_status_is_null);
    BindStringParam(params_bind[6], params.pay_deadline_at, pay_deadline_length, pay_deadline_is_null);
    statement.BindParams(params_bind);
    statement.Execute();

    const auto created = ::auction::repository::FindOrderById(connection(), statement.InsertId(), false);
    if (!created.has_value()) {
        throw std::runtime_error("created order could not be loaded");
    }
    return *created;
}

std::vector<std::uint64_t> OrderRepository::ListExpiredPendingOrderIds() const {
    const auto result = ExecuteQuery(
        connection(),
        "SELECT order_id FROM order_info WHERE order_status = 'PENDING_PAYMENT' "
        "AND pay_deadline_at IS NOT NULL AND pay_deadline_at <= CURRENT_TIMESTAMP(3) "
        "ORDER BY order_id ASC"
    );

    std::vector<std::uint64_t> order_ids;
    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(result.get())) != nullptr) {
        order_ids.push_back(ReadRowUint64(row, 0));
    }
    return order_ids;
}

std::optional<ExpiredOrderAggregate> OrderRepository::FindExpiredOrderAggregateByIdForUpdate(
    const std::uint64_t order_id
) const {
    const auto result = ExecuteQuery(
        connection(),
        "SELECT o.order_id, o.order_no, o.auction_id, o.buyer_id, o.seller_id, "
        "CAST(o.final_amount AS CHAR), o.order_status, COALESCE(CAST(o.pay_deadline_at AS CHAR), ''), "
        "COALESCE(CAST(o.paid_at AS CHAR), ''), COALESCE(CAST(o.shipped_at AS CHAR), ''), "
        "COALESCE(CAST(o.completed_at AS CHAR), ''), COALESCE(CAST(o.closed_at AS CHAR), ''), "
        "CAST(o.created_at AS CHAR), CAST(o.updated_at AS CHAR), "
        "a.auction_id, a.status, i.item_id, i.item_status, i.title "
        "FROM order_info o "
        "INNER JOIN auction a ON a.auction_id = o.auction_id "
        "INNER JOIN item i ON i.item_id = a.item_id "
        "WHERE o.order_id = " +
            std::to_string(order_id) + " LIMIT 1 FOR UPDATE"
    );

    if (MYSQL_ROW row = mysql_fetch_row(result.get()); row != nullptr) {
        return ExpiredOrderAggregate{
            .order = BuildOrderRecord(row, 0),
            .auction_id = ReadRowUint64(row, 14),
            .auction_status = ReadRowString(row, 15),
            .item_id = ReadRowUint64(row, 16),
            .item_status = ReadRowString(row, 17),
            .item_title = ReadRowString(row, 18),
        };
    }

    return std::nullopt;
}

bool OrderRepository::IsOrderPaymentDeadlineExpired(const std::uint64_t order_id) const {
    return QueryCount(
               "SELECT COUNT(*) FROM order_info WHERE order_id = " + std::to_string(order_id) +
               " AND pay_deadline_at <= CURRENT_TIMESTAMP(3)"
    ) > 0;
}

void OrderRepository::UpdateOrderStatusToClosed(const std::uint64_t order_id) const {
    PreparedStatement statement(
        connection().native_handle(),
        "UPDATE order_info SET order_status = 'CLOSED', closed_at = CURRENT_TIMESTAMP(3), "
        "updated_at = CURRENT_TIMESTAMP(3) WHERE order_id = ?"
    );

    auto mutable_order_id = order_id;
    bool order_id_is_null = false;
    MYSQL_BIND params_bind[1]{};
    BindUint64Param(params_bind[0], mutable_order_id, order_id_is_null);
    statement.BindParams(params_bind);
    statement.Execute();
}

void OrderRepository::UpdateOrderStatusToShipped(const std::uint64_t order_id) const {
    PreparedStatement statement(
        connection().native_handle(),
        "UPDATE order_info SET order_status = 'SHIPPED', shipped_at = CURRENT_TIMESTAMP(3), "
        "updated_at = CURRENT_TIMESTAMP(3) WHERE order_id = ?"
    );

    auto mutable_order_id = order_id;
    bool order_id_is_null = false;
    MYSQL_BIND params_bind[1]{};
    BindUint64Param(params_bind[0], mutable_order_id, order_id_is_null);
    statement.BindParams(params_bind);
    statement.Execute();
}

void OrderRepository::UpdateOrderStatusToCompleted(const std::uint64_t order_id) const {
    PreparedStatement statement(
        connection().native_handle(),
        "UPDATE order_info SET order_status = 'COMPLETED', completed_at = CURRENT_TIMESTAMP(3), "
        "updated_at = CURRENT_TIMESTAMP(3) WHERE order_id = ?"
    );

    auto mutable_order_id = order_id;
    bool order_id_is_null = false;
    MYSQL_BIND params_bind[1]{};
    BindUint64Param(params_bind[0], mutable_order_id, order_id_is_null);
    statement.BindParams(params_bind);
    statement.Execute();
}

void OrderRepository::CloseOpenPaymentsByOrder(const std::uint64_t order_id) const {
    PreparedStatement statement(
        connection().native_handle(),
        "UPDATE payment_record SET pay_status = 'CLOSED', updated_at = CURRENT_TIMESTAMP(3) "
        "WHERE order_id = ? AND pay_status IN ('INITIATED', 'WAITING_CALLBACK', 'FAILED')"
    );

    auto mutable_order_id = order_id;
    bool order_id_is_null = false;
    MYSQL_BIND params_bind[1]{};
    BindUint64Param(params_bind[0], mutable_order_id, order_id_is_null);
    statement.BindParams(params_bind);
    statement.Execute();
}

void OrderRepository::UpdateAuctionStatus(
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
    MYSQL_BIND params_bind[2]{};
    BindStringParam(params_bind[0], status, status_length, status_is_null);
    BindUint64Param(params_bind[1], mutable_auction_id, auction_id_is_null);
    statement.BindParams(params_bind);
    statement.Execute();
}

void OrderRepository::UpdateItemStatus(
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
    MYSQL_BIND params_bind[2]{};
    BindStringParam(params_bind[0], item_status, item_status_length, item_status_is_null);
    BindUint64Param(params_bind[1], mutable_item_id, item_id_is_null);
    statement.BindParams(params_bind);
    statement.Execute();
}

void OrderRepository::InsertTaskLog(const OrderTaskLogParams& params) const {
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

void OrderRepository::InsertOperationLog(
    const std::optional<std::uint64_t>& operator_id,
    const std::string& operator_role,
    const std::string& operation_name,
    const std::string& biz_key,
    const std::string& result,
    const std::string& detail
) const {
    PreparedStatement statement(
        connection().native_handle(),
        "INSERT INTO operation_log (operator_id, operator_role, module_name, operation_name, "
        "biz_key, result, detail) VALUES (?, ?, 'order', ?, ?, ?, ?)"
    );

    auto mutable_operator_id = operator_id.value_or(0);
    const bool has_operator_id = operator_id.has_value();
    unsigned long role_length = 0;
    unsigned long operation_length = 0;
    unsigned long biz_key_length = 0;
    unsigned long result_length = 0;
    unsigned long detail_length = 0;
    bool operator_id_is_null = false;
    bool role_is_null = false;
    bool operation_is_null = false;
    bool biz_key_is_null = false;
    bool result_is_null = false;
    bool detail_is_null = false;
    MYSQL_BIND params_bind[6]{};
    BindNullableUint64Param(
        params_bind[0],
        mutable_operator_id,
        operator_id_is_null,
        has_operator_id
    );
    BindStringParam(params_bind[1], operator_role, role_length, role_is_null);
    BindStringParam(params_bind[2], operation_name, operation_length, operation_is_null);
    BindStringParam(params_bind[3], biz_key, biz_key_length, biz_key_is_null);
    BindStringParam(params_bind[4], result, result_length, result_is_null);
    BindStringParam(params_bind[5], detail, detail_length, detail_is_null);
    statement.BindParams(params_bind);
    statement.Execute();
}

}  // namespace auction::repository
