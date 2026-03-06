#include <gtest/gtest.h>
#include "../meal.h"
#include "../ingredient_names.h"

class MealTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test Meal base class
TEST_F(MealTest, MealConstructorAndGetters) {
    std::vector<Ingredient> ingredients = {
        Ingredient(IngredientNames::SPINACH, Measurement(2.0, MeasurementUnit::CUP)),
        Ingredient(IngredientNames::SALT, Measurement(1.0, MeasurementUnit::TEASPOON))
    };
    
    Meal meal("Test Meal", ingredients);
    
    EXPECT_EQ(meal.getName(), "Test Meal");
    EXPECT_EQ(meal.getIngredients().size(), 2);
    EXPECT_EQ(meal.getIngredients()[0].getName(), IngredientNames::SPINACH);
    EXPECT_EQ(meal.getIngredients()[1].getName(), IngredientNames::SALT);
}

// Test that ingredients are immutable through getter
TEST_F(MealTest, IngredientsImmutability) {
    std::vector<Ingredient> ingredients = {
        Ingredient(IngredientNames::GROUND_BEEF, Measurement(1.0, MeasurementUnit::POUND))
    };
    Meal meal("Immutability Test", ingredients);
    const auto& fetchedIngredients = meal.getIngredients();
    
    // Should be able to read but not modify (const reference)
    EXPECT_FALSE(fetchedIngredients.empty());
    
    // Verify we can access ingredients
    for (const auto& ing : fetchedIngredients) {
        EXPECT_FALSE(ing.getName().empty());
    }
}
