#pragma once

#include <chrono>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>

namespace auction::modules::auth {

inline constexpr std::string_view kRoleUser = "USER";
inline constexpr std::string_view kRoleAdmin = "ADMIN";
inline constexpr std::string_view kRoleSupport = "SUPPORT";

inline constexpr std::string_view kStatusActive = "ACTIVE";
inline constexpr std::string_view kStatusFrozen = "FROZEN";
inline constexpr std::string_view kStatusDisabled = "DISABLED";

struct AuthUserRecord {
    std::uint64_t user_id{0};
    std::string username;
    std::string password_hash;
    std::string phone;
    std::string email;
    std::string role_code;
    std::string status;
    std::string nickname;
};

struct UserProfile {
    std::uint64_t user_id{0};
    std::string username;
    std::string nickname;
    std::string role_code;
    std::string status;
};

struct RegisterUserRequest {
    std::string username;
    std::string password;
    std::string phone;
    std::string email;
    std::string nickname;
};

struct RegisterUserResult {
    std::uint64_t user_id{0};
    std::string username;
    std::string role_code;
    std::string status;
};

struct LoginRequest {
    std::string principal;
    std::string password;
};

struct AuthContext {
    std::uint64_t user_id{0};
    std::string username;
    std::string nickname;
    std::string role_code;
    std::string status;
    std::string token_id;
    std::string token;
    std::int64_t expire_at_epoch_seconds{0};
};

struct LoginResult {
    std::string token;
    std::string expire_at;
    UserProfile user_info;
};

struct ChangeUserStatusResult {
    std::uint64_t user_id{0};
    std::string old_status;
    std::string new_status;
    std::string updated_at;
};

struct AuthSession {
    std::string token_id;
    std::uint64_t user_id{0};
    std::string role_code;
    std::int64_t expire_at_epoch_seconds{0};
};

struct TokenPayload {
    std::string token_id;
    std::uint64_t user_id{0};
    std::string role_code;
    std::int64_t expire_at_epoch_seconds{0};
};

inline UserProfile ToUserProfile(const AuthUserRecord& record) {
    return UserProfile{
        .user_id = record.user_id,
        .username = record.username,
        .nickname = record.nickname,
        .role_code = record.role_code,
        .status = record.status,
    };
}

inline bool IsSupportedRole(const std::string_view role_code) {
    return role_code == kRoleUser || role_code == kRoleAdmin || role_code == kRoleSupport;
}

inline bool IsSupportedStatus(const std::string_view status) {
    return status == kStatusActive || status == kStatusFrozen || status == kStatusDisabled;
}

inline bool IsValidStatusTransition(
    const std::string_view old_status,
    const std::string_view new_status
) {
    if (!IsSupportedStatus(old_status) || !IsSupportedStatus(new_status)) {
        return false;
    }

    if (old_status == new_status) {
        return true;
    }

    if (old_status == kStatusActive && new_status == kStatusFrozen) {
        return true;
    }

    if (old_status == kStatusFrozen && new_status == kStatusActive) {
        return true;
    }

    if ((old_status == kStatusActive || old_status == kStatusFrozen) &&
        new_status == kStatusDisabled) {
        return true;
    }

    return false;
}

inline std::int64_t CurrentEpochSeconds() {
    return std::chrono::duration_cast<std::chrono::seconds>(
               std::chrono::system_clock::now().time_since_epoch()
    )
        .count();
}

inline std::string ToIsoTimestamp(const std::int64_t epoch_seconds) {
    const auto time_value = static_cast<std::time_t>(epoch_seconds);
    std::tm local_tm{};
    localtime_r(&time_value, &local_tm);

    std::ostringstream output;
    output << std::put_time(&local_tm, "%Y-%m-%dT%H:%M:%S%z");
    return output.str();
}

}  // namespace auction::modules::auth
