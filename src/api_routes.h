#pragma once

#include "config_parser.h"
#include "db_manager.h"
#include "meal_factory.h"
#include <crow.h>
#include <memory>

void setupRoutes(crow::SimpleApp &app, std::shared_ptr<DBManager> dbManager,
                 MealFactory &factory, const Config &config);
