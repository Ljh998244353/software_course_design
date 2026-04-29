#include "modules/bid/bid_service.h"

#include <chrono>
#include <cctype>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

#include "common/errors/error_code.h"
#include "common/logging/logger.h"
#include "modules/auth/auth_types.h"
#include "modules/bid/bid_exception.h"
#include "repository/bid_repository.h"

namespace auction::modules::bid {

namespace {

constexpr double kAmountEpsilon = 0.0001;

std::string TrimWhitespace(std::string value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) {
        value.erase(value.begin());
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
        value.pop_back();
    }
    return value;
}

double NormalizeAmount(const double value) {
    return std::round(value * 100.0) / 100.0;
}

bool AmountEquals(const double left, const double right) {
    return std::fabs(NormalizeAmount(left) - NormalizeAmount(right)) < kAmountEpsilon;
}

bool HasAtMostTwoDecimalPlaces(const double value) {
    return std::fabs(value * 100.0 - std::round(value * 100.0)) < kAmountEpsilon;
}

std::string FormatAmount(const double value) {
    std::ostringstream output;
    output << std::fixed << std::setprecision(2) << NormalizeAmount(value);
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

std::string CurrentTimestampText() {
    const auto now = std::chrono::system_clock::now();
    const auto time_value = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm{};
    localtime_r(&time_value, &local_tm);

    std::ostringstream output;
    output << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
    return output.str();
}

void ThrowBidError(const common::errors::ErrorCode code, const std::string_view message) {
    throw BidException(code, std::string(message));
}

void ValidateBidRequest(const PlaceBidRequest& request) {
    const auto request_id = TrimWhitespace(request.request_id);
    if (request_id.empty() || request_id.size() > 64) {
        throw std::invalid_argument("request_id must be between 1 and 64 characters");
    }
    if (!std::isfinite(request.bid_amount) || request.bid_amount <= 0.0) {
        ThrowBidError(common::errors::ErrorCode::kBidAmountInvalid, "bid amount must be positive");
    }
    if (!HasAtMostTwoDecimalPlaces(request.bid_amount)) {
        ThrowBidError(
            common::errors::ErrorCode::kBidAmountInvalid,
            "bid amount supports at most two decimal places"
        );
    }
}

void ValidatePagination(const BidHistoryQuery& query) {
    if (query.page_no <= 0) {
        ThrowBidError(common::errors::ErrorCode::kBidHistoryQueryInvalid, "page_no must be positive");
    }
    if (query.page_size <= 0 || query.page_size > 100) {
        ThrowBidError(
            common::errors::ErrorCode::kBidHistoryQueryInvalid,
            "page_size must be between 1 and 100"
        );
    }
}

double CalculateMinimumNextBid(const AuctionBidSnapshot& auction) {
    if (!auction.highest_bidder_id.has_value()) {
        return NormalizeAmount(auction.current_price);
    }
    return NormalizeAmount(auction.current_price + auction.bid_step);
}

repository::BidRepository MakeRepository(common::db::MysqlConnection& connection) {
    return repository::BidRepository(connection);
}

AuctionBidSnapshot LoadAuctionOrThrow(
    repository::BidRepository& repository,
    const std::uint64_t auction_id,
    const bool for_update
) {
    const auto auction = for_update ? repository.FindAuctionSnapshotByIdForUpdate(auction_id)
                                    : repository.FindAuctionSnapshotById(auction_id);
    if (!auction.has_value()) {
        ThrowBidError(common::errors::ErrorCode::kBidAuctionNotFound, "auction not found");
    }
    return *auction;
}

void ValidateAuctionAcceptsBid(
    repository::BidRepository& repository,
    const AuctionBidSnapshot& auction
) {
    if (auction.status == "PENDING_START") {
        ThrowBidError(
            common::errors::ErrorCode::kBidAuctionNotStarted,
            "auction has not started"
        );
    }

    if (auction.status != "RUNNING") {
        if (auction.status == "SETTLING" || auction.status == "SOLD" ||
            auction.status == "UNSOLD" || auction.status == "CANCELLED") {
            ThrowBidError(
                common::errors::ErrorCode::kBidAuctionClosed,
                "auction no longer accepts bids"
            );
        }
        ThrowBidError(
            common::errors::ErrorCode::kBidAuctionStatusInvalid,
            "auction status does not allow bidding"
        );
    }

    if (repository.IsAuctionExpired(auction.auction_id)) {
        ThrowBidError(
            common::errors::ErrorCode::kBidAuctionClosed,
            "auction has already ended"
        );
    }
}

bool HasConcurrentConflict(
    const std::optional<AuctionBidSnapshot>& precheck_auction,
    const AuctionBidSnapshot& locked_auction
) {
    return precheck_auction.has_value() && precheck_auction->version != locked_auction.version;
}

BidCacheSnapshot BuildCacheSnapshot(
    const AuctionBidSnapshot& auction,
    const std::string& server_time
) {
    return BidCacheSnapshot{
        .auction_id = auction.auction_id,
        .current_price = NormalizeAmount(auction.current_price),
        .minimum_next_bid = CalculateMinimumNextBid(auction),
        .highest_bidder_masked = MaskUsername(auction.highest_bidder_username),
        .end_time = auction.end_time,
        .server_time = server_time,
    };
}

PlaceBidResult BuildPlaceBidResult(
    const AuctionBidSnapshot& auction,
    const BidRecord& bid,
    const bool extended,
    const std::string& server_time
) {
    return PlaceBidResult{
        .bid_id = bid.bid_id,
        .auction_id = auction.auction_id,
        .bid_amount = NormalizeAmount(bid.bid_amount),
        .bid_status = bid.bid_status,
        .current_price = NormalizeAmount(auction.current_price),
        .highest_bidder_masked = MaskUsername(auction.highest_bidder_username),
        .end_time = auction.end_time,
        .extended = extended,
        .server_time = server_time,
    };
}

void RefreshBidCacheSafely(
    BidCacheStore& cache_store,
    const AuctionBidSnapshot& auction,
    const std::string& server_time
) {
    try {
        cache_store.UpsertAuctionSnapshot(BuildCacheSnapshot(auction, server_time));
    } catch (const std::exception& exception) {
        common::logging::Logger::Instance().Warn(
            "bid cache refresh failed: " + std::string(exception.what())
        );
    }
}

}  // namespace

BidService::BidService(
    const common::config::AppConfig& config,
    std::filesystem::path project_root,
    middleware::AuthMiddleware& auth_middleware,
    notification::NotificationService& notification_service,
    BidCacheStore& cache_store
) : mysql_config_(config.mysql),
    project_root_(std::move(project_root)),
    auth_middleware_(&auth_middleware),
    notification_service_(&notification_service),
    cache_store_(&cache_store) {}

PlaceBidResult BidService::PlaceBid(
    const std::string_view authorization_header,
    const std::uint64_t auction_id,
    const PlaceBidRequest& request
) {
    const auto auth_context = auth_middleware_->RequireAnyRole(
        authorization_header,
        {modules::auth::kRoleUser}
    );
    ValidateBidRequest(request);

    auto precheck_connection = CreateConnection();
    auto precheck_repository = MakeRepository(precheck_connection);
    const auto precheck_auction = precheck_repository.FindAuctionSnapshotById(auction_id);

    const auto normalized_request_id = TrimWhitespace(request.request_id);
    const auto normalized_amount = NormalizeAmount(request.bid_amount);

    auto connection = CreateConnection();
    auto repository = MakeRepository(connection);
    bool transaction_started = false;
    try {
        connection.BeginTransaction();
        transaction_started = true;

        const auto locked_auction = LoadAuctionOrThrow(repository, auction_id, true);
        const auto existing_bid =
            repository.FindBidByRequestId(auction_id, auth_context.user_id, normalized_request_id);
        if (existing_bid.has_value()) {
            if (!AmountEquals(existing_bid->bid_amount, normalized_amount)) {
                ThrowBidError(
                    common::errors::ErrorCode::kBidIdempotencyConflict,
                    "request_id already exists with a different bid amount"
                );
            }

            const auto current_auction = LoadAuctionOrThrow(repository, auction_id, false);
            connection.Rollback();
            transaction_started = false;
            return BuildPlaceBidResult(
                current_auction,
                *existing_bid,
                false,
                CurrentTimestampText()
            );
        }

        ValidateAuctionAcceptsBid(repository, locked_auction);

        const auto minimum_next_bid = CalculateMinimumNextBid(locked_auction);
        if (normalized_amount + kAmountEpsilon < minimum_next_bid) {
            if (HasConcurrentConflict(precheck_auction, locked_auction)) {
                ThrowBidError(
                    common::errors::ErrorCode::kBidConflict,
                    "auction state changed before bid commit"
                );
            }
            ThrowBidError(
                common::errors::ErrorCode::kBidAmountTooLow,
                "bid amount does not reach the minimum next bid"
            );
        }

        std::optional<std::uint64_t> outbid_user_id;
        if (locked_auction.highest_bidder_id.has_value() &&
            *locked_auction.highest_bidder_id != auth_context.user_id) {
            outbid_user_id = locked_auction.highest_bidder_id;
        }

        repository.MarkWinningBidOutbid(auction_id);
        const auto created_bid = repository.CreateBidRecord(repository::CreateBidRecordParams{
            .auction_id = auction_id,
            .bidder_id = auth_context.user_id,
            .request_id = normalized_request_id,
            .bid_amount = FormatAmount(normalized_amount),
            .bid_status = std::string(kBidStatusWinning),
        });

        const bool should_extend =
            locked_auction.anti_sniping_window_seconds > 0 && locked_auction.extend_seconds > 0 &&
            repository.IsWithinAntiSnipingWindow(
                auction_id,
                locked_auction.anti_sniping_window_seconds
            );
        if (should_extend) {
            repository.UpdateAuctionPriceAndExtendEndTime(
                auction_id,
                FormatAmount(normalized_amount),
                auth_context.user_id,
                locked_auction.extend_seconds
            );
        } else {
            repository.UpdateAuctionPrice(
                auction_id,
                FormatAmount(normalized_amount),
                auth_context.user_id
            );
        }

        const auto updated_auction = LoadAuctionOrThrow(repository, auction_id, false);
        connection.Commit();
        transaction_started = false;

        const auto server_time = CurrentTimestampText();
        RefreshBidCacheSafely(*cache_store_, updated_auction, server_time);
        try {
            notification_service_->PublishBidEvents(notification::AuctionBidNotificationContext{
                .auction_id = updated_auction.auction_id,
                .current_price = NormalizeAmount(updated_auction.current_price),
                .highest_bidder_masked = MaskUsername(updated_auction.highest_bidder_username),
                .end_time = updated_auction.end_time,
                .server_time = server_time,
                .extended = should_extend,
                .outbid_user_id = outbid_user_id,
            });
        } catch (const std::exception& exception) {
            common::logging::Logger::Instance().Warn(
                "bid notification publish failed after commit: " +
                std::string(exception.what())
            );
        }

        return BuildPlaceBidResult(updated_auction, created_bid, should_extend, server_time);
    } catch (...) {
        if (transaction_started) {
            connection.Rollback();
        }
        throw;
    }
}

BidHistoryResult BidService::ListAuctionBidHistory(
    const std::uint64_t auction_id,
    const BidHistoryQuery& query
) {
    ValidatePagination(query);

    auto connection = CreateConnection();
    auto repository = MakeRepository(connection);
    const auto auction = repository.FindAuctionSnapshotById(auction_id);
    if (!auction.has_value()) {
        ThrowBidError(common::errors::ErrorCode::kBidAuctionNotFound, "auction not found");
    }

    return BidHistoryResult{
        .records = repository.ListAuctionBidHistory(auction_id, query),
        .page_no = query.page_no,
        .page_size = query.page_size,
    };
}

MyBidStatusResult BidService::GetMyBidStatus(
    const std::string_view authorization_header,
    const std::uint64_t auction_id
) {
    const auto auth_context = auth_middleware_->RequireAnyRole(
        authorization_header,
        {modules::auth::kRoleUser}
    );

    auto connection = CreateConnection();
    auto repository = MakeRepository(connection);
    const auto auction = repository.FindAuctionSnapshotById(auction_id);
    if (!auction.has_value()) {
        ThrowBidError(common::errors::ErrorCode::kBidAuctionNotFound, "auction not found");
    }

    const auto latest_bid = repository.FindLatestBidByBidder(auction_id, auth_context.user_id);
    return MyBidStatusResult{
        .auction_id = auction_id,
        .user_id = auth_context.user_id,
        .my_highest_bid = latest_bid.has_value() ? NormalizeAmount(latest_bid->bid_amount) : 0.0,
        .is_current_highest =
            auction->highest_bidder_id.has_value() &&
            *auction->highest_bidder_id == auth_context.user_id,
        .last_bid_time = latest_bid.has_value() ? latest_bid->bid_time : "",
        .current_price = NormalizeAmount(auction->current_price),
        .end_time = auction->end_time,
    };
}

common::db::MysqlConnection BidService::CreateConnection() const {
    return common::db::MysqlConnection(mysql_config_, project_root_);
}

}  // namespace auction::modules::bid
