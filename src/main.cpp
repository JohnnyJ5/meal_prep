#include <crow.h>
#include <curl/curl.h>

#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "core/config/config_parser.h"
#include "core/db/db_connection.h"
#include "core/db/schema.h"
#include "core/http/api_routes.h"
#include "core/http/middleware.h"
#include "features/meals/meal_factory.h"
#include "features/meals/meal_planner.h"
#include "features/meals/meals_repository.h"
#include "features/workouts/workouts_repository.h"
#include "integrations/google/calendar_service.h"
#include "integrations/google/google_oauth.h"
#include "integrations/google/google_tokens_repository.h"

struct CurlGlobalGuard {
    CurlGlobalGuard() { curl_global_init(CURL_GLOBAL_DEFAULT); }
    ~CurlGlobalGuard() { curl_global_cleanup(); }
};

int main(int argc, char **argv) {
    try {
        CurlGlobalGuard curlGuard;

        std::vector<std::string> mealNames;
        bool listMeals = false;
        bool serveWeb = false;

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--list" || arg == "-l") {
                listMeals = true;
            } else if ((arg == "--meal" || arg == "-m") && i + 1 < argc) {
                mealNames.emplace_back(argv[++i]);
            } else if (arg == "--serve" || arg == "-s") {
                serveWeb = true;
            }
        }

        Config config = loadConfig("meal_prep.conf.json");

        auto db = std::make_shared<DbConnection>(config.db_path);
        if (!initializeSchema(*db)) {
            std::cerr << "Failed to initialize database schema" << std::endl;
            return 1;
        }

        auto mealsRepo = std::make_shared<MealsRepository>(db);
        auto workoutsRepo = std::make_shared<WorkoutsRepository>(db);
        auto tokensRepo = std::make_shared<GoogleTokensRepository>(db);

        mealsRepo->seedDefaultMeals();
        mealsRepo->seedDefaultIngredients();

        MealFactory factory(mealsRepo);

        if (serveWeb) {
            auto googleOAuth = std::make_shared<GoogleOAuth>(config, tokensRepo);
            auto calendarService = std::make_shared<CalendarService>(googleOAuth);

            crow::App<RequestTimerMiddleware> app;
            setupRoutes(app, mealsRepo, workoutsRepo, factory, googleOAuth, calendarService);

            std::cout << "Starting Meal Prep API on http://0.0.0.0:" << config.port << std::endl;
            app.bindaddr("0.0.0.0").port(config.port).multithreaded().run();
            return 0;
        }

        if (listMeals) {
            std::cout << "Available meals:" << std::endl;
            std::vector<std::tuple<int, std::string, std::string>> meals;
            factory.getAvailableMeals(meals);
            for (const auto &mealTuple : meals) {
                std::cout << "-m " << std::get<1>(mealTuple) << " [" << std::get<2>(mealTuple)
                          << "]" << std::endl;
            }
            return 0;
        }

        std::vector<std::unique_ptr<Meal>> meals;
        if (!mealNames.empty()) {
            for (const auto &mealName : mealNames) {
                auto meal = factory.createMeal(mealName);
                if (meal) {
                    meals.push_back(std::move(meal));
                } else {
                    std::cout << "Unknown meal: " << mealName << std::endl;
                    return 1;
                }
            }
        } else {
            std::cout << "No meals to create" << std::endl;
            return 0;
        }

        std::vector<std::reference_wrapper<Meal>> mealRefs;
        std::map<std::string, std::vector<std::string>> schedule;
        for (const auto &meal : meals) {
            mealRefs.emplace_back(*meal);
            schedule["Monday"].push_back(meal->getName());
        }

        std::map<std::string, Ingredient> allIngredients;
        ConsolidateAllIngredients(allIngredients, mealRefs);
        PrintWeeklySchedule(std::cout, schedule);

    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
