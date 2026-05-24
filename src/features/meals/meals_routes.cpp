#include "features/meals/meals_routes.h"

#include <algorithm>
#include <set>
#include <tuple>

#include "features/meals/meal_planner.h"

void registerMealsRoutes(crow::App<RequestTimerMiddleware>& app,
                         std::shared_ptr<MealsRepository> mealsRepo, MealFactory& factory) {
    // Route: Get all available meals
    CROW_ROUTE(app, "/api/meals")
    ([&factory, mealsRepo]() {
        std::vector<std::tuple<int, std::string, std::string>> meals;
        factory.getAvailableMeals(meals);
        std::set<int> idsWithOptional = mealsRepo->getMealIdsWithOptionalIngredients();
        crow::json::wvalue res;
        for (size_t i = 0; i < meals.size(); ++i) {
            int id = std::get<0>(meals[i]);
            res[i]["id"] = id;
            res[i]["name"] = std::get<1>(meals[i]);
            res[i]["category"] = std::get<2>(meals[i]);
            res[i]["has_optional_ingredients"] = idsWithOptional.count(id) > 0;
        }
        CROW_LOG_INFO << "Successfully retrieved " << meals.size() << " available meals";
        return res;
    });

    // Route: Get all available ingredients
    CROW_ROUTE(app, "/api/ingredients")
    ([mealsRepo]() {
        std::vector<std::pair<std::string, std::string>> ingredients;
        mealsRepo->getAllIngredients(ingredients);
        crow::json::wvalue res;
        for (size_t i = 0; i < ingredients.size(); ++i) {
            res[i]["name"] = ingredients[i].first;
            res[i]["category"] = ingredients[i].second;
        }
        CROW_LOG_INFO << "Successfully retrieved " << ingredients.size()
                      << " available ingredients";
        return res;
    });

    // Route: Add a new ingredient
    CROW_ROUTE(app, "/api/ingredients/add")
        .methods(crow::HTTPMethod::POST)([mealsRepo](const crow::request& req) {
            auto body = crow::json::load(req.body);
            if (!body) {
                CROW_LOG_ERROR << "Invalid JSON for /api/ingredients/add POST";
                return crow::response(400, "Invalid JSON");
            }

            if (!body.has("name") || !body.has("category")) {
                CROW_LOG_ERROR << "Missing name or category for /api/ingredients/add POST";
                return crow::response(400, "Missing name or category");
            }

            std::string name = body["name"].s();
            std::string category = body["category"].s();

            if (mealsRepo->addIngredient(name, category)) {
                CROW_LOG_INFO << "Successfully added ingredient: " << name;
                return crow::response(200, "Ingredient added successfully");
            } else {
                CROW_LOG_ERROR << "Failed to add ingredient: " << name;
                return crow::response(500, "Failed to add ingredient");
            }
        });

    // Route: Add a new meal
    CROW_ROUTE(app, "/api/meals/add")
        .methods(crow::HTTPMethod::POST)([mealsRepo](const crow::request& req) {
            auto body = crow::json::load(req.body);
            if (!body) {
                CROW_LOG_ERROR << "Invalid JSON for /api/meals/add POST";
                return crow::response(400, "Invalid JSON");
            }

            try {
                std::string mealName = body["name"].s();
                std::vector<Ingredient> ingredients;

                for (const auto& ingJson : body["ingredients"]) {
                    std::string ingName = ingJson["name"].s();
                    double amount = ingJson["amount"].d();
                    int unit = static_cast<int>(ingJson["unit"].i());
                    std::string prep = "None";
                    if (ingJson.has("preparation")) {
                        prep = ingJson["preparation"].s();
                    }
                    bool isOptional = false;
                    if (ingJson.has("optional")) {
                        isOptional = ingJson["optional"].b();
                    }

                    ingredients.emplace_back(
                        ingName, Measurement(amount, static_cast<MeasurementUnit>(unit)), prep,
                        isOptional);
                }

                std::string category = "Uncategorized";
                if (body.has("category")) {
                    category = body["category"].s();
                }

                Meal newMeal(mealName, ingredients, category);
                if (mealsRepo->addMeal(newMeal)) {
                    CROW_LOG_INFO << "Successfully added meal: " << mealName;
                    return crow::response(200, "Meal added successfully");
                } else {
                    CROW_LOG_ERROR << "Failed to add meal: " << mealName
                                   << " (might already exist)";
                    return crow::response(500,
                                          "Failed to add meal to database. "
                                          "Name might already exist.");
                }
            } catch (const std::exception& e) {
                CROW_LOG_ERROR << "Invalid meal data format in /api/meals/add POST: " << e.what();
                return crow::response(400, "Invalid meal data format");
            }
        });

    // Route: Update an existing meal
    CROW_ROUTE(app, "/api/meals/<string>")
        .methods(crow::HTTPMethod::PUT)(
            [mealsRepo](const crow::request& req, const std::string& mealName) {
                auto body = crow::json::load(req.body);
                if (!body) {
                    CROW_LOG_ERROR << "Invalid JSON for /api/meals/" << mealName << " PUT";
                    return crow::response(400, "Invalid JSON");
                }

                try {
                    std::vector<Ingredient> ingredients;

                    for (const auto& ingJson : body["ingredients"]) {
                        std::string ingName = ingJson["name"].s();
                        double amount = ingJson["amount"].d();
                        int unit = static_cast<int>(ingJson["unit"].i());
                        std::string prep = "None";
                        if (ingJson.has("preparation")) {
                            prep = ingJson["preparation"].s();
                        }
                        bool isOptional = false;
                        if (ingJson.has("optional")) {
                            isOptional = ingJson["optional"].b();
                        }

                        ingredients.emplace_back(
                            ingName, Measurement(amount, static_cast<MeasurementUnit>(unit)), prep,
                            isOptional);
                    }

                    std::string category = "Uncategorized";
                    if (body.has("category")) {
                        category = body["category"].s();
                    }

                    Meal updatedMeal(mealName, ingredients, category);
                    if (mealsRepo->updateMeal(updatedMeal)) {
                        CROW_LOG_INFO << "Successfully updated meal: " << mealName;
                        return crow::response(200, "Meal updated successfully");
                    } else {
                        CROW_LOG_ERROR << "Failed to update meal: " << mealName;
                        return crow::response(500, "Failed to update meal in database.");
                    }
                } catch (const std::exception& e) {
                    CROW_LOG_ERROR << "Invalid meal data format in /api/meals/" << mealName
                                   << " PUT: " << e.what();
                    return crow::response(400, "Invalid meal data format");
                }
            });

    // Route: Delete a meal
    CROW_ROUTE(app, "/api/meals/<string>")
        .methods(crow::HTTPMethod::DELETE)([mealsRepo](const std::string& mealName) {
            if (mealsRepo->deleteMeal(mealName)) {
                CROW_LOG_INFO << "Successfully deleted meal: " << mealName;
                return crow::response(200, "Meal deleted successfully");
            } else {
                CROW_LOG_ERROR << "Failed to delete meal: " << mealName << " (not found or error)";
                return crow::response(404, "Meal not found or could not be deleted");
            }
        });

    // Route: Get a specific meal
    CROW_ROUTE(app, "/api/meals/<string>")
        .methods(crow::HTTPMethod::GET)([mealsRepo](const std::string& mealName) {
            auto meal = mealsRepo->getMeal(mealName);
            if (meal) {
                CROW_LOG_INFO << "Successfully retrieved meal: " << mealName;
                crow::json::wvalue res;
                res["name"] = meal->getName();
                res["category"] = meal->getCategory();
                for (size_t i = 0; i < meal->getIngredients().size(); ++i) {
                    const auto& ing = meal->getIngredients()[i];
                    res["ingredients"][i]["name"] = ing.getName();
                    res["ingredients"][i]["amount"] = ing.getAmount().getValue();
                    res["ingredients"][i]["unit"] = static_cast<int>(ing.getAmount().getUnit());
                    res["ingredients"][i]["preparation"] = ing.getPreparation();
                    res["ingredients"][i]["optional"] = ing.isOptional();
                }
                return crow::response(std::move(res));
            } else {
                CROW_LOG_WARNING << "Meal not found: " << mealName;
                return crow::response(404, "Meal not found");
            }
        });

    // Route: Plan selected meals and trigger email
    CROW_ROUTE(app, "/api/plan")
        .methods(crow::HTTPMethod::POST)([&factory](const crow::request& req) {
            auto body = crow::json::load(req.body);
            if (!body) {
                CROW_LOG_ERROR << "Invalid JSON for /api/plan POST";
                return crow::response(400, "Invalid JSON");
            }

            std::vector<std::unique_ptr<Meal>> createdMeals;
            std::vector<std::reference_wrapper<Meal>> mealRefs;
            std::vector<std::string> failedMeals;
            std::map<std::string, std::vector<std::string>> schedule;

            std::vector<std::string> days = {"Monday", "Tuesday",  "Wednesday", "Thursday",
                                             "Friday", "Saturday", "Sunday"};
            for (const auto& day : days) {
                if (body.has(day)) {
                    for (const auto& mealJson : body[day]) {
                        // Accept either a bare meal name string or
                        // { "name": "...", "add_ons": ["..."] } for meals
                        // that include selected optional ingredients.
                        std::string mealName;
                        std::set<std::string> addOns;
                        if (mealJson.t() == crow::json::type::String) {
                            mealName = mealJson.s();
                        } else {
                            mealName = mealJson["name"].s();
                            if (mealJson.has("add_ons")) {
                                for (const auto& a : mealJson["add_ons"]) {
                                    addOns.insert(std::string(a.s()));
                                }
                            }
                        }
                        schedule[day].push_back(mealName);
                        if (auto meal = factory.createMeal(mealName, addOns)) {
                            createdMeals.push_back(std::move(meal));
                        } else {
                            failedMeals.push_back(mealName);
                        }
                    }
                }
            }

            if (createdMeals.empty()) {
                CROW_LOG_WARNING << "No valid meals selected for plan";
                return crow::response(400, "No valid meals selected.");
            }

            std::transform(createdMeals.begin(), createdMeals.end(), std::back_inserter(mealRefs),
                           [](const auto& m) -> std::reference_wrapper<Meal> { return *m; });

            std::map<std::string, Ingredient> allIngredients;
            ConsolidateAllIngredients(allIngredients, mealRefs);

            std::stringstream ingredientsSS;
            ingredientsSS << "Whole Foods Order - Ingredients:\n";
            for (const auto& pair : allIngredients) {
                ingredientsSS << "- " << pair.second << "\n";
            }

            CROW_LOG_INFO << "Successfully planned meals, failed subset size: "
                          << failedMeals.size();
            crow::json::wvalue res;
            res["status"] = "success";
            res["ingredients_text"] = ingredientsSS.str();
            if (!failedMeals.empty()) {
                for (size_t i = 0; i < failedMeals.size(); ++i) {
                    res["failed_meals"][i] = failedMeals[i];
                }
            }
            return crow::response(std::move(res));
        });

    // Route: Health check
    CROW_ROUTE(app, "/api/health")([]() { return "OK"; });
}
