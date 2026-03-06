#include "db_manager.h"
#include "ingredient_names.h"
#include "measurement.h"
#include <iostream>

DBManager::DBManager(const std::string &dbPath)
    : d_dbPath(dbPath), d_db(nullptr) {
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
  char *errMsg = nullptr;
  if (sqlite3_exec(d_db, query.c_str(), 0, 0, &errMsg) != SQLITE_OK) {
    std::cerr << "SQL error: " << errMsg << "\n";
    sqlite3_free(errMsg);
    return false;
  }
  return true;
}

bool DBManager::initializeSchema() {
  if (!d_db)
    return false;

  std::string createMealsTable = "CREATE TABLE IF NOT EXISTS meals ("
                                 "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                 "name TEXT UNIQUE NOT NULL"
                                 ");";

  std::string createIngredientsTable =
      "CREATE TABLE IF NOT EXISTS ingredients ("
      "id INTEGER PRIMARY KEY AUTOINCREMENT, "
      "meal_id INTEGER NOT NULL, "
      "name TEXT NOT NULL, "
      "amount REAL NOT NULL, "
      "unit INTEGER NOT NULL, "
      "preparation TEXT NOT NULL, "
      "FOREIGN KEY(meal_id) REFERENCES meals(id) ON DELETE CASCADE"
      ");";

  // Enable foreign keys
  executeQuery("PRAGMA foreign_keys = ON;");

  if (!executeQuery(createMealsTable))
    return false;
  if (!executeQuery(createIngredientsTable))
    return false;

  return true;
}

int DBManager::getMealId(const std::string &mealName) {
  if (!d_db)
    return -1;

  std::string query = "SELECT id FROM meals WHERE name = ?;";
  sqlite3_stmt *stmt;
  int mealId = -1;

  if (sqlite3_prepare_v2(d_db, query.c_str(), -1, &stmt, nullptr) ==
      SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, mealName.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
      mealId = sqlite3_column_int(stmt, 0);
    }
  }
  sqlite3_finalize(stmt);
  return mealId;
}

