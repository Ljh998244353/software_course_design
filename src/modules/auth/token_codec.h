#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "modules/auth/auth_types.h"

namespace auction::modules::auth {

struct IssuedToken {
    std::string token;
    std::string token_id;
    std::int64_t expire_at_epoch_seconds{0};
};

class TokenCodec {
public:
    TokenCodec(std::string secret, int token_expire_minutes);

    [[nodiscard]] IssuedToken IssueToken(
        std::uint64_t user_id,
        std::string_view role_code
    ) const;

    [[nodiscard]] TokenPayload ParseAndVerify(std::string_view token) const;

private:
    std::string secret_;
    int token_expire_minutes_;
};

}  // namespace auction::modules::auth
