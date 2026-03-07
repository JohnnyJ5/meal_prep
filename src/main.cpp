#include "api_routes.h"
#include "config_parser.h"
#include "db_manager.h"
#include "meal_factory.h"
#include "meal_planner.h"
#include <crow.h>
#include <curl/curl.h>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

int main(int argc, char **argv) {
  try {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    // Simple command line parsing for now
    std::vector<std::string> mealNames;
    bool listMeals = false;
    bool serveWeb = false;

    for (int i = 1; i < argc; ++i) {
      std::string arg = argv[i];
      if (arg == "--list" || arg == "-l") {
        listMeals = true;
      } else if ((arg == "--meal" || arg == "-m") && i + 1 < argc) {
        mealNames.push_back(argv[++i]);
      } else if (arg == "--serve" || arg == "-s") {
        serveWeb = true;
      }
    }

    // Initialize Database
    auto dbManager = std::make_shared<DBManager>("meals.db");
    if (!dbManager->initializeSchema()) {
      std::cerr << "Failed to initialize database schema" << std::endl;
      return 1;
    }
    dbManager->seedDefaultMeals();

    MealFactory factory(dbManager);

    Config config = loadConfig("meal_prep.conf.json");

    if (serveWeb) {
      crow::SimpleApp app;
      setupRoutes(app, dbManager, factory, config);

      // Start the server
      std::cout << "Starting Meal Prep API on http://0.0.0.0:" << config.port
                << std::endl;
      app.port(config.port).multithreaded().run();
      return 0;
    }

    if (listMeals) {
      std::cout << "Available meals:" << std::endl;
      std::vector<std::string> meals;
      factory.getAvailableMeals(meals);
      for (const auto &mealName : meals) {
        std::cout << "-m " << mealName << std::endl;
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
    for (auto &meal : meals) {
      mealRefs.push_back(*meal);
      schedule["Monday"].push_back(meal->getName()); // Dummy assignment for CLI
    }

    std::map<std::string, Ingredient> allIngredients;
    ConsolidateAllIngredients(allIngredients, mealRefs);
    SendPlanEmail(allIngredients, schedule, config);
    PrintWeeklySchedule(std::cout, schedule);

    curl_global_cleanup();
  } catch (const std::exception &e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}