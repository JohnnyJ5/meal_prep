#pragma once

#include <functional>
#include <map>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "ingredient.h"
#include "meal.h"

/**
 * @brief Consolidates ingredients from a list of meals into a single map.
 *
 * Aggregates amounts of the same ingredients across different meals.
 *
 * @param allIngredients The map to populate with consolidated ingredients.
 * @param meals The list of meals whose ingredients should be consolidated.
 */
void ConsolidateAllIngredients(std::map<std::string, Ingredient> &allIngredients,
                               const std::vector<std::reference_wrapper<Meal>> &meals);

/**
 * @brief Formats the Whole Foods order description with meal names and categorized ingredients.
 *
 * @param os The output stream to write the formatted order text to.
 * @param meals The list of meals used to create the consolidated ingredient list.
 * @param allIngredients The consolidated ingredients map.
 * @param availableIngredients Pairs of (ingredient name, category) from the database.
 */
void FormatWholeFoodsOrder(
    std::ostream &os, const std::vector<std::reference_wrapper<Meal>> &meals,
    const std::map<std::string, Ingredient> &allIngredients,
    const std::vector<std::pair<std::string, std::string>> &availableIngredients);

/**
 * @brief Prints the weekly meal schedule to the given output stream.
 *
 * @param os The output stream to print to.
 * @param schedule The weekly meal schedule.
 */
void PrintWeeklySchedule(std::ostream &os,
                         const std::map<std::string, std::vector<std::string>> &schedule);
