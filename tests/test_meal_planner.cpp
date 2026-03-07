#include "../src/ingredient.h"
#include "../src/ingredient_names.h"
#include "../src/meal.h"
#include "../src/meal_planner.h"
#include <gtest/gtest.h>

class MealPlannerTest : public ::testing::Test {
protected:
  void SetUp() override {}
  void TearDown() override {}
};

// Test empty list of meals gives empty consolidation
TEST_F(MealPlannerTest, ConsolidateEmpty) {
  std::map<std::string, Ingredient> allIngredients;
  std::vector<std::reference_wrapper<Meal>> meals;

  ConsolidateAllIngredients(allIngredients, meals);

  EXPECT_TRUE(allIngredients.empty());
}

// Test multiple meals with disjoint ingredients
TEST_F(MealPlannerTest, ConsolidateDisjoint) {
  std::vector<Ingredient> ing1 = {Ingredient(
      IngredientNames::SPINACH, Measurement(1.0, MeasurementUnit::CUP))};
  std::vector<Ingredient> ing2 = {
      Ingredient(IngredientNames::CHICKEN_BREAST,
                 Measurement(1.0, MeasurementUnit::POUND))};

  Meal meal1("Meal 1", ing1);
  Meal meal2("Meal 2", ing2);

  std::vector<std::reference_wrapper<Meal>> meals = {meal1, meal2};
  std::map<std::string, Ingredient> allIngredients;

  ConsolidateAllIngredients(allIngredients, meals);

  EXPECT_EQ(allIngredients.size(), 2);
  EXPECT_TRUE(allIngredients.find(IngredientNames::SPINACH) !=
              allIngredients.end());
  EXPECT_TRUE(allIngredients.find(IngredientNames::CHICKEN_BREAST) !=
              allIngredients.end());

  EXPECT_DOUBLE_EQ(
      allIngredients[IngredientNames::SPINACH].getAmount().getValue(), 1.0);
  EXPECT_DOUBLE_EQ(
      allIngredients[IngredientNames::CHICKEN_BREAST].getAmount().getValue(),
      1.0);
}

// Test multiple meals with overlapping ingredients (same units)
TEST_F(MealPlannerTest, ConsolidateOverlapping) {
  std::vector<Ingredient> ing1 = {Ingredient(
      IngredientNames::SPINACH, Measurement(1.0, MeasurementUnit::CUP))};
  std::vector<Ingredient> ing2 = {Ingredient(
      IngredientNames::SPINACH, Measurement(2.0, MeasurementUnit::CUP))};

  Meal meal1("Meal 1", ing1);
  Meal meal2("Meal 2", ing2);

  std::vector<std::reference_wrapper<Meal>> meals = {meal1, meal2};
  std::map<std::string, Ingredient> allIngredients;

  ConsolidateAllIngredients(allIngredients, meals);

  EXPECT_EQ(allIngredients.size(), 1);
  EXPECT_TRUE(allIngredients.find(IngredientNames::SPINACH) !=
              allIngredients.end());

  EXPECT_DOUBLE_EQ(
      allIngredients[IngredientNames::SPINACH].getAmount().getValue(), 3.0);
}

// Test multiple meals with overlapping ingredients (different units requiring
// conversion)
TEST_F(MealPlannerTest, ConsolidateOverlappingDifferentUnits) {
  std::vector<Ingredient> ing1 = {
      Ingredient(IngredientNames::OLIVE_OIL,
                 Measurement(1.0, MeasurementUnit::TABLESPOON))};
  std::vector<Ingredient> ing2 = {Ingredient(
      IngredientNames::OLIVE_OIL, Measurement(3.0, MeasurementUnit::TEASPOON))};

  Meal meal1("Meal 1", ing1);
  Meal meal2("Meal 2", ing2);

  std::vector<std::reference_wrapper<Meal>> meals = {meal1, meal2};
  std::map<std::string, Ingredient> allIngredients;

  ConsolidateAllIngredients(allIngredients, meals);

  EXPECT_EQ(allIngredients.size(), 1);
  EXPECT_TRUE(allIngredients.find(IngredientNames::OLIVE_OIL) !=
              allIngredients.end());

  // 1 tbsp + 3 tsp = 2 tbsp
  EXPECT_NEAR(allIngredients[IngredientNames::OLIVE_OIL].getAmount().getValue(),
              2.0, 0.001);
  EXPECT_EQ(allIngredients[IngredientNames::OLIVE_OIL].getAmount().getUnit(),
            MeasurementUnit::TABLESPOON);
}
