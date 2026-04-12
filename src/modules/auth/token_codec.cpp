#include "modules/auth/token_codec.h"

#include <array>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <json/json.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

#include "common/errors/error_code.h"
#include "modules/auth/auth_exception.h"

namespace auction::modules::auth {

namespace {

std::string Base64UrlEncode(const unsigned char* input, const std::size_t size) {
    if (size == 0) {
        return {};
    }

    std::string encoded(((size + 2) / 3) * 4, '\0');
    const int encoded_length =
        EVP_EncodeBlock(reinterpret_cast<unsigned char*>(encoded.data()), input, size);
    if (encoded_length < 0) {
        throw std::runtime_error("failed to base64 encode data");
    }

    encoded.resize(static_cast<std::size_t>(encoded_length));
    for (char& character : encoded) {
        if (character == '+') {
            character = '-';
        } else if (character == '/') {
            character = '_';
        }
    }

    while (!encoded.empty() && encoded.back() == '=') {
        encoded.pop_back();
    }
    return encoded;
}

std::vector<unsigned char> Base64UrlDecode(std::string_view input) {
    std::string normalized(input);
    for (char& character : normalized) {
        if (character == '-') {
            character = '+';
        } else if (character == '_') {
            character = '/';
        }
    }

    const auto padding = (4 - (normalized.size() % 4)) % 4;
    normalized.append(padding, '=');

    std::vector<unsigned char> decoded((normalized.size() / 4) * 3, 0);
    const int decoded_length = EVP_DecodeBlock(
        decoded.data(),
        reinterpret_cast<const unsigned char*>(normalized.data()),
        static_cast<int>(normalized.size())
    );
    if (decoded_length < 0) {
        throw AuthException(common::errors::ErrorCode::kAuthSessionInvalid, "token decode failed");
    }

    decoded.resize(static_cast<std::size_t>(decoded_length) - padding);
    return decoded;
}

std::string SignToken(std::string_view secret, std::string_view payload_segment) {
    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    unsigned int digest_length = 0;

    const unsigned char* signed_result = HMAC(
        EVP_sha256(),
        secret.data(),
        static_cast<int>(secret.size()),
        reinterpret_cast<const unsigned char*>(payload_segment.data()),
        payload_segment.size(),
        digest.data(),
        &digest_length
    );

    if (signed_result == nullptr) {
        throw std::runtime_error("failed to sign token");
    }

    return Base64UrlEncode(digest.data(), digest_length);
}

bool ConstantTimeEquals(std::string_view left, std::string_view right) {
    if (left.size() != right.size()) {
        return false;
    }
    return CRYPTO_memcmp(left.data(), right.data(), left.size()) == 0;
}

std::string GenerateTokenId() {
    std::array<unsigned char, 16> bytes{};
    if (RAND_bytes(bytes.data(), static_cast<int>(bytes.size())) != 1) {
        throw std::runtime_error("failed to generate token id");
    }
    return Base64UrlEncode(bytes.data(), bytes.size());
}

}  // namespace

TokenCodec::TokenCodec(std::string secret, const int token_expire_minutes)
    : secret_(std::move(secret)), token_expire_minutes_(token_expire_minutes) {
    if (secret_.empty()) {
        throw std::invalid_argument("auth token secret must not be empty");
    }
}

IssuedToken TokenCodec::IssueToken(const std::uint64_t user_id, const std::string_view role_code) const {
    Json::Value payload(Json::objectValue);
    const auto expire_at_epoch_seconds =
        CurrentEpochSeconds() + static_cast<std::int64_t>(token_expire_minutes_) * 60;
    const auto token_id = GenerateTokenId();

    payload["userId"] = Json::UInt64(user_id);
    payload["roleCode"] = std::string(role_code);
    payload["tokenId"] = token_id;
    payload["expireAt"] = Json::Int64(expire_at_epoch_seconds);

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    const auto payload_json = Json::writeString(builder, payload);
    const auto payload_segment = Base64UrlEncode(
        reinterpret_cast<const unsigned char*>(payload_json.data()),
        payload_json.size()
    );
    const auto signature_segment = SignToken(secret_, payload_segment);

    return IssuedToken{
        .token = payload_segment + "." + signature_segment,
        .token_id = token_id,
        .expire_at_epoch_seconds = expire_at_epoch_seconds,
    };
}

TokenPayload TokenCodec::ParseAndVerify(const std::string_view token) const {
    if (token.empty()) {
        throw AuthException(common::errors::ErrorCode::kAuthTokenMissing, "token is missing");
    }

    const auto delimiter_position = token.find('.');
    if (delimiter_position == std::string_view::npos) {
        throw AuthException(
            common::errors::ErrorCode::kAuthSessionInvalid,
            "token format is invalid"
        );
    }

    const auto payload_segment = token.substr(0, delimiter_position);
    const auto signature_segment = token.substr(delimiter_position + 1);
    if (payload_segment.empty() || signature_segment.empty()) {
        throw AuthException(
            common::errors::ErrorCode::kAuthSessionInvalid,
            "token format is invalid"
        );
    }

    const auto expected_signature = SignToken(secret_, payload_segment);
    if (!ConstantTimeEquals(signature_segment, expected_signature)) {
        throw AuthException(common::errors::ErrorCode::kAuthSessionInvalid, "token signature invalid");
    }

    const auto payload_bytes = Base64UrlDecode(payload_segment);
    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;
    Json::Value payload;
    std::string errors;
    std::istringstream stream(
        std::string(reinterpret_cast<const char*>(payload_bytes.data()), payload_bytes.size())
    );
    if (!Json::parseFromStream(builder, stream, &payload, &errors)) {
        throw AuthException(common::errors::ErrorCode::kAuthSessionInvalid, "token payload invalid");
    }

    TokenPayload parsed;
    parsed.user_id = payload["userId"].asUInt64();
    parsed.role_code = payload["roleCode"].asString();
    parsed.token_id = payload["tokenId"].asString();
    parsed.expire_at_epoch_seconds = payload["expireAt"].asInt64();

    if (parsed.user_id == 0 || parsed.role_code.empty() || parsed.token_id.empty()) {
        throw AuthException(common::errors::ErrorCode::kAuthSessionInvalid, "token payload invalid");
    }

    if (parsed.expire_at_epoch_seconds <= CurrentEpochSeconds()) {
        throw AuthException(common::errors::ErrorCode::kAuthTokenExpired, "token expired");
    }

    return parsed;
}

}  // namespace auction::modules::auth
