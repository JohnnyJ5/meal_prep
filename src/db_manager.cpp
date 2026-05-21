#include "db_manager.h"

#include <iostream>

#include "measurement.h"
#include "token_encryption.h"

DBManager::DBManager(const std::string &dbPath) : d_dbPath(dbPath) {
    if (sqlite3_open(dbPath.c_str(), &d_db) != SQLITE_OK) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(d_db) << "\n";
        d_db = nullptr;
    }
}

DBManager::~DBManager() {
    if (d_db) {
        sqlite3_close(d_db);
    }
}

bool DBManager::executeQuery(const std::string &query) {
    if (!d_db) {
        std::cerr << "Database connection is not initialized.\n";
        return false;
    }
    char *errMsg = nullptr;
    if (sqlite3_exec(d_db, query.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << "\n";
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool DBManager::initializeSchema() {
    std::lock_guard<std::recursive_mutex> lock(d_mutex);
    if (!d_db) return false;

    std::string createMealsTable =
        "CREATE TABLE IF NOT EXISTS meals ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT UNIQUE NOT NULL, "
        "category TEXT DEFAULT 'Uncategorized'"
        ");";

    std::string createIngredientsTable =
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

    std::string createAvailableIngredientsTable =
        "CREATE TABLE IF NOT EXISTS available_ingredients ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT UNIQUE NOT NULL, "
        "category TEXT DEFAULT 'Uncategorized'"
        ");";

    std::string createGoogleTokensTable =
        "CREATE TABLE IF NOT EXISTS google_tokens ("
        "id INTEGER PRIMARY KEY CHECK (id = 1), "
        "access_token TEXT NOT NULL, "
        "refresh_token TEXT, "
        "expiry_time INTEGER NOT NULL"
        ");";

    std::string createWorkoutsTable =
        "CREATE TABLE IF NOT EXISTS workouts ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT, "
        "performed_on TEXT NOT NULL, "
        "duration_seconds INTEGER NOT NULL, "
        "notes TEXT, "
        "created_at INTEGER NOT NULL"
        ");";

    std::string createWorkoutBlocksTable =
        "CREATE TABLE IF NOT EXISTS workout_blocks ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "workout_id INTEGER NOT NULL, "
        "position INTEGER NOT NULL, "
        "block_type TEXT NOT NULL, "
        "rounds INTEGER NOT NULL DEFAULT 1, "
        "rest_seconds INTEGER, "
        "FOREIGN KEY(workout_id) REFERENCES workouts(id) ON DELETE CASCADE"
        ");";

    std::string createWorkoutExercisesTable =
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

    // Enable foreign keys
    if (!executeQuery("PRAGMA foreign_keys = ON;")) return false;

    if (!executeQuery(createMealsTable)) return false;

    // Add category column to existing databases if it doesn't exist
    bool columnExists = false;
    sqlite3_stmt *stmt = nullptr;
    const std::string checkColumn = "PRAGMA table_info(meals);";
    if (sqlite3_prepare_v2(d_db, checkColumn.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *colName = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
            if (colName && std::string(colName) == "category") {
                columnExists = true;
                break;
            }
        }
        sqlite3_finalize(stmt);
    }

    if (!columnExists) {
        executeQuery("ALTER TABLE meals ADD COLUMN category TEXT DEFAULT 'Uncategorized';");
    }

    if (!executeQuery(createIngredientsTable)) return false;

    // Add is_optional column to existing ingredients tables if it doesn't exist
    bool optionalColumnExists = false;
    sqlite3_stmt *stmtIngCols = nullptr;
    const std::string checkIngColumn = "PRAGMA table_info(ingredients);";
    if (sqlite3_prepare_v2(d_db, checkIngColumn.c_str(), -1, &stmtIngCols, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmtIngCols) == SQLITE_ROW) {
            const char *colName =
                reinterpret_cast<const char *>(sqlite3_column_text(stmtIngCols, 1));
            if (colName && std::string(colName) == "is_optional") {
                optionalColumnExists = true;
                break;
            }
        }
        sqlite3_finalize(stmtIngCols);
    }
    if (!optionalColumnExists) {
        executeQuery("ALTER TABLE ingredients ADD COLUMN is_optional INTEGER NOT NULL DEFAULT 0;");
    }

    if (!executeQuery(createAvailableIngredientsTable)) return false;

    if (!executeQuery(createGoogleTokensTable)) return false;

    if (!executeQuery(createWorkoutsTable)) return false;
    if (!executeQuery(createWorkoutBlocksTable)) return false;
    if (!executeQuery(createWorkoutExercisesTable)) return false;

    return true;
}

int DBManager::getMealId(const std::string &mealName) {
    if (!d_db) return -1;

    std::string query = "SELECT id FROM meals WHERE name = ?;";
    sqlite3_stmt *stmt = nullptr;
    int mealId = -1;

    if (sqlite3_prepare_v2(d_db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, mealName.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            mealId = sqlite3_column_int(stmt, 0);
        }
    }
    sqlite3_finalize(stmt);
    return mealId;
}

bool DBManager::addMeal(const Meal &meal) {
    std::lock_guard<std::recursive_mutex> lock(d_mutex);
    if (!d_db) return false;

    // Begin transaction
    executeQuery("BEGIN TRANSACTION;");

    std::string insertMeal = "INSERT INTO meals (name, category) VALUES (?, ?);";
    sqlite3_stmt *stmtMeal = nullptr;
    if (sqlite3_prepare_v2(d_db, insertMeal.c_str(), -1, &stmtMeal, nullptr) != SQLITE_OK) {
        executeQuery("ROLLBACK;");
        return false;
    }

    sqlite3_bind_text(stmtMeal, 1, meal.getName().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmtMeal, 2, meal.getCategory().c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmtMeal) != SQLITE_DONE) {
        sqlite3_finalize(stmtMeal);
        executeQuery("ROLLBACK;");
        return false;
    }
    sqlite3_finalize(stmtMeal);

    int mealId = static_cast<int>(sqlite3_last_insert_rowid(d_db));

    std::string insertIngredient =
        "INSERT INTO ingredients (meal_id, name, amount, unit, preparation, is_optional) "
        "VALUES (?, ?, ?, ?, ?, ?);";
    sqlite3_stmt *stmtIngred = nullptr;
    if (sqlite3_prepare_v2(d_db, insertIngredient.c_str(), -1, &stmtIngred, nullptr) != SQLITE_OK) {
        executeQuery("ROLLBACK;");
        return false;
    }

    for (const auto &ing : meal.getIngredients()) {
        sqlite3_bind_int(stmtIngred, 1, mealId);
        sqlite3_bind_text(stmtIngred, 2, ing.getName().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmtIngred, 3, ing.getAmount().getValue());
        sqlite3_bind_int(stmtIngred, 4, static_cast<int>(ing.getAmount().getUnit()));
        sqlite3_bind_text(stmtIngred, 5, ing.getPreparation().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmtIngred, 6, ing.isOptional() ? 1 : 0);

        if (sqlite3_step(stmtIngred) != SQLITE_DONE) {
            sqlite3_finalize(stmtIngred);
            executeQuery("ROLLBACK;");
            return false;
        }
        sqlite3_reset(stmtIngred);
    }
    sqlite3_finalize(stmtIngred);

    executeQuery("COMMIT;");
    return true;
}

bool DBManager::updateMeal(const Meal &meal) {
    std::lock_guard<std::recursive_mutex> lock(d_mutex);
    if (!d_db) return false;

    executeQuery("BEGIN TRANSACTION;");

    // Delete old meal (cascade removes ingredients)
    std::string delQuery = "DELETE FROM meals WHERE name = ?;";
    sqlite3_stmt *stmtDel = nullptr;
    if (sqlite3_prepare_v2(d_db, delQuery.c_str(), -1, &stmtDel, nullptr) != SQLITE_OK) {
        executeQuery("ROLLBACK;");
        return false;
    }
    sqlite3_bind_text(stmtDel, 1, meal.getName().c_str(), -1, SQLITE_TRANSIENT);
    int delResult = sqlite3_step(stmtDel);
    sqlite3_finalize(stmtDel);
    if (delResult != SQLITE_DONE) {
        executeQuery("ROLLBACK;");
        return false;
    }

    // Insert updated meal row
    std::string insertMeal = "INSERT INTO meals (name, category) VALUES (?, ?);";
    sqlite3_stmt *stmtMeal = nullptr;
    if (sqlite3_prepare_v2(d_db, insertMeal.c_str(), -1, &stmtMeal, nullptr) != SQLITE_OK) {
        executeQuery("ROLLBACK;");
        return false;
    }
    sqlite3_bind_text(stmtMeal, 1, meal.getName().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmtMeal, 2, meal.getCategory().c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmtMeal) != SQLITE_DONE) {
        sqlite3_finalize(stmtMeal);
        executeQuery("ROLLBACK;");
        return false;
    }
    sqlite3_finalize(stmtMeal);

    int mealId = static_cast<int>(sqlite3_last_insert_rowid(d_db));

    // Insert updated ingredients
    std::string insertIngredient =
        "INSERT INTO ingredients (meal_id, name, amount, unit, preparation, is_optional) "
        "VALUES (?, ?, ?, ?, ?, ?);";
    sqlite3_stmt *stmtIngred = nullptr;
    if (sqlite3_prepare_v2(d_db, insertIngredient.c_str(), -1, &stmtIngred, nullptr) != SQLITE_OK) {
        executeQuery("ROLLBACK;");
        return false;
    }
    for (const auto &ing : meal.getIngredients()) {
        sqlite3_bind_int(stmtIngred, 1, mealId);
        sqlite3_bind_text(stmtIngred, 2, ing.getName().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmtIngred, 3, ing.getAmount().getValue());
        sqlite3_bind_int(stmtIngred, 4, static_cast<int>(ing.getAmount().getUnit()));
        sqlite3_bind_text(stmtIngred, 5, ing.getPreparation().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmtIngred, 6, ing.isOptional() ? 1 : 0);
        if (sqlite3_step(stmtIngred) != SQLITE_DONE) {
            sqlite3_finalize(stmtIngred);
            executeQuery("ROLLBACK;");
            return false;
        }
        sqlite3_reset(stmtIngred);
    }
    sqlite3_finalize(stmtIngred);

    executeQuery("COMMIT;");
    return true;
}

