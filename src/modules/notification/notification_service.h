#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "common/config/app_config.h"
#include "common/db/mysql_connection.h"
#include "ws/auction_event_gateway.h"

namespace auction::modules::notification {

inline constexpr std::string_view kNoticeTypeOutbid = "OUTBID";
inline constexpr std::string_view kReadStatusUnread = "UNREAD";
inline constexpr std::string_view kReadStatusRead = "READ";
inline constexpr std::string_view kPushStatusPending = "PENDING";
inline constexpr std::string_view kPushStatusSent = "SENT";
inline constexpr std::string_view kPushStatusFailed = "FAILED";

inline constexpr std::string_view kTaskTypeNotificationPush = "NOTIFICATION_PUSH";
inline constexpr std::string_view kTaskTypeAuctionEventPush = "AUCTION_EVENT_PUSH";
inline constexpr std::string_view kTaskStatusSuccess = "SUCCESS";
inline constexpr std::string_view kTaskStatusFailed = "FAILED";
inline constexpr std::string_view kTaskStatusSkipped = "SKIPPED";

struct AuctionBidNotificationContext {
    std::uint64_t auction_id{0};
    double current_price{0.0};
    std::string highest_bidder_masked;
    std::string end_time;
    std::string server_time;
    bool extended{false};
    std::optional<std::uint64_t> outbid_user_id;
};

struct StationNoticeRequest {
    std::uint64_t user_id{0};
    std::string notice_type;
    std::string title;
    std::string content;
    std::string biz_type;
    std::optional<std::uint64_t> biz_id;
};

struct NotificationRetryRequest {
    std::optional<std::uint64_t> notification_id;
    int limit{20};
};

struct NotificationRetryResult {
    int scanned{0};
    int succeeded{0};
    int failed{0};
    int skipped{0};
    std::vector<std::uint64_t> affected_notification_ids;
};

struct NotificationQuery {
    int limit{20};
    bool unread_only{false};
    std::string biz_type;
    std::optional<std::uint64_t> biz_id;
};

struct UserNotificationEntry {
    std::uint64_t notification_id{0};
    std::uint64_t user_id{0};
    std::string notice_type;
    std::string title;
    std::string content;
    std::string biz_type;
    std::optional<std::uint64_t> biz_id;
    std::string read_status;
    std::string push_status;
    std::string created_at;
    std::string read_at;
};

struct UserNotificationListResult {
    std::vector<UserNotificationEntry> records;
    int limit{20};
};

struct MarkNotificationReadResult {
    std::uint64_t notification_id{0};
    std::string read_status;
    std::string read_at;
};

class NotificationService {
public:
    NotificationService(
        const common::config::AppConfig& config,
        std::filesystem::path project_root,
        ws::AuctionEventGateway& event_gateway
    );

    void PublishBidEvents(const AuctionBidNotificationContext& context);
    void CreateStationNotice(const StationNoticeRequest& request);
    [[nodiscard]] UserNotificationListResult ListUserNotifications(
        std::uint64_t user_id,
        const NotificationQuery& query
    );
    [[nodiscard]] MarkNotificationReadResult MarkUserNotificationRead(
        std::uint64_t user_id,
        std::uint64_t notification_id
    );
    [[nodiscard]] NotificationRetryResult RetryFailedNotifications(
        const NotificationRetryRequest& request
    );

private:
    [[nodiscard]] common::db::MysqlConnection CreateConnection() const;

    common::config::MysqlConfig mysql_config_;
    std::filesystem::path project_root_;
    ws::AuctionEventGateway* event_gateway_;
};

}  // namespace auction::modules::notification
