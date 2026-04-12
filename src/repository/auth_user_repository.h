#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "modules/auth/auth_types.h"
#include "repository/base_repository.h"

namespace auction::repository {

struct CreateAuthUserParams {
    std::string username;
    std::string password_hash;
    std::string phone;
    std::string email;
    std::string role_code;
    std::string status;
    std::string nickname;
};

class AuthUserRepository : public BaseRepository {
public:
    using BaseRepository::BaseRepository;

    [[nodiscard]] bool ExistsByUsername(const std::string& username) const;
    [[nodiscard]] bool ExistsByPhone(const std::string& phone) const;
    [[nodiscard]] bool ExistsByEmail(const std::string& email) const;

    [[nodiscard]] std::optional<modules::auth::AuthUserRecord> FindByUsername(
        const std::string& username
    ) const;
    [[nodiscard]] std::optional<modules::auth::AuthUserRecord> FindByPhone(
        const std::string& phone
    ) const;
    [[nodiscard]] std::optional<modules::auth::AuthUserRecord> FindByEmail(
        const std::string& email
    ) const;
    [[nodiscard]] std::optional<modules::auth::AuthUserRecord> FindByUserId(
        std::uint64_t user_id
    ) const;

    [[nodiscard]] modules::auth::AuthUserRecord CreateUser(const CreateAuthUserParams& params) const;
    void UpdateLastLoginAt(std::uint64_t user_id) const;
    void UpdateUserStatus(std::uint64_t user_id, const std::string& new_status) const;
    void InsertOperationLog(
        std::uint64_t operator_id,
        const std::string& operator_role,
        const std::string& operation_name,
        const std::string& biz_key,
        const std::string& result,
        const std::string& detail
    ) const;
};

}  // namespace auction::repository
