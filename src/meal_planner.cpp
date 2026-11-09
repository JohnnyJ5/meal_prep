#include "meal_planner.h"

#include <sstream>
#include <unordered_map>

void ConsolidateAllIngredients(std::map<std::string, Ingredient> &allIngredients,
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

void FormatWholeFoodsOrder(
    std::ostream &os, const std::vector<std::reference_wrapper<Meal>> &meals,
    const std::map<std::string, Ingredient> &allIngredients,
    const std::vector<std::pair<std::string, std::string>> &availableIngredients) {
    std::unordered_map<std::string, std::string> categoryLookup;
    categoryLookup.reserve(availableIngredients.size());
    for (const auto &pair : availableIngredients) {
        if (!pair.second.empty()) {
            categoryLookup[pair.first] = pair.second;
        }
    }

    os << "Whole Foods Order\n\nMeals:\n";
    for (const auto &mealRef : meals) {
        os << "- " << mealRef.get().getName() << "\n";
    }

    std::map<std::string, std::vector<Ingredient>> byCategory;
    for (const auto &pair : allIngredients) {
        auto it = categoryLookup.find(pair.first);
        const std::string category = (it != categoryLookup.end()) ? it->second : "Uncategorized";
        byCategory[category].push_back(pair.second);
    }

    os << "\nIngredients:\n";
    for (const auto &catPair : byCategory) {
        os << "\n" << catPair.first << ":\n";
        for (const auto &ing : catPair.second) {
            os << "- " << ing << "\n";
        }
    }
}

void PrintWeeklySchedule(std::ostream &os,
                         const std::map<std::string, std::vector<std::string>> &schedule) {
    os << "\nWeekly Meal Prep Schedule:\n";
    std::vector<std::string> days = {"Monday", "Tuesday",  "Wednesday", "Thursday",
                                     "Friday", "Saturday", "Sunday"};
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
