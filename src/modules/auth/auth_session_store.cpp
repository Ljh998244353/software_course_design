#include "modules/auth/auth_session_store.h"

namespace auction::modules::auth {

void InMemoryAuthSessionStore::Save(const AuthSession& session) {
    std::lock_guard lock(mutex_);
    CleanupExpiredLocked();

    sessions_by_token_id_[session.token_id] = session;
    token_ids_by_user_id_[session.user_id].insert(session.token_id);
}

std::optional<AuthSession> InMemoryAuthSessionStore::Find(const std::string& token_id) {
    std::lock_guard lock(mutex_);
    CleanupExpiredLocked();

    const auto iterator = sessions_by_token_id_.find(token_id);
    if (iterator == sessions_by_token_id_.end()) {
        return std::nullopt;
    }
    return iterator->second;
}

void InMemoryAuthSessionStore::Remove(const std::string& token_id) {
    std::lock_guard lock(mutex_);

    const auto iterator = sessions_by_token_id_.find(token_id);
    if (iterator == sessions_by_token_id_.end()) {
        return;
    }

    const auto user_iterator = token_ids_by_user_id_.find(iterator->second.user_id);
    if (user_iterator != token_ids_by_user_id_.end()) {
        user_iterator->second.erase(token_id);
        if (user_iterator->second.empty()) {
            token_ids_by_user_id_.erase(user_iterator);
        }
    }

    sessions_by_token_id_.erase(iterator);
}

void InMemoryAuthSessionStore::RemoveByUser(const std::uint64_t user_id) {
    std::lock_guard lock(mutex_);

    const auto user_iterator = token_ids_by_user_id_.find(user_id);
    if (user_iterator == token_ids_by_user_id_.end()) {
        return;
    }

    for (const auto& token_id : user_iterator->second) {
        sessions_by_token_id_.erase(token_id);
    }
    token_ids_by_user_id_.erase(user_iterator);
}

void InMemoryAuthSessionStore::CleanupExpiredLocked() {
    const auto now = CurrentEpochSeconds();

    for (auto iterator = sessions_by_token_id_.begin(); iterator != sessions_by_token_id_.end();) {
        if (iterator->second.expire_at_epoch_seconds > now) {
            ++iterator;
            continue;
        }

        const auto user_iterator = token_ids_by_user_id_.find(iterator->second.user_id);
        if (user_iterator != token_ids_by_user_id_.end()) {
            user_iterator->second.erase(iterator->first);
            if (user_iterator->second.empty()) {
                token_ids_by_user_id_.erase(user_iterator);
            }
        }

        iterator = sessions_by_token_id_.erase(iterator);
    }
}

}  // namespace auction::modules::auth
