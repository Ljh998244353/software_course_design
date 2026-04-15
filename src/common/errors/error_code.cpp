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
        case ErrorCode::kItemNotFound:
            return "item not found";
        case ErrorCode::kItemOwnerMismatch:
            return "item owner mismatch";
        case ErrorCode::kItemEditStatusInvalid:
            return "item edit status invalid";
        case ErrorCode::kItemSubmitStatusInvalid:
            return "item submit status invalid";
        case ErrorCode::kItemAuditStatusInvalid:
            return "item audit status invalid";
        case ErrorCode::kItemCategoryInvalid:
            return "item category invalid";
        case ErrorCode::kItemImageUploadFailed:
            return "item image upload failed";
        case ErrorCode::kItemImageInvalid:
            return "item image invalid";
        case ErrorCode::kItemAuditReasonRequired:
            return "item audit reason required";
        case ErrorCode::kItemAuditResultInvalid:
            return "item audit result invalid";
        case ErrorCode::kAuctionNotFound:
            return "auction not found";
        case ErrorCode::kAuctionItemNotFound:
            return "auction item not found";
        case ErrorCode::kAuctionItemStatusInvalid:
            return "auction item status invalid";
        case ErrorCode::kAuctionStartTimeInvalid:
            return "auction start time invalid";
        case ErrorCode::kAuctionEndTimeInvalid:
            return "auction end time invalid";
        case ErrorCode::kAuctionStartPriceInvalid:
            return "auction start price invalid";
        case ErrorCode::kAuctionBidStepInvalid:
            return "auction bid step invalid";
        case ErrorCode::kAuctionAntiSnipingInvalid:
            return "auction anti sniping invalid";
        case ErrorCode::kAuctionDuplicateActiveByItem:
            return "auction duplicate active by item";
        case ErrorCode::kAuctionUpdateStatusInvalid:
            return "auction update status invalid";
        case ErrorCode::kAuctionCancelStatusInvalid:
            return "auction cancel status invalid";
        case ErrorCode::kAuctionStartStatusInvalid:
            return "auction start status invalid";
        case ErrorCode::kAuctionFinishStatusInvalid:
            return "auction finish status invalid";
        case ErrorCode::kAuctionStatusConflict:
            return "auction status conflict";
        case ErrorCode::kAuctionRelatedItemStatusInvalid:
            return "auction related item status invalid";
        case ErrorCode::kBidAuctionNotFound:
            return "bid auction not found";
        case ErrorCode::kBidAuctionNotStarted:
            return "bid auction not started";
        case ErrorCode::kBidAuctionClosed:
            return "bid auction closed";
        case ErrorCode::kBidAuctionStatusInvalid:
            return "bid auction status invalid";
        case ErrorCode::kBidAmountInvalid:
            return "bid amount invalid";
        case ErrorCode::kBidAmountTooLow:
            return "bid amount too low";
        case ErrorCode::kBidUserStatusInvalid:
            return "bid user status invalid";
        case ErrorCode::kBidIdempotencyConflict:
            return "bid idempotency conflict";
        case ErrorCode::kBidRateLimited:
            return "bid rate limited";
        case ErrorCode::kBidConflict:
            return "bid conflict";
        case ErrorCode::kBidSubscriptionInvalid:
            return "bid subscription invalid";
        case ErrorCode::kBidHistoryQueryInvalid:
            return "bid history query invalid";
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
