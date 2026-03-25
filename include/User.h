#pragma once

#include <string>
#include <cstdint>
#include <ctime>

namespace auction {

enum class UserRole {
    USER,
    SELLER,
    ADMIN
};

class User {
public:
    int64_t id;
    std::string username;
    std::string email;
    std::string phone;
    std::string password_hash;
    UserRole role;
    std::string real_name;
    std::string avatar_url;
    bool verified;
    double balance;
    double frozen_balance;
    double credit_score;
    time_t created_at;
    time_t updated_at;

    User() : id(0), role(UserRole::USER), verified(false),
             balance(0.0), frozen_balance(0.0), credit_score(5.0),
             created_at(0), updated_at(0) {}

    bool isAdmin() const { return role == UserRole::ADMIN; }
    bool isSeller() const { return role == UserRole::SELLER || role == UserRole::ADMIN; }
};

} // namespace auction
