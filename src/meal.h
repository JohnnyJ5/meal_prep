#pragma once

#include <string>
#include <vector>

#include "ingredient.h"

/**
 * @brief Represents a meal and its required ingredients.
 */
class Meal {
   public:
    /**
     * @brief Constructs a Meal.
     * @param name The name of the meal.
     * @param ingredients The list of ingredients required.
     * @param category The category of the meal.
     * @param verified Whether the recipe has been verified.
     */
    Meal(const std::string &name, const std::vector<Ingredient> &ingredients,
         const std::string &category = "Uncategorized", bool verified = false)
        : d_name(name), d_ingredients(ingredients), d_category(category), d_verified(verified) {}

    std::string getName() const { return d_name; }
    const std::vector<Ingredient> &getIngredients() const { return d_ingredients; }
    std::string getCategory() const { return d_category; }
    bool isVerified() const { return d_verified; }

   private:
    std::string d_name;
    std::vector<Ingredient> d_ingredients;
    std::string d_category;
    bool d_verified{false};
};
