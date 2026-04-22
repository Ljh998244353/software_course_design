#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "repository/base_repository.h"

namespace auction::repository {

struct CreateNotificationParams {
    std::uint64_t user_id{0};
    std::string notice_type;
    std::string title;
    std::string content;
    std::string biz_type;
    std::optional<std::uint64_t> biz_id;
    std::string read_status;
    std::string push_status;
};

struct NotificationRecord {
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
};

struct NotificationTaskLogParams {
    std::string task_type;
    std::string biz_key;
    std::string task_status;
    int retry_count{0};
    std::string last_error;
    std::string scheduled_at;
};

struct NotificationRetryCandidate {
    NotificationRecord notification;
    std::uint64_t auction_id{0};
    double current_price{0.0};
    std::string highest_bidder_username;
    std::string end_time;
};

class NotificationRepository : public BaseRepository {
public:
    using BaseRepository::BaseRepository;

    [[nodiscard]] std::optional<NotificationRecord> FindNotificationById(
        std::uint64_t notification_id
    ) const;
    [[nodiscard]] std::vector<std::uint64_t> ListFailedNotificationIds(int limit) const;
    [[nodiscard]] int QueryMaxTaskRetryCount(
        const std::string& task_type,
        const std::string& biz_key
    ) const;
    [[nodiscard]] std::optional<NotificationRetryCandidate> FindAuctionRetryCandidate(
        std::uint64_t notification_id
    ) const;
    [[nodiscard]] NotificationRecord CreateNotification(
        const CreateNotificationParams& params
    ) const;
    void UpdatePushStatus(std::uint64_t notification_id, const std::string& push_status) const;
    void InsertTaskLog(const NotificationTaskLogParams& params) const;
};

}  // namespace auction::repository
