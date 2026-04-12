#pragma once

#include <cstdint>
#include <string>

#include "repository/base_repository.h"

namespace auction::repository {

struct DatabaseMetaSnapshot {
    std::uint64_t table_count{0};
    std::uint64_t required_table_count{0};
    std::uint64_t expected_table_count{0};
    std::uint64_t admin_user_count{0};
    std::uint64_t category_count{0};
    std::string server_version;
};

class DatabaseMetaRepository : public BaseRepository {
public:
    using BaseRepository::BaseRepository;

    [[nodiscard]] DatabaseMetaSnapshot LoadSnapshot(const std::string& database_name) const;
};

}  // namespace auction::repository
