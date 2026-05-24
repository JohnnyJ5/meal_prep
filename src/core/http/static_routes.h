#pragma once

#include <crow.h>

#include "core/http/middleware.h"

void registerStaticRoutes(crow::App<RequestTimerMiddleware>& app);
