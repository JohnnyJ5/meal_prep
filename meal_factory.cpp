#include "meal_factory.h"
#include "meal.h"
#include <string>
#include <unordered_map>
#include <vector>

// // Factory function type
// using MealFactory = std::function<std::unique_ptr<Meal>()>;

MealFactory::MealFactory() {
    d_mealFactories = {
        {"turkey-burgers", []() { return std::make_unique<TurkeyBurgers>(); }},
        {"turkey-meatballs", []() { return std::make_unique<TurkeyMeatballs>(); }},
        {"creamy-garlic-chicken-penne-spinach", []() { return std::make_unique<CreamyGarlicChickenPenneSpinach>(); }},
        {"creamy-garlic-chicken", []() { return std::make_unique<CreamyGarlicChicken>(); }},
        {"baked-chicken-breast", []() { return std::make_unique<BakedChickenBreast>(); }},
        {"cheesy-hamburger-pasta-skillet", []() { return std::make_unique<CheesyHamburgerPastaSkillet>(); }},
        {"cottage-cheese-pancakes", []() { return std::make_unique<CottageCheesePancakes>(); }},
        {"chicken-stir-fry", []() { return std::make_unique<ChickenStirFry>(); }}
    };
}

// Meal factory function
std::unique_ptr<Meal> MealFactory::createMeal(const std::string& mealName) {
    auto it = d_mealFactories.find(mealName);
    if (it != d_mealFactories.end()) {
        return it->second();
    }
    return nullptr; // Meal not found
}

// Function to get all available meal names
void MealFactory::getAvailableMeals(std::vector<std::string> & meals)
{
    for (const auto& pair : d_mealFactories)
    {
        meals.push_back(pair.first);
    }
}