#pragma once

#include <filesystem>
#include <string_view>

#include "common/config/app_config.h"
#include "common/db/mysql_connection.h"
#include "middleware/auth_middleware.h"
#include "modules/bid/bid_cache_store.h"
#include "modules/bid/bid_types.h"
#include "modules/notification/notification_service.h"

namespace auction::modules::bid {

class BidService {
public:
    BidService(
        const common::config::AppConfig& config,
        std::filesystem::path project_root,
        middleware::AuthMiddleware& auth_middleware,
        notification::NotificationService& notification_service,
        BidCacheStore& cache_store
    );

    [[nodiscard]] PlaceBidResult PlaceBid(
        std::string_view authorization_header,
        std::uint64_t auction_id,
        const PlaceBidRequest& request
    );
    [[nodiscard]] BidHistoryResult ListAuctionBidHistory(
        std::uint64_t auction_id,
        const BidHistoryQuery& query
    );
    [[nodiscard]] MyBidStatusResult GetMyBidStatus(
        std::string_view authorization_header,
        std::uint64_t auction_id
    );

private:
    [[nodiscard]] common::db::MysqlConnection CreateConnection() const;

    common::config::MysqlConfig mysql_config_;
    std::filesystem::path project_root_;
    middleware::AuthMiddleware* auth_middleware_;
    notification::NotificationService* notification_service_;
    BidCacheStore* cache_store_;
};

}  // namespace auction::modules::bid