bool DBManager::deleteMeal(const std::string &mealName) {
    std::lock_guard<std::recursive_mutex> lock(d_mutex);
    if (!d_db) return false;
    std::string query = "DELETE FROM meals WHERE name = ?;";
    sqlite3_stmt *stmt = nullptr;
    bool success = false;
    if (sqlite3_prepare_v2(d_db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, mealName.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_DONE) {
            success = true;
        }
    }
    sqlite3_finalize(stmt);
    return success;
}

std::unique_ptr<Meal> DBManager::getMeal(const std::string &mealName) {
    std::lock_guard<std::recursive_mutex> lock(d_mutex);
    if (!d_db) return nullptr;

    int mealId = getMealId(mealName);
    if (mealId == -1) return nullptr;

    std::string getCategoryQuery = "SELECT category FROM meals WHERE id = ?;";
    sqlite3_stmt *stmtCat = nullptr;
    std::string category = "Uncategorized";
    if (sqlite3_prepare_v2(d_db, getCategoryQuery.c_str(), -1, &stmtCat, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmtCat, 1, mealId);
        if (sqlite3_step(stmtCat) == SQLITE_ROW) {
            if (const char *catText =
                    reinterpret_cast<const char *>(sqlite3_column_text(stmtCat, 0))) {
                category = catText;
            }
        }
    }
    sqlite3_finalize(stmtCat);

    std::string query =
        "SELECT name, amount, unit, preparation, is_optional FROM ingredients "
        "WHERE meal_id = ?;";
    sqlite3_stmt *stmt = nullptr;
    std::vector<Ingredient> ingredients;

    if (sqlite3_prepare_v2(d_db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, mealId);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string ingName = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
            double amount = sqlite3_column_double(stmt, 1);
            int unit = sqlite3_column_int(stmt, 2);
            std::string prep = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
            bool isOptional = sqlite3_column_int(stmt, 4) != 0;

            ingredients.emplace_back(ingName,
                                     Measurement(amount, static_cast<MeasurementUnit>(unit)), prep,
                                     isOptional);
        }
    }
    sqlite3_finalize(stmt);

    return std::make_unique<Meal>(mealName, ingredients, category);
}

