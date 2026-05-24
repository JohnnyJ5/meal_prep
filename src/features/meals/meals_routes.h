#pragma once

#include <crow.h>

#include <memory>

#include "core/db/db_manager.h"
#include "core/http/middleware.h"
#include "features/meals/meal_factory.h"

void registerMealsRoutes(crow::App<RequestTimerMiddleware>& app,
                         std::shared_ptr<DBManager> dbManager, MealFactory& factory);
