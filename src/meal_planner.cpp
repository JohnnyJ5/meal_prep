#include "meal_planner.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_map>

namespace {

std::string normalizeIngredientKey(const std::string &name) {
    size_t start = name.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = name.find_last_not_of(" \t\r\n");
    std::string trimmed = name.substr(start, end - start + 1);
    std::transform(trimmed.begin(), trimmed.end(), trimmed.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return trimmed;
}

bool isVolumeUnit(MeasurementUnit unit) {
    return unit == MeasurementUnit::TABLESPOON || unit == MeasurementUnit::TEASPOON ||
           unit == MeasurementUnit::CUP || unit == MeasurementUnit::OUNCE;
}

std::string unitCompatibilityGroup(MeasurementUnit unit) {
    if (isVolumeUnit(unit)) {
        return "vol";
    }
    return std::to_string(static_cast<int>(unit));
}

}  // namespace

void ConsolidateAllIngredients(std::map<std::string, Ingredient> &allIngredients,
                               const std::vector<std::reference_wrapper<Meal>> &meals) {
    std::unordered_map<std::string, std::string> compositeKeyToMapKey;
    std::unordered_map<std::string, std::string> normNameToDisplayName;
    for (const auto &pair : allIngredients) {
        std::string normName = normalizeIngredientKey(pair.second.getName());
        std::string compKey =
            normName + "|" + unitCompatibilityGroup(pair.second.getAmount().getUnit());
        compositeKeyToMapKey[compKey] = pair.first;
        normNameToDisplayName.try_emplace(normName, pair.second.getName());
    }

    for (const auto &mealRef : meals) {
        const Meal &meal = mealRef.get();
        for (const auto &ingredient : meal.getIngredients()) {
            std::string normName = normalizeIngredientKey(ingredient.getName());
            std::string unitGroup = unitCompatibilityGroup(ingredient.getAmount().getUnit());
            std::string compKey = normName + "|" + unitGroup;

            auto compIt = compositeKeyToMapKey.find(compKey);
            if (compIt != compositeKeyToMapKey.end()) {
                Ingredient &existing = allIngredients[compIt->second];
                Ingredient normalized(existing.getName(), ingredient.getAmount(),
                                      ingredient.getPreparation(), ingredient.isOptional());
                existing += normalized;
            } else {
                std::string displayName = ingredient.getName();
                auto nameIt = normNameToDisplayName.find(normName);
                if (nameIt != normNameToDisplayName.end()) {
                    displayName = nameIt->second;
                } else {
                    normNameToDisplayName[normName] = displayName;
                }

                // Use plain displayName for the first unit group of this ingredient;
                // append #unitGroup if another incompatible unit group already uses displayName.
                std::string mapKey = (allIngredients.find(displayName) == allIngredients.end())
                                         ? displayName
                                         : (displayName + "#" + unitGroup);
                compositeKeyToMapKey[compKey] = mapKey;
                allIngredients[mapKey] =
                    Ingredient(displayName, ingredient.getAmount(), ingredient.getPreparation(),
                               ingredient.isOptional());
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
            categoryLookup[normalizeIngredientKey(pair.first)] = pair.second;
        }
    }

    os << "Whole Foods Order\n\nMeals:\n";
    for (const auto &mealRef : meals) {
        os << "- " << mealRef.get().getName() << "\n";
    }

    std::map<std::string, std::vector<Ingredient>> byCategory;
    for (const auto &pair : allIngredients) {
        std::string lookupKey = normalizeIngredientKey(
            pair.second.getName().empty() ? pair.first : pair.second.getName());
        auto it = categoryLookup.find(lookupKey);
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
