#ifndef MEAL_FACTORY_H
#define MEAL_FACTORY_H

#include "meal.h"
#include "db_manager.h"
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>

class MealFactory
{
public:
    MealFactory(std::shared_ptr<DBManager> dbManager);
    ~MealFactory() = default;
    
    std::unique_ptr<Meal> createMeal(const std::string & mealName);
    void getAvailableMeals(std::vector<std::string> & meals);
    
private:
    std::shared_ptr<DBManager> d_dbManager;
};

#endif // MEAL_FACTORY_H