bool DBManager::addMeal(const Meal &meal) {
  if (!d_db)
    return false;

  // Begin transaction
  executeQuery("BEGIN TRANSACTION;");

  std::string insertMeal = "INSERT INTO meals (name) VALUES (?);";
  sqlite3_stmt *stmtMeal;
  if (sqlite3_prepare_v2(d_db, insertMeal.c_str(), -1, &stmtMeal, nullptr) !=
      SQLITE_OK) {
    executeQuery("ROLLBACK;");
    return false;
  }

  sqlite3_bind_text(stmtMeal, 1, meal.getName().c_str(), -1, SQLITE_TRANSIENT);
  if (sqlite3_step(stmtMeal) != SQLITE_DONE) {
    sqlite3_finalize(stmtMeal);
    executeQuery("ROLLBACK;");
    return false;
  }
  sqlite3_finalize(stmtMeal);

  int mealId = sqlite3_last_insert_rowid(d_db);

  std::string insertIngredient =
      "INSERT INTO ingredients (meal_id, name, amount, unit, preparation) "
      "VALUES (?, ?, ?, ?, ?);";
  sqlite3_stmt *stmtIngred;
  if (sqlite3_prepare_v2(d_db, insertIngredient.c_str(), -1, &stmtIngred,
                         nullptr) != SQLITE_OK) {
    executeQuery("ROLLBACK;");
    return false;
  }

  for (const auto &ing : meal.getIngredients()) {
    sqlite3_bind_int(stmtIngred, 1, mealId);
    sqlite3_bind_text(stmtIngred, 2, ing.getName().c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_double(stmtIngred, 3, ing.getAmount().getValue());
    sqlite3_bind_int(stmtIngred, 4,
                     static_cast<int>(ing.getAmount().getUnit()));

    // Handling preparation string, replacing empty/none with "None" if needed,
    // though ingredient handles it However, looking at Ingredient, we don't
    // have a public getter for preparation. Let's assume it's exposed or we
    // need to add it. Wait, looking at `ingredient.h`, `d_preparation` does NOT
    // have a getter. I need to fix that or ignore for now. I will add
    // getPreparation() to ingredient.h first, or write it directly. For now,
    // I'll assume we'll update it or just use "None". Let me check my thought
    // process: I will use a dummy "None" for now and update `ingredient.h`
    // shortly.

    // sqlite3_bind_text(stmtIngred, 5, ing.getPreparation().c_str(), -1,
    // SQLITE_TRANSIENT);
    sqlite3_bind_text(
        stmtIngred, 5, "None", -1,
        SQLITE_TRANSIENT); // Temporary until ingredient.h is checked

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
  if (!d_db)
    return false;

  // To update, the simplest approach is to delete the old meal ingredients and
  // recreate them. However we should probably keep the meal ID. Alternatively,
  // delete the meal and add it again (cascade handles ingredients).

  // Begin transaction
  executeQuery("BEGIN TRANSACTION;");

  // First, delete
  std::string delQuery = "DELETE FROM meals WHERE name = ?;";
  sqlite3_stmt *stmtDel;
  if (sqlite3_prepare_v2(d_db, delQuery.c_str(), -1, &stmtDel, nullptr) ==
      SQLITE_OK) {
    sqlite3_bind_text(stmtDel, 1, meal.getName().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmtDel);
  }
  sqlite3_finalize(stmtDel);

  executeQuery("COMMIT;");

  // Then add
  return addMeal(meal);
}

bool DBManager::deleteMeal(const std::string &mealName) {
  if (!d_db)
    return false;
  std::string query = "DELETE FROM meals WHERE name = ?;";
  sqlite3_stmt *stmt;
  bool success = false;
  if (sqlite3_prepare_v2(d_db, query.c_str(), -1, &stmt, nullptr) ==
      SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, mealName.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_DONE) {
      success = true;
    }
  }
  sqlite3_finalize(stmt);
  return success;
}

std::unique_ptr<Meal> DBManager::getMeal(const std::string &mealName) {
  if (!d_db)
    return nullptr;

  int mealId = getMealId(mealName);
  if (mealId == -1)
    return nullptr;

  std::string query = "SELECT name, amount, unit, preparation FROM ingredients "
                      "WHERE meal_id = ?;";
  sqlite3_stmt *stmt;
  std::vector<Ingredient> ingredients;

  if (sqlite3_prepare_v2(d_db, query.c_str(), -1, &stmt, nullptr) ==
      SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, mealId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      std::string ingName =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
      double amount = sqlite3_column_double(stmt, 1);
      int unit = sqlite3_column_int(stmt, 2);
      std::string prep =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));

      // Reconstruct ingredient. Assuming getPreparation exists soon.
      // Ingredient(name, Measurement(value, unit), prep)
      ingredients.push_back(Ingredient(
          ingName, Measurement(amount, static_cast<MeasurementUnit>(unit)),
          prep));
    }
  }
  sqlite3_finalize(stmt);

  if (ingredients.empty())
    return nullptr;

  return std::make_unique<Meal>(mealName, ingredients);
}

bool DBManager::getAllMeals(std::vector<std::string> &meals) {
  if (!d_db)
    return false;

  std::string query = "SELECT name FROM meals ORDER BY name ASC;";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(d_db, query.c_str(), -1, &stmt, nullptr) ==
      SQLITE_OK) {
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      meals.push_back(
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)));
    }
  }
  sqlite3_finalize(stmt);
  return true;
}

