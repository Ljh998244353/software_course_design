#include "modules/auth/auth_service.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

#include "common/db/mysql_connection.h"
#include "common/errors/error_code.h"
#include "modules/auth/auth_exception.h"
#include "repository/auth_user_repository.h"

namespace auction::modules::auth {

namespace {

bool IsDigitsOnly(const std::string_view value) {
    return !value.empty() &&
           std::all_of(value.begin(), value.end(), [](const unsigned char character) {
               return std::isdigit(character) != 0;
           });
}

bool IsEmailLike(const std::string_view value) {
    const auto at_position = value.find('@');
    if (at_position == std::string_view::npos || at_position == 0 ||
        at_position == value.size() - 1) {
        return false;
    }

    const auto dot_position = value.find('.', at_position);
    return dot_position != std::string_view::npos && dot_position < value.size() - 1;
}

bool IsUsernameLike(const std::string_view value) {
    return std::all_of(value.begin(), value.end(), [](const unsigned char character) {
        return std::isalnum(character) != 0 || character == '_';
    });
}

std::string NormalizeField(std::string value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) {
        value.erase(value.begin());
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
        value.pop_back();
    }
    return value;
}

void ThrowAuthError(const common::errors::ErrorCode code, const std::string_view message) {
    throw AuthException(code, std::string(message));
}

}  // namespace

AuthService::AuthService(
    const common::config::AppConfig& config,
    std::filesystem::path project_root,
    AuthSessionStore& session_store
) : mysql_config_(config.mysql),
    project_root_(std::move(project_root)),
    password_hasher_(config.auth.password_hash_rounds),
    token_codec_(config.auth.token_secret, config.auth.token_expire_minutes),
    session_store_(&session_store) {}

RegisterUserResult AuthService::RegisterUser(const RegisterUserRequest& request) {
    ValidateRegisterRequest(request);

    auto connection = CreateConnection();
    repository::AuthUserRepository repository(connection);

    if (repository.ExistsByUsername(request.username)) {
        ThrowAuthError(
            common::errors::ErrorCode::kAuthIdentityAlreadyExists,
            "username already exists"
        );
    }
    if (!request.phone.empty() && repository.ExistsByPhone(request.phone)) {
        ThrowAuthError(common::errors::ErrorCode::kAuthIdentityAlreadyExists, "phone already exists");
    }
    if (!request.email.empty() && repository.ExistsByEmail(request.email)) {
        ThrowAuthError(common::errors::ErrorCode::kAuthIdentityAlreadyExists, "email already exists");
    }

    const auto created = repository.CreateUser(repository::CreateAuthUserParams{
        .username = request.username,
        .password_hash = password_hasher_.HashPassword(request.password),
        .phone = request.phone,
        .email = request.email,
        .role_code = std::string(kRoleUser),
        .status = std::string(kStatusActive),
        .nickname = request.nickname,
    });

    return RegisterUserResult{
        .user_id = created.user_id,
        .username = created.username,
        .role_code = created.role_code,
        .status = created.status,
    };
}

LoginResult AuthService::Login(const LoginRequest& request) {
    ValidateLoginRequest(request);

    auto connection = CreateConnection();
    repository::AuthUserRepository repository(connection);
    const auto user = LoadUserByPrincipal(repository, request.principal);
    ValidateUserStatusForLogin(user);

    if (!password_hasher_.VerifyPassword(request.password, user.password_hash)) {
        ThrowAuthError(
            common::errors::ErrorCode::kAuthCredentialInvalid,
            "username or password is incorrect"
        );
    }

    repository.UpdateLastLoginAt(user.user_id);

    const auto issued = token_codec_.IssueToken(user.user_id, user.role_code);
    session_store_->Save(AuthSession{
        .token_id = issued.token_id,
        .user_id = user.user_id,
        .role_code = user.role_code,
        .expire_at_epoch_seconds = issued.expire_at_epoch_seconds,
    });

    return LoginResult{
        .token = issued.token,
        .expire_at = ToIsoTimestamp(issued.expire_at_epoch_seconds),
        .user_info = ToUserProfile(user),
    };
}

void AuthService::Logout(const std::string_view token) {
    const auto token_payload = token_codec_.ParseAndVerify(token);
    const auto session = session_store_->Find(token_payload.token_id);
    if (!session.has_value()) {
        ThrowAuthError(common::errors::ErrorCode::kAuthSessionInvalid, "token session is invalid");
    }
    session_store_->Remove(token_payload.token_id);
}

AuthContext AuthService::ValidateToken(const std::string_view token) {
    const auto token_payload = token_codec_.ParseAndVerify(token);
    const auto session = session_store_->Find(token_payload.token_id);
    if (!session.has_value() || session->user_id != token_payload.user_id) {
        ThrowAuthError(common::errors::ErrorCode::kAuthSessionInvalid, "token session is invalid");
    }

    auto connection = CreateConnection();
    repository::AuthUserRepository repository(connection);
    const auto user = repository.FindByUserId(token_payload.user_id);
    if (!user.has_value()) {
        session_store_->Remove(token_payload.token_id);
        ThrowAuthError(common::errors::ErrorCode::kResourceNotFound, "user not found");
    }

    if (user->status == kStatusFrozen) {
        session_store_->RemoveByUser(user->user_id);
        ThrowAuthError(common::errors::ErrorCode::kAuthUserFrozen, "user is frozen");
    }

    if (user->status == kStatusDisabled) {
        session_store_->RemoveByUser(user->user_id);
        ThrowAuthError(common::errors::ErrorCode::kAuthUserDisabled, "user is disabled");
    }

    return BuildContext(*user, token_payload, std::string(token));
}

