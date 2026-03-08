#ifndef MEAL_H
#define MEAL_H

#include "ingredient.h"
#include <string>
#include <vector>

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
   */
  Meal(const std::string &name, const std::vector<Ingredient> &ingredients,
       const std::string &category = "Uncategorized")
      : d_name(name), d_ingredients(ingredients), d_category(category) {}

  std::string getName() const { return d_name; }
  const std::vector<Ingredient> &getIngredients() const {
    return d_ingredients;
  }
  std::string getCategory() const { return d_category; }

private:
  std::string d_name;
  std::vector<Ingredient> d_ingredients;
  std::string d_category;
};

#endif // MEAL_H