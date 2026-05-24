#include "features/meals/meals_repository.h"

#include <sqlite3.h>

#include <utility>

#include "features/meals/measurement.h"

MealsRepository::MealsRepository(std::shared_ptr<DbConnection> conn) : d_conn(std::move(conn)) {}

int MealsRepository::getMealId(const std::string& mealName) {
    sqlite3* db = d_conn->raw();
    if (!db) return -1;

    const std::string query = "SELECT id FROM meals WHERE name = ?;";
    sqlite3_stmt* stmt = nullptr;
    int mealId = -1;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, mealName.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            mealId = sqlite3_column_int(stmt, 0);
        }
    }
    sqlite3_finalize(stmt);
    return mealId;
}

bool MealsRepository::addMeal(const Meal& meal) {
    std::lock_guard<std::recursive_mutex> lock(d_conn->mutex());
    sqlite3* db = d_conn->raw();
    if (!db) return false;

    d_conn->executeQuery("BEGIN TRANSACTION;");

    const std::string insertMeal = "INSERT INTO meals (name, category) VALUES (?, ?);";
    sqlite3_stmt* stmtMeal = nullptr;
    if (sqlite3_prepare_v2(db, insertMeal.c_str(), -1, &stmtMeal, nullptr) != SQLITE_OK) {
        d_conn->executeQuery("ROLLBACK;");
        return false;
    }
    sqlite3_bind_text(stmtMeal, 1, meal.getName().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmtMeal, 2, meal.getCategory().c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmtMeal) != SQLITE_DONE) {
        sqlite3_finalize(stmtMeal);
        d_conn->executeQuery("ROLLBACK;");
        return false;
    }
    sqlite3_finalize(stmtMeal);

    int mealId = static_cast<int>(sqlite3_last_insert_rowid(db));

    const std::string insertIngredient =
        "INSERT INTO ingredients (meal_id, name, amount, unit, preparation, is_optional) "
        "VALUES (?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmtIngred = nullptr;
    if (sqlite3_prepare_v2(db, insertIngredient.c_str(), -1, &stmtIngred, nullptr) != SQLITE_OK) {
        d_conn->executeQuery("ROLLBACK;");
        return false;
    }

    for (const auto& ing : meal.getIngredients()) {
        sqlite3_bind_int(stmtIngred, 1, mealId);
        sqlite3_bind_text(stmtIngred, 2, ing.getName().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmtIngred, 3, ing.getAmount().getValue());
        sqlite3_bind_int(stmtIngred, 4, static_cast<int>(ing.getAmount().getUnit()));
        sqlite3_bind_text(stmtIngred, 5, ing.getPreparation().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmtIngred, 6, ing.isOptional() ? 1 : 0);

        if (sqlite3_step(stmtIngred) != SQLITE_DONE) {
            sqlite3_finalize(stmtIngred);
            d_conn->executeQuery("ROLLBACK;");
            return false;
        }
        sqlite3_reset(stmtIngred);
    }
    sqlite3_finalize(stmtIngred);

    d_conn->executeQuery("COMMIT;");
    return true;
}

bool MealsRepository::updateMeal(const Meal& meal) {
    std::lock_guard<std::recursive_mutex> lock(d_conn->mutex());
    sqlite3* db = d_conn->raw();
    if (!db) return false;

    d_conn->executeQuery("BEGIN TRANSACTION;");

    const std::string delQuery = "DELETE FROM meals WHERE name = ?;";
    sqlite3_stmt* stmtDel = nullptr;
    if (sqlite3_prepare_v2(db, delQuery.c_str(), -1, &stmtDel, nullptr) != SQLITE_OK) {
        d_conn->executeQuery("ROLLBACK;");
        return false;
    }
    sqlite3_bind_text(stmtDel, 1, meal.getName().c_str(), -1, SQLITE_TRANSIENT);
    int delResult = sqlite3_step(stmtDel);
    sqlite3_finalize(stmtDel);
    if (delResult != SQLITE_DONE) {
        d_conn->executeQuery("ROLLBACK;");
        return false;
    }

    const std::string insertMeal = "INSERT INTO meals (name, category) VALUES (?, ?);";
    sqlite3_stmt* stmtMeal = nullptr;
    if (sqlite3_prepare_v2(db, insertMeal.c_str(), -1, &stmtMeal, nullptr) != SQLITE_OK) {
        d_conn->executeQuery("ROLLBACK;");
        return false;
    }
    sqlite3_bind_text(stmtMeal, 1, meal.getName().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmtMeal, 2, meal.getCategory().c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmtMeal) != SQLITE_DONE) {
        sqlite3_finalize(stmtMeal);
        d_conn->executeQuery("ROLLBACK;");
        return false;
    }
    sqlite3_finalize(stmtMeal);

    int mealId = static_cast<int>(sqlite3_last_insert_rowid(db));

    const std::string insertIngredient =
        "INSERT INTO ingredients (meal_id, name, amount, unit, preparation, is_optional) "
        "VALUES (?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmtIngred = nullptr;
    if (sqlite3_prepare_v2(db, insertIngredient.c_str(), -1, &stmtIngred, nullptr) != SQLITE_OK) {
        d_conn->executeQuery("ROLLBACK;");
        return false;
    }
    for (const auto& ing : meal.getIngredients()) {
        sqlite3_bind_int(stmtIngred, 1, mealId);
        sqlite3_bind_text(stmtIngred, 2, ing.getName().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmtIngred, 3, ing.getAmount().getValue());
        sqlite3_bind_int(stmtIngred, 4, static_cast<int>(ing.getAmount().getUnit()));
        sqlite3_bind_text(stmtIngred, 5, ing.getPreparation().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmtIngred, 6, ing.isOptional() ? 1 : 0);
        if (sqlite3_step(stmtIngred) != SQLITE_DONE) {
            sqlite3_finalize(stmtIngred);
            d_conn->executeQuery("ROLLBACK;");
            return false;
        }
        sqlite3_reset(stmtIngred);
    }
    sqlite3_finalize(stmtIngred);

    d_conn->executeQuery("COMMIT;");
    return true;
}

