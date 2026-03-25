#pragma once

#include "User.h"
#include <optional>
#include <string>

namespace auction {

class UserService {
public:
    static UserService& instance();

    std::optional<User> registerUser(const std::string& username,
                                    const std::string& password,
                                    const std::string& email,
                                    const std::string& phone = "");

    std::optional<std::pair<std::string, User>> login(const std::string& username,
                                                      const std::string& password);

    std::optional<User> getUserById(int64_t user_id);

    bool updateBalance(int64_t user_id, double amount);
    bool freezeBalance(int64_t user_id, double amount);

private:
    UserService() = default;

    static UserRole stringToRole(const std::string& role);
    static std::string roleToString(UserRole role);
};

} // namespace auction
