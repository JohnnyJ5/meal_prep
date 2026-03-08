#include "meal_factory.h"
#include "meal.h"
#include <string>
#include <unordered_map>
#include <vector>

// MealFactory constructor
MealFactory::MealFactory(std::shared_ptr<DBManager> dbManager)
    : d_dbManager(dbManager) {}

// Meal factory function
std::unique_ptr<Meal> MealFactory::createMeal(const std::string &mealName) {
  if (d_dbManager) {
    return d_dbManager->getMeal(mealName);
  }
  return nullptr;
}

// Function to get all available meal names and categories
void MealFactory::getAvailableMeals(
    std::vector<std::pair<std::string, std::string>> &meals) {
  if (d_dbManager) {
    d_dbManager->getAllMeals(meals);
  }
}