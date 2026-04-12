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
        case ErrorCode::kAuthUnauthorized:
            return "unauthorized";
        case ErrorCode::kAuthTokenInvalid:
            return "token invalid";
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
    }
    return "unknown error";
}

}  // namespace auction::common::errors

