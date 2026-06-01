#include <gtest/gtest.h>

#include "../src/meal.h"

class MealTest : public ::testing::Test {
   protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test Meal base class
TEST_F(MealTest, MealConstructorAndGetters) {
    std::vector<Ingredient> ingredients = {
        Ingredient("Spinach", Measurement(2.0, MeasurementUnit::CUP)),
        Ingredient("Salt", Measurement(1.0, MeasurementUnit::TEASPOON))};

    Meal meal("Test Meal", ingredients, "Dinner");

    EXPECT_EQ(meal.getName(), "Test Meal");
    EXPECT_EQ(meal.getCategory(), "Dinner");
    EXPECT_EQ(meal.getIngredients().size(), 2);
    EXPECT_EQ(meal.getIngredients()[0].getName(), "Spinach");
    EXPECT_EQ(meal.getIngredients()[1].getName(), "Salt");
    EXPECT_FALSE(meal.isVerified());  // defaults to unverified
}

// Test the verified flag through the constructor
TEST_F(MealTest, VerifiedFlag) {
    std::vector<Ingredient> ingredients = {
        Ingredient("Spinach", Measurement(2.0, MeasurementUnit::CUP))};

    Meal verifiedMeal("Verified Meal", ingredients, "Dinner", true);
    EXPECT_TRUE(verifiedMeal.isVerified());

    Meal unverifiedMeal("Unverified Meal", ingredients, "Dinner", false);
    EXPECT_FALSE(unverifiedMeal.isVerified());
}

// Test that ingredients are immutable through getter
TEST_F(MealTest, IngredientsImmutability) {
    std::vector<Ingredient> ingredients = {
        Ingredient("Ground Beef", Measurement(1.0, MeasurementUnit::POUND))};
    Meal meal("Immutability Test", ingredients);
    const auto &fetchedIngredients = meal.getIngredients();

    // Should be able to read but not modify (const reference)
    EXPECT_FALSE(fetchedIngredients.empty());

    // Verify we can access ingredients
    for (const auto &ing : fetchedIngredients) {
        EXPECT_FALSE(ing.getName().empty());
    }
}
