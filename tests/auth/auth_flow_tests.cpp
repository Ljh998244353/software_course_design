#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>

#include "common/config/config_loader.h"
#include "common/db/mysql_connection.h"
#include "common/errors/error_code.h"
#include "middleware/auth_middleware.h"
#include "modules/auth/auth_exception.h"
#include "modules/auth/auth_service.h"
#include "modules/auth/auth_session_store.h"
#include "repository/auth_user_repository.h"

namespace {

using auction::common::errors::ErrorCode;
using auction::modules::auth::AuthException;

std::string BuildUniqueSuffix() {
    return std::to_string(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        )
            .count()
    );
}

template <typename Fn>
void ExpectAuthError(const ErrorCode expected_code, Fn&& function) {
    try {
        function();
        assert(false && "expected auth exception");
    } catch (const AuthException& exception) {
        assert(exception.code() == expected_code);
    }
}

}  // namespace

int main() {
    namespace fs = std::filesystem;

    const fs::path project_root(AUCTION_PROJECT_ROOT);
    const fs::path config_path = auction::common::config::ConfigLoader::ResolveConfigPath(project_root);
    const auto config = auction::common::config::ConfigLoader::LoadFromFile(config_path);

    auction::modules::auth::InMemoryAuthSessionStore session_store;
    auction::modules::auth::AuthService auth_service(config, project_root, session_store);
    auction::middleware::AuthMiddleware auth_middleware(auth_service);

    const auto unique_suffix = BuildUniqueSuffix();
    const auto username = "user_" + unique_suffix;
    const auto phone = "139" + unique_suffix.substr(unique_suffix.size() - 8);
    const auto email = username + "@auction.local";
    const auto password = "UserPass1";

    const auto registered = auth_service.RegisterUser({
        .username = username,
        .password = password,
        .phone = phone,
        .email = email,
        .nickname = "auth flow user",
    });
    assert(registered.user_id > 0);
    assert(registered.role_code == "USER");
    assert(registered.status == "ACTIVE");

    {
        auction::common::db::MysqlConnection connection(config.mysql, project_root);
        auction::repository::AuthUserRepository repository(connection);
        const auto loaded = repository.FindByUsername(username);
        assert(loaded.has_value());
        assert(loaded->password_hash != password);
        assert(loaded->password_hash.rfind("$6$", 0) == 0);
    }

    ExpectAuthError(ErrorCode::kAuthIdentityAlreadyExists, [&] {
        const auto ignored = auth_service.RegisterUser({
            .username = username,
            .password = "OtherPass1",
            .phone = "13800000001",
            .email = "dup1@auction.local",
            .nickname = "dup",
        });
        (void)ignored;
    });

    ExpectAuthError(ErrorCode::kAuthIdentityAlreadyExists, [&] {
        const auto ignored = auth_service.RegisterUser({
            .username = username + "_2",
            .password = "OtherPass1",
            .phone = phone,
            .email = "dup2@auction.local",
            .nickname = "dup",
        });
        (void)ignored;
    });

    ExpectAuthError(ErrorCode::kAuthCredentialInvalid, [&] {
        const auto ignored = auth_service.Login({
            .principal = username,
            .password = "WrongPass1",
        });
        (void)ignored;
    });

    const auto login = auth_service.Login({
        .principal = username,
        .password = password,
    });
    assert(!login.token.empty());
    assert(login.user_info.username == username);

    ExpectAuthError(ErrorCode::kAuthTokenMissing, [&] {
        const auto ignored = auth_middleware.RequireAuthenticated("");
        (void)ignored;
    });

    const auto me = auth_service.GetCurrentUser(login.token);
    assert(me.user_id == registered.user_id);
    assert(me.role_code == "USER");

    const auto user_context =
        auth_middleware.RequireAnyRole("Bearer " + login.token, {"USER", "ADMIN"});
    assert(user_context.user_id == registered.user_id);

    ExpectAuthError(ErrorCode::kAuthPermissionDenied, [&] {
        const auto ignored = auth_middleware.RequireAnyRole("Bearer " + login.token, {"ADMIN"});
        (void)ignored;
    });

    const auto admin_login = auth_service.Login({
        .principal = "admin",
        .password = "Admin@123",
    });
    const auto admin_context =
        auth_middleware.RequireAnyRole("Bearer " + admin_login.token, {"ADMIN"});
    assert(admin_context.role_code == "ADMIN");

    const auto support_login = auth_service.Login({
        .principal = "support",
        .password = "Support@123",
    });
    ExpectAuthError(ErrorCode::kAuthPermissionDenied, [&] {
        const auto ignored =
            auth_middleware.RequireAnyRole("Bearer " + support_login.token, {"ADMIN"});
        (void)ignored;
    });

    const auto frozen = auth_service.ChangeUserStatus(
        admin_login.token,
        registered.user_id,
        "FROZEN",
        "risk control"
    );
    assert(frozen.old_status == "ACTIVE");
    assert(frozen.new_status == "FROZEN");

    ExpectAuthError(ErrorCode::kAuthUserFrozen, [&] {
        const auto ignored = auth_service.Login({
            .principal = username,
            .password = password,
        });
        (void)ignored;
    });

    ExpectAuthError(ErrorCode::kAuthSessionInvalid, [&] {
        const auto ignored = auth_service.GetCurrentUser(login.token);
        (void)ignored;
    });

    const auto reactivated = auth_service.ChangeUserStatus(
        admin_login.token,
        registered.user_id,
        "ACTIVE",
        "restore access"
    );
    assert(reactivated.new_status == "ACTIVE");

    const auto relogin = auth_service.Login({
        .principal = email,
        .password = password,
    });
    assert(relogin.user_info.status == "ACTIVE");

    auto expired_config = config;
    expired_config.auth.token_expire_minutes = 0;
    auction::modules::auth::InMemoryAuthSessionStore expired_session_store;
    auction::modules::auth::AuthService expired_auth_service(
        expired_config,
        project_root,
        expired_session_store
    );
    const auto expired_login = expired_auth_service.Login({
        .principal = username,
        .password = password,
    });
    ExpectAuthError(ErrorCode::kAuthTokenExpired, [&] {
        const auto ignored = expired_auth_service.GetCurrentUser(expired_login.token);
        (void)ignored;
    });

    auth_service.Logout(relogin.token);
    ExpectAuthError(ErrorCode::kAuthSessionInvalid, [&] {
        const auto ignored = auth_service.GetCurrentUser(relogin.token);
        (void)ignored;
    });

    const auto disabled = auth_service.ChangeUserStatus(
        admin_login.token,
        registered.user_id,
        "DISABLED",
        "permanent ban"
    );
    assert(disabled.new_status == "DISABLED");

    ExpectAuthError(ErrorCode::kAuthUserDisabled, [&] {
        const auto ignored = auth_service.Login({
            .principal = phone,
            .password = password,
        });
        (void)ignored;
    });

    std::cout << "auction_auth_flow_tests passed\n";
    return 0;
}
