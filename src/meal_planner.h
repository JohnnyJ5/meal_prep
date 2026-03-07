#pragma once

#include "config_parser.h"
#include "ingredient.h"
#include "meal.h"
#include <map>
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
 * @brief Formats the weekly schedule and consolidated ingredients, then emails
 * them.
 *
 * @param allIngredients The consolidated map of ingredients.
 * @param schedule The weekly meal schedule.
 * @param config The application configuration containing email credentials.
 */
void SendPlanEmail(
    const std::map<std::string, Ingredient> &allIngredients,
    const std::map<std::string, std::vector<std::string>> &schedule,
    const Config &config);

#include <ostream>

/**
 * @brief Prints the weekly meal schedule to the given output stream.
 *
 * @param os The output stream to print to.
 * @param schedule The weekly meal schedule.
 */
void PrintWeeklySchedule(
    std::ostream &os,
    const std::map<std::string, std::vector<std::string>> &schedule);
