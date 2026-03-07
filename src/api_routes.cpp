#include "api_routes.h"
#include "meal_planner.h"
#include <iostream>

void setupRoutes(crow::SimpleApp &app, std::shared_ptr<DBManager> dbManager,
                 MealFactory &factory) {
  // Route: Get all available meals
  CROW_ROUTE(app, "/api/meals")([&factory]() {
    std::vector<std::string> meals;
    factory.getAvailableMeals(meals);
    crow::json::wvalue res;
    for (size_t i = 0; i < meals.size(); ++i) {
      res[i] = meals[i];
    }
    return res;
  });

  // Route: Add a new meal
  CROW_ROUTE(app, "/api/meals/add")
      .methods(crow::HTTPMethod::POST)([&dbManager](const crow::request &req) {
        auto body = crow::json::load(req.body);
        if (!body)
          return crow::response(400, "Invalid JSON");

        try {
          std::string mealName = body["name"].s();
          std::vector<Ingredient> ingredients;

          for (const auto &ingJson : body["ingredients"]) {
            std::string ingName = ingJson["name"].s();
            double amount = ingJson["amount"].d();
            int unit = ingJson["unit"].i();
            // Preparation is optional in the JSON
            std::string prep = "None";
            if (ingJson.has("preparation")) {
              prep = ingJson["preparation"].s();
            }

            ingredients.push_back(Ingredient(
                ingName,
                Measurement(amount, static_cast<MeasurementUnit>(unit)), prep));
          }

          Meal newMeal(mealName, ingredients);
          if (dbManager->addMeal(newMeal)) {
            return crow::response(200, "Meal added successfully");
          } else {
            return crow::response(500, "Failed to add meal to database. "
                                       "Name might already exist.");
          }
        } catch (const std::exception &e) {
          return crow::response(400, "Invalid meal data format");
        }
      });

  // Route: Update an existing meal
  CROW_ROUTE(app, "/api/meals/<string>")
      .methods(crow::HTTPMethod::PUT)([&dbManager](const crow::request &req,
                                                   std::string mealName) {
        auto body = crow::json::load(req.body);
        if (!body)
          return crow::response(400, "Invalid JSON");

        try {
          // We expect the body to contain the new ingredients (and
          // optionally a new name, though for simplicity we bind to the
          // URL name)
          std::vector<Ingredient> ingredients;

          for (const auto &ingJson : body["ingredients"]) {
            std::string ingName = ingJson["name"].s();
            double amount = ingJson["amount"].d();
            int unit = ingJson["unit"].i();
            std::string prep = "None";
            if (ingJson.has("preparation")) {
              prep = ingJson["preparation"].s();
            }

            ingredients.push_back(Ingredient(
                ingName,
                Measurement(amount, static_cast<MeasurementUnit>(unit)), prep));
          }

          // Name from URL is used
          Meal updatedMeal(mealName, ingredients);
          if (dbManager->updateMeal(updatedMeal)) {
            return crow::response(200, "Meal updated successfully");
          } else {
            return crow::response(500, "Failed to update meal in database.");
          }
        } catch (const std::exception &e) {
          return crow::response(400, "Invalid meal data format");
        }
      });

  // Route: Delete a meal
  CROW_ROUTE(app, "/api/meals/<string>")
      .methods(crow::HTTPMethod::DELETE)([&dbManager](std::string mealName) {
        if (dbManager->deleteMeal(mealName)) {
          return crow::response(200, "Meal deleted successfully");
        } else {
          return crow::response(404, "Meal not found or could not be deleted");
        }
      });

  // Route: Get a specific meal
  CROW_ROUTE(app, "/api/meals/<string>")
      .methods(crow::HTTPMethod::GET)([&dbManager](std::string mealName) {
        auto meal = dbManager->getMeal(mealName);
        if (meal) {
          crow::json::wvalue res;
          res["name"] = meal->getName();
          for (size_t i = 0; i < meal->getIngredients().size(); ++i) {
            const auto &ing = meal->getIngredients()[i];
            res["ingredients"][i]["name"] = ing.getName();
            res["ingredients"][i]["amount"] = ing.getAmount().getValue();
            res["ingredients"][i]["unit"] =
                static_cast<int>(ing.getAmount().getUnit());
            res["ingredients"][i]["preparation"] = ing.getPreparation();
          }
          return crow::response(std::move(res));
        } else {
          return crow::response(404, "Meal not found");
        }
      });

  // Route: Plan selected meals and trigger email
  CROW_ROUTE(app, "/api/plan")
      .methods(crow::HTTPMethod::POST)([&factory](const crow::request &req) {
        auto body = crow::json::load(req.body);
        if (!body)
          return crow::response(400, "Invalid JSON");

        std::vector<std::unique_ptr<Meal>> createdMeals;
        std::vector<std::reference_wrapper<Meal>> mealRefs;
        std::vector<std::string> failedMeals;
        std::map<std::string, std::vector<std::string>> schedule;

        std::vector<std::string> days = {"Monday",   "Tuesday", "Wednesday",
                                         "Thursday", "Friday",  "Saturday",
                                         "Sunday"};
        for (const auto &day : days) {
          if (body.has(day)) {
            for (const auto &mealJson : body[day]) {
              std::string mealName = mealJson.s();
              schedule[day].push_back(mealName);
              if (auto meal = factory.createMeal(mealName)) {
                createdMeals.push_back(std::move(meal));
              } else {
                failedMeals.push_back(mealName);
              }
            }
          }
        }

        if (createdMeals.empty()) {
          return crow::response(400, "No valid meals selected.");
        }

        for (auto &m : createdMeals) {
          mealRefs.push_back(*m);
        }

        std::map<std::string, Ingredient> allIngredients;
        ConsolidateAllIngredients(allIngredients, mealRefs);

        // Re-route console output to capture schedule response for frontend
        std::stringstream scheduleOutput;
        std::streambuf *oldCoutStreamBuf = std::cout.rdbuf();
        std::cout.rdbuf(scheduleOutput.rdbuf());

        PrintWeeklySchedule(schedule);

        std::cout.rdbuf(oldCoutStreamBuf); // Restore standard output

        // Send Email
        SendPlanEmail(allIngredients, schedule);

        crow::json::wvalue res;
        res["status"] = "success";
        res["schedule"] = scheduleOutput.str();
        if (!failedMeals.empty()) {
          for (size_t i = 0; i < failedMeals.size(); ++i) {
            res["failed_meals"][i] = failedMeals[i];
          }
        }
        return crow::response(std::move(res));
      });

  // Route: Serve index.html at root
  CROW_ROUTE(app, "/")([]() {
    crow::response res;
    res.set_static_file_info("static/index.html");
    return res;
  });

  // Route: Serve all other static files (CSS, JS)
  CROW_ROUTE(app, "/<string>")([](std::string path) {
    crow::response res;
    res.set_static_file_info("static/" + path);
    return res;
  });
}
