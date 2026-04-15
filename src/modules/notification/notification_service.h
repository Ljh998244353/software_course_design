#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include "common/config/app_config.h"
#include "common/db/mysql_connection.h"
#include "ws/auction_event_gateway.h"

namespace auction::modules::notification {

inline constexpr std::string_view kNoticeTypeOutbid = "OUTBID";
inline constexpr std::string_view kReadStatusUnread = "UNREAD";
inline constexpr std::string_view kPushStatusPending = "PENDING";
inline constexpr std::string_view kPushStatusSent = "SENT";
inline constexpr std::string_view kPushStatusFailed = "FAILED";

inline constexpr std::string_view kTaskTypeNotificationPush = "NOTIFICATION_PUSH";
inline constexpr std::string_view kTaskTypeAuctionEventPush = "AUCTION_EVENT_PUSH";
inline constexpr std::string_view kTaskStatusSuccess = "SUCCESS";
inline constexpr std::string_view kTaskStatusFailed = "FAILED";

struct AuctionBidNotificationContext {
    std::uint64_t auction_id{0};
    double current_price{0.0};
    std::string highest_bidder_masked;
    std::string end_time;
    std::string server_time;
    bool extended{false};
    std::optional<std::uint64_t> outbid_user_id;
};

class NotificationService {
public:
    NotificationService(
        const common::config::AppConfig& config,
        std::filesystem::path project_root,
        ws::AuctionEventGateway& event_gateway
    );

    void PublishBidEvents(const AuctionBidNotificationContext& context);

private:
    [[nodiscard]] common::db::MysqlConnection CreateConnection() const;

    common::config::MysqlConfig mysql_config_;
    std::filesystem::path project_root_;
    ws::AuctionEventGateway* event_gateway_;
};

}  // namespace auction::modules::notification
