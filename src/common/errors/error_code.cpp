#include "common/errors/error_code.h"

namespace auction::common::errors {

std::string_view ErrorCodeMessage(const ErrorCode error_code) {
    switch (error_code) {
        case ErrorCode::kOk:
            return "success";
        case ErrorCode::kInvalidArgument:
            return "invalid argument";
        case ErrorCode::kResourceNotFound:
            return "resource not found";
        case ErrorCode::kPermissionDenied:
            return "permission denied";
        case ErrorCode::kAuthIdentityAlreadyExists:
            return "identity already exists";
        case ErrorCode::kAuthCredentialInvalid:
            return "credential invalid";
        case ErrorCode::kAuthTokenMissing:
            return "token missing";
        case ErrorCode::kAuthTokenExpired:
            return "token expired";
        case ErrorCode::kAuthSessionInvalid:
            return "session invalid";
        case ErrorCode::kAuthUserFrozen:
            return "user frozen";
        case ErrorCode::kAuthUserDisabled:
            return "user disabled";
        case ErrorCode::kAuthPermissionDenied:
            return "auth permission denied";
        case ErrorCode::kAuthStatusTransitionInvalid:
            return "auth status transition invalid";
        case ErrorCode::kAuctionNotFound:
            return "auction not found";
        case ErrorCode::kBidConflict:
            return "bid conflict";
        case ErrorCode::kInternalError:
            return "internal error";
        case ErrorCode::kConfigError:
            return "config error";
        case ErrorCode::kDependencyMissing:
            return "dependency missing";
        case ErrorCode::kDatabaseUnavailable:
            return "database unavailable";
        case ErrorCode::kDatabaseQueryFailed:
            return "database query failed";
    }
    return "unknown error";
}

}  // namespace auction::common::errors
