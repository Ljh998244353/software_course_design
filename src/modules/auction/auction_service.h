#pragma once

#include <filesystem>
#include <string_view>
#include <vector>

#include "common/config/app_config.h"
#include "common/db/mysql_connection.h"
#include "middleware/auth_middleware.h"
#include "modules/auction/auction_types.h"

namespace auction::modules::auction {

class AuctionService {
public:
    AuctionService(
        const common::config::AppConfig& config,
        std::filesystem::path project_root,
        middleware::AuthMiddleware& auth_middleware
    );

    [[nodiscard]] CreateAuctionResult CreateAuction(
        std::string_view authorization_header,
        const CreateAuctionRequest& request
    );
    [[nodiscard]] UpdateAuctionResult UpdatePendingAuction(
        std::string_view authorization_header,
        std::uint64_t auction_id,
        const UpdateAuctionRequest& request
    );
    [[nodiscard]] CancelAuctionResult CancelPendingAuction(
        std::string_view authorization_header,
        std::uint64_t auction_id,
        const CancelAuctionRequest& request
    );
    [[nodiscard]] std::vector<AdminAuctionSummary> ListAdminAuctions(
        std::string_view authorization_header,
        const AdminAuctionQuery& query
    );
    [[nodiscard]] AdminAuctionDetail GetAdminAuctionDetail(
        std::string_view authorization_header,
        std::uint64_t auction_id
    );
    [[nodiscard]] std::vector<PublicAuctionSummary> ListPublicAuctions(
        const PublicAuctionQuery& query
    );
    [[nodiscard]] PublicAuctionDetail GetPublicAuctionDetail(std::uint64_t auction_id);
    [[nodiscard]] AuctionScheduleResult StartDueAuctions();
    [[nodiscard]] AuctionScheduleResult FinishDueAuctions();

private:
    [[nodiscard]] common::db::MysqlConnection CreateConnection() const;

    common::config::MysqlConfig mysql_config_;
    std::filesystem::path project_root_;
    middleware::AuthMiddleware* auth_middleware_;
};

}  // namespace auction::modules::auction