bool MealsRepository::deleteMeal(const std::string& mealName) {
    std::lock_guard<std::recursive_mutex> lock(d_conn->mutex());
    sqlite3* db = d_conn->raw();
    if (!db) return false;
    const std::string query = "DELETE FROM meals WHERE name = ?;";
    sqlite3_stmt* stmt = nullptr;
    bool success = false;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, mealName.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_DONE) {
            success = true;
        }
    }
    sqlite3_finalize(stmt);
    return success;
}

std::unique_ptr<Meal> MealsRepository::getMeal(const std::string& mealName) {
    std::lock_guard<std::recursive_mutex> lock(d_conn->mutex());
    sqlite3* db = d_conn->raw();
    if (!db) return nullptr;

    int mealId = getMealId(mealName);
    if (mealId == -1) return nullptr;

    const std::string getCategoryQuery = "SELECT category FROM meals WHERE id = ?;";
    sqlite3_stmt* stmtCat = nullptr;
    std::string category = "Uncategorized";
    if (sqlite3_prepare_v2(db, getCategoryQuery.c_str(), -1, &stmtCat, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmtCat, 1, mealId);
        if (sqlite3_step(stmtCat) == SQLITE_ROW) {
            if (const char* catText =
                    reinterpret_cast<const char*>(sqlite3_column_text(stmtCat, 0))) {
                category = catText;
            }
        }
    }
    sqlite3_finalize(stmtCat);

    const std::string query =
        "SELECT name, amount, unit, preparation, is_optional FROM ingredients "
        "WHERE meal_id = ?;";
    sqlite3_stmt* stmt = nullptr;
    std::vector<Ingredient> ingredients;

    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, mealId);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string ingName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            double amount = sqlite3_column_double(stmt, 1);
            int unit = sqlite3_column_int(stmt, 2);
            std::string prep = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            bool isOptional = sqlite3_column_int(stmt, 4) != 0;

            ingredients.emplace_back(ingName,
                                     Measurement(amount, static_cast<MeasurementUnit>(unit)), prep,
                                     isOptional);
        }
    }
    sqlite3_finalize(stmt);

    return std::make_unique<Meal>(mealName, ingredients, category);
}

bool MealsRepository::getAllMeals(std::vector<std::tuple<int, std::string, std::string>>& meals) {
    std::lock_guard<std::recursive_mutex> lock(d_conn->mutex());
    sqlite3* db = d_conn->raw();
    if (!db) return false;

    const std::string query = "SELECT id, name, category FROM meals ORDER BY name ASC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int id = sqlite3_column_int(stmt, 0);
            std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            std::string category = "Uncategorized";
            if (const char* catText =
                    reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2))) {
                category = catText;
            }
            meals.emplace_back(id, name, category);
        }
    }
    sqlite3_finalize(stmt);
    return true;
}

std::set<int> MealsRepository::getMealIdsWithOptionalIngredients() {
    std::lock_guard<std::recursive_mutex> lock(d_conn->mutex());
    std::set<int> ids;
    sqlite3* db = d_conn->raw();
    if (!db) return ids;

    const std::string query = "SELECT DISTINCT meal_id FROM ingredients WHERE is_optional = 1;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            ids.insert(sqlite3_column_int(stmt, 0));
        }
    }
    sqlite3_finalize(stmt);
    return ids;
}

