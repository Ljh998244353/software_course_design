#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "modules/ops/ops_types.h"
#include "repository/base_repository.h"

namespace auction::repository {

class OpsRepository : public BaseRepository {
public:
    using BaseRepository::BaseRepository;

    [[nodiscard]] std::vector<modules::ops::OperationLogRecord> ListOperationLogs(
        const modules::ops::OperationLogQuery& query
    ) const;
    [[nodiscard]] std::vector<modules::ops::TaskLogRecord> ListTaskLogs(
        const modules::ops::TaskLogQuery& query
    ) const;
    [[nodiscard]] std::vector<modules::ops::SystemExceptionRecord> ListSystemExceptions(
        int limit
    ) const;

    void InsertOperationLog(
        const std::optional<std::uint64_t>& operator_id,
        const std::string& operator_role,
        const std::string& operation_name,
        const std::string& biz_key,
        const std::string& result,
        const std::string& detail
    ) const;
};

}  // namespace auction::repository
