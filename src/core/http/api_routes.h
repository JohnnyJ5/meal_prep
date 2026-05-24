#pragma once

#include <crow.h>

#include <memory>

#include "core/config/config_parser.h"
#include "core/db/db_manager.h"
#include "core/http/middleware.h"
#include "features/meals/meal_factory.h"
#include "integrations/google/calendar_service.h"
#include "integrations/google/google_oauth.h"

/**
 * @brief Registers every HTTP route on the Crow app.
 *
 * Aggregates per-feature route modules (meals, workouts, Google integration,
 * static files). New feature modules should add their own
 * `register<Feature>Routes()` call here.
 */
void setupRoutes(crow::App<RequestTimerMiddleware>& app, std::shared_ptr<DBManager> dbManager,
                 MealFactory& factory, const Config& config,
                 const std::shared_ptr<GoogleOAuth>& googleOAuth,
                 const std::shared_ptr<CalendarService>& calendarService);
