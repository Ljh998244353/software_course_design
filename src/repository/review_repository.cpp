#include "repository/review_repository.h"

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

modules::review::ReviewRecord BuildReviewRecord(MYSQL_ROW row, const unsigned int offset) {
    return modules::review::ReviewRecord{
        .review_id = ReadRowUint64(row, offset + 0),
        .order_id = ReadRowUint64(row, offset + 1),
        .reviewer_id = ReadRowUint64(row, offset + 2),
        .reviewee_id = ReadRowUint64(row, offset + 3),
        .review_type = ReadRowString(row, offset + 4),
        .rating = static_cast<int>(ReadRowUint64(row, offset + 5)),
        .content = ReadRowString(row, offset + 6),
        .created_at = ReadRowString(row, offset + 7),
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

std::optional<modules::review::ReviewRecord> FindReviewById(
    common::db::MysqlConnection& connection,
    const std::uint64_t review_id
) {
    const auto result = ExecuteQuery(
        connection,
        "SELECT review_id, order_id, reviewer_id, reviewee_id, review_type, rating, "
        "COALESCE(content, ''), CAST(created_at AS CHAR) "
        "FROM review WHERE review_id = " +
            std::to_string(review_id) + " LIMIT 1"
    );

    if (MYSQL_ROW row = mysql_fetch_row(result.get()); row != nullptr) {
        return BuildReviewRecord(row, 0);
    }
    return std::nullopt;
}

std::optional<OrderReviewAggregate> LoadOrderReviewAggregate(
    common::db::MysqlConnection& connection,
    const std::uint64_t order_id,
    const bool for_update
) {
    const auto result = ExecuteQuery(
        connection,
        "SELECT o.order_id, o.order_no, o.auction_id, o.buyer_id, o.seller_id, "
        "CAST(o.final_amount AS CHAR), o.order_status, COALESCE(CAST(o.pay_deadline_at AS CHAR), ''), "
        "COALESCE(CAST(o.paid_at AS CHAR), ''), COALESCE(CAST(o.shipped_at AS CHAR), ''), "
        "COALESCE(CAST(o.completed_at AS CHAR), ''), COALESCE(CAST(o.closed_at AS CHAR), ''), "
        "CAST(o.created_at AS CHAR), CAST(o.updated_at AS CHAR), i.title "
        "FROM order_info o "
        "INNER JOIN auction a ON a.auction_id = o.auction_id "
        "INNER JOIN item i ON i.item_id = a.item_id "
        "WHERE o.order_id = " +
            std::to_string(order_id) + " LIMIT 1" + (for_update ? " FOR UPDATE" : "")
    );

    if (MYSQL_ROW row = mysql_fetch_row(result.get()); row != nullptr) {
        return OrderReviewAggregate{
            .order = BuildOrderRecord(row, 0),
            .item_title = ReadRowString(row, 14),
        };
    }

    return std::nullopt;
}

}  // namespace

std::optional<OrderReviewAggregate> ReviewRepository::FindOrderReviewAggregateById(
    const std::uint64_t order_id
) const {
    return LoadOrderReviewAggregate(connection(), order_id, false);
}

std::optional<OrderReviewAggregate> ReviewRepository::FindOrderReviewAggregateByIdForUpdate(
    const std::uint64_t order_id
) const {
    return LoadOrderReviewAggregate(connection(), order_id, true);
}

std::optional<modules::review::ReviewRecord> ReviewRepository::FindReviewByOrderAndReviewer(
    const std::uint64_t order_id,
    const std::uint64_t reviewer_id
) const {
    const auto result = ExecuteQuery(
        connection(),
        "SELECT review_id, order_id, reviewer_id, reviewee_id, review_type, rating, "
        "COALESCE(content, ''), CAST(created_at AS CHAR) "
        "FROM review WHERE order_id = " +
            std::to_string(order_id) + " AND reviewer_id = " + std::to_string(reviewer_id) +
            " LIMIT 1"
    );

    if (MYSQL_ROW row = mysql_fetch_row(result.get()); row != nullptr) {
        return BuildReviewRecord(row, 0);
    }
    return std::nullopt;
}

modules::review::ReviewRecord ReviewRepository::CreateReview(const CreateReviewParams& params) const {
    PreparedStatement statement(
        connection().native_handle(),
        "INSERT INTO review (order_id, reviewer_id, reviewee_id, review_type, rating, content) "
        "VALUES (?, ?, ?, ?, ?, ?)"
    );

    auto order_id = params.order_id;
    auto reviewer_id = params.reviewer_id;
    auto reviewee_id = params.reviewee_id;
    auto rating = params.rating;
    unsigned long review_type_length = 0;
    unsigned long content_length = 0;
    bool order_id_is_null = false;
    bool reviewer_id_is_null = false;
    bool reviewee_id_is_null = false;
    bool review_type_is_null = false;
    bool rating_is_null = false;
    bool content_is_null = false;
    MYSQL_BIND params_bind[6]{};
    BindUint64Param(params_bind[0], order_id, order_id_is_null);
    BindUint64Param(params_bind[1], reviewer_id, reviewer_id_is_null);
    BindUint64Param(params_bind[2], reviewee_id, reviewee_id_is_null);
    BindStringParam(params_bind[3], params.review_type, review_type_length, review_type_is_null);
    BindIntParam(params_bind[4], rating, rating_is_null);
    BindNullableStringParam(params_bind[5], params.content, content_length, content_is_null);
    statement.BindParams(params_bind);
    statement.Execute();

    const auto created = FindReviewById(connection(), statement.InsertId());
    if (!created.has_value()) {
        throw std::runtime_error("created review could not be loaded");
    }
    return *created;
}

std::vector<modules::review::ReviewRecord> ReviewRepository::ListReviewsByOrder(
    const std::uint64_t order_id
) const {
    const auto result = ExecuteQuery(
        connection(),
        "SELECT review_id, order_id, reviewer_id, reviewee_id, review_type, rating, "
        "COALESCE(content, ''), CAST(created_at AS CHAR) "
        "FROM review WHERE order_id = " +
            std::to_string(order_id) + " ORDER BY created_at ASC, review_id ASC"
    );

    std::vector<modules::review::ReviewRecord> reviews;
    MYSQL_ROW row = nullptr;
    while ((row = mysql_fetch_row(result.get())) != nullptr) {
        reviews.push_back(BuildReviewRecord(row, 0));
    }
    return reviews;
}

std::uint64_t ReviewRepository::CountReviewsByOrder(const std::uint64_t order_id) const {
    return QueryCount(
        "SELECT COUNT(*) FROM review WHERE order_id = " + std::to_string(order_id)
    );
}

modules::review::ReviewSummary ReviewRepository::GetReviewSummaryByReviewee(
    const std::uint64_t user_id
) const {
    const auto result = ExecuteQuery(
        connection(),
        "SELECT COUNT(*), COALESCE(CAST(AVG(rating) AS CHAR), '0'), "
        "COALESCE(SUM(CASE WHEN rating >= 4 THEN 1 ELSE 0 END), 0), "
        "COALESCE(SUM(CASE WHEN rating = 3 THEN 1 ELSE 0 END), 0), "
        "COALESCE(SUM(CASE WHEN rating <= 2 THEN 1 ELSE 0 END), 0) "
        "FROM review WHERE reviewee_id = " +
            std::to_string(user_id)
    );

    if (MYSQL_ROW row = mysql_fetch_row(result.get()); row != nullptr) {
        return modules::review::ReviewSummary{
            .user_id = user_id,
            .total_reviews = static_cast<int>(ReadRowUint64(row, 0)),
            .average_rating = ReadRowDouble(row, 1),
            .positive_reviews = static_cast<int>(ReadRowUint64(row, 2)),
            .neutral_reviews = static_cast<int>(ReadRowUint64(row, 3)),
            .negative_reviews = static_cast<int>(ReadRowUint64(row, 4)),
        };
    }

    return modules::review::ReviewSummary{.user_id = user_id};
}

void ReviewRepository::UpdateOrderStatus(
    const std::uint64_t order_id,
    const std::string& order_status
) const {
    PreparedStatement statement(
        connection().native_handle(),
        "UPDATE order_info SET order_status = ?, updated_at = CURRENT_TIMESTAMP(3) "
        "WHERE order_id = ?"
    );

    auto mutable_order_id = order_id;
    unsigned long order_status_length = 0;
    bool order_status_is_null = false;
    bool order_id_is_null = false;
    MYSQL_BIND params_bind[2]{};
    BindStringParam(params_bind[0], order_status, order_status_length, order_status_is_null);
    BindUint64Param(params_bind[1], mutable_order_id, order_id_is_null);
    statement.BindParams(params_bind);
    statement.Execute();
}

void ReviewRepository::InsertOperationLog(
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
        "biz_key, result, detail) VALUES (?, ?, 'review', ?, ?, ?, ?)"
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
