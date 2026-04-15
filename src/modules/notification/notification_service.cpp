#include "modules/notification/notification_service.h"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

#include "common/logging/logger.h"
#include "repository/notification_repository.h"

namespace auction::modules::notification {

namespace {

std::string FormatAmount(const double value) {
    std::ostringstream output;
    output << std::fixed << std::setprecision(2) << (std::round(value * 100.0) / 100.0);
    return output.str();
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

common::db::MysqlConnection NotificationService::CreateConnection() const {
    return common::db::MysqlConnection(mysql_config_, project_root_);
}

}  // namespace auction::modules::notification
