#pragma once

#include "ingredient.h"
#include "meal.h"
#include <map>
#include <string>
#include <vector>

void ConsolidateAllIngredients(
    std::map<std::string, Ingredient> &allIngredients,
    const std::vector<std::reference_wrapper<Meal>> &meals);

void SendPlanEmail(
    const std::map<std::string, Ingredient> &allIngredients,
    const std::map<std::string, std::vector<std::string>> &schedule);

#include <ostream>

void PrintWeeklySchedule(
    std::ostream &os,
    const std::map<std::string, std::vector<std::string>> &schedule);