bool DBManager::getAllMeals(std::vector<std::tuple<int, std::string, std::string>> &meals) {
    std::lock_guard<std::recursive_mutex> lock(d_mutex);
    if (!d_db) return false;

    std::string query = "SELECT id, name, category FROM meals ORDER BY name ASC;";
    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v2(d_db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int id = sqlite3_column_int(stmt, 0);
            std::string name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
            std::string category = "Uncategorized";
            if (const char *catText =
                    reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2))) {
                category = catText;
            }
            meals.emplace_back(id, name, category);
        }
    }
    sqlite3_finalize(stmt);
    return true;
}

std::set<int> DBManager::getMealIdsWithOptionalIngredients() {
    std::lock_guard<std::recursive_mutex> lock(d_mutex);
    std::set<int> ids;
    if (!d_db) return ids;

    const std::string query =
        "SELECT DISTINCT meal_id FROM ingredients WHERE is_optional = 1;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(d_db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            ids.insert(sqlite3_column_int(stmt, 0));
        }
    }
    sqlite3_finalize(stmt);
    return ids;
}

bool DBManager::getAllIngredients(std::vector<std::pair<std::string, std::string>> &ingredients) {
    std::lock_guard<std::recursive_mutex> lock(d_mutex);
    if (!d_db) return false;

    std::string query = "SELECT name, category FROM available_ingredients ORDER BY name ASC;";
    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v2(d_db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
            std::string category = "Uncategorized";
            if (const char *catText =
                    reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1))) {
                category = catText;
            }
            ingredients.emplace_back(name, category);
        }
    }
    sqlite3_finalize(stmt);
    return true;
}

