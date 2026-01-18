#ifndef MEAL_H
#define MEAL_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include "ingredient.h"
#include "measurement.h"
#include "ingredient_names.h"

class Meal {
public:
    Meal(const std::string& name, const std::vector<Ingredient>& ingredients)
        : d_name(name), d_ingredients(ingredients) {}
    
    std::string getName() const { return d_name; }
    const std::vector<Ingredient>& getIngredients() const { return d_ingredients; }
    
private:
    std::string d_name;
    std::vector<Ingredient> d_ingredients;
};

class TurkeyBurgers : public Meal {
public:
    TurkeyBurgers()
        : Meal("Turkey Burgers", {
            Ingredient(IngredientNames::SPINACH, Measurement(2.0, MeasurementUnit::CUP), Preparation::k_CHOPPED),
            Ingredient(IngredientNames::FETA, Measurement(4.0, MeasurementUnit::OUNCE)),
            Ingredient(IngredientNames::BREADCRUMBS, Measurement(0.25, MeasurementUnit::CUP)),
            Ingredient(IngredientNames::GROUND_TURKEY, Measurement(1.0, MeasurementUnit::POUND)),
            Ingredient(IngredientNames::SALT, Measurement(1.0, MeasurementUnit::TEASPOON)),
            Ingredient(IngredientNames::PEPPER, Measurement(0.5, MeasurementUnit::TEASPOON)),
            Ingredient(IngredientNames::GARLIC_POWDER, Measurement(1.0, MeasurementUnit::TEASPOON)),
            Ingredient(IngredientNames::ONION_POWDER, Measurement(0.5, MeasurementUnit::TEASPOON)),
            Ingredient(IngredientNames::ITALIAN_SEASONING, Measurement(1.0, MeasurementUnit::TEASPOON)),
            Ingredient(IngredientNames::OLIVE_OIL, Measurement(1.0, MeasurementUnit::TABLESPOON)),
        }) {}
};

class TurkeyMeatballs : public Meal {
public:
    TurkeyMeatballs()
        : Meal("Turkey Meatballs", {
            Ingredient(IngredientNames::GROUND_TURKEY, Measurement(1.0, MeasurementUnit::POUND)),
            Ingredient(IngredientNames::BREADCRUMBS, Measurement(1.0, MeasurementUnit::CUP)),
            Ingredient(IngredientNames::ITALIAN_SEASONING, Measurement(1.0, MeasurementUnit::TABLESPOON)),
            Ingredient(IngredientNames::ONION, Measurement(1.0, MeasurementUnit::SMALL), Preparation::k_DICED),
            Ingredient(IngredientNames::GARLIC, Measurement(3.0, MeasurementUnit::CLOVE), Preparation::k_MINCED),
            Ingredient(IngredientNames::EGG, Measurement(2.0, MeasurementUnit::WHOLE)),
            Ingredient(IngredientNames::MILK, Measurement(0.25, MeasurementUnit::CUP)),
            Ingredient(IngredientNames::SALT, Measurement(0.5, MeasurementUnit::TEASPOON)),
            Ingredient(IngredientNames::PEPPER, Measurement(0.25, MeasurementUnit::TEASPOON)),
        }) {}
};

class CreamyGarlicChickenPenneSpinach : public Meal {
public:
    CreamyGarlicChickenPenneSpinach()
        : Meal("Creamy Garlic Chicken Penne Spinach", {
            Ingredient(IngredientNames::CHICKEN_BREAST, Measurement(2.0, MeasurementUnit::WHOLE), Preparation::k_STRIPS),
            Ingredient(IngredientNames::PENNE_PASTA, Measurement(2.0, MeasurementUnit::CUP)),
            Ingredient(IngredientNames::BROCCOLI, Measurement(1.5, MeasurementUnit::CUP)),
            Ingredient(IngredientNames::SPINACH, Measurement(1.0, MeasurementUnit::CUP)),
            Ingredient(IngredientNames::OLIVE_OIL, Measurement(2.0, MeasurementUnit::TABLESPOON)),
            Ingredient(IngredientNames::SALT, Measurement(1.0, MeasurementUnit::TEASPOON)),
            Ingredient(IngredientNames::PEPPER, Measurement(0.5, MeasurementUnit::TEASPOON)),
            Ingredient(IngredientNames::PAPRIKA, Measurement(0.5, MeasurementUnit::TEASPOON)),
            Ingredient(IngredientNames::GARLIC, Measurement(3.0, MeasurementUnit::CLOVE), Preparation::k_MINCED),
            Ingredient(IngredientNames::HEAVY_CREAM, Measurement(1.0, MeasurementUnit::CUP)),
            Ingredient(IngredientNames::PARMESAN_CHEESE, Measurement(0.5, MeasurementUnit::CUP), Preparation::k_GRATED),
        }) {}
};

class CreamyGarlicChicken : public Meal {
public:
    CreamyGarlicChicken()
        : Meal("Creamy Garlic Chicken", {
            Ingredient(IngredientNames::CHICKEN_BREAST, Measurement(4.0, MeasurementUnit::WHOLE), Preparation::k_THIN_SLICED),
            Ingredient(IngredientNames::FLOUR, Measurement(0.25, MeasurementUnit::CUP)),
            Ingredient(IngredientNames::BUTTER, Measurement(1.5, MeasurementUnit::TABLESPOON)),
            Ingredient(IngredientNames::OLIVE_OIL, Measurement(1.0, MeasurementUnit::TABLESPOON)),
            Ingredient(IngredientNames::GARLIC, Measurement(1.0, MeasurementUnit::HEAD), Preparation::K_PEELED),
            Ingredient(IngredientNames::CHICKEN_BROTH, Measurement(0.5, MeasurementUnit::CUP)),
            Ingredient(IngredientNames::HEAVY_CREAM, Measurement(1.0, MeasurementUnit::CUP)),
            Ingredient(IngredientNames::SPINACH, Measurement(2.0, MeasurementUnit::CUP)),
        }) {}
};

