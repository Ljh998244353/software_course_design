#include "modules/auth/password_hasher.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <stdexcept>

#include <crypt.h>
#include <openssl/rand.h>

namespace auction::modules::auth {

namespace {

constexpr std::string_view kSaltAlphabet =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

std::string BuildErrorMessage(const std::string_view message) {
    return std::string(message);
}

bool ConstantTimeEquals(const std::string_view left, const std::string_view right) {
    if (left.size() != right.size()) {
        return false;
    }

    unsigned char diff = 0;
    for (std::size_t index = 0; index < left.size(); ++index) {
        diff |= static_cast<unsigned char>(left[index] ^ right[index]);
    }
    return diff == 0;
}

std::string HashWithCrypt(const std::string_view plain_password, const std::string& setting) {
    crypt_data data{};
    data.initialized = 0;

    char* result = crypt_r(std::string(plain_password).c_str(), setting.c_str(), &data);
    if (result == nullptr) {
        throw std::runtime_error("failed to hash password with crypt");
    }
    return result;
}

}  // namespace

PasswordHasher::PasswordHasher(const unsigned int rounds) : rounds_(std::max(5000U, rounds)) {}

std::string PasswordHasher::HashPassword(const std::string_view plain_password) const {
    std::string error_message;
    if (!MeetsComplexity(plain_password, &error_message)) {
        throw std::invalid_argument(error_message);
    }

    const auto setting = "$6$rounds=" + std::to_string(rounds_) + "$" + BuildSalt() + "$";
    return HashWithCrypt(plain_password, setting);
}

bool PasswordHasher::VerifyPassword(
    const std::string_view plain_password,
    const std::string_view stored_hash
) const {
    if (stored_hash.empty()) {
        return false;
    }

    if (stored_hash.rfind("$6$", 0) != 0) {
        return false;
    }

    return ConstantTimeEquals(HashWithCrypt(plain_password, std::string(stored_hash)), stored_hash);
}

bool PasswordHasher::MeetsComplexity(
    const std::string_view plain_password,
    std::string* error_message
) {
    if (plain_password.size() < 8 || plain_password.size() > 64) {
        if (error_message != nullptr) {
            *error_message = BuildErrorMessage("password length must be between 8 and 64");
        }
        return false;
    }

    bool has_upper = false;
    bool has_lower = false;
    bool has_digit = false;

    for (const unsigned char character : plain_password) {
        has_upper = has_upper || std::isupper(character) != 0;
        has_lower = has_lower || std::islower(character) != 0;
        has_digit = has_digit || std::isdigit(character) != 0;
    }

    if (!has_upper || !has_lower || !has_digit) {
        if (error_message != nullptr) {
            *error_message =
                BuildErrorMessage("password must contain uppercase, lowercase and digit");
        }
        return false;
    }

    return true;
}

std::string PasswordHasher::BuildSalt() const {
    std::array<unsigned char, 16> random_bytes{};
    if (RAND_bytes(random_bytes.data(), static_cast<int>(random_bytes.size())) != 1) {
        throw std::runtime_error("failed to generate password salt");
    }

    std::string salt;
    salt.reserve(random_bytes.size());
    for (const auto byte : random_bytes) {
        salt.push_back(kSaltAlphabet[byte % kSaltAlphabet.size()]);
    }
    return salt;
}

}  // namespace auction::modules::auth