bool DBManager::addIngredient(const std::string &name, const std::string &category) {
    std::lock_guard<std::recursive_mutex> lock(d_mutex);
    if (!d_db) return false;

    std::string insertIngredient =
        "INSERT INTO available_ingredients (name, category) VALUES (?, ?);";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(d_db, insertIngredient.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, category.c_str(), -1, SQLITE_TRANSIENT);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

bool DBManager::seedDefaultIngredients() {
    std::lock_guard<std::recursive_mutex> lock(d_mutex);
    std::vector<std::pair<std::string, std::string>> existing;
    getAllIngredients(existing);
    if (!existing.empty()) return true;  // Already seeded

    std::vector<std::pair<std::string, std::string>> defaultIngredients = {
        // Vegetables
        {"Spinach", "Vegetables"},
        {"Broccoli", "Vegetables"},
        {"Onion", "Vegetables"},
        {"Carrots", "Vegetables"},
        {"Yellow Bell Pepper", "Vegetables"},
        {"Red Bell Pepper", "Vegetables"},

        // Proteins
        {"Chicken Breast", "Proteins"},
        {"Ground Turkey", "Proteins"},
        {"Ground Beef", "Proteins"},
        {"Eggs", "Proteins"},
        {"Egg", "Proteins"},
        {"Cottage Cheese", "Proteins"},

        // Dairy
        {"Feta", "Dairy"},
        {"Parmesan Cheese", "Dairy"},
        {"Sharp Cheddar Cheese", "Dairy"},
        {"Milk", "Dairy"},
        {"Heavy Cream", "Dairy"},
        {"Butter", "Dairy"},

        // Pantry Staples
        {"Salt", "Pantry Staples"},
        {"Pepper", "Pantry Staples"},
        {"Olive Oil", "Pantry Staples"},
        {"Flour", "Pantry Staples"},
        {"All-Purpose Flour", "Pantry Staples"},
        {"Corn Starch", "Pantry Staples"},
        {"Baking Powder", "Pantry Staples"},
        {"Breadcrumbs", "Pantry Staples"},

        // Seasonings & Spices
        {"Garlic Powder", "Seasonings & Spices"},
        {"Onion Powder", "Seasonings & Spices"},
        {"Italian Seasoning", "Seasonings & Spices"},
        {"Paprika", "Seasonings & Spices"},
        {"Ginger", "Seasonings & Spices"},
        {"Vanilla Extract", "Seasonings & Spices"},

        // Produce
        {"Garlic", "Produce"},
        {"Crushed Tomatoes", "Produce"},

        // Grains & Pasta
        {"Penne Pasta", "Grains & Pasta"},
        {"Pasta", "Grains & Pasta"},

        // Broths & Sauces
        {"Chicken Broth", "Broths & Sauces"},
        {"Beef Broth", "Broths & Sauces"},
        {"Soy Sauce", "Broths & Sauces"},

        // Sweeteners
        {"Maple Syrup", "Sweeteners"},
        {"Honey", "Sweeteners"},

        // Other
        {"Sesame Seeds", "Other"},
        {"Fruit of Choice", "Other"}};

    bool allSuccess = true;
    executeQuery("BEGIN TRANSACTION;");
    for (const auto &ing : defaultIngredients) {
        if (!addIngredient(ing.first, ing.second)) {
            allSuccess = false;
        }
    }
    if (allSuccess) {
        executeQuery("COMMIT;");
    } else {
        executeQuery("ROLLBACK;");
    }

    return allSuccess;
}

bool DBManager::seedDefaultMeals() {
    std::lock_guard<std::recursive_mutex> lock(d_mutex);
    // This will seed the database with the initial hardcoded values if empty.
    std::vector<std::tuple<int, std::string, std::string>> existing;
    getAllMeals(existing);
    if (!existing.empty()) return true;  // Already seeded

    std::vector<Meal> defaultMeals = {
        Meal("turkey-burgers",
             {Ingredient("Spinach", Measurement(2.0, MeasurementUnit::CUP), "Chopped"),
              Ingredient("Feta", Measurement(4.0, MeasurementUnit::OUNCE)),
              Ingredient("Breadcrumbs", Measurement(0.25, MeasurementUnit::CUP)),
              Ingredient("Ground Turkey", Measurement(1.0, MeasurementUnit::POUND)),
              Ingredient("Salt", Measurement(1.0, MeasurementUnit::TEASPOON)),
              Ingredient("Pepper", Measurement(0.5, MeasurementUnit::TEASPOON)),
              Ingredient("Garlic Powder", Measurement(1.0, MeasurementUnit::TEASPOON)),
              Ingredient("Onion Powder", Measurement(0.5, MeasurementUnit::TEASPOON)),
              Ingredient("Italian Seasoning", Measurement(1.0, MeasurementUnit::TEASPOON)),
              Ingredient("Olive Oil", Measurement(1.0, MeasurementUnit::TABLESPOON))},
             "Poultry"),
        Meal("turkey-meatballs",
             {Ingredient("Ground Turkey", Measurement(1.0, MeasurementUnit::POUND)),
              Ingredient("Breadcrumbs", Measurement(1.0, MeasurementUnit::CUP)),
              Ingredient("Italian Seasoning", Measurement(1.0, MeasurementUnit::TABLESPOON)),
              Ingredient("Onion", Measurement(1.0, MeasurementUnit::SMALL), "Diced"),
              Ingredient("Garlic", Measurement(3.0, MeasurementUnit::CLOVE), "Minced"),
              Ingredient("Egg", Measurement(2.0, MeasurementUnit::WHOLE)),
              Ingredient("Milk", Measurement(0.25, MeasurementUnit::CUP)),
              Ingredient("Salt", Measurement(0.5, MeasurementUnit::TEASPOON)),
              Ingredient("Pepper", Measurement(0.25, MeasurementUnit::TEASPOON))},
             "Poultry"),
        Meal("creamy-garlic-chicken-penne-spinach",
             {Ingredient("Chicken Breast", Measurement(2.0, MeasurementUnit::WHOLE), "Strips"),
              Ingredient("Penne Pasta", Measurement(2.0, MeasurementUnit::CUP)),
              Ingredient("Broccoli", Measurement(1.5, MeasurementUnit::CUP)),
              Ingredient("Spinach", Measurement(1.0, MeasurementUnit::CUP)),
              Ingredient("Olive Oil", Measurement(2.0, MeasurementUnit::TABLESPOON)),
              Ingredient("Salt", Measurement(1.0, MeasurementUnit::TEASPOON)),
              Ingredient("Pepper", Measurement(0.5, MeasurementUnit::TEASPOON)),
              Ingredient("Paprika", Measurement(0.5, MeasurementUnit::TEASPOON)),
              Ingredient("Garlic", Measurement(3.0, MeasurementUnit::CLOVE), "Minced"),
              Ingredient("Heavy Cream", Measurement(1.0, MeasurementUnit::CUP)),
              Ingredient("Parmesan Cheese", Measurement(0.5, MeasurementUnit::CUP), "Grated")},
             "Poultry"),
        Meal("creamy-garlic-chicken",
             {Ingredient("Chicken Breast", Measurement(4.0, MeasurementUnit::WHOLE), "Thin Sliced"),
              Ingredient("Flour", Measurement(0.25, MeasurementUnit::CUP)),
              Ingredient("Butter", Measurement(1.5, MeasurementUnit::TABLESPOON)),
              Ingredient("Olive Oil", Measurement(1.0, MeasurementUnit::TABLESPOON)),
              Ingredient("Garlic", Measurement(1.0, MeasurementUnit::HEAD), "Peeled"),
              Ingredient("Chicken Broth", Measurement(0.5, MeasurementUnit::CUP)),
              Ingredient("Heavy Cream", Measurement(1.0, MeasurementUnit::CUP)),
              Ingredient("Spinach", Measurement(2.0, MeasurementUnit::CUP))},
             "Poultry"),
        Meal("baked-chicken-breast",
             {Ingredient("Chicken Breast", Measurement(4.0, MeasurementUnit::WHOLE)),
              Ingredient("Olive Oil", Measurement(1.0, MeasurementUnit::TABLESPOON)),
              Ingredient("Paprika", Measurement(2.0, MeasurementUnit::TEASPOON)),
              Ingredient("Italian Seasoning", Measurement(1.0, MeasurementUnit::TEASPOON)),
              Ingredient("Garlic Powder", Measurement(0.5, MeasurementUnit::TEASPOON)),
              Ingredient("Salt", Measurement(0.5, MeasurementUnit::TEASPOON)),
              Ingredient("Pepper", Measurement(0.25, MeasurementUnit::TEASPOON))},
             "Poultry"),
        Meal("cheesy-hamburger-pasta-skillet",
             {Ingredient("Olive Oil", Measurement(1.0, MeasurementUnit::TABLESPOON)),
              Ingredient("Ground Beef", Measurement(1.0, MeasurementUnit::POUND)),
              Ingredient("Salt", Measurement(2.0, MeasurementUnit::TEASPOON)),
              Ingredient("Beef Broth", Measurement(1.5, MeasurementUnit::CUP)),
              Ingredient("Pasta", Measurement(8.0, MeasurementUnit::OUNCE)),
              Ingredient("Crushed Tomatoes", Measurement(14.5, MeasurementUnit::OUNCE)),
              Ingredient("Sharp Cheddar Cheese", Measurement(2.0, MeasurementUnit::CUP)),
              Ingredient("Heavy Cream", Measurement(0.5, MeasurementUnit::CUP))},
             "Beef"),
        Meal("cottage-cheese-pancakes",
             {Ingredient("Eggs", Measurement(4.0, MeasurementUnit::WHOLE)),
              Ingredient("Cottage Cheese", Measurement(1.5, MeasurementUnit::CUP)),
              Ingredient("Maple Syrup", Measurement(3.0, MeasurementUnit::TABLESPOON)),
              Ingredient("Vanilla Extract", Measurement(1.0, MeasurementUnit::TEASPOON)),
              Ingredient("All-Purpose Flour", Measurement(1.0, MeasurementUnit::CUP)),
              Ingredient("Baking Powder", Measurement(0.5, MeasurementUnit::TABLESPOON)),
              Ingredient("Fruit of Choice", Measurement(1.0, MeasurementUnit::CUP))},
             "Breakfast"),
        Meal("chicken-stir-fry",
             {Ingredient("Chicken Breast", Measurement(3.0, MeasurementUnit::WHOLE)),
              Ingredient("Salt", Measurement(0.5, MeasurementUnit::TEASPOON)),
              Ingredient("Pepper", Measurement(0.5, MeasurementUnit::TEASPOON)),
              Ingredient("Olive Oil", Measurement(2.0, MeasurementUnit::TABLESPOON)),
              Ingredient("Broccoli", Measurement(2.0, MeasurementUnit::CUP)),
              Ingredient("Yellow Bell Pepper", Measurement(1.0, MeasurementUnit::HALF)),
              Ingredient("Red Bell Pepper", Measurement(1.0, MeasurementUnit::HALF)),
              Ingredient("Carrots", Measurement(0.5, MeasurementUnit::CUP)),
              Ingredient("Ginger", Measurement(0.5, MeasurementUnit::TEASPOON)),
              Ingredient("Garlic", Measurement(2.0, MeasurementUnit::TEASPOON)),
              Ingredient("Sesame Seeds", Measurement(2.0, MeasurementUnit::TABLESPOON)),
              Ingredient("Corn Starch", Measurement(1.0, MeasurementUnit::TABLESPOON)),
              Ingredient("Chicken Broth", Measurement(0.25, MeasurementUnit::CUP)),
              Ingredient("Soy Sauce", Measurement(0.25, MeasurementUnit::CUP)),
              Ingredient("Honey", Measurement(2.0, MeasurementUnit::TABLESPOON))},
             "Poultry")};

    bool allSuccess = true;
    for (const auto &meal : defaultMeals) {
        if (!addMeal(meal)) {
            allSuccess = false;
        }
    }

    return allSuccess;
}

bool DBManager::saveGoogleTokens(const std::string &accessToken, const std::string &refreshToken,
                                 int64_t expiryTime) {
    std::lock_guard<std::recursive_mutex> lock(d_mutex);
    if (!d_db) return false;
    std::string query =
        "INSERT OR REPLACE INTO google_tokens (id, access_token, refresh_token, "
        "expiry_time) "
        "VALUES (1, ?, ?, ?);";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(d_db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return false;
    std::string encAccessToken = TokenEncryption::encrypt(accessToken);
    std::string encRefreshToken =
        refreshToken.empty() ? "" : TokenEncryption::encrypt(refreshToken);
    sqlite3_bind_text(stmt, 1, encAccessToken.c_str(), -1, SQLITE_TRANSIENT);
    if (encRefreshToken.empty()) {
        sqlite3_bind_null(stmt, 2);
    } else {
        sqlite3_bind_text(stmt, 2, encRefreshToken.c_str(), -1, SQLITE_TRANSIENT);
    }
    sqlite3_bind_int64(stmt, 3, expiryTime);
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

bool DBManager::getGoogleTokens(std::string &accessToken, std::string &refreshToken,
                                int64_t &expiryTime) {
    std::lock_guard<std::recursive_mutex> lock(d_mutex);
    if (!d_db) return false;
    std::string query =
        "SELECT access_token, refresh_token, expiry_time FROM "
        "google_tokens WHERE id = 1;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(d_db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return false;
    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *access = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        if (access) {
            accessToken = TokenEncryption::decrypt(access);
        }
        const char *refresh = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        if (refresh) {
            refreshToken = TokenEncryption::decrypt(refresh);
        } else {
            refreshToken = "";
        }
        expiryTime = sqlite3_column_int64(stmt, 2);
        found = true;
    }
    sqlite3_finalize(stmt);
    return found;
}

namespace {

void bindNullableInt(sqlite3_stmt *stmt, int idx, int value, bool hasValue) {
    if (hasValue) {
        sqlite3_bind_int(stmt, idx, value);
    } else {
        sqlite3_bind_null(stmt, idx);
    }
}

void bindNullableDouble(sqlite3_stmt *stmt, int idx, double value, bool hasValue) {
    if (hasValue) {
        sqlite3_bind_double(stmt, idx, value);
    } else {
        sqlite3_bind_null(stmt, idx);
    }
}

void bindNullableText(sqlite3_stmt *stmt, int idx, const std::string &value) {
    if (value.empty()) {
        sqlite3_bind_null(stmt, idx);
    } else {
        sqlite3_bind_text(stmt, idx, value.c_str(), -1, SQLITE_TRANSIENT);
    }
}

// Inserts a workout's blocks and exercises. Caller owns the transaction.
bool insertBlocksAndExercises(sqlite3 *db, int workoutId,
                              const std::vector<WorkoutBlock> &blocks) {
    const std::string insertBlock =
        "INSERT INTO workout_blocks "
        "(workout_id, position, block_type, rounds, rest_seconds) "
        "VALUES (?, ?, ?, ?, ?);";
    const std::string insertExercise =
        "INSERT INTO workout_exercises "
        "(block_id, position, name, exercise_type, sets, reps, weight_lbs, "
        " distance, distance_unit, duration_seconds, rest_seconds) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    for (size_t bi = 0; bi < blocks.size(); ++bi) {
        const auto &block = blocks[bi];
        sqlite3_stmt *stmtBlock = nullptr;
        if (sqlite3_prepare_v2(db, insertBlock.c_str(), -1, &stmtBlock, nullptr) != SQLITE_OK) {
            return false;
        }
        sqlite3_bind_int(stmtBlock, 1, workoutId);
        sqlite3_bind_int(stmtBlock, 2, static_cast<int>(bi));
        const std::string blockTypeStr = blockTypeToString(block.type);
        sqlite3_bind_text(stmtBlock, 3, blockTypeStr.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmtBlock, 4, block.rounds <= 0 ? 1 : block.rounds);
        bindNullableInt(stmtBlock, 5, block.rest_seconds, block.rest_seconds > 0);
        if (sqlite3_step(stmtBlock) != SQLITE_DONE) {
            sqlite3_finalize(stmtBlock);
            return false;
        }
        sqlite3_finalize(stmtBlock);
        int blockId = static_cast<int>(sqlite3_last_insert_rowid(db));

        for (size_t ei = 0; ei < block.exercises.size(); ++ei) {
            const auto &ex = block.exercises[ei];
            sqlite3_stmt *stmtEx = nullptr;
            if (sqlite3_prepare_v2(db, insertExercise.c_str(), -1, &stmtEx, nullptr) != SQLITE_OK) {
                return false;
            }
            sqlite3_bind_int(stmtEx, 1, blockId);
            sqlite3_bind_int(stmtEx, 2, static_cast<int>(ei));
            sqlite3_bind_text(stmtEx, 3, ex.name.c_str(), -1, SQLITE_TRANSIENT);
            const std::string typeStr = exerciseTypeToString(ex.type);
            sqlite3_bind_text(stmtEx, 4, typeStr.c_str(), -1, SQLITE_TRANSIENT);
            bindNullableInt(stmtEx, 5, ex.sets, ex.sets > 0);
            bindNullableInt(stmtEx, 6, ex.reps, ex.reps > 0);
            bindNullableDouble(stmtEx, 7, ex.weight_lbs, ex.weight_lbs > 0.0);
            bindNullableDouble(stmtEx, 8, ex.distance, ex.distance > 0.0);
            bindNullableText(stmtEx, 9, ex.distance_unit);
            bindNullableInt(stmtEx, 10, ex.duration_seconds, ex.duration_seconds > 0);
            bindNullableInt(stmtEx, 11, ex.rest_seconds, ex.rest_seconds > 0);

            if (sqlite3_step(stmtEx) != SQLITE_DONE) {
                sqlite3_finalize(stmtEx);
                return false;
            }
            sqlite3_finalize(stmtEx);
        }
    }
    return true;
}

}  // namespace

bool DBManager::addWorkout(Workout &workout) {
    std::lock_guard<std::recursive_mutex> lock(d_mutex);
    if (!d_db) return false;

    executeQuery("BEGIN TRANSACTION;");

    const std::string insertWorkout =
        "INSERT INTO workouts (name, performed_on, duration_seconds, notes, created_at) "
        "VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(d_db, insertWorkout.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        executeQuery("ROLLBACK;");
        return false;
    }
    bindNullableText(stmt, 1, workout.name);
    sqlite3_bind_text(stmt, 2, workout.performed_on.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, workout.duration_seconds);
    bindNullableText(stmt, 4, workout.notes);
    sqlite3_bind_int64(stmt, 5, workout.created_at);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        executeQuery("ROLLBACK;");
        return false;
    }
    sqlite3_finalize(stmt);
    workout.id = static_cast<int>(sqlite3_last_insert_rowid(d_db));

    if (!insertBlocksAndExercises(d_db, workout.id, workout.blocks)) {
        executeQuery("ROLLBACK;");
        return false;
    }

    executeQuery("COMMIT;");
    return true;
}

