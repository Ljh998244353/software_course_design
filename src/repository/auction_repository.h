#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "modules/auction/auction_types.h"
#include "repository/base_repository.h"

namespace auction::repository {

struct CreateAuctionParams {
    std::uint64_t item_id{0};
    std::uint64_t seller_id{0};
    std::string start_time;
    std::string end_time;
    std::string start_price;
    std::string current_price;
    std::string bid_step;
    int anti_sniping_window_seconds{0};
    int extend_seconds{0};
};

struct UpdateAuctionParams {
    std::uint64_t auction_id{0};
    std::string start_time;
    std::string end_time;
    std::string start_price;
    std::string bid_step;
    int anti_sniping_window_seconds{0};
    int extend_seconds{0};
};

struct CreateTaskLogParams {
    std::string task_type;
    std::string biz_key;
    std::string task_status;
    int retry_count{0};
    std::string last_error;
    std::string scheduled_at;
};

class AuctionRepository : public BaseRepository {
public:
    using BaseRepository::BaseRepository;
    using BaseRepository::EscapeString;

    [[nodiscard]] std::optional<modules::auction::AuctionItemSnapshot> FindItemSnapshotById(
        std::uint64_t item_id
    ) const;
    [[nodiscard]] std::optional<modules::auction::AuctionItemSnapshot> FindItemSnapshotByIdForUpdate(
        std::uint64_t item_id
    ) const;
    [[nodiscard]] bool HasNonTerminalAuction(std::uint64_t item_id) const;

    [[nodiscard]] std::optional<modules::auction::AuctionRecord> FindAuctionById(
        std::uint64_t auction_id
    ) const;
    [[nodiscard]] std::optional<modules::auction::AuctionRecord> FindAuctionByIdForUpdate(
        std::uint64_t auction_id
    ) const;

    [[nodiscard]] modules::auction::AuctionRecord CreateAuction(const CreateAuctionParams& params) const;
    [[nodiscard]] modules::auction::AuctionRecord UpdatePendingAuction(
        const UpdateAuctionParams& params
    ) const;
    void UpdateAuctionStatus(std::uint64_t auction_id, const std::string& status) const;
    void UpdateItemStatus(std::uint64_t item_id, const std::string& item_status) const;

    [[nodiscard]] bool IsStartDue(std::uint64_t auction_id) const;
    [[nodiscard]] bool IsFinishDue(std::uint64_t auction_id) const;
    [[nodiscard]] std::vector<std::uint64_t> ListDueStartAuctionIds() const;
    [[nodiscard]] std::vector<std::uint64_t> ListDueFinishAuctionIds() const;

    [[nodiscard]] std::vector<modules::auction::AdminAuctionSummary> ListAdminAuctions(
        const modules::auction::AdminAuctionQuery& query
    ) const;
    [[nodiscard]] std::optional<modules::auction::AdminAuctionDetail> FindAdminAuctionDetail(
        std::uint64_t auction_id
    ) const;
    [[nodiscard]] std::vector<modules::auction::PublicAuctionSummary> ListPublicAuctions(
        const modules::auction::PublicAuctionQuery& query
    ) const;
    [[nodiscard]] std::optional<modules::auction::PublicAuctionDetail> FindPublicAuctionDetail(
        std::uint64_t auction_id
    ) const;

    void InsertTaskLog(const CreateTaskLogParams& params) const;
};

}  // namespace auction::repository
