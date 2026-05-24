#pragma once

#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "features/meals/meal.h"
#include "features/meals/meals_repository.h"

/// Creates Meal objects from the MealsRepository, with optional add-on
/// filtering.
class MealFactory {
   public:
    explicit MealFactory(std::shared_ptr<MealsRepository> meals);
    ~MealFactory() = default;

    /// Creates a Meal excluding all optional ingredients.
    std::unique_ptr<Meal> createMeal(const std::string& mealName);

    /// Creates a Meal including only the optional ingredients named in
    /// @p enabledAddOns. Non-optional ingredients are always included.
    std::unique_ptr<Meal> createMeal(const std::string& mealName,
                                     const std::set<std::string>& enabledAddOns);

    /// Fills @p meals with (id, name, category) for every meal in the DB.
    void getAvailableMeals(std::vector<std::tuple<int, std::string, std::string>>& meals);

   private:
    std::shared_ptr<MealsRepository> d_meals;
};