bool DBManager::updateWorkout(const Workout &workout) {
    std::lock_guard<std::recursive_mutex> lock(d_mutex);
    if (!d_db) return false;

    executeQuery("BEGIN TRANSACTION;");

    // Wipe existing blocks (cascades to exercises) so we can re-insert cleanly.
    {
        const std::string delBlocks = "DELETE FROM workout_blocks WHERE workout_id = ?;";
        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(d_db, delBlocks.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            executeQuery("ROLLBACK;");
            return false;
        }
        sqlite3_bind_int(stmt, 1, workout.id);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            executeQuery("ROLLBACK;");
            return false;
        }
        sqlite3_finalize(stmt);
    }

    {
        const std::string updateRow =
            "UPDATE workouts SET name = ?, performed_on = ?, duration_seconds = ?, notes = ? "
            "WHERE id = ?;";
        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(d_db, updateRow.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            executeQuery("ROLLBACK;");
            return false;
        }
        bindNullableText(stmt, 1, workout.name);
        sqlite3_bind_text(stmt, 2, workout.performed_on.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, workout.duration_seconds);
        bindNullableText(stmt, 4, workout.notes);
        sqlite3_bind_int(stmt, 5, workout.id);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            executeQuery("ROLLBACK;");
            return false;
        }
        sqlite3_finalize(stmt);
    }

    if (!insertBlocksAndExercises(d_db, workout.id, workout.blocks)) {
        executeQuery("ROLLBACK;");
        return false;
    }

    executeQuery("COMMIT;");
    return true;
}

