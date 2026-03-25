#include "UserService.h"
#include "MySQLPool.h"
#include "JwtHelper.h"
#include "Logger.h"
#include <openssl/md5.h>

namespace auction {

UserService& UserService::instance() {
    static UserService instance;
    return instance;
}

std::optional<User> UserService::registerUser(const std::string& username,
                                               const std::string& password,
                                               const std::string& email,
                                               const std::string& phone) {
    (void)username;
    (void)password;
    (void)email;
    (void)phone;

    User user;
    user.username = username;
    user.email = email;
    user.phone = phone;
    user.role = UserRole::USER;
    return user;
}

std::optional<std::pair<std::string, User>> UserService::login(const std::string& username,
                                                               const std::string& password) {
    (void)username;
    (void)password;

    User user;
    user.id = 1;
    user.username = username;
    user.role = UserRole::USER;
    user.email = "";
    user.balance = 0.0;
    user.credit_score = 5.0;

    std::string token = JwtHelper::generateToken(user.id, roleToString(user.role), 24);

    return std::make_pair(token, user);
}

std::optional<User> UserService::getUserById(int64_t user_id) {
    (void)user_id;
    User user;
    user.id = user_id;
    return user;
}

bool UserService::updateBalance(int64_t user_id, double amount) {
    (void)user_id;
    (void)amount;
    return true;
}

bool UserService::freezeBalance(int64_t user_id, double amount) {
    (void)user_id;
    (void)amount;
    return true;
}

UserRole UserService::stringToRole(const std::string& role) {
    if (role == "admin") return UserRole::ADMIN;
    if (role == "seller") return UserRole::SELLER;
    return UserRole::USER;
}

std::string UserService::roleToString(UserRole role) {
    switch (role) {
        case UserRole::ADMIN: return "admin";
        case UserRole::SELLER: return "seller";
        default: return "user";
    }
}

} // namespace auction
