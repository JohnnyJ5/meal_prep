#include "integrations/google/google_routes.h"

#include <iostream>

void registerGoogleRoutes(crow::App<RequestTimerMiddleware>& app,
                          const std::shared_ptr<GoogleOAuth>& googleOAuth,
                          const std::shared_ptr<CalendarService>& calendarService) {
    // Route: Serve index.html at root (also handles OAuth callbacks landing on /)
    CROW_ROUTE(app, "/")
    ([googleOAuth](const crow::request& req) {
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
        res.set_static_file_info("static/pages/planner/index.html");
        return res;
    });

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
    ([googleOAuth](const crow::request& req) {
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

    // Route: List upcoming events from Google Calendar
    CROW_ROUTE(app, "/api/calendar/events")
    ([calendarService](const crow::request& req) {
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
        .methods(crow::HTTPMethod::POST)([calendarService](const crow::request& req) {
            auto body = crow::json::load(req.body);
            if (!body) {
                CROW_LOG_ERROR << "Invalid JSON for /api/calendar/sync POST";
                return crow::response(400, "Invalid JSON");
            }

            static const std::vector<std::string> kDays = {
                "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};

            int successCount = 0, failCount = 0;
            std::vector<std::string> syncedIds;
            for (const auto& day : kDays) {
                if (!body.has(day)) continue;
                const auto& dayData = body[day];
                if (!dayData.has("date") || !dayData.has("meals")) continue;

                std::string dateStr = dayData["date"].s();
                if (dateStr.empty()) continue;

                const auto& mealsArr = dayData["meals"];
                for (const auto& i : mealsArr) {
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
        .methods(crow::HTTPMethod::POST)([calendarService](const crow::request& req) {
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
        .methods(crow::HTTPMethod::POST)([calendarService](const crow::request& req) {
            auto body = crow::json::load(req.body);
            if (!body || !body.has("event_ids")) {
                return crow::response(400, "Missing event_ids");
            }
            const auto& ids = body["event_ids"];
            int deleted = 0;
            for (const auto& id : ids) {
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
