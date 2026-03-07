#ifndef MEAL_FACTORY_H
#define MEAL_FACTORY_H

#include "db_manager.h"
#include "meal.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @brief Factory class for creating and retrieving meals from the database.
 */
class MealFactory {
public:
  /**
   * @brief Constructs a MealFactory with the given database manager.
   * @param dbManager Shared pointer to the database manager.
   */
  MealFactory(std::shared_ptr<DBManager> dbManager);
  ~MealFactory() = default;

  /**
   * @brief Creates a Meal object by retrieving its details from the database.
   * @param mealName The name of the meal to create.
   * @return unique_ptr to the created Meal, or nullptr if not found.
   */
  std::unique_ptr<Meal> createMeal(const std::string &mealName);

  /**
   * @brief Populates a vector with the names of all available meals.
   * @param meals The vector to populate.
   */
  void getAvailableMeals(std::vector<std::string> &meals);

private:
  std::shared_ptr<DBManager> d_dbManager;
};

#endif // MEAL_FACTORY_H
