#include "modules/notification/notification_service.h"

#include <chrono>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "common/logging/logger.h"
#include "repository/notification_repository.h"

namespace auction::modules::notification {

namespace {

std::string FormatAmount(const double value) {
    std::ostringstream output;
    output << std::fixed << std::setprecision(2) << (std::round(value * 100.0) / 100.0);
    return output.str();
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

std::string MaskUsername(const std::string& username) {
    if (username.empty()) {
        return {};
    }
    if (username.size() <= 2) {
        return username.substr(0, 1) + "***";
    }
    return username.substr(0, 1) + "***" + username.substr(username.size() - 1);
}

ws::AuctionEventMessage BuildAuctionEventMessage(
    const std::string& event_name,
    const AuctionBidNotificationContext& context
) {
    return ws::AuctionEventMessage{
        .event = event_name,
        .auction_id = context.auction_id,
        .current_price = context.current_price,
        .highest_bidder_masked = context.highest_bidder_masked,
        .end_time = context.end_time,
        .server_time = context.server_time,
    };
}

void InsertTaskLogSafely(
    repository::NotificationRepository& repository,
    const repository::NotificationTaskLogParams& params
) {
    try {
        repository.InsertTaskLog(params);
    } catch (...) {
    }
}

std::string BuildAuctionEventBizKey(
    const std::uint64_t auction_id,
    const std::string& event_name
) {
    return "auction:" + std::to_string(auction_id) + ":" + event_name;
}

UserNotificationEntry ToUserNotificationEntry(
    const repository::NotificationRecord& record
) {
    return UserNotificationEntry{
        .notification_id = record.notification_id,
        .user_id = record.user_id,
        .notice_type = record.notice_type,
        .title = record.title,
        .content = record.content,
        .biz_type = record.biz_type,
        .biz_id = record.biz_id,
        .read_status = record.read_status,
        .push_status = record.push_status,
        .created_at = record.created_at,
        .read_at = record.read_at,
    };
}

void ValidateNotificationQuery(const NotificationQuery& query) {
    if (query.limit <= 0 || query.limit > 100) {
        throw std::invalid_argument("notification query limit must be between 1 and 100");
    }
    if (query.biz_type.size() > 32) {
        throw std::invalid_argument("biz_type must be at most 32 characters");
    }
}

}  // namespace

NotificationService::NotificationService(
    const common::config::AppConfig& config,
    std::filesystem::path project_root,
    ws::AuctionEventGateway& event_gateway
) : mysql_config_(config.mysql),
    project_root_(std::move(project_root)),
    event_gateway_(&event_gateway) {}

void NotificationService::PublishBidEvents(const AuctionBidNotificationContext& context) {
    auto connection = CreateConnection();
    repository::NotificationRepository repository(connection);

    const auto price_updated = BuildAuctionEventMessage("PRICE_UPDATED", context);
    try {
        event_gateway_->BroadcastToAuction(context.auction_id, price_updated);
    } catch (const std::exception& exception) {
        common::logging::Logger::Instance().Warn(
            "bid price update broadcast failed: " + std::string(exception.what())
        );
        InsertTaskLogSafely(
            repository,
            repository::NotificationTaskLogParams{
                .task_type = std::string(kTaskTypeAuctionEventPush),
                .biz_key = BuildAuctionEventBizKey(context.auction_id, "PRICE_UPDATED"),
                .task_status = std::string(kTaskStatusFailed),
                .retry_count = 0,
                .last_error = exception.what(),
                .scheduled_at = context.server_time,
            }
        );
    }

    if (context.extended) {
        const auto extended = BuildAuctionEventMessage("AUCTION_EXTENDED", context);
        try {
            event_gateway_->BroadcastToAuction(context.auction_id, extended);
        } catch (const std::exception& exception) {
            common::logging::Logger::Instance().Warn(
                "auction extension broadcast failed: " + std::string(exception.what())
            );
            InsertTaskLogSafely(
                repository,
                repository::NotificationTaskLogParams{
                    .task_type = std::string(kTaskTypeAuctionEventPush),
                    .biz_key = BuildAuctionEventBizKey(context.auction_id, "AUCTION_EXTENDED"),
                    .task_status = std::string(kTaskStatusFailed),
                    .retry_count = 0,
                    .last_error = exception.what(),
                    .scheduled_at = context.server_time,
                }
            );
        }
    }

    if (!context.outbid_user_id.has_value()) {
        return;
    }

    try {
        const auto notification = repository.CreateNotification(repository::CreateNotificationParams{
            .user_id = *context.outbid_user_id,
            .notice_type = std::string(kNoticeTypeOutbid),
            .title = "You have been outbid",
            .content = "Auction " + std::to_string(context.auction_id) +
                       " now has a higher bid of " + FormatAmount(context.current_price) + ".",
            .biz_type = "AUCTION",
            .biz_id = context.auction_id,
            .read_status = std::string(kReadStatusUnread),
            .push_status = std::string(kPushStatusPending),
        });

        try {
            event_gateway_->PushToUser(
                *context.outbid_user_id,
                BuildAuctionEventMessage("OUTBID", context)
            );
            repository.UpdatePushStatus(notification.notification_id, std::string(kPushStatusSent));
        } catch (const std::exception& exception) {
            common::logging::Logger::Instance().Warn(
                "outbid direct push failed: " + std::string(exception.what())
            );
            repository.UpdatePushStatus(notification.notification_id, std::string(kPushStatusFailed));
            InsertTaskLogSafely(
                repository,
                repository::NotificationTaskLogParams{
                    .task_type = std::string(kTaskTypeNotificationPush),
                    .biz_key = std::to_string(notification.notification_id),
                    .task_status = std::string(kTaskStatusFailed),
                    .retry_count = 0,
                    .last_error = exception.what(),
                    .scheduled_at = context.server_time,
                }
            );
        }
    } catch (const std::exception& exception) {
        common::logging::Logger::Instance().Warn(
            "outbid notification persistence failed: " + std::string(exception.what())
        );
    }
}

void NotificationService::CreateStationNotice(const StationNoticeRequest& request) {
    auto connection = CreateConnection();
    repository::NotificationRepository repository(connection);
    const auto notification = repository.CreateNotification(repository::CreateNotificationParams{
        .user_id = request.user_id,
        .notice_type = request.notice_type,
        .title = request.title,
        .content = request.content,
        .biz_type = request.biz_type,
        .biz_id = request.biz_id,
        .read_status = std::string(kReadStatusUnread),
        .push_status = std::string(kPushStatusPending),
    });
    (void)notification;
}

UserNotificationListResult NotificationService::ListUserNotifications(
    const std::uint64_t user_id,
    const NotificationQuery& query
) {
    if (user_id == 0) {
        throw std::invalid_argument("user_id must be positive");
    }
    ValidateNotificationQuery(query);

    auto connection = CreateConnection();
    repository::NotificationRepository repository(connection);
    const auto records = repository.ListUserNotifications(
        user_id,
        query.limit,
        query.unread_only,
        query.biz_type,
        query.biz_id
    );

    UserNotificationListResult result;
    result.limit = query.limit;
    result.records.reserve(records.size());
    for (const auto& record : records) {
        result.records.push_back(ToUserNotificationEntry(record));
    }
    return result;
}

MarkNotificationReadResult NotificationService::MarkUserNotificationRead(
    const std::uint64_t user_id,
    const std::uint64_t notification_id
) {
    if (user_id == 0 || notification_id == 0) {
        throw std::invalid_argument("notification id and user id must be positive");
    }

    auto connection = CreateConnection();
    repository::NotificationRepository repository(connection);
    repository.MarkNotificationRead(notification_id, user_id);
    const auto updated = repository.FindNotificationByIdAndUser(notification_id, user_id);
    if (!updated.has_value()) {
        throw std::invalid_argument("notification not found for current user");
    }

    return MarkNotificationReadResult{
        .notification_id = updated->notification_id,
        .read_status = updated->read_status,
        .read_at = updated->read_at,
    };
}

NotificationRetryResult NotificationService::RetryFailedNotifications(
    const NotificationRetryRequest& request
) {
    if (!request.notification_id.has_value() && (request.limit <= 0 || request.limit > 100)) {
        throw std::invalid_argument("notification retry limit must be between 1 and 100");
    }

    auto connection = CreateConnection();
    repository::NotificationRepository repository(connection);

    std::vector<std::uint64_t> notification_ids;
    if (request.notification_id.has_value()) {
        notification_ids.push_back(*request.notification_id);
    } else {
        notification_ids = repository.ListFailedNotificationIds(request.limit);
    }

    NotificationRetryResult result;
    result.scanned = static_cast<int>(notification_ids.size());

    for (const auto notification_id : notification_ids) {
        const auto retry_candidate = repository.FindAuctionRetryCandidate(notification_id);
        const auto retry_count = repository.QueryMaxTaskRetryCount(
                                     std::string(kTaskTypeNotificationPush),
                                     std::to_string(notification_id)
                                 ) +
                                 1;
        if (!retry_candidate.has_value()) {
            ++result.skipped;
            InsertTaskLogSafely(
                repository,
                repository::NotificationTaskLogParams{
                    .task_type = std::string(kTaskTypeNotificationPush),
                    .biz_key = std::to_string(notification_id),
                    .task_status = std::string(kTaskStatusSkipped),
                    .retry_count = retry_count,
                    .last_error = "notification not found for retry",
                    .scheduled_at = CurrentTimestampText(),
                }
            );
            continue;
        }

        const auto& candidate = *retry_candidate;
        if (candidate.notification.push_status != kPushStatusFailed ||
            candidate.notification.notice_type != kNoticeTypeOutbid ||
            candidate.notification.biz_type != "AUCTION" ||
            !candidate.notification.biz_id.has_value() || candidate.auction_id == 0 ||
            candidate.end_time.empty()) {
            ++result.skipped;
            InsertTaskLogSafely(
                repository,
                repository::NotificationTaskLogParams{
                    .task_type = std::string(kTaskTypeNotificationPush),
                    .biz_key = std::to_string(notification_id),
                    .task_status = std::string(kTaskStatusSkipped),
                    .retry_count = retry_count,
                    .last_error = "notification is not eligible for direct retry",
                    .scheduled_at = candidate.notification.created_at,
                }
            );
            continue;
        }

        try {
            const auto server_time = CurrentTimestampText();
            event_gateway_->PushToUser(
                candidate.notification.user_id,
                ws::AuctionEventMessage{
                    .event = "OUTBID",
                    .auction_id = candidate.auction_id,
                    .current_price = candidate.current_price,
                    .highest_bidder_masked = MaskUsername(candidate.highest_bidder_username),
                    .end_time = candidate.end_time,
                    .server_time = server_time,
                }
            );
            repository.UpdatePushStatus(notification_id, std::string(kPushStatusSent));
            InsertTaskLogSafely(
                repository,
                repository::NotificationTaskLogParams{
                    .task_type = std::string(kTaskTypeNotificationPush),
                    .biz_key = std::to_string(notification_id),
                    .task_status = std::string(kTaskStatusSuccess),
                    .retry_count = retry_count,
                    .last_error = "",
                    .scheduled_at = candidate.notification.created_at,
                }
            );
            ++result.succeeded;
            result.affected_notification_ids.push_back(notification_id);
        } catch (const std::exception& exception) {
            common::logging::Logger::Instance().Warn(
                "notification retry failed: " + std::string(exception.what())
            );
            repository.UpdatePushStatus(notification_id, std::string(kPushStatusFailed));
            InsertTaskLogSafely(
                repository,
                repository::NotificationTaskLogParams{
                    .task_type = std::string(kTaskTypeNotificationPush),
                    .biz_key = std::to_string(notification_id),
                    .task_status = std::string(kTaskStatusFailed),
                    .retry_count = retry_count,
                    .last_error = exception.what(),
                    .scheduled_at = candidate.notification.created_at,
                }
            );
            ++result.failed;
        }
    }

    return result;
}

common::db::MysqlConnection NotificationService::CreateConnection() const {
    return common::db::MysqlConnection(mysql_config_, project_root_);
}

}  // namespace auction::modules::notification