bool DBManager::deleteWorkout(int id) {
    std::lock_guard<std::recursive_mutex> lock(d_mutex);
    if (!d_db) return false;
    const std::string del = "DELETE FROM workouts WHERE id = ?;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(d_db, del.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, id);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

Workout DBManager::getWorkout(int id) {
    std::lock_guard<std::recursive_mutex> lock(d_mutex);
    Workout out;
    if (!d_db) return out;

    {
        const std::string q =
            "SELECT id, name, performed_on, duration_seconds, notes, created_at "
            "FROM workouts WHERE id = ?;";
        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(d_db, q.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return out;
        sqlite3_bind_int(stmt, 1, id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            out.id = sqlite3_column_int(stmt, 0);
            if (const char *n = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1))) {
                out.name = n;
            }
            if (const char *d = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2))) {
                out.performed_on = d;
            }
            out.duration_seconds = sqlite3_column_int(stmt, 3);
            if (const char *notes = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4))) {
                out.notes = notes;
            }
            out.created_at = sqlite3_column_int64(stmt, 5);
        }
        sqlite3_finalize(stmt);
    }

    if (out.id == 0) return out;

    {
        const std::string q =
            "SELECT id, position, block_type, rounds, rest_seconds "
            "FROM workout_blocks WHERE workout_id = ? ORDER BY position ASC;";
        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(d_db, q.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return out;
        sqlite3_bind_int(stmt, 1, id);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            WorkoutBlock b;
            b.id = sqlite3_column_int(stmt, 0);
            b.position = sqlite3_column_int(stmt, 1);
            if (const char *bt = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2))) {
                b.type = blockTypeFromString(bt);
            }
            b.rounds = sqlite3_column_int(stmt, 3);
            b.rest_seconds = sqlite3_column_type(stmt, 4) == SQLITE_NULL
                                 ? 0
                                 : sqlite3_column_int(stmt, 4);
            out.blocks.push_back(std::move(b));
        }
        sqlite3_finalize(stmt);
    }

    for (auto &b : out.blocks) {
        const std::string q =
            "SELECT id, position, name, exercise_type, sets, reps, weight_lbs, "
            "       distance, distance_unit, duration_seconds, rest_seconds "
            "FROM workout_exercises WHERE block_id = ? ORDER BY position ASC;";
        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(d_db, q.c_str(), -1, &stmt, nullptr) != SQLITE_OK) continue;
        sqlite3_bind_int(stmt, 1, b.id);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            WorkoutExercise e;
            e.id = sqlite3_column_int(stmt, 0);
            e.position = sqlite3_column_int(stmt, 1);
            if (const char *n = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2))) {
                e.name = n;
            }
            if (const char *t = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3))) {
                e.type = exerciseTypeFromString(t);
            }
            e.sets = sqlite3_column_type(stmt, 4) == SQLITE_NULL ? 0 : sqlite3_column_int(stmt, 4);
            e.reps = sqlite3_column_type(stmt, 5) == SQLITE_NULL ? 0 : sqlite3_column_int(stmt, 5);
            e.weight_lbs = sqlite3_column_type(stmt, 6) == SQLITE_NULL
                               ? 0.0
                               : sqlite3_column_double(stmt, 6);
            e.distance = sqlite3_column_type(stmt, 7) == SQLITE_NULL
                             ? 0.0
                             : sqlite3_column_double(stmt, 7);
            if (const char *u = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 8))) {
                e.distance_unit = u;
            }
            e.duration_seconds = sqlite3_column_type(stmt, 9) == SQLITE_NULL
                                     ? 0
                                     : sqlite3_column_int(stmt, 9);
            e.rest_seconds = sqlite3_column_type(stmt, 10) == SQLITE_NULL
                                 ? 0
                                 : sqlite3_column_int(stmt, 10);
            b.exercises.push_back(std::move(e));
        }
        sqlite3_finalize(stmt);
    }

    return out;
}

std::vector<WorkoutSummary> DBManager::listWorkouts() {
    std::lock_guard<std::recursive_mutex> lock(d_mutex);
    std::vector<WorkoutSummary> out;
    if (!d_db) return out;

    const std::string q =
        "SELECT w.id, w.name, w.performed_on, w.duration_seconds, "
        "       COUNT(e.id) AS exercise_count "
        "FROM workouts w "
        "LEFT JOIN workout_blocks b ON b.workout_id = w.id "
        "LEFT JOIN workout_exercises e ON e.block_id = b.id "
        "GROUP BY w.id "
        "ORDER BY w.performed_on DESC, w.created_at DESC;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(d_db, q.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return out;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        WorkoutSummary s;
        s.id = sqlite3_column_int(stmt, 0);
        if (const char *n = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1))) {
            s.name = n;
        }
        if (const char *d = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2))) {
            s.performed_on = d;
        }
        s.duration_seconds = sqlite3_column_int(stmt, 3);
        s.exercise_count = sqlite3_column_int(stmt, 4);
        out.push_back(std::move(s));
    }
    sqlite3_finalize(stmt);
    return out;
}
