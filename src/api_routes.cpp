#include "api_routes.h"

#include <algorithm>
#include <iostream>

#include "meal_planner.h"

void setupRoutes(crow::App<RequestTimerMiddleware> &app, std::shared_ptr<DBManager> dbManager,
                 MealFactory &factory, const Config &config,
                 const std::shared_ptr<GoogleOAuth> &googleOAuth,
                 const std::shared_ptr<CalendarService> &calendarService) {
    // Route: Get all available meals
    CROW_ROUTE(app, "/api/meals")
    ([&factory]() {
        std::vector<std::pair<std::string, std::string>> meals;
        factory.getAvailableMeals(meals);
        crow::json::wvalue res;
        for (size_t i = 0; i < meals.size(); ++i) {
            res[i]["name"] = meals[i].first;
            res[i]["category"] = meals[i].second;
        }
        CROW_LOG_INFO << "Successfully retrieved " << meals.size() << " available meals";
        return res;
    });

    // Route: Get all available ingredients
    CROW_ROUTE(app, "/api/ingredients")
    ([&dbManager]() {
        std::vector<std::pair<std::string, std::string>> ingredients;
        dbManager->getAllIngredients(ingredients);
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
        .methods(crow::HTTPMethod::POST)([&dbManager](const crow::request &req) {
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

            if (dbManager->addIngredient(name, category)) {
                CROW_LOG_INFO << "Successfully added ingredient: " << name;
                return crow::response(200, "Ingredient added successfully");
            } else {
                CROW_LOG_ERROR << "Failed to add ingredient: " << name;
                return crow::response(500, "Failed to add ingredient");
            }
        });

    // Route: Add a new meal
    CROW_ROUTE(app, "/api/meals/add")
        .methods(crow::HTTPMethod::POST)([&dbManager](const crow::request &req) {
            auto body = crow::json::load(req.body);
            if (!body) {
                CROW_LOG_ERROR << "Invalid JSON for /api/meals/add POST";
                return crow::response(400, "Invalid JSON");
            }

            try {
                std::string mealName = body["name"].s();
                std::vector<Ingredient> ingredients;

                for (const auto &ingJson : body["ingredients"]) {
                    std::string ingName = ingJson["name"].s();
                    double amount = ingJson["amount"].d();
                    int unit = static_cast<int>(ingJson["unit"].i());
                    // Preparation is optional in the JSON
                    std::string prep = "None";
                    if (ingJson.has("preparation")) {
                        prep = ingJson["preparation"].s();
                    }

                    ingredients.emplace_back(
                        ingName, Measurement(amount, static_cast<MeasurementUnit>(unit)), prep);
                }

                std::string category = "Uncategorized";
                if (body.has("category")) {
                    category = body["category"].s();
                }

                Meal newMeal(mealName, ingredients, category);
                if (dbManager->addMeal(newMeal)) {
                    CROW_LOG_INFO << "Successfully added meal: " << mealName;
                    return crow::response(200, "Meal added successfully");
                } else {
                    CROW_LOG_ERROR << "Failed to add meal: " << mealName
                                   << " (might already exist)";
                    return crow::response(500,
                                          "Failed to add meal to database. "
                                          "Name might already exist.");
                }
            } catch (const std::exception &e) {
                CROW_LOG_ERROR << "Invalid meal data format in /api/meals/add POST: " << e.what();
                return crow::response(400, "Invalid meal data format");
            }
        });

    // Route: Update an existing meal
    CROW_ROUTE(app, "/api/meals/<string>")
        .methods(crow::HTTPMethod::PUT)(
            [&dbManager](const crow::request &req, const std::string &mealName) {
                auto body = crow::json::load(req.body);
                if (!body) {
                    CROW_LOG_ERROR << "Invalid JSON for /api/meals/" << mealName << " PUT";
                    return crow::response(400, "Invalid JSON");
                }

                try {
                    // We expect the body to contain the new ingredients (and
                    // optionally a new name, though for simplicity we bind to the
                    // URL name)
                    std::vector<Ingredient> ingredients;

                    for (const auto &ingJson : body["ingredients"]) {
                        std::string ingName = ingJson["name"].s();
                        double amount = ingJson["amount"].d();
                        int unit = static_cast<int>(ingJson["unit"].i());
                        std::string prep = "None";
                        if (ingJson.has("preparation")) {
                            prep = ingJson["preparation"].s();
                        }

                        ingredients.emplace_back(
                            ingName, Measurement(amount, static_cast<MeasurementUnit>(unit)), prep);
                    }

                    std::string category = "Uncategorized";
                    if (body.has("category")) {
                        category = body["category"].s();
                    }

                    // Name from URL is used
                    Meal updatedMeal(mealName, ingredients, category);
                    if (dbManager->updateMeal(updatedMeal)) {
                        CROW_LOG_INFO << "Successfully updated meal: " << mealName;
                        return crow::response(200, "Meal updated successfully");
                    } else {
                        CROW_LOG_ERROR << "Failed to update meal: " << mealName;
                        return crow::response(500, "Failed to update meal in database.");
                    }
                } catch (const std::exception &e) {
                    CROW_LOG_ERROR << "Invalid meal data format in /api/meals/" << mealName
                                   << " PUT: " << e.what();
                    return crow::response(400, "Invalid meal data format");
                }
            });

    // Route: Delete a meal
    CROW_ROUTE(app, "/api/meals/<string>")
        .methods(crow::HTTPMethod::DELETE)([&dbManager](const std::string &mealName) {
            if (dbManager->deleteMeal(mealName)) {
                CROW_LOG_INFO << "Successfully deleted meal: " << mealName;
                return crow::response(200, "Meal deleted successfully");
            } else {
                CROW_LOG_ERROR << "Failed to delete meal: " << mealName << " (not found or error)";
                return crow::response(404, "Meal not found or could not be deleted");
            }
        });

    // Route: Get a specific meal
    CROW_ROUTE(app, "/api/meals/<string>")
        .methods(crow::HTTPMethod::GET)([&dbManager](const std::string &mealName) {
            auto meal = dbManager->getMeal(mealName);
            if (meal) {
                CROW_LOG_INFO << "Successfully retrieved meal: " << mealName;
                crow::json::wvalue res;
                res["name"] = meal->getName();
                res["category"] = meal->getCategory();
                for (size_t i = 0; i < meal->getIngredients().size(); ++i) {
                    const auto &ing = meal->getIngredients()[i];
                    res["ingredients"][i]["name"] = ing.getName();
                    res["ingredients"][i]["amount"] = ing.getAmount().getValue();
                    res["ingredients"][i]["unit"] = static_cast<int>(ing.getAmount().getUnit());
                    res["ingredients"][i]["preparation"] = ing.getPreparation();
                }
                return crow::response(std::move(res));
            } else {
                CROW_LOG_WARNING << "Meal not found: " << mealName;
                return crow::response(404, "Meal not found");
            }
        });

    // Route: Plan selected meals and trigger email
    CROW_ROUTE(app, "/api/plan")
        .methods(crow::HTTPMethod::POST)([&factory](const crow::request &req) {
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
                CROW_LOG_WARNING << "No valid meals selected for plan";
                return crow::response(400, "No valid meals selected.");
            }

            std::transform(createdMeals.begin(), createdMeals.end(), std::back_inserter(mealRefs),
                           [](const auto &m) -> std::reference_wrapper<Meal> { return *m; });

            std::map<std::string, Ingredient> allIngredients;
            ConsolidateAllIngredients(allIngredients, mealRefs);

            std::stringstream ingredientsSS;
            ingredientsSS << "Whole Foods Order - Ingredients:\n";
            for (const auto &pair : allIngredients) {
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

    // Route: Serve index.html at root
    CROW_ROUTE(app, "/")
    ([googleOAuth](const crow::request &req) {
        if (auto code = req.url_params.get("code")) {
            CROW_LOG_INFO << "Received OAuth code at root route. Exchanging...";
            auto stateParam = req.url_params.get("state");
            if (!stateParam || !googleOAuth->validateState(stateParam)) {
                CROW_LOG_ERROR << "OAuth state validation failed at root callback";
                return crow::response(400, "Invalid OAuth state");
            }
            if (googleOAuth->exchangeCodeForTokens(code)) {
                CROW_LOG_INFO << "Successfully exchanged authorization code for tokens";
            } else {
                CROW_LOG_ERROR << "Failed to exchange authorization code";
            }
            crow::response res;
            res.redirect("/");
            return res;
        }

        if (auto error = req.url_params.get("error")) {
            CROW_LOG_ERROR << "OAuth Error received at root: " << error;
        }

        crow::response res;
        res.set_static_file_info("static/index.html");
        return res;
    });

    // Route: Serve all other static files (CSS, JS)
    CROW_ROUTE(app, "/<string>")
    ([](const std::string &path) {
        if (path.find("..") != std::string::npos || path.find('/') != std::string::npos) {
            return crow::response(400, "Invalid path");
        }
        crow::response res;
        res.set_static_file_info("static/" + path);
        if (res.code == 404) {
            std::cerr << "Static file not found: static/" << path << std::endl;
        }
        return res;
    });

    // --- Google OAuth2 Routes ---

    // Route: Redirect to Google OAuth2 consent screen
    CROW_ROUTE(app, "/auth/google")
    ([googleOAuth]() {
        auto url = googleOAuth->getAuthUrl();
        crow::response res;
        res.redirect(url);
        return res;
    });

    // Route: Handle the callback from Google
    CROW_ROUTE(app, "/auth/google/callback")
    ([googleOAuth](const crow::request &req) {
        auto code = req.url_params.get("code");
        if (!code) {
            CROW_LOG_ERROR << "Authorization code not found in /auth/google/callback";
            return crow::response(400, "Authorization code not found");
        }

        auto stateParam = req.url_params.get("state");
        if (!stateParam || !googleOAuth->validateState(stateParam)) {
            CROW_LOG_ERROR << "OAuth state validation failed in /auth/google/callback";
            return crow::response(400, "Invalid OAuth state");
        }

        if (googleOAuth->exchangeCodeForTokens(code)) {
            CROW_LOG_INFO << "Successfully exchanged authorization code for tokens";
            crow::response res;
            res.redirect("/");
            return res;
        } else {
            CROW_LOG_ERROR << "Failed to exchange authorization code";
            return crow::response(500, "Failed to exchange authorization code");
        }
    });

    // --- Google Calendar Routes ---

    // Route: List upcoming events from Google Calendar
    CROW_ROUTE(app, "/api/calendar/events")
    ([calendarService](const crow::request &req) {
        std::string timeMin = "";
        std::string timeMax = "";
        if (req.url_params.get("timeMin")) timeMin = req.url_params.get("timeMin");
        if (req.url_params.get("timeMax")) timeMax = req.url_params.get("timeMax");

        auto eventsList = calendarService->listEvents(timeMin, timeMax);
        if (eventsList.empty()) {
            CROW_LOG_WARNING << "Failed to fetch events or no events found";
            crow::response err(403);
            err.set_header("Content-Type", "application/json");
            err.body =
                R"({"linked":false,"message":"Google account not linked or error fetching events"})";
            return err;
        }

        crow::json::wvalue res;
        res = crow::json::wvalue::list();
        for (size_t i = 0; i < eventsList.size(); ++i) {
            crow::json::wvalue cal;
            cal["summary"] = eventsList[i].summary;
            cal["backgroundColor"] = eventsList[i].backgroundColor;
            cal["foregroundColor"] = eventsList[i].foregroundColor;
            cal["events"] = crow::json::load(eventsList[i].eventsJson);
            res[i] = std::move(cal);
        }

        std::string dumped = res.dump();
        CROW_LOG_INFO << "Successfully fetched events from " << eventsList.size() << " calendars";

        crow::response res_final;
        res_final.code = 200;
        res_final.set_header("Content-Type", "application/json");
        res_final.body = std::move(dumped);
        return res_final;
    });

    // Route: Add a meal plan to Google Calendar
    CROW_ROUTE(app, "/api/calendar/sync")
        .methods(crow::HTTPMethod::POST)([calendarService](const crow::request &req) {
            auto body = crow::json::load(req.body);
            if (!body) {
                CROW_LOG_ERROR << "Invalid JSON for /api/calendar/sync POST";
                return crow::response(400, "Invalid JSON");
            }

            // Body is { "Monday": { "date": "YYYY-MM-DD", "meals": [...] }, ... }
            static const std::vector<std::string> kDays = {
                "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};

            int successCount = 0, failCount = 0;
            std::vector<std::string> syncedIds;
            for (const auto &day : kDays) {
                if (!body.has(day)) continue;
                const auto &dayData = body[day];
                if (!dayData.has("date") || !dayData.has("meals")) continue;

                std::string dateStr = dayData["date"].s();  // "YYYY-MM-DD"
                if (dateStr.empty()) continue;

                const auto &mealsArr = dayData["meals"];
                for (const auto &i : mealsArr) {
                    std::string mealName = i.s();
                    if (mealName.empty()) continue;

                    std::string startTime = dateStr + "T18:00:00Z";
                    std::string endTime = dateStr + "T19:00:00Z";
                    std::string eventId = calendarService->createEvent(
                        mealName, "Meal planned via app", startTime, endTime);
                    if (!eventId.empty()) {
                        ++successCount;
                        syncedIds.push_back(eventId);
                    } else {
                        ++failCount;
                    }
                }
            }

            if (failCount > 0 && successCount == 0) {
                CROW_LOG_ERROR << "Failed to sync any meals to Google Calendar";
                crow::response err(403);
                err.set_header("Content-Type", "application/json");
                err.body =
                    R"({"linked":false,"message":"Google account not linked or authorization failed"})";
                return err;
            }

            CROW_LOG_INFO << "Synced " << successCount << " meal(s) to Google Calendar";
            crow::json::wvalue result;
            result["synced"] = successCount;
            result["failed"] = failCount;
            for (size_t i = 0; i < syncedIds.size(); ++i) {
                result["event_ids"][i] = syncedIds[i];
            }
            crow::response ok(200);
            ok.set_header("Content-Type", "application/json");
            ok.body = result.dump();
            return ok;
        });

    // Route: Create Whole Foods order calendar event
    CROW_ROUTE(app, "/api/calendar/order")
        .methods(crow::HTTPMethod::POST)([calendarService](const crow::request &req) {
            auto body = crow::json::load(req.body);
            if (!body || !body.has("start") || !body.has("end") || !body.has("ingredients")) {
                return crow::response(400, "Missing fields: start, end, ingredients required");
            }
            std::string start = body["start"].s();
            std::string end = body["end"].s();
            std::string ingredients = body["ingredients"].s();

            std::string eventId =
                calendarService->createEvent("Whole Foods Order", ingredients, start, end, true);
            if (!eventId.empty()) {
                CROW_LOG_INFO << "Created Whole Foods order calendar event";
                crow::json::wvalue result;
                result["status"] = "success";
                crow::response ok(200);
                ok.set_header("Content-Type", "application/json");
                ok.body = result.dump();
                return ok;
            }
            CROW_LOG_ERROR << "Failed to create Whole Foods order calendar event";
            crow::response err(403);
            err.set_header("Content-Type", "application/json");
            err.body = R"({"error":"Failed to create event — is Google account linked?"})";
            return err;
        });

    // Route: Delete calendar events by ID (used to undo a plan sync)
    CROW_ROUTE(app, "/api/calendar/delete-events")
        .methods(crow::HTTPMethod::POST)([calendarService](const crow::request &req) {
            auto body = crow::json::load(req.body);
            if (!body || !body.has("event_ids")) {
                return crow::response(400, "Missing event_ids");
            }
            const auto &ids = body["event_ids"];
            int deleted = 0;
            for (const auto &id : ids) {
                if (calendarService->deleteEvent(std::string(id.s()))) {
                    ++deleted;  // cppcheck-suppress useStlAlgorithm
                }
            }
            CROW_LOG_INFO << "Deleted " << deleted << " calendar event(s)";
            crow::json::wvalue result;
            result["deleted"] = deleted;
            crow::response ok(200);
            ok.set_header("Content-Type", "application/json");
            ok.body = result.dump();
            return ok;
        });
}