UserProfile AuthService::GetCurrentUser(const std::string_view token) {
    const auto context = ValidateToken(token);
    return UserProfile{
        .user_id = context.user_id,
        .username = context.username,
        .nickname = context.nickname,
        .role_code = context.role_code,
        .status = context.status,
    };
}

void AuthService::RequireRole(
    const AuthContext& context,
    const std::initializer_list<std::string_view> allowed_roles
) const {
    const auto iterator = std::find(allowed_roles.begin(), allowed_roles.end(), context.role_code);
    if (iterator == allowed_roles.end()) {
        ThrowAuthError(common::errors::ErrorCode::kAuthPermissionDenied, "permission denied");
    }
}

ChangeUserStatusResult AuthService::ChangeUserStatus(
    const std::string_view operator_token,
    const std::uint64_t target_user_id,
    std::string new_status,
    std::string reason
) {
    auto operator_context = ValidateToken(operator_token);
    RequireRole(operator_context, {kRoleAdmin});
    ValidateChangeStatusRequest(new_status);
    reason = NormalizeField(std::move(reason));

    auto connection = CreateConnection();
    repository::AuthUserRepository repository(connection);
    const auto target_user = repository.FindByUserId(target_user_id);
    if (!target_user.has_value()) {
        ThrowAuthError(common::errors::ErrorCode::kResourceNotFound, "target user not found");
    }

    if (!IsValidStatusTransition(target_user->status, new_status)) {
        ThrowAuthError(
            common::errors::ErrorCode::kAuthStatusTransitionInvalid,
            "status transition is invalid"
        );
    }

    connection.BeginTransaction();
    try {
        repository.UpdateUserStatus(target_user_id, new_status);
        repository.InsertOperationLog(
            operator_context.user_id,
            operator_context.role_code,
            "change_user_status",
            std::to_string(target_user_id),
            "SUCCESS",
            reason.empty() ? "status changed by admin" : reason
        );
        connection.Commit();
    } catch (...) {
        connection.Rollback();
        throw;
    }

    session_store_->RemoveByUser(target_user_id);
    return ChangeUserStatusResult{
        .user_id = target_user_id,
        .old_status = target_user->status,
        .new_status = new_status,
        .updated_at = ToIsoTimestamp(CurrentEpochSeconds()),
    };
}

AuthContext AuthService::BuildContext(
    const AuthUserRecord& user,
    const TokenPayload& token_payload,
    std::string token
) const {
    return AuthContext{
        .user_id = user.user_id,
        .username = user.username,
        .nickname = user.nickname,
        .role_code = user.role_code,
        .status = user.status,
        .token_id = token_payload.token_id,
        .token = std::move(token),
        .expire_at_epoch_seconds = token_payload.expire_at_epoch_seconds,
    };
}

AuthUserRecord AuthService::LoadUserByPrincipal(
    repository::AuthUserRepository& repository,
    const std::string& principal
) const {
    if (const auto user = repository.FindByUsername(principal); user.has_value()) {
        return *user;
    }

    if (IsDigitsOnly(principal)) {
        if (const auto user = repository.FindByPhone(principal); user.has_value()) {
            return *user;
        }
    }

    if (IsEmailLike(principal)) {
        if (const auto user = repository.FindByEmail(principal); user.has_value()) {
            return *user;
        }
    }

    ThrowAuthError(
        common::errors::ErrorCode::kAuthCredentialInvalid,
        "username or password is incorrect"
    );
    throw std::logic_error("unreachable");
}

void AuthService::ValidateRegisterRequest(const RegisterUserRequest& request) {
    if (request.username.size() < 4 || request.username.size() > 50 ||
        !IsUsernameLike(request.username)) {
        throw std::invalid_argument(
            "username must be 4-50 chars and contain only letters, digits or underscore"
        );
    }

    if (!request.phone.empty() && (!IsDigitsOnly(request.phone) || request.phone.size() > 20)) {
        throw std::invalid_argument("phone must contain digits only and be at most 20 chars");
    }

    if (!request.email.empty() && !IsEmailLike(request.email)) {
        throw std::invalid_argument("email format is invalid");
    }

    if (request.nickname.size() > 50) {
        throw std::invalid_argument("nickname must be at most 50 chars");
    }

    std::string password_error;
    if (!PasswordHasher::MeetsComplexity(request.password, &password_error)) {
        throw std::invalid_argument(password_error);
    }
}

void AuthService::ValidateLoginRequest(const LoginRequest& request) {
    if (request.principal.empty()) {
        throw std::invalid_argument("login principal must not be empty");
    }
    if (request.password.empty()) {
        throw std::invalid_argument("login password must not be empty");
    }
}

void AuthService::ValidateUserStatusForLogin(const AuthUserRecord& user) {
    if (user.status == kStatusFrozen) {
        ThrowAuthError(common::errors::ErrorCode::kAuthUserFrozen, "user is frozen");
    }
    if (user.status == kStatusDisabled) {
        ThrowAuthError(common::errors::ErrorCode::kAuthUserDisabled, "user is disabled");
    }
}

void AuthService::ValidateChangeStatusRequest(const std::string& new_status) {
    if (!IsSupportedStatus(new_status)) {
        throw std::invalid_argument("new status is invalid");
    }
}

common::db::MysqlConnection AuthService::CreateConnection() const {
    return common::db::MysqlConnection(mysql_config_, project_root_);
}

}  // namespace auction::modules::auth
