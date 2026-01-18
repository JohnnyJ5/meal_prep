#include "meal.h"
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>

class MealFactory
{
public:
    MealFactory();
    ~MealFactory() = default;
    std::unique_ptr<Meal> createMeal(const std::string & mealName);
    void getAvailableMeals(std::vector<std::string> & meals);
private:
    std::unordered_map<std::string, std::function<std::unique_ptr<Meal>()>> d_mealFactories;
};

