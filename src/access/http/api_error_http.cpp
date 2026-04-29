#include "access/http/api_error_http.h"

#include "common/http/http_utils.h"

#if AUCTION_HAS_DROGON
#include <drogon/drogon.h>
#endif

namespace auction::access::http {

void RegisterApiErrorHttpHandlers() {
#if AUCTION_HAS_DROGON
    const auto not_found_response = common::http::BuildJsonResponse(
        common::http::BuildApiRouteNotFoundResponse(),
        drogon::k404NotFound
    );
    drogon::app().setCustom404Page(not_found_response, false);
#endif
}

}  // namespace auction::access::http
