#pragma once

#include <filesystem>
#include <initializer_list>
#include <string>
#include <string_view>

#include "common/config/app_config.h"
#include "repository/auth_user_repository.h"
#include "modules/auth/auth_session_store.h"
#include "modules/auth/auth_types.h"
#include "modules/auth/password_hasher.h"
#include "modules/auth/token_codec.h"

namespace auction::modules::auth {

class AuthService {
public:
    AuthService(
        const common::config::AppConfig& config,
        std::filesystem::path project_root,
        AuthSessionStore& session_store
    );

    [[nodiscard]] RegisterUserResult RegisterUser(const RegisterUserRequest& request);
    [[nodiscard]] LoginResult Login(const LoginRequest& request);
    void Logout(std::string_view token);
    [[nodiscard]] AuthContext ValidateToken(std::string_view token);
    [[nodiscard]] UserProfile GetCurrentUser(std::string_view token);
    void RequireRole(
        const AuthContext& context,
        std::initializer_list<std::string_view> allowed_roles
    ) const;
    [[nodiscard]] ChangeUserStatusResult ChangeUserStatus(
        std::string_view operator_token,
        std::uint64_t target_user_id,
        std::string new_status,
        std::string reason
    );

private:
    [[nodiscard]] AuthContext BuildContext(
        const AuthUserRecord& user,
        const TokenPayload& token_payload,
        std::string token
    ) const;
    [[nodiscard]] AuthUserRecord LoadUserByPrincipal(
        repository::AuthUserRepository& repository,
        const std::string& principal
    ) const;
    static void ValidateRegisterRequest(const RegisterUserRequest& request);
    static void ValidateLoginRequest(const LoginRequest& request);
    static void ValidateUserStatusForLogin(const AuthUserRecord& user);
    static void ValidateChangeStatusRequest(const std::string& new_status);
    [[nodiscard]] common::db::MysqlConnection CreateConnection() const;

    common::config::MysqlConfig mysql_config_;
    std::filesystem::path project_root_;
    PasswordHasher password_hasher_;
    TokenCodec token_codec_;
    AuthSessionStore* session_store_;
};

}  // namespace auction::modules::auth
