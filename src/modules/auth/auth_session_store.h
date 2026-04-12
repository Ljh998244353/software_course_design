#pragma once

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "modules/auth/auth_types.h"

namespace auction::modules::auth {

class AuthSessionStore {
public:
    virtual ~AuthSessionStore() = default;

    virtual void Save(const AuthSession& session) = 0;
    [[nodiscard]] virtual std::optional<AuthSession> Find(const std::string& token_id) = 0;
    virtual void Remove(const std::string& token_id) = 0;
    virtual void RemoveByUser(std::uint64_t user_id) = 0;
};

class InMemoryAuthSessionStore final : public AuthSessionStore {
public:
    void Save(const AuthSession& session) override;
    [[nodiscard]] std::optional<AuthSession> Find(const std::string& token_id) override;
    void Remove(const std::string& token_id) override;
    void RemoveByUser(std::uint64_t user_id) override;

private:
    void CleanupExpiredLocked();

    std::mutex mutex_;
    std::unordered_map<std::string, AuthSession> sessions_by_token_id_;
    std::unordered_map<std::uint64_t, std::unordered_set<std::string>> token_ids_by_user_id_;
};

}  // namespace auction::modules::auth
