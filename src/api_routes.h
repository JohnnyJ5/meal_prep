#pragma once

#include "config_parser.h"
#include "db_manager.h"
#include "meal_factory.h"
#include "middleware.h"
#include "google_oauth.h"
#include "calendar_service.h"
#include <crow.h>
#include <memory>

/**
 * @brief Configures all the REST API routes for the Meal Prep application.
 *
 * This function registers the endpoints for viewing, adding, updating, and
 * deleting meals, as well as generating meal plans and serving static frontend
 * files.
 *
 * @param app The Crow web application instance.
 * @param dbManager Shared pointer to the database manager for database
 * operations.
 * @param factory Reference to the meal factory for instantiating meals.
 * @param config The application configuration containing settings such as email
 * credentials.
 */
void setupRoutes(crow::App<RequestTimerMiddleware> &app,
                 std::shared_ptr<DBManager> dbManager, MealFactory &factory,
                 const Config &config, std::shared_ptr<GoogleOAuth> googleOAuth,
                 std::shared_ptr<CalendarService> calendarService);
