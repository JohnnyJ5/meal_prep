#pragma once

#include <functional>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "db_manager.h"
#include "meal.h"

/**
 * @brief Factory class for creating and retrieving meals from the database.
 */
class MealFactory {
   public:
    /**
     * @brief Constructs a MealFactory with the given database manager.
     * @param dbManager Shared pointer to the database manager.
     */
    explicit MealFactory(std::shared_ptr<DBManager> dbManager);
    ~MealFactory() = default;

    /**
     * @brief Creates a Meal object by retrieving its details from the database.
     *
     * Optional (add-on) ingredients are excluded by default. Use the overload
     * below to include a specific set of add-ons.
     *
     * @param mealName The name of the meal to create.
     * @return unique_ptr to the created Meal, or nullptr if not found.
     */
    std::unique_ptr<Meal> createMeal(const std::string &mealName);

    /**
     * @brief Creates a Meal object including only the selected optional
     * (add-on) ingredients.
     *
     * Base (non-optional) ingredients are always included. An optional
     * ingredient is included iff its name appears in @p enabledAddOns.
     *
     * @param mealName The name of the meal to create.
     * @param enabledAddOns Names of optional ingredients to include.
     * @return unique_ptr to the created Meal, or nullptr if not found.
     */
    std::unique_ptr<Meal> createMeal(const std::string &mealName,
                                     const std::set<std::string> &enabledAddOns);

    /**
     * @brief Populates a vector with the names and categories of all available
     * meals.
     * @param meals The vector to populate.
     */
    void getAvailableMeals(std::vector<std::tuple<int, std::string, std::string>> &meals);

   private:
    std::shared_ptr<DBManager> d_dbManager;
};
