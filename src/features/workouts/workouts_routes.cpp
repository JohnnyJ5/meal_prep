#include "features/workouts/workouts_routes.h"

#include <ctime>

#include "features/workouts/workout.h"

void registerWorkoutsRoutes(crow::App<RequestTimerMiddleware>& app,
                            std::shared_ptr<WorkoutsRepository> workouts) {
    auto workoutToJson = [](const Workout& w) {
        crow::json::wvalue res;
        res["id"] = w.id;
        res["name"] = w.name;
        res["performed_on"] = w.performed_on;
        res["duration_seconds"] = w.duration_seconds;
        res["notes"] = w.notes;
        res["created_at"] = static_cast<int64_t>(w.created_at);
        for (size_t bi = 0; bi < w.blocks.size(); ++bi) {
            const auto& b = w.blocks[bi];
            res["blocks"][bi]["id"] = b.id;
            res["blocks"][bi]["position"] = b.position;
            res["blocks"][bi]["block_type"] = blockTypeToString(b.type);
            res["blocks"][bi]["rounds"] = b.rounds;
            res["blocks"][bi]["rest_seconds"] = b.rest_seconds;
            for (size_t ei = 0; ei < b.exercises.size(); ++ei) {
                const auto& e = b.exercises[ei];
                res["blocks"][bi]["exercises"][ei]["id"] = e.id;
                res["blocks"][bi]["exercises"][ei]["position"] = e.position;
                res["blocks"][bi]["exercises"][ei]["name"] = e.name;
                res["blocks"][bi]["exercises"][ei]["exercise_type"] = exerciseTypeToString(e.type);
                res["blocks"][bi]["exercises"][ei]["sets"] = e.sets;
                res["blocks"][bi]["exercises"][ei]["reps"] = e.reps;
                res["blocks"][bi]["exercises"][ei]["weight_lbs"] = e.weight_lbs;
                res["blocks"][bi]["exercises"][ei]["distance"] = e.distance;
                res["blocks"][bi]["exercises"][ei]["distance_unit"] = e.distance_unit;
                res["blocks"][bi]["exercises"][ei]["duration_seconds"] = e.duration_seconds;
                res["blocks"][bi]["exercises"][ei]["rest_seconds"] = e.rest_seconds;
            }
        }
        return res;
    };

    auto workoutFromJson = [](const crow::json::rvalue& body) {
        Workout w;
        if (body.has("name")) w.name = std::string(body["name"].s());
        if (body.has("performed_on")) w.performed_on = std::string(body["performed_on"].s());
        if (body.has("duration_seconds")) {
            w.duration_seconds = static_cast<int>(body["duration_seconds"].i());
        }
        if (body.has("notes")) w.notes = std::string(body["notes"].s());
        if (body.has("blocks")) {
            for (const auto& bJson : body["blocks"]) {
                WorkoutBlock b;
                if (bJson.has("block_type")) {
                    b.type = blockTypeFromString(std::string(bJson["block_type"].s()));
                }
                if (bJson.has("rounds")) b.rounds = static_cast<int>(bJson["rounds"].i());
                if (b.rounds < 1) b.rounds = 1;
                if (bJson.has("rest_seconds")) {
                    b.rest_seconds = static_cast<int>(bJson["rest_seconds"].i());
                }
                if (bJson.has("exercises")) {
                    for (const auto& eJson : bJson["exercises"]) {
                        WorkoutExercise e;
                        if (eJson.has("name")) e.name = std::string(eJson["name"].s());
                        if (eJson.has("exercise_type")) {
                            e.type = exerciseTypeFromString(
                                std::string(eJson["exercise_type"].s()));
                        }
                        if (eJson.has("sets")) e.sets = static_cast<int>(eJson["sets"].i());
                        if (eJson.has("reps")) e.reps = static_cast<int>(eJson["reps"].i());
                        if (eJson.has("weight_lbs")) e.weight_lbs = eJson["weight_lbs"].d();
                        if (eJson.has("distance")) e.distance = eJson["distance"].d();
                        if (eJson.has("distance_unit")) {
                            e.distance_unit = std::string(eJson["distance_unit"].s());
                        }
                        if (eJson.has("duration_seconds")) {
                            e.duration_seconds = static_cast<int>(eJson["duration_seconds"].i());
                        }
                        if (eJson.has("rest_seconds")) {
                            e.rest_seconds = static_cast<int>(eJson["rest_seconds"].i());
                        }
                        b.exercises.push_back(std::move(e));
                    }
                }
                w.blocks.push_back(std::move(b));
            }
        }
        return w;
    };

    // Route: List all workouts (summary)
    CROW_ROUTE(app, "/api/workouts")
        .methods(crow::HTTPMethod::GET)([workouts]() {
            auto list = workouts->listWorkouts();
            crow::json::wvalue res = crow::json::wvalue::list();
            for (size_t i = 0; i < list.size(); ++i) {
                res[i]["id"] = list[i].id;
                res[i]["name"] = list[i].name;
                res[i]["performed_on"] = list[i].performed_on;
                res[i]["duration_seconds"] = list[i].duration_seconds;
                res[i]["exercise_count"] = list[i].exercise_count;
            }
            CROW_LOG_INFO << "Listed " << list.size() << " workout(s)";
            return crow::response(std::move(res));
        });

    // Route: Get one workout (full nested document)
    CROW_ROUTE(app, "/api/workouts/<int>")
        .methods(crow::HTTPMethod::GET)([workouts, workoutToJson](int id) {
            Workout w = workouts->getWorkout(id);
            if (w.id == 0) {
                return crow::response(404, "Workout not found");
            }
            return crow::response(workoutToJson(w));
        });

    // Route: Create a workout
    CROW_ROUTE(app, "/api/workouts")
        .methods(crow::HTTPMethod::POST)(
            [workouts, workoutFromJson, workoutToJson](const crow::request& req) {
                auto body = crow::json::load(req.body);
                if (!body) return crow::response(400, "Invalid JSON");
                try {
                    Workout w = workoutFromJson(body);
                    if (w.performed_on.empty()) {
                        return crow::response(400, "performed_on is required");
                    }
                    w.created_at = static_cast<int64_t>(std::time(nullptr));
                    if (!workouts->addWorkout(w)) {
                        return crow::response(500, "Failed to save workout");
                    }
                    CROW_LOG_INFO << "Saved workout id=" << w.id << " name=" << w.name;
                    return crow::response(workoutToJson(workouts->getWorkout(w.id)));
                } catch (const std::exception& e) {
                    CROW_LOG_ERROR << "Invalid workout JSON: " << e.what();
                    return crow::response(400, "Invalid workout data");
                }
            });

    // Route: Update a workout
    CROW_ROUTE(app, "/api/workouts/<int>")
        .methods(crow::HTTPMethod::PUT)(
            [workouts, workoutFromJson, workoutToJson](const crow::request& req, int id) {
                auto body = crow::json::load(req.body);
                if (!body) return crow::response(400, "Invalid JSON");
                try {
                    Workout w = workoutFromJson(body);
                    w.id = id;
                    if (w.performed_on.empty()) {
                        return crow::response(400, "performed_on is required");
                    }
                    if (!workouts->updateWorkout(w)) {
                        return crow::response(500, "Failed to update workout");
                    }
                    CROW_LOG_INFO << "Updated workout id=" << id;
                    return crow::response(workoutToJson(workouts->getWorkout(id)));
                } catch (const std::exception& e) {
                    CROW_LOG_ERROR << "Invalid workout JSON: " << e.what();
                    return crow::response(400, "Invalid workout data");
                }
            });

    // Route: Delete a workout
    CROW_ROUTE(app, "/api/workouts/<int>")
        .methods(crow::HTTPMethod::DELETE)([workouts](int id) {
            if (!workouts->deleteWorkout(id)) {
                return crow::response(500, "Failed to delete workout");
            }
            CROW_LOG_INFO << "Deleted workout id=" << id;
            return crow::response(200, "Workout deleted");
        });

    // --- Workout Template Routes ---

    auto templateToJson = [](const WorkoutTemplate& t) {
        crow::json::wvalue res;
        res["id"] = t.id;
        res["name"] = t.name;
        res["created_at"] = static_cast<int64_t>(t.created_at);
        for (size_t bi = 0; bi < t.blocks.size(); ++bi) {
            const auto& b = t.blocks[bi];
            res["blocks"][bi]["id"] = b.id;
            res["blocks"][bi]["position"] = b.position;
            res["blocks"][bi]["block_type"] = blockTypeToString(b.type);
            res["blocks"][bi]["rounds"] = b.rounds;
            res["blocks"][bi]["rest_seconds"] = b.rest_seconds;
            for (size_t ei = 0; ei < b.exercises.size(); ++ei) {
                const auto& e = b.exercises[ei];
                res["blocks"][bi]["exercises"][ei]["id"] = e.id;
                res["blocks"][bi]["exercises"][ei]["position"] = e.position;
                res["blocks"][bi]["exercises"][ei]["name"] = e.name;
                res["blocks"][bi]["exercises"][ei]["exercise_type"] = exerciseTypeToString(e.type);
                res["blocks"][bi]["exercises"][ei]["sets"] = e.sets;
                res["blocks"][bi]["exercises"][ei]["reps"] = e.reps;
                res["blocks"][bi]["exercises"][ei]["weight_lbs"] = e.weight_lbs;
                res["blocks"][bi]["exercises"][ei]["distance"] = e.distance;
                res["blocks"][bi]["exercises"][ei]["distance_unit"] = e.distance_unit;
                res["blocks"][bi]["exercises"][ei]["duration_seconds"] = e.duration_seconds;
                res["blocks"][bi]["exercises"][ei]["rest_seconds"] = e.rest_seconds;
            }
        }
        return res;
    };

    auto templateFromJson = [workoutFromJson](const crow::json::rvalue& body) {
        WorkoutTemplate t;
        if (body.has("name")) t.name = std::string(body["name"].s());
        // Reuse workout's block parser: workoutFromJson reads "blocks" and ignores the
        // workout-only fields if absent.
        Workout w = workoutFromJson(body);
        t.blocks = std::move(w.blocks);
        return t;
    };

    // Route: List all templates
    CROW_ROUTE(app, "/api/workout-templates")
        .methods(crow::HTTPMethod::GET)([workouts]() {
            auto list = workouts->listTemplates();
            crow::json::wvalue res = crow::json::wvalue::list();
            for (size_t i = 0; i < list.size(); ++i) {
                res[i]["id"] = list[i].id;
                res[i]["name"] = list[i].name;
                res[i]["exercise_count"] = list[i].exercise_count;
            }
            CROW_LOG_INFO << "Listed " << list.size() << " template(s)";
            return crow::response(std::move(res));
        });

    // Route: Get one template
    CROW_ROUTE(app, "/api/workout-templates/<int>")
        .methods(crow::HTTPMethod::GET)([workouts, templateToJson](int id) {
            WorkoutTemplate t = workouts->getTemplate(id);
            if (t.id == 0) return crow::response(404, "Template not found");
            return crow::response(templateToJson(t));
        });

    // Route: Create a template
    CROW_ROUTE(app, "/api/workout-templates")
        .methods(crow::HTTPMethod::POST)(
            [workouts, templateFromJson, templateToJson](const crow::request& req) {
                auto body = crow::json::load(req.body);
                if (!body) return crow::response(400, "Invalid JSON");
                try {
                    WorkoutTemplate t = templateFromJson(body);
                    if (t.name.empty()) return crow::response(400, "name is required");
                    t.created_at = static_cast<int64_t>(std::time(nullptr));
                    if (!workouts->addTemplate(t)) {
                        return crow::response(500,
                                              "Failed to save template (name may already exist)");
                    }
                    CROW_LOG_INFO << "Saved template id=" << t.id << " name=" << t.name;
                    return crow::response(templateToJson(workouts->getTemplate(t.id)));
                } catch (const std::exception& e) {
                    CROW_LOG_ERROR << "Invalid template JSON: " << e.what();
                    return crow::response(400, "Invalid template data");
                }
            });

    // Route: Update a template
    CROW_ROUTE(app, "/api/workout-templates/<int>")
        .methods(crow::HTTPMethod::PUT)(
            [workouts, templateFromJson, templateToJson](const crow::request& req, int id) {
                auto body = crow::json::load(req.body);
                if (!body) return crow::response(400, "Invalid JSON");
                try {
                    WorkoutTemplate t = templateFromJson(body);
                    t.id = id;
                    if (t.name.empty()) return crow::response(400, "name is required");
                    if (!workouts->updateTemplate(t)) {
                        return crow::response(500, "Failed to update template");
                    }
                    CROW_LOG_INFO << "Updated template id=" << id;
                    return crow::response(templateToJson(workouts->getTemplate(id)));
                } catch (const std::exception& e) {
                    CROW_LOG_ERROR << "Invalid template JSON: " << e.what();
                    return crow::response(400, "Invalid template data");
                }
            });

    // Route: Delete a template
    CROW_ROUTE(app, "/api/workout-templates/<int>")
        .methods(crow::HTTPMethod::DELETE)([workouts](int id) {
            if (!workouts->deleteTemplate(id)) {
                return crow::response(500, "Failed to delete template");
            }
            CROW_LOG_INFO << "Deleted template id=" << id;
            return crow::response(200, "Template deleted");
        });
}
