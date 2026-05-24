#pragma once

#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "core/db/db_connection.h"
#include "features/meals/meal.h"

/// SQLite-backed CRUD for meals, their ingredients, and the available
/// ingredients catalog.
class MealsRepository {
   public:
    explicit MealsRepository(std::shared_ptr<DbConnection> conn);

    bool addMeal(const Meal& meal);
    bool updateMeal(const Meal& meal);
    bool deleteMeal(const std::string& mealName);
    std::unique_ptr<Meal> getMeal(const std::string& mealName);
    bool getAllMeals(std::vector<std::tuple<int, std::string, std::string>>& meals);
    std::set<int> getMealIdsWithOptionalIngredients();

    bool addIngredient(const std::string& name, const std::string& category);
    bool getAllIngredients(std::vector<std::pair<std::string, std::string>>& ingredients);

    /// Seeds curated default meals and ingredients only if the tables are empty.
    bool seedDefaultMeals();
    bool seedDefaultIngredients();

   private:
    std::shared_ptr<DbConnection> d_conn;
    int getMealId(const std::string& mealName);
};
