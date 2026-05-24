#pragma once

#include <crow.h>

#include <memory>

#include "core/http/middleware.h"
#include "features/meals/meal_factory.h"
#include "features/meals/meals_repository.h"

void registerMealsRoutes(crow::App<RequestTimerMiddleware>& app,
                         std::shared_ptr<MealsRepository> mealsRepo, MealFactory& factory);
