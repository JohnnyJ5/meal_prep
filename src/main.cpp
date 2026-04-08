#include <crow.h>
#include <curl/curl.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "api_routes.h"
#include "calendar_service.h"
#include "config_parser.h"
#include "db_manager.h"
#include "google_oauth.h"
#include "meal_factory.h"
#include "meal_planner.h"
#include "middleware.h"

struct CurlGlobalGuard {
    CurlGlobalGuard() { curl_global_init(CURL_GLOBAL_DEFAULT); }
    ~CurlGlobalGuard() { curl_global_cleanup(); }
};

int main(int argc, char **argv) {
    try {
        CurlGlobalGuard curlGuard;

        // Simple command line parsing for now
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

        // Initialize Database
        auto dbManager = std::make_shared<DBManager>(config.db_path);
        if (!dbManager->initializeSchema()) {
            std::cerr << "Failed to initialize database schema" << std::endl;
            return 1;
        }
        dbManager->seedDefaultMeals();
        dbManager->seedDefaultIngredients();

        MealFactory factory(dbManager);

        if (serveWeb) {
            auto googleOAuth = std::make_shared<GoogleOAuth>(config, dbManager);
            auto calendarService = std::make_shared<CalendarService>(googleOAuth);

            crow::App<RequestTimerMiddleware> app;
            setupRoutes(app, dbManager, factory, config, googleOAuth, calendarService);

            // Start the server
            std::cout << "Starting Meal Prep API on http://0.0.0.0:" << config.port << std::endl;
            app.bindaddr("0.0.0.0").port(config.port).multithreaded().run();
            return 0;
        }

        if (listMeals) {
            std::cout << "Available meals:" << std::endl;
            std::vector<std::pair<std::string, std::string>> meals;
            factory.getAvailableMeals(meals);
            for (const auto &mealPair : meals) {
                std::cout << "-m " << mealPair.first << " [" << mealPair.second << "]" << std::endl;
            }
            return 0;
        }

        // Create meals using factory
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

        // Convert to vector of references for consolidation
        std::vector<std::reference_wrapper<Meal>> mealRefs;
        std::map<std::string, std::vector<std::string>> schedule;
        for (const auto &meal : meals) {
            mealRefs.emplace_back(*meal);
            schedule["Monday"].push_back(meal->getName());  // Dummy assignment for CLI
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