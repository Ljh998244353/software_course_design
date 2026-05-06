#pragma once

#include <memory>

#include "common/http/http_service_context.h"

namespace auction::access::http {

void RegisterReviewHttpRoutes(std::shared_ptr<common::http::HttpServiceContext> services);

}  // namespace auction::access::http
