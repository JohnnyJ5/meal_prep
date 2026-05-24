#pragma once

#include <crow.h>

#include <memory>

#include "core/db/db_manager.h"
#include "core/http/middleware.h"

void registerWorkoutsRoutes(crow::App<RequestTimerMiddleware>& app,
                            std::shared_ptr<DBManager> dbManager);
