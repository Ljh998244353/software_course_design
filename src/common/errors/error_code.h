#pragma once

#include <string_view>

namespace auction::common::errors {

enum class ErrorCode : int {
    kOk = 0,
    kInvalidArgument = 4001,
    kResourceNotFound = 4002,
    kPermissionDenied = 4004,
    kAuthIdentityAlreadyExists = 4101,
    kAuthCredentialInvalid = 4102,
    kAuthTokenMissing = 4103,
    kAuthTokenExpired = 4104,
    kAuthSessionInvalid = 4105,
    kAuthUserFrozen = 4106,
    kAuthUserDisabled = 4107,
    kAuthPermissionDenied = 4108,
    kAuthStatusTransitionInvalid = 4109,
    kAuctionNotFound = 4301,
    kBidConflict = 4410,
    kInternalError = 5001,
    kConfigError = 5002,
    kDependencyMissing = 5003,
    kDatabaseUnavailable = 5004,
    kDatabaseQueryFailed = 5005,
};

[[nodiscard]] std::string_view ErrorCodeMessage(ErrorCode error_code);

}  // namespace auction::common::errors
