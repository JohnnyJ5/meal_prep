#include "meal_planner.h"
#include <sstream>

void ConsolidateAllIngredients(
    std::map<std::string, Ingredient> &allIngredients,
    const std::vector<std::reference_wrapper<Meal>> &meals) {
  for (const auto &mealRef : meals) {
    const Meal &meal = mealRef.get();
    for (const auto &ingredient : meal.getIngredients()) {
      auto it = allIngredients.find(ingredient.getName());
      if (it != allIngredients.end()) {
        it->second += ingredient;
      } else {
        allIngredients[ingredient.getName()] = ingredient;
      }
    }
  }
}


void PrintWeeklySchedule(
    std::ostream &os,
    const std::map<std::string, std::vector<std::string>> &schedule) {
  os << "\nWeekly Meal Prep Schedule:\n";
  std::vector<std::string> days = {"Monday",   "Tuesday", "Wednesday",
                                   "Thursday", "Friday",  "Saturday",
                                   "Sunday"};
  for (const auto &day : days) {
    os << day << ":\n";
    auto it = schedule.find(day);
    if (it != schedule.end() && !it->second.empty()) {
      for (const auto &m : it->second) {
        os << "  - " << m << "\n";
      }
    } else {
      os << "  - (Nothing planned)\n";
    }
  }
}
