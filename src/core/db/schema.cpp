#include "core/db/schema.h"

#include <sqlite3.h>

#include <string>

bool initializeSchema(DbConnection& conn) {
    std::lock_guard<std::recursive_mutex> lock(conn.mutex());
    if (!conn.isOpen()) return false;
    sqlite3* db = conn.raw();

    const std::string createMealsTable =
        "CREATE TABLE IF NOT EXISTS meals ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT UNIQUE NOT NULL, "
        "category TEXT DEFAULT 'Uncategorized'"
        ");";

    const std::string createIngredientsTable =
        "CREATE TABLE IF NOT EXISTS ingredients ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "meal_id INTEGER NOT NULL, "
        "name TEXT NOT NULL, "
        "amount REAL NOT NULL, "
        "unit INTEGER NOT NULL, "
        "preparation TEXT NOT NULL, "
        "is_optional INTEGER NOT NULL DEFAULT 0, "
        "FOREIGN KEY(meal_id) REFERENCES meals(id) ON DELETE CASCADE"
        ");";

    const std::string createAvailableIngredientsTable =
        "CREATE TABLE IF NOT EXISTS available_ingredients ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT UNIQUE NOT NULL, "
        "category TEXT DEFAULT 'Uncategorized'"
        ");";

    const std::string createGoogleTokensTable =
        "CREATE TABLE IF NOT EXISTS google_tokens ("
        "id INTEGER PRIMARY KEY CHECK (id = 1), "
        "access_token TEXT NOT NULL, "
        "refresh_token TEXT, "
        "expiry_time INTEGER NOT NULL"
        ");";

    const std::string createWorkoutsTable =
        "CREATE TABLE IF NOT EXISTS workouts ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT, "
        "performed_on TEXT NOT NULL, "
        "duration_seconds INTEGER NOT NULL, "
        "notes TEXT, "
        "created_at INTEGER NOT NULL"
        ");";

    const std::string createWorkoutBlocksTable =
        "CREATE TABLE IF NOT EXISTS workout_blocks ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "workout_id INTEGER NOT NULL, "
        "position INTEGER NOT NULL, "
        "block_type TEXT NOT NULL, "
        "rounds INTEGER NOT NULL DEFAULT 1, "
        "rest_seconds INTEGER, "
        "FOREIGN KEY(workout_id) REFERENCES workouts(id) ON DELETE CASCADE"
        ");";

    const std::string createWorkoutExercisesTable =
        "CREATE TABLE IF NOT EXISTS workout_exercises ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "block_id INTEGER NOT NULL, "
        "position INTEGER NOT NULL, "
        "name TEXT NOT NULL, "
        "exercise_type TEXT NOT NULL, "
        "sets INTEGER, "
        "reps INTEGER, "
        "weight_lbs REAL, "
        "distance REAL, "
        "distance_unit TEXT, "
        "duration_seconds INTEGER, "
        "rest_seconds INTEGER, "
        "FOREIGN KEY(block_id) REFERENCES workout_blocks(id) ON DELETE CASCADE"
        ");";

    const std::string createWorkoutTemplatesTable =
        "CREATE TABLE IF NOT EXISTS workout_templates ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT UNIQUE NOT NULL, "
        "created_at INTEGER NOT NULL"
        ");";

    const std::string createTemplateBlocksTable =
        "CREATE TABLE IF NOT EXISTS template_blocks ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "template_id INTEGER NOT NULL, "
        "position INTEGER NOT NULL, "
        "block_type TEXT NOT NULL, "
        "rounds INTEGER NOT NULL DEFAULT 1, "
        "rest_seconds INTEGER, "
        "FOREIGN KEY(template_id) REFERENCES workout_templates(id) ON DELETE CASCADE"
        ");";

    const std::string createTemplateExercisesTable =
        "CREATE TABLE IF NOT EXISTS template_exercises ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "block_id INTEGER NOT NULL, "
        "position INTEGER NOT NULL, "
        "name TEXT NOT NULL, "
        "exercise_type TEXT NOT NULL, "
        "sets INTEGER, "
        "reps INTEGER, "
        "weight_lbs REAL, "
        "distance REAL, "
        "distance_unit TEXT, "
        "duration_seconds INTEGER, "
        "rest_seconds INTEGER, "
        "FOREIGN KEY(block_id) REFERENCES template_blocks(id) ON DELETE CASCADE"
        ");";

    if (!conn.executeQuery("PRAGMA foreign_keys = ON;")) return false;
    if (!conn.executeQuery(createMealsTable)) return false;

    // Older databases may be missing the category column; add it if needed.
    bool categoryExists = false;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, "PRAGMA table_info(meals);", -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* colName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            if (colName && std::string(colName) == "category") {
                categoryExists = true;
                break;
            }
        }
        sqlite3_finalize(stmt);
    }
    if (!categoryExists) {
        conn.executeQuery("ALTER TABLE meals ADD COLUMN category TEXT DEFAULT 'Uncategorized';");
    }

    if (!conn.executeQuery(createIngredientsTable)) return false;

    // Older databases may be missing the is_optional column; add it if needed.
    bool optionalExists = false;
    sqlite3_stmt* stmtIngCols = nullptr;
    if (sqlite3_prepare_v2(db, "PRAGMA table_info(ingredients);", -1, &stmtIngCols, nullptr) ==
        SQLITE_OK) {
        while (sqlite3_step(stmtIngCols) == SQLITE_ROW) {
            const char* colName = reinterpret_cast<const char*>(sqlite3_column_text(stmtIngCols, 1));
            if (colName && std::string(colName) == "is_optional") {
                optionalExists = true;
                break;
            }
        }
        sqlite3_finalize(stmtIngCols);
    }
    if (!optionalExists) {
        conn.executeQuery("ALTER TABLE ingredients ADD COLUMN is_optional INTEGER NOT NULL DEFAULT 0;");
    }

    if (!conn.executeQuery(createAvailableIngredientsTable)) return false;
    if (!conn.executeQuery(createGoogleTokensTable)) return false;
    if (!conn.executeQuery(createWorkoutsTable)) return false;
    if (!conn.executeQuery(createWorkoutBlocksTable)) return false;
    if (!conn.executeQuery(createWorkoutExercisesTable)) return false;
    if (!conn.executeQuery(createWorkoutTemplatesTable)) return false;
    if (!conn.executeQuery(createTemplateBlocksTable)) return false;
    if (!conn.executeQuery(createTemplateExercisesTable)) return false;

    return true;
}
