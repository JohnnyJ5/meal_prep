#ifndef MEAL_H
#define MEAL_H

#include "ingredient.h"
#include <string>
#include <vector>

class Meal {
public:
  Meal(const std::string &name, const std::vector<Ingredient> &ingredients)
      : d_name(name), d_ingredients(ingredients) {}

  std::string getName() const { return d_name; }
  const std::vector<Ingredient> &getIngredients() const {
    return d_ingredients;
  }

private:
  std::string d_name;
  std::vector<Ingredient> d_ingredients;
};

#endif // MEAL_H