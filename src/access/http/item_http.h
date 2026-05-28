#pragma once

#include "modules/item/item_service.h"

namespace auction::access::http {

void RegisterItemHttpRoutes(
    modules::item::ItemService& item_service
);

}  // namespace auction::access::http
