#pragma once

#include <string>
#include <string_view>

namespace auction::modules::auth {

class PasswordHasher {
public:
    explicit PasswordHasher(unsigned int rounds);

    [[nodiscard]] std::string HashPassword(std::string_view plain_password) const;
    [[nodiscard]] bool VerifyPassword(
        std::string_view plain_password,
        std::string_view stored_hash
    ) const;

    [[nodiscard]] static bool MeetsComplexity(
        std::string_view plain_password,
        std::string* error_message = nullptr
    );

private:
    [[nodiscard]] std::string BuildSalt() const;

    unsigned int rounds_;
};

}  // namespace auction::modules::auth
