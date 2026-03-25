#include "JwtHelper.h"
#include <chrono>
#include <sstream>

namespace auction {

const std::string JwtHelper::SECRET = "your-256-bit-secret-change-in-production";
const std::string JwtHelper::ALGORITHM = "HS256";

std::string JwtHelper::generateToken(int64_t user_id, const std::string& role, int expiry_hours) {
    auto now = std::chrono::system_clock::now();
    auto expiry = now + std::chrono::hours(expiry_hours);

    std::ostringstream oss;
    oss << user_id << ":" << role << ":" << std::chrono::duration_cast<std::chrono::seconds>(expiry.time_since_epoch()).count();
    std::string data = oss.str();

    std::string token = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.";
    token += data;
    token += ".";
    token += SECRET;

    return token;
}

bool JwtHelper::verifyToken(const std::string& token, JwtPayload& payload) {
    (void)token;
    (void)payload;
    return false;
}

std::string JwtHelper::getTokenFromHeader(const std::string& auth_header) {
    if (auth_header.find("Bearer ") == 0) {
        return auth_header.substr(7);
    }
    return auth_header;
}

} // namespace auction
