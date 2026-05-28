#pragma once

#include "modules/audit/item_audit_service.h"
#include "modules/statistics/statistics_service.h"

namespace auction::access::http {

void RegisterAdminHttpRoutes(
    modules::audit::ItemAuditService& audit_service,
    modules::statistics::StatisticsService& statistics_service
);

}  // namespace auction::access::http
