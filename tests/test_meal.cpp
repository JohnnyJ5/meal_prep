#include "../src/meal.h"
#include <gtest/gtest.h>

class MealTest : public ::testing::Test {
protected:
  void SetUp() override {}
  void TearDown() override {}
};

// Test Meal base class
TEST_F(MealTest, MealConstructorAndGetters) {
  std::vector<Ingredient> ingredients = {
      Ingredient("Spinach",
                 Measurement(2.0, MeasurementUnit::CUP)),
      Ingredient("Salt",
                 Measurement(1.0, MeasurementUnit::TEASPOON))};

  Meal meal("Test Meal", ingredients, "Dinner");

  EXPECT_EQ(meal.getName(), "Test Meal");
  EXPECT_EQ(meal.getCategory(), "Dinner");
  EXPECT_EQ(meal.getIngredients().size(), 2);
  EXPECT_EQ(meal.getIngredients()[0].getName(), "Spinach");
  EXPECT_EQ(meal.getIngredients()[1].getName(), "Salt");
}

// Test that ingredients are immutable through getter
TEST_F(MealTest, IngredientsImmutability) {
  std::vector<Ingredient> ingredients = {Ingredient(
      "Ground Beef", Measurement(1.0, MeasurementUnit::POUND))};
  Meal meal("Immutability Test", ingredients);
  const auto &fetchedIngredients = meal.getIngredients();

  // Should be able to read but not modify (const reference)
  EXPECT_FALSE(fetchedIngredients.empty());

  // Verify we can access ingredients
  for (const auto &ing : fetchedIngredients) {
    EXPECT_FALSE(ing.getName().empty());
  }
}
