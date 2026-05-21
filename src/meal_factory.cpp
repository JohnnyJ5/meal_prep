#include "meal_factory.h"

#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "meal.h"

// MealFactory constructor
MealFactory::MealFactory(std::shared_ptr<DBManager> dbManager)
    : d_dbManager(std::move(dbManager)) {}

// Meal factory function — excludes all optional ingredients
std::unique_ptr<Meal> MealFactory::createMeal(const std::string &mealName) {
    return createMeal(mealName, {});
}

std::unique_ptr<Meal> MealFactory::createMeal(const std::string &mealName,
                                              const std::set<std::string> &enabledAddOns) {
    if (!d_dbManager) return nullptr;
    auto raw = d_dbManager->getMeal(mealName);
    if (!raw) return nullptr;

    std::vector<Ingredient> filtered;
    filtered.reserve(raw->getIngredients().size());
    for (const auto &ing : raw->getIngredients()) {
        if (ing.isOptional() && enabledAddOns.find(ing.getName()) == enabledAddOns.end()) {
            continue;
        }
        filtered.push_back(ing);
    }
    return std::make_unique<Meal>(raw->getName(), filtered, raw->getCategory());
}

// Function to get all available meal names and categories
void MealFactory::getAvailableMeals(std::vector<std::tuple<int, std::string, std::string>> &meals) {
    if (d_dbManager) {
        d_dbManager->getAllMeals(meals);
    }
}