bool MealsRepository::getAllIngredients(
    std::vector<std::pair<std::string, std::string>>& ingredients) {
    std::lock_guard<std::recursive_mutex> lock(d_conn->mutex());
    sqlite3* db = d_conn->raw();
    if (!db) return false;

    const std::string query =
        "SELECT name, category FROM available_ingredients ORDER BY name ASC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            std::string category = "Uncategorized";
            if (const char* catText =
                    reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1))) {
                category = catText;
            }
            ingredients.emplace_back(name, category);
        }
    }
    sqlite3_finalize(stmt);
    return true;
}

bool MealsRepository::addIngredient(const std::string& name, const std::string& category) {
    std::lock_guard<std::recursive_mutex> lock(d_conn->mutex());
    sqlite3* db = d_conn->raw();
    if (!db) return false;

    const std::string insertIngredient =
        "INSERT INTO available_ingredients (name, category) VALUES (?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, insertIngredient.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, category.c_str(), -1, SQLITE_TRANSIENT);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

bool MealsRepository::seedDefaultIngredients() {
    std::lock_guard<std::recursive_mutex> lock(d_conn->mutex());
    std::vector<std::pair<std::string, std::string>> existing;
    getAllIngredients(existing);
    if (!existing.empty()) return true;

    std::vector<std::pair<std::string, std::string>> defaultIngredients = {
        {"Spinach", "Vegetables"},
        {"Broccoli", "Vegetables"},
        {"Onion", "Vegetables"},
        {"Carrots", "Vegetables"},
        {"Yellow Bell Pepper", "Vegetables"},
        {"Red Bell Pepper", "Vegetables"},
        {"Chicken Breast", "Proteins"},
        {"Ground Turkey", "Proteins"},
        {"Ground Beef", "Proteins"},
        {"Eggs", "Proteins"},
        {"Egg", "Proteins"},
        {"Cottage Cheese", "Proteins"},
        {"Feta", "Dairy"},
        {"Parmesan Cheese", "Dairy"},
        {"Sharp Cheddar Cheese", "Dairy"},
        {"Milk", "Dairy"},
        {"Heavy Cream", "Dairy"},
        {"Butter", "Dairy"},
        {"Salt", "Pantry Staples"},
        {"Pepper", "Pantry Staples"},
        {"Olive Oil", "Pantry Staples"},
        {"Flour", "Pantry Staples"},
        {"All-Purpose Flour", "Pantry Staples"},
        {"Corn Starch", "Pantry Staples"},
        {"Baking Powder", "Pantry Staples"},
        {"Breadcrumbs", "Pantry Staples"},
        {"Garlic Powder", "Seasonings & Spices"},
        {"Onion Powder", "Seasonings & Spices"},
        {"Italian Seasoning", "Seasonings & Spices"},
        {"Paprika", "Seasonings & Spices"},
        {"Ginger", "Seasonings & Spices"},
        {"Vanilla Extract", "Seasonings & Spices"},
        {"Garlic", "Produce"},
        {"Crushed Tomatoes", "Produce"},
        {"Penne Pasta", "Grains & Pasta"},
        {"Pasta", "Grains & Pasta"},
        {"Chicken Broth", "Broths & Sauces"},
        {"Beef Broth", "Broths & Sauces"},
        {"Soy Sauce", "Broths & Sauces"},
        {"Maple Syrup", "Sweeteners"},
        {"Honey", "Sweeteners"},
        {"Sesame Seeds", "Other"},
        {"Fruit of Choice", "Other"}};

    bool allSuccess = true;
    d_conn->executeQuery("BEGIN TRANSACTION;");
    for (const auto& ing : defaultIngredients) {
        if (!addIngredient(ing.first, ing.second)) {
            allSuccess = false;
        }
    }
    if (allSuccess) {
        d_conn->executeQuery("COMMIT;");
    } else {
        d_conn->executeQuery("ROLLBACK;");
    }

    return allSuccess;
}

bool MealsRepository::seedDefaultMeals() {
    std::lock_guard<std::recursive_mutex> lock(d_conn->mutex());
    std::vector<std::tuple<int, std::string, std::string>> existing;
    getAllMeals(existing);
    if (!existing.empty()) return true;

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
    for (const auto& meal : defaultMeals) {
        if (!addMeal(meal)) {
            allSuccess = false;
        }
    }
    return allSuccess;
}
