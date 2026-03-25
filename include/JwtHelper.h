#pragma once

#include <string>
#include <inttypes.h>

namespace auction {

struct JwtPayload {
    int64_t user_id;
    std::string role;
    int64_t exp;
    int64_t iat;
};

class JwtHelper {
public:
    static std::string generateToken(int64_t user_id, const std::string& role, int expiry_hours);
    static bool verifyToken(const std::string& token, JwtPayload& payload);
    static std::string getTokenFromHeader(const std::string& auth_header);

private:
    static const std::string SECRET;
    static const std::string ALGORITHM;
};

} // namespace auction
