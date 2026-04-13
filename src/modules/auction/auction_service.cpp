#include "modules/auction/auction_service.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

#include "common/db/mysql_connection.h"
#include "common/errors/error_code.h"
#include "modules/auth/auth_types.h"
#include "modules/auction/auction_exception.h"
#include "modules/item/item_types.h"
#include "repository/auction_repository.h"

namespace auction::modules::auction {

namespace {

struct NormalizedAuctionConfig {
    std::string start_time;
    std::string end_time;
    double start_price{0.0};
    double bid_step{0.0};
    int anti_sniping_window_seconds{0};
    int extend_seconds{0};
};

std::string TrimWhitespace(std::string value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) {
        value.erase(value.begin());
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
        value.pop_back();
    }
    return value;
}

std::string NormalizeDateTimeText(std::string value) {
    value = TrimWhitespace(std::move(value));
    std::replace(value.begin(), value.end(), 'T', ' ');
    if (!value.empty() && value.back() == 'Z') {
        value.pop_back();
    }
    const auto dot_position = value.find('.');
    if (dot_position != std::string::npos) {
        value.erase(dot_position);
    }
    return value;
}

std::time_t ParseDateTime(const std::string& value) {
    const auto normalized = NormalizeDateTimeText(value);
    if (normalized.size() != 19) {
        throw std::invalid_argument("datetime format must be YYYY-MM-DD HH:MM:SS");
    }

    std::tm parsed{};
    std::istringstream input(normalized);
    input >> std::get_time(&parsed, "%Y-%m-%d %H:%M:%S");
    if (input.fail()) {
        throw std::invalid_argument("datetime format must be YYYY-MM-DD HH:MM:SS");
    }
    parsed.tm_isdst = -1;

    const auto epoch = std::mktime(&parsed);
    if (epoch == static_cast<std::time_t>(-1)) {
        throw std::invalid_argument("datetime value is invalid");
    }
    return epoch;
}

std::string FormatAmount(const double value) {
    std::ostringstream output;
    output << std::fixed << std::setprecision(2) << (std::round(value * 100.0) / 100.0);
    return output.str();
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

void ThrowAuctionError(const common::errors::ErrorCode code, const std::string_view message) {
    throw AuctionException(code, std::string(message));
}

void ValidatePagination(const int page_no, const int page_size) {
    if (page_no <= 0) {
        throw std::invalid_argument("page_no must be positive");
    }
    if (page_size <= 0 || page_size > 100) {
        throw std::invalid_argument("page_size must be between 1 and 100");
    }
}

NormalizedAuctionConfig NormalizeAndValidateConfig(
    std::string start_time,
    std::string end_time,
    const double start_price,
    const double bid_step,
    const int anti_sniping_window_seconds,
    const int extend_seconds
) {
    start_time = NormalizeDateTimeText(std::move(start_time));
    end_time = NormalizeDateTimeText(std::move(end_time));

    const auto now = std::time(nullptr);
    const auto start_epoch = ParseDateTime(start_time);
    const auto end_epoch = ParseDateTime(end_time);
    if (start_epoch <= now) {
        ThrowAuctionError(common::errors::ErrorCode::kAuctionStartTimeInvalid, "start_time must be in the future");
    }
    if (end_epoch <= start_epoch) {
        ThrowAuctionError(common::errors::ErrorCode::kAuctionEndTimeInvalid, "end_time must be later than start_time");
    }
    if (start_price <= 0.0) {
        ThrowAuctionError(common::errors::ErrorCode::kAuctionStartPriceInvalid, "start_price must be positive");
    }
    if (bid_step <= 0.0) {
        ThrowAuctionError(common::errors::ErrorCode::kAuctionBidStepInvalid, "bid_step must be positive");
    }
    if (anti_sniping_window_seconds < 0 || extend_seconds < 0) {
        ThrowAuctionError(
            common::errors::ErrorCode::kAuctionAntiSnipingInvalid,
            "anti-sniping config must not be negative"
        );
    }
    const bool anti_sniping_disabled =
        anti_sniping_window_seconds == 0 && extend_seconds == 0;
    const bool anti_sniping_enabled =
        anti_sniping_window_seconds > 0 && extend_seconds > 0;
    if (!anti_sniping_disabled && !anti_sniping_enabled) {
        ThrowAuctionError(
            common::errors::ErrorCode::kAuctionAntiSnipingInvalid,
            "anti-sniping config must be both zero or both positive"
        );
    }

    return NormalizedAuctionConfig{
        .start_time = std::move(start_time),
        .end_time = std::move(end_time),
        .start_price = start_price,
        .bid_step = bid_step,
        .anti_sniping_window_seconds = anti_sniping_window_seconds,
        .extend_seconds = extend_seconds,
    };
}

repository::AuctionRepository MakeRepository(common::db::MysqlConnection& connection) {
    return repository::AuctionRepository(connection);
}

AuctionRecord LoadAuctionOrThrow(
    repository::AuctionRepository& repository,
    const std::uint64_t auction_id,
    const bool for_update
) {
    const auto auction = for_update ? repository.FindAuctionByIdForUpdate(auction_id)
                                    : repository.FindAuctionById(auction_id);
    if (!auction.has_value()) {
        ThrowAuctionError(common::errors::ErrorCode::kAuctionNotFound, "auction not found");
    }
    return *auction;
}

AuctionItemSnapshot LoadItemOrThrow(
    repository::AuctionRepository& repository,
    const std::uint64_t item_id,
    const bool for_update
) {
    const auto item = for_update ? repository.FindItemSnapshotByIdForUpdate(item_id)
                                 : repository.FindItemSnapshotById(item_id);
    if (!item.has_value()) {
        ThrowAuctionError(common::errors::ErrorCode::kAuctionItemNotFound, "auction item not found");
    }
    return *item;
}

void RequirePendingStartForUpdate(const AuctionRecord& auction) {
    if (auction.status != kAuctionStatusPendingStart) {
        ThrowAuctionError(
            common::errors::ErrorCode::kAuctionUpdateStatusInvalid,
            "auction status does not allow update"
        );
    }
}

void RequirePendingStartForCancel(const AuctionRecord& auction) {
    if (auction.status != kAuctionStatusPendingStart) {
        ThrowAuctionError(
            common::errors::ErrorCode::kAuctionCancelStatusInvalid,
            "auction status does not allow cancel"
        );
    }
}

void ValidateStatusFilter(const std::optional<std::string>& status) {
    if (status.has_value() && !IsKnownAuctionStatus(*status)) {
        throw std::invalid_argument("auction status filter is invalid");
    }
}

std::string DescribeException(const std::exception& exception) {
    return TrimWhitespace(exception.what());
}

void InsertTaskLogSafely(
    repository::AuctionRepository& repository,
    const repository::CreateTaskLogParams& params
) {
    try {
        repository.InsertTaskLog(params);
    } catch (...) {
    }
}

}  // namespace

AuctionService::AuctionService(
    const common::config::AppConfig& config,
    std::filesystem::path project_root,
    middleware::AuthMiddleware& auth_middleware
) : mysql_config_(config.mysql),
    project_root_(std::move(project_root)),
    auth_middleware_(&auth_middleware) {}

CreateAuctionResult AuctionService::CreateAuction(
    const std::string_view authorization_header,
    const CreateAuctionRequest& request
) {
    const auto ignored = auth_middleware_->RequireAnyRole(
        authorization_header,
        {modules::auth::kRoleAdmin}
    );
    (void)ignored;

    const auto normalized = NormalizeAndValidateConfig(
        request.start_time,
        request.end_time,
        request.start_price,
        request.bid_step,
        request.anti_sniping_window_seconds,
        request.extend_seconds
    );

    auto connection = CreateConnection();
    auto repository = MakeRepository(connection);

    connection.BeginTransaction();
    try {
        const auto item = LoadItemOrThrow(repository, request.item_id, true);
        if (item.item_status != modules::item::kItemStatusReadyForAuction) {
            ThrowAuctionError(
                common::errors::ErrorCode::kAuctionItemStatusInvalid,
                "item status does not allow auction creation"
            );
        }
        if (repository.HasNonTerminalAuction(request.item_id)) {
            ThrowAuctionError(
                common::errors::ErrorCode::kAuctionDuplicateActiveByItem,
                "item already has an active auction"
            );
        }

        const auto created = repository.CreateAuction(repository::CreateAuctionParams{
            .item_id = request.item_id,
            .seller_id = item.seller_id,
            .start_time = normalized.start_time,
            .end_time = normalized.end_time,
            .start_price = FormatAmount(normalized.start_price),
            .current_price = FormatAmount(normalized.start_price),
            .bid_step = FormatAmount(normalized.bid_step),
            .anti_sniping_window_seconds = normalized.anti_sniping_window_seconds,
            .extend_seconds = normalized.extend_seconds,
        });
        connection.Commit();

        return CreateAuctionResult{
            .auction_id = created.auction_id,
            .item_id = created.item_id,
            .status = created.status,
            .current_price = created.current_price,
            .created_at = created.created_at,
        };
    } catch (...) {
        connection.Rollback();
        throw;
    }
}

UpdateAuctionResult AuctionService::UpdatePendingAuction(
    const std::string_view authorization_header,
    const std::uint64_t auction_id,
    const UpdateAuctionRequest& request
) {
    const auto ignored = auth_middleware_->RequireAnyRole(
        authorization_header,
        {modules::auth::kRoleAdmin}
    );
    (void)ignored;

    const bool has_changes = request.start_time.has_value() || request.end_time.has_value() ||
                             request.start_price.has_value() || request.bid_step.has_value() ||
                             request.anti_sniping_window_seconds.has_value() ||
                             request.extend_seconds.has_value();
    if (!has_changes) {
        throw std::invalid_argument("at least one field must be provided for update");
    }

    auto connection = CreateConnection();
    auto repository = MakeRepository(connection);

    connection.BeginTransaction();
    try {
        const auto auction = LoadAuctionOrThrow(repository, auction_id, true);
        RequirePendingStartForUpdate(auction);

        const auto normalized = NormalizeAndValidateConfig(
            request.start_time.value_or(auction.start_time),
            request.end_time.value_or(auction.end_time),
            request.start_price.value_or(auction.start_price),
            request.bid_step.value_or(auction.bid_step),
            request.anti_sniping_window_seconds.value_or(auction.anti_sniping_window_seconds),
            request.extend_seconds.value_or(auction.extend_seconds)
        );

        const auto updated = repository.UpdatePendingAuction(repository::UpdateAuctionParams{
            .auction_id = auction_id,
            .start_time = normalized.start_time,
            .end_time = normalized.end_time,
            .start_price = FormatAmount(normalized.start_price),
            .bid_step = FormatAmount(normalized.bid_step),
            .anti_sniping_window_seconds = normalized.anti_sniping_window_seconds,
            .extend_seconds = normalized.extend_seconds,
        });
        connection.Commit();

        return UpdateAuctionResult{
            .auction_id = updated.auction_id,
            .status = updated.status,
            .updated_at = updated.updated_at,
        };
    } catch (...) {
        connection.Rollback();
        throw;
    }
}

CancelAuctionResult AuctionService::CancelPendingAuction(
    const std::string_view authorization_header,
    const std::uint64_t auction_id,
    const CancelAuctionRequest& request
) {
    const auto ignored = auth_middleware_->RequireAnyRole(
        authorization_header,
        {modules::auth::kRoleAdmin}
    );
    (void)ignored;
    const auto ignored_reason = TrimWhitespace(request.reason);
    (void)ignored_reason;

    auto connection = CreateConnection();
    auto repository = MakeRepository(connection);

    connection.BeginTransaction();
    try {
        const auto auction = LoadAuctionOrThrow(repository, auction_id, true);
        RequirePendingStartForCancel(auction);

        repository.UpdateAuctionStatus(auction_id, std::string(kAuctionStatusCancelled));
        connection.Commit();

        const auto updated = LoadAuctionOrThrow(repository, auction_id, false);
        return CancelAuctionResult{
            .auction_id = updated.auction_id,
            .old_status = auction.status,
            .new_status = updated.status,
            .cancelled_at = updated.updated_at,
        };
    } catch (...) {
        connection.Rollback();
        throw;
    }
}

std::vector<AdminAuctionSummary> AuctionService::ListAdminAuctions(
    const std::string_view authorization_header,
    const AdminAuctionQuery& query
) {
    const auto ignored = auth_middleware_->RequireAnyRole(
        authorization_header,
        {modules::auth::kRoleAdmin, modules::auth::kRoleSupport}
    );
    (void)ignored;

    ValidateStatusFilter(query.status);
    ValidatePagination(query.page_no, query.page_size);

    auto connection = CreateConnection();
    auto repository = MakeRepository(connection);
    return repository.ListAdminAuctions(query);
}

AdminAuctionDetail AuctionService::GetAdminAuctionDetail(
    const std::string_view authorization_header,
    const std::uint64_t auction_id
) {
    const auto ignored = auth_middleware_->RequireAnyRole(
        authorization_header,
        {modules::auth::kRoleAdmin, modules::auth::kRoleSupport}
    );
    (void)ignored;

    auto connection = CreateConnection();
    auto repository = MakeRepository(connection);
    const auto detail = repository.FindAdminAuctionDetail(auction_id);
    if (!detail.has_value()) {
        ThrowAuctionError(common::errors::ErrorCode::kAuctionNotFound, "auction not found");
    }
    return *detail;
}

std::vector<PublicAuctionSummary> AuctionService::ListPublicAuctions(
    const PublicAuctionQuery& query
) {
    ValidateStatusFilter(query.status);
    ValidatePagination(query.page_no, query.page_size);

    auto connection = CreateConnection();
    auto repository = MakeRepository(connection);
    return repository.ListPublicAuctions(query);
}

PublicAuctionDetail AuctionService::GetPublicAuctionDetail(const std::uint64_t auction_id) {
    auto connection = CreateConnection();
    auto repository = MakeRepository(connection);
    const auto detail = repository.FindPublicAuctionDetail(auction_id);
    if (!detail.has_value()) {
        ThrowAuctionError(common::errors::ErrorCode::kAuctionNotFound, "auction not found");
    }

    auto value = *detail;
    value.highest_bidder_masked = MaskUsername(value.highest_bidder_masked);
    return value;
}

AuctionScheduleResult AuctionService::StartDueAuctions() {
    auto connection = CreateConnection();
    auto repository = MakeRepository(connection);
    const auto due_auction_ids = repository.ListDueStartAuctionIds();

    AuctionScheduleResult result;
    result.scanned = static_cast<int>(due_auction_ids.size());

    for (const auto auction_id : due_auction_ids) {
        bool transaction_started = false;
        std::string scheduled_at;
        try {
            connection.BeginTransaction();
            transaction_started = true;

            const auto auction = LoadAuctionOrThrow(repository, auction_id, true);
            scheduled_at = auction.start_time;
            if (auction.status != kAuctionStatusPendingStart) {
                connection.Rollback();
                transaction_started = false;
                ++result.skipped;
                InsertTaskLogSafely(
                    repository,
                    repository::CreateTaskLogParams{
                        .task_type = std::string(kTaskTypeAuctionStart),
                        .biz_key = std::to_string(auction_id),
                        .task_status = std::string(kTaskStatusSkipped),
                        .retry_count = 0,
                        .last_error = "auction status changed before scheduler",
                        .scheduled_at = scheduled_at,
                    }
                );
                continue;
            }
            if (!repository.IsStartDue(auction_id)) {
                connection.Rollback();
                transaction_started = false;
                ++result.skipped;
                InsertTaskLogSafely(
                    repository,
                    repository::CreateTaskLogParams{
                        .task_type = std::string(kTaskTypeAuctionStart),
                        .biz_key = std::to_string(auction_id),
                        .task_status = std::string(kTaskStatusSkipped),
                        .retry_count = 0,
                        .last_error = "auction start_time is no longer due",
                        .scheduled_at = scheduled_at,
                    }
                );
                continue;
            }

            const auto item = LoadItemOrThrow(repository, auction.item_id, true);
            if (item.item_status != modules::item::kItemStatusReadyForAuction) {
                ThrowAuctionError(
                    common::errors::ErrorCode::kAuctionRelatedItemStatusInvalid,
                    "item status does not allow auction start"
                );
            }

            repository.UpdateAuctionStatus(auction_id, std::string(kAuctionStatusRunning));
            repository.UpdateItemStatus(item.item_id, std::string(modules::item::kItemStatusInAuction));
            connection.Commit();
            transaction_started = false;

            ++result.succeeded;
            result.affected_auction_ids.push_back(auction_id);
            InsertTaskLogSafely(
                repository,
                repository::CreateTaskLogParams{
                    .task_type = std::string(kTaskTypeAuctionStart),
                    .biz_key = std::to_string(auction_id),
                    .task_status = std::string(kTaskStatusSuccess),
                    .retry_count = 0,
                    .last_error = "",
                    .scheduled_at = scheduled_at,
                }
            );
        } catch (const std::exception& exception) {
            if (transaction_started) {
                connection.Rollback();
            }
            ++result.failed;
            InsertTaskLogSafely(
                repository,
                repository::CreateTaskLogParams{
                    .task_type = std::string(kTaskTypeAuctionStart),
                    .biz_key = std::to_string(auction_id),
                    .task_status = std::string(kTaskStatusFailed),
                    .retry_count = 0,
                    .last_error = DescribeException(exception),
                    .scheduled_at = scheduled_at,
                }
            );
        }
    }

    return result;
}

AuctionScheduleResult AuctionService::FinishDueAuctions() {
    auto connection = CreateConnection();
    auto repository = MakeRepository(connection);
    const auto due_auction_ids = repository.ListDueFinishAuctionIds();

    AuctionScheduleResult result;
    result.scanned = static_cast<int>(due_auction_ids.size());

    for (const auto auction_id : due_auction_ids) {
        bool transaction_started = false;
        std::string scheduled_at;
        try {
            connection.BeginTransaction();
            transaction_started = true;

            const auto auction = LoadAuctionOrThrow(repository, auction_id, true);
            scheduled_at = auction.end_time;
            if (auction.status != kAuctionStatusRunning) {
                connection.Rollback();
                transaction_started = false;
                ++result.skipped;
                InsertTaskLogSafely(
                    repository,
                    repository::CreateTaskLogParams{
                        .task_type = std::string(kTaskTypeAuctionFinish),
                        .biz_key = std::to_string(auction_id),
                        .task_status = std::string(kTaskStatusSkipped),
                        .retry_count = 0,
                        .last_error = "auction status changed before finish",
                        .scheduled_at = scheduled_at,
                    }
                );
                continue;
            }
            if (!repository.IsFinishDue(auction_id)) {
                connection.Rollback();
                transaction_started = false;
                ++result.skipped;
                InsertTaskLogSafely(
                    repository,
                    repository::CreateTaskLogParams{
                        .task_type = std::string(kTaskTypeAuctionFinish),
                        .biz_key = std::to_string(auction_id),
                        .task_status = std::string(kTaskStatusSkipped),
                        .retry_count = 0,
                        .last_error = "auction end_time is no longer due",
                        .scheduled_at = scheduled_at,
                    }
                );
                continue;
            }

            const auto item = LoadItemOrThrow(repository, auction.item_id, true);
            if (item.item_status != modules::item::kItemStatusInAuction) {
                ThrowAuctionError(
                    common::errors::ErrorCode::kAuctionRelatedItemStatusInvalid,
                    "item status does not allow auction finish"
                );
            }

            if (auction.highest_bidder_id.has_value()) {
                repository.UpdateAuctionStatus(auction_id, std::string(kAuctionStatusSettling));
            } else {
                repository.UpdateAuctionStatus(auction_id, std::string(kAuctionStatusUnsold));
                repository.UpdateItemStatus(item.item_id, std::string(modules::item::kItemStatusUnsold));
            }
            connection.Commit();
            transaction_started = false;

            ++result.succeeded;
            result.affected_auction_ids.push_back(auction_id);
            InsertTaskLogSafely(
                repository,
                repository::CreateTaskLogParams{
                    .task_type = std::string(kTaskTypeAuctionFinish),
                    .biz_key = std::to_string(auction_id),
                    .task_status = std::string(kTaskStatusSuccess),
                    .retry_count = 0,
                    .last_error = "",
                    .scheduled_at = scheduled_at,
                }
            );
        } catch (const std::exception& exception) {
            if (transaction_started) {
                connection.Rollback();
            }
            ++result.failed;
            InsertTaskLogSafely(
                repository,
                repository::CreateTaskLogParams{
                    .task_type = std::string(kTaskTypeAuctionFinish),
                    .biz_key = std::to_string(auction_id),
                    .task_status = std::string(kTaskStatusFailed),
                    .retry_count = 0,
                    .last_error = DescribeException(exception),
                    .scheduled_at = scheduled_at,
                }
            );
        }
    }

    return result;
}

common::db::MysqlConnection AuctionService::CreateConnection() const {
    return common::db::MysqlConnection(mysql_config_, project_root_);
}

}  // namespace auction::modules::auction
