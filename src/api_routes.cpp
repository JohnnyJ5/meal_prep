#include "api_routes.h"
#include "meal_planner.h"
#include <iostream>

void setupRoutes(crow::SimpleApp &app, std::shared_ptr<DBManager> dbManager,
                 MealFactory &factory, const Config &config) {
  // Route: Get all available meals
  CROW_ROUTE(app, "/api/meals")([&factory]() {
    std::vector<std::pair<std::string, std::string>> meals;
    factory.getAvailableMeals(meals);
    crow::json::wvalue res;
    for (size_t i = 0; i < meals.size(); ++i) {
      res[i]["name"] = meals[i].first;
      res[i]["category"] = meals[i].second;
    }
    return res;
  });

  // Route: Get all available ingredients
  CROW_ROUTE(app, "/api/ingredients")([&dbManager]() {
    std::vector<std::pair<std::string, std::string>> ingredients;
    dbManager->getAllIngredients(ingredients);
    crow::json::wvalue res;
    for (size_t i = 0; i < ingredients.size(); ++i) {
      res[i]["name"] = ingredients[i].first;
      res[i]["category"] = ingredients[i].second;
    }
    return res;
  });

  // Route: Add a new ingredient
  CROW_ROUTE(app, "/api/ingredients/add")
      .methods(crow::HTTPMethod::POST)([&dbManager](const crow::request &req) {
        auto body = crow::json::load(req.body);
        if (!body)
          return crow::response(400, "Invalid JSON");

        if (!body.has("name") || !body.has("category")) {
          return crow::response(400, "Missing name or category");
        }

        std::string name = body["name"].s();
        std::string category = body["category"].s();

        if (dbManager->addIngredient(name, category)) {
          return crow::response(200, "Ingredient added successfully");
        } else {
          return crow::response(500, "Failed to add ingredient");
        }
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

          std::string category = "Uncategorized";
          if (body.has("category")) {
            category = body["category"].s();
          }

          Meal newMeal(mealName, ingredients, category);
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

          std::string category = "Uncategorized";
          if (body.has("category")) {
            category = body["category"].s();
          }

          // Name from URL is used
          Meal updatedMeal(mealName, ingredients, category);
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
          res["category"] = meal->getCategory();
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
      .methods(crow::HTTPMethod::POST)(
          [&factory, &config](const crow::request &req) {
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

            std::stringstream scheduleOutput;
            PrintWeeklySchedule(scheduleOutput, schedule);

            // Send Email
            SendPlanEmail(allIngredients, schedule, config);

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

  // Route: Health check
  CROW_ROUTE(app, "/api/health")([]() {
    return "OK";
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
    if (res.code == 404) {
      std::cerr << "Static file not found: static/" << path << std::endl;
    }
    return res;
  });
}