bool DBManager::seedDefaultMeals() {
  // This will seed the database with the initial hardcoded values if empty.
  std::vector<std::string> existing;
  getAllMeals(existing);
  if (!existing.empty())
    return true; // Already seeded

  std::vector<Meal> defaultMeals = {
      Meal("turkey-burgers",
           {Ingredient(IngredientNames::SPINACH,
                       Measurement(2.0, MeasurementUnit::CUP), "Chopped"),
            Ingredient(IngredientNames::FETA,
                       Measurement(4.0, MeasurementUnit::OUNCE)),
            Ingredient(IngredientNames::BREADCRUMBS,
                       Measurement(0.25, MeasurementUnit::CUP)),
            Ingredient(IngredientNames::GROUND_TURKEY,
                       Measurement(1.0, MeasurementUnit::POUND)),
            Ingredient(IngredientNames::SALT,
                       Measurement(1.0, MeasurementUnit::TEASPOON)),
            Ingredient(IngredientNames::PEPPER,
                       Measurement(0.5, MeasurementUnit::TEASPOON)),
            Ingredient(IngredientNames::GARLIC_POWDER,
                       Measurement(1.0, MeasurementUnit::TEASPOON)),
            Ingredient(IngredientNames::ONION_POWDER,
                       Measurement(0.5, MeasurementUnit::TEASPOON)),
            Ingredient(IngredientNames::ITALIAN_SEASONING,
                       Measurement(1.0, MeasurementUnit::TEASPOON)),
            Ingredient(IngredientNames::OLIVE_OIL,
                       Measurement(1.0, MeasurementUnit::TABLESPOON))}),
      Meal("turkey-meatballs",
           {Ingredient(IngredientNames::GROUND_TURKEY,
                       Measurement(1.0, MeasurementUnit::POUND)),
            Ingredient(IngredientNames::BREADCRUMBS,
                       Measurement(1.0, MeasurementUnit::CUP)),
            Ingredient(IngredientNames::ITALIAN_SEASONING,
                       Measurement(1.0, MeasurementUnit::TABLESPOON)),
            Ingredient(IngredientNames::ONION,
                       Measurement(1.0, MeasurementUnit::SMALL), "Diced"),
            Ingredient(IngredientNames::GARLIC,
                       Measurement(3.0, MeasurementUnit::CLOVE), "Minced"),
            Ingredient(IngredientNames::EGG,
                       Measurement(2.0, MeasurementUnit::WHOLE)),
            Ingredient(IngredientNames::MILK,
                       Measurement(0.25, MeasurementUnit::CUP)),
            Ingredient(IngredientNames::SALT,
                       Measurement(0.5, MeasurementUnit::TEASPOON)),
            Ingredient(IngredientNames::PEPPER,
                       Measurement(0.25, MeasurementUnit::TEASPOON))}),
      Meal("creamy-garlic-chicken-penne-spinach",
           {Ingredient(IngredientNames::CHICKEN_BREAST,
                       Measurement(2.0, MeasurementUnit::WHOLE), "Strips"),
            Ingredient(IngredientNames::PENNE_PASTA,
                       Measurement(2.0, MeasurementUnit::CUP)),
            Ingredient(IngredientNames::BROCCOLI,
                       Measurement(1.5, MeasurementUnit::CUP)),
            Ingredient(IngredientNames::SPINACH,
                       Measurement(1.0, MeasurementUnit::CUP)),
            Ingredient(IngredientNames::OLIVE_OIL,
                       Measurement(2.0, MeasurementUnit::TABLESPOON)),
            Ingredient(IngredientNames::SALT,
                       Measurement(1.0, MeasurementUnit::TEASPOON)),
            Ingredient(IngredientNames::PEPPER,
                       Measurement(0.5, MeasurementUnit::TEASPOON)),
            Ingredient(IngredientNames::PAPRIKA,
                       Measurement(0.5, MeasurementUnit::TEASPOON)),
            Ingredient(IngredientNames::GARLIC,
                       Measurement(3.0, MeasurementUnit::CLOVE), "Minced"),
            Ingredient(IngredientNames::HEAVY_CREAM,
                       Measurement(1.0, MeasurementUnit::CUP)),
            Ingredient(IngredientNames::PARMESAN_CHEESE,
                       Measurement(0.5, MeasurementUnit::CUP), "Grated")}),
      Meal("creamy-garlic-chicken",
           {Ingredient(IngredientNames::CHICKEN_BREAST,
                       Measurement(4.0, MeasurementUnit::WHOLE), "Thin Sliced"),
            Ingredient(IngredientNames::FLOUR,
                       Measurement(0.25, MeasurementUnit::CUP)),
            Ingredient(IngredientNames::BUTTER,
                       Measurement(1.5, MeasurementUnit::TABLESPOON)),
            Ingredient(IngredientNames::OLIVE_OIL,
                       Measurement(1.0, MeasurementUnit::TABLESPOON)),
            Ingredient(IngredientNames::GARLIC,
                       Measurement(1.0, MeasurementUnit::HEAD), "Peeled"),
            Ingredient(IngredientNames::CHICKEN_BROTH,
                       Measurement(0.5, MeasurementUnit::CUP)),
            Ingredient(IngredientNames::HEAVY_CREAM,
                       Measurement(1.0, MeasurementUnit::CUP)),
            Ingredient(IngredientNames::SPINACH,
                       Measurement(2.0, MeasurementUnit::CUP))}),
      Meal("baked-chicken-breast",
           {Ingredient(IngredientNames::CHICKEN_BREAST,
                       Measurement(4.0, MeasurementUnit::WHOLE)),
            Ingredient(IngredientNames::OLIVE_OIL,
                       Measurement(1.0, MeasurementUnit::TABLESPOON)),
            Ingredient(IngredientNames::PAPRIKA,
                       Measurement(2.0, MeasurementUnit::TEASPOON)),
            Ingredient(IngredientNames::ITALIAN_SEASONING,
                       Measurement(1.0, MeasurementUnit::TEASPOON)),
            Ingredient(IngredientNames::GARLIC_POWDER,
                       Measurement(0.5, MeasurementUnit::TEASPOON)),
            Ingredient(IngredientNames::SALT,
                       Measurement(0.5, MeasurementUnit::TEASPOON)),
            Ingredient(IngredientNames::PEPPER,
                       Measurement(0.25, MeasurementUnit::TEASPOON))}),
      Meal("cheesy-hamburger-pasta-skillet",
           {Ingredient(IngredientNames::OLIVE_OIL,
                       Measurement(1.0, MeasurementUnit::TABLESPOON)),
            Ingredient(IngredientNames::GROUND_BEEF,
                       Measurement(1.0, MeasurementUnit::POUND)),
            Ingredient(IngredientNames::SALT,
                       Measurement(2.0, MeasurementUnit::TEASPOON)),
            Ingredient(IngredientNames::BEEF_BROTH,
                       Measurement(1.5, MeasurementUnit::CUP)),
            Ingredient(IngredientNames::PASTA,
                       Measurement(8.0, MeasurementUnit::OUNCE)),
            Ingredient(IngredientNames::CRUSHED_TOMATOES,
                       Measurement(14.5, MeasurementUnit::OUNCE)),
            Ingredient(IngredientNames::SHARP_CHEDDAR_CHEESE,
                       Measurement(2.0, MeasurementUnit::CUP)),
            Ingredient(IngredientNames::HEAVY_CREAM,
                       Measurement(0.5, MeasurementUnit::CUP))}),
      Meal("cottage-cheese-pancakes",
           {Ingredient(IngredientNames::EGGS,
                       Measurement(4.0, MeasurementUnit::WHOLE)),
            Ingredient(IngredientNames::COTTAGE_CHEESE,
                       Measurement(1.5, MeasurementUnit::CUP)),
            Ingredient(IngredientNames::MAPLE_SYRUP,
                       Measurement(3.0, MeasurementUnit::TABLESPOON)),
            Ingredient(IngredientNames::VANILLA_EXTRACT,
                       Measurement(1.0, MeasurementUnit::TEASPOON)),
            Ingredient(IngredientNames::ALL_PURPOSE_FLOUR,
                       Measurement(1.0, MeasurementUnit::CUP)),
            Ingredient(IngredientNames::BAKING_POWDER,
                       Measurement(0.5, MeasurementUnit::TABLESPOON)),
            Ingredient(IngredientNames::FRUIT_OF_CHOICE,
                       Measurement(1.0, MeasurementUnit::CUP))}),
      Meal("chicken-stir-fry",
           {Ingredient(IngredientNames::CHICKEN_BREAST,
                       Measurement(3.0, MeasurementUnit::WHOLE)),
            Ingredient(IngredientNames::SALT,
                       Measurement(0.5, MeasurementUnit::TEASPOON)),
            Ingredient(IngredientNames::PEPPER,
                       Measurement(0.5, MeasurementUnit::TEASPOON)),
            Ingredient(IngredientNames::OLIVE_OIL,
                       Measurement(2.0, MeasurementUnit::TABLESPOON)),
            Ingredient(IngredientNames::BROCCOLI,
                       Measurement(2.0, MeasurementUnit::CUP)),
            Ingredient(IngredientNames::YELLOW_BELL_PEPPER,
                       Measurement(1.0, MeasurementUnit::HALF)),
            Ingredient(IngredientNames::RED_BELL_PEPPER,
                       Measurement(1.0, MeasurementUnit::HALF)),
            Ingredient(IngredientNames::CARROTS,
                       Measurement(0.5, MeasurementUnit::CUP)),
            Ingredient(IngredientNames::GINGER,
                       Measurement(0.5, MeasurementUnit::TEASPOON)),
            Ingredient(IngredientNames::GARLIC,
                       Measurement(2.0, MeasurementUnit::TEASPOON)),
            Ingredient(IngredientNames::SESAME_SEEDS,
                       Measurement(2.0, MeasurementUnit::TABLESPOON)),
            Ingredient(IngredientNames::CORN_STARCH,
                       Measurement(1.0, MeasurementUnit::TABLESPOON)),
            Ingredient(IngredientNames::CHICKEN_BROTH,
                       Measurement(0.25, MeasurementUnit::CUP)),
            Ingredient(IngredientNames::SOY_SAUCE,
                       Measurement(0.25, MeasurementUnit::CUP)),
            Ingredient(IngredientNames::HONEY,
                       Measurement(2.0, MeasurementUnit::TABLESPOON))})};

  bool allSuccess = true;
  for (const auto &meal : defaultMeals) {
    if (!addMeal(meal)) {
      allSuccess = false;
    }
  }

  return allSuccess;
}
