#include "features/meals/meal_factory.h"

#include <utility>

MealFactory::MealFactory(std::shared_ptr<MealsRepository> meals) : d_meals(std::move(meals)) {}

std::unique_ptr<Meal> MealFactory::createMeal(const std::string& mealName) {
    return createMeal(mealName, {});
}

std::unique_ptr<Meal> MealFactory::createMeal(const std::string& mealName,
                                              const std::set<std::string>& enabledAddOns) {
    if (!d_meals) return nullptr;
    auto raw = d_meals->getMeal(mealName);
    if (!raw) return nullptr;

    std::vector<Ingredient> filtered;
    filtered.reserve(raw->getIngredients().size());
    for (const auto& ing : raw->getIngredients()) {
        if (ing.isOptional() && enabledAddOns.find(ing.getName()) == enabledAddOns.end()) {
            continue;
        }
        filtered.push_back(ing);
    }
    return std::make_unique<Meal>(raw->getName(), filtered, raw->getCategory());
}

void MealFactory::getAvailableMeals(std::vector<std::tuple<int, std::string, std::string>>& meals) {
    if (d_meals) {
        d_meals->getAllMeals(meals);
    }
}
