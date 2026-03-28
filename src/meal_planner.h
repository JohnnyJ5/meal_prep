#pragma once

#include "ingredient.h"
#include "meal.h"
#include <functional>
#include <map>
#include <ostream>
#include <string>
#include <vector>

/**
 * @brief Consolidates ingredients from a list of meals into a single map.
 *
 * Aggregates amounts of the same ingredients across different meals.
 *
 * @param allIngredients The map to populate with consolidated ingredients.
 * @param meals The list of meals whose ingredients should be consolidated.
 */
void ConsolidateAllIngredients(
    std::map<std::string, Ingredient> &allIngredients,
    const std::vector<std::reference_wrapper<Meal>> &meals);

/**
 * @brief Prints the weekly meal schedule to the given output stream.
 *
 * @param os The output stream to print to.
 * @param schedule The weekly meal schedule.
 */
void PrintWeeklySchedule(
    std::ostream &os,
    const std::map<std::string, std::vector<std::string>> &schedule);
