#pragma once

#include <cstdint>
#include <optional>
#include <string>

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

class NotificationRepository : public BaseRepository {
public:
    using BaseRepository::BaseRepository;

    [[nodiscard]] NotificationRecord CreateNotification(
        const CreateNotificationParams& params
    ) const;
    void UpdatePushStatus(std::uint64_t notification_id, const std::string& push_status) const;
    void InsertTaskLog(const NotificationTaskLogParams& params) const;
};

}  // namespace auction::repository