class BakedChickenBreast : public Meal {
public:
    BakedChickenBreast()
        : Meal("Baked Chicken Breast", {
            Ingredient(IngredientNames::CHICKEN_BREAST, Measurement(4.0, MeasurementUnit::WHOLE)),
            Ingredient(IngredientNames::OLIVE_OIL, Measurement(1.0, MeasurementUnit::TABLESPOON)),
            Ingredient(IngredientNames::PAPRIKA, Measurement(2.0, MeasurementUnit::TEASPOON)),
            Ingredient(IngredientNames::ITALIAN_SEASONING, Measurement(1.0, MeasurementUnit::TEASPOON)),
            Ingredient(IngredientNames::GARLIC_POWDER, Measurement(0.5, MeasurementUnit::TEASPOON)),
            Ingredient(IngredientNames::SALT, Measurement(0.5, MeasurementUnit::TEASPOON)),
            Ingredient(IngredientNames::PEPPER, Measurement(0.25, MeasurementUnit::TEASPOON)),
        }) {}
};

class CheesyHamburgerPastaSkillet : public Meal {
public:
    CheesyHamburgerPastaSkillet()
        : Meal("Cheesy Hamburger Pasta Skillet", {
            Ingredient(IngredientNames::OLIVE_OIL, Measurement(1.0, MeasurementUnit::TABLESPOON)),
            Ingredient(IngredientNames::GROUND_BEEF, Measurement(1.0, MeasurementUnit::POUND)),
            Ingredient(IngredientNames::SALT, Measurement(2.0, MeasurementUnit::TEASPOON)),
            Ingredient(IngredientNames::BEEF_BROTH, Measurement(1.5, MeasurementUnit::CUP)),
            Ingredient(IngredientNames::PASTA, Measurement(8.0, MeasurementUnit::OUNCE)),
            Ingredient(IngredientNames::CRUSHED_TOMATOES, Measurement(14.5, MeasurementUnit::OUNCE)),
            Ingredient(IngredientNames::SHARP_CHEDDAR_CHEESE, Measurement(2.0, MeasurementUnit::CUP)),
            Ingredient(IngredientNames::HEAVY_CREAM, Measurement(0.5, MeasurementUnit::CUP))
        }) {}
};

class CottageCheesePancakes : public Meal {
public:
    CottageCheesePancakes()
        : Meal("Cottage Cheese Pancakes", {
            Ingredient(IngredientNames::EGGS, Measurement(4.0, MeasurementUnit::WHOLE)),
            Ingredient(IngredientNames::COTTAGE_CHEESE, Measurement(1.5, MeasurementUnit::CUP)),
            Ingredient(IngredientNames::MAPLE_SYRUP, Measurement(3.0, MeasurementUnit::TABLESPOON)),
            Ingredient(IngredientNames::VANILLA_EXTRACT, Measurement(1.0, MeasurementUnit::TEASPOON)),
            Ingredient(IngredientNames::ALL_PURPOSE_FLOUR, Measurement(1.0, MeasurementUnit::CUP)),
            Ingredient(IngredientNames::BAKING_POWDER, Measurement(0.5, MeasurementUnit::TABLESPOON)),
            Ingredient(IngredientNames::FRUIT_OF_CHOICE, Measurement(1.0, MeasurementUnit::CUP)),
        }) {}
};

class ChickenStirFry : public Meal {
public:
    ChickenStirFry()
        : Meal("Chicken Stir Fry", {
            Ingredient(IngredientNames::CHICKEN_BREAST, Measurement(3.0, MeasurementUnit::WHOLE)),
            Ingredient(IngredientNames::SALT, Measurement(0.5, MeasurementUnit::TEASPOON)),
            Ingredient(IngredientNames::PEPPER, Measurement(0.5, MeasurementUnit::TEASPOON)),
            Ingredient(IngredientNames::OLIVE_OIL, Measurement(2.0, MeasurementUnit::TABLESPOON)),
            Ingredient(IngredientNames::BROCCOLI, Measurement(2.0, MeasurementUnit::CUP)),
            Ingredient(IngredientNames::YELLOW_BELL_PEPPER, Measurement(1.0, MeasurementUnit::HALF)),
            Ingredient(IngredientNames::RED_BELL_PEPPER, Measurement(1.0, MeasurementUnit::HALF)),
            Ingredient(IngredientNames::CARROTS, Measurement(0.5, MeasurementUnit::CUP)),
            Ingredient(IngredientNames::GINGER, Measurement(0.5, MeasurementUnit::TEASPOON)),
            Ingredient(IngredientNames::GARLIC, Measurement(2.0, MeasurementUnit::TEASPOON)),
            Ingredient(IngredientNames::SESAME_SEEDS, Measurement(2.0, MeasurementUnit::TABLESPOON)),
            Ingredient(IngredientNames::CORN_STARCH, Measurement(1.0, MeasurementUnit::TABLESPOON)),
            Ingredient(IngredientNames::CHICKEN_BROTH, Measurement(0.25, MeasurementUnit::CUP)),
            Ingredient(IngredientNames::SOY_SAUCE, Measurement(0.25, MeasurementUnit::CUP)),
            Ingredient(IngredientNames::HONEY, Measurement(2.0, MeasurementUnit::TABLESPOON))
        }) {}
};



#endif // MEAL_H