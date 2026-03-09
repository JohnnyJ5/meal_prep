#include "../src/ingredient.h"
#include "../src/measurement.h"
#include <gtest/gtest.h>

class IngredientTest : public ::testing::Test {
protected:
  void SetUp() override {}
  void TearDown() override {}
};

// Test default constructor
TEST_F(IngredientTest, DefaultConstructor) {
  Ingredient ing;
  EXPECT_EQ(ing.getName(), "");
}

// Test constructor with parameters
TEST_F(IngredientTest, ConstructorWithParameters) {
  Measurement m(2.0, MeasurementUnit::CUP);
  Ingredient ing("Spinach", m, Preparation::k_CHOPPED);

  EXPECT_EQ(ing.getName(), "Spinach");
  EXPECT_DOUBLE_EQ(ing.getAmount().getValue(), 2.0);
  EXPECT_EQ(ing.getAmount().getUnit(), MeasurementUnit::CUP);
}

// Test constructor without preparation
TEST_F(IngredientTest, ConstructorWithoutPreparation) {
  Measurement m(1.0, MeasurementUnit::POUND);
  Ingredient ing("Ground Turkey", m);

  EXPECT_EQ(ing.getName(), "Ground Turkey");
  EXPECT_DOUBLE_EQ(ing.getAmount().getValue(), 1.0);
}

// Test addition of same ingredients
TEST_F(IngredientTest, AdditionSameIngredients) {
  Measurement m1(1.0, MeasurementUnit::CUP);
  Measurement m2(2.0, MeasurementUnit::CUP);
  Ingredient ing1("Spinach", m1);
  Ingredient ing2("Spinach", m2);

  Ingredient result = ing1 + ing2;

  EXPECT_EQ(result.getName(), "Spinach");
  EXPECT_DOUBLE_EQ(result.getAmount().getValue(), 3.0);
  EXPECT_EQ(result.getAmount().getUnit(), MeasurementUnit::CUP);
}

// Test addition of different ingredients (should throw)
TEST_F(IngredientTest, AdditionDifferentIngredientsThrows) {
  Measurement m1(1.0, MeasurementUnit::CUP);
  Measurement m2(2.0, MeasurementUnit::CUP);
  Ingredient ing1("Spinach", m1);
  Ingredient ing2("Broccoli", m2);

  EXPECT_THROW({ Ingredient result = ing1 + ing2; }, std::invalid_argument);
}

// Test in-place addition
TEST_F(IngredientTest, InPlaceAddition) {
  Measurement m1(1.0, MeasurementUnit::TABLESPOON);
  Measurement m2(2.0, MeasurementUnit::TABLESPOON);
  Ingredient ing1("Olive Oil", m1);
  Ingredient ing2("Olive Oil", m2);

  ing1 += ing2;

  EXPECT_EQ(ing1.getName(), "Olive Oil");
  EXPECT_DOUBLE_EQ(ing1.getAmount().getValue(), 3.0);
}

// Test equality operator
TEST_F(IngredientTest, EqualityOperator) {
  Measurement m1(1.0, MeasurementUnit::CUP);
  Measurement m2(2.0, MeasurementUnit::CUP);
  Ingredient ing1("Spinach", m1);
  Ingredient ing2("Spinach", m2);
  Ingredient ing3("Broccoli", m1);

  EXPECT_TRUE(ing1 == ing2);  // Same name
  EXPECT_FALSE(ing1 == ing3); // Different name
}

// Test less-than operator
TEST_F(IngredientTest, LessThanOperator) {
  Measurement m(1.0, MeasurementUnit::CUP);
  Ingredient ing1("Broccoli", m);
  Ingredient ing2("Spinach", m);

  EXPECT_TRUE(ing1 < ing2); // "Broccoli" < "Spinach" alphabetically
  EXPECT_FALSE(ing2 < ing1);
}

// Test addition with unit conversion
TEST_F(IngredientTest, AdditionWithUnitConversion) {
  Measurement m1(1.0, MeasurementUnit::TABLESPOON);
  Measurement m2(3.0, MeasurementUnit::TEASPOON);
  Ingredient ing1("Olive Oil", m1);
  Ingredient ing2("Olive Oil", m2);

  Ingredient result = ing1 + ing2;

  EXPECT_EQ(result.getName(), "Olive Oil");
  // 1 tbsp + 3 tsp = 2 tbsp
  EXPECT_NEAR(result.getAmount().getValue(), 2.0, 0.001);
}

// Test copy constructor
TEST_F(IngredientTest, CopyConstructor) {
  Measurement m(5.0, MeasurementUnit::POUND);
  Ingredient ing1("Ground Turkey", m);
  Ingredient ing2(ing1);

  EXPECT_EQ(ing2.getName(), "Ground Turkey");
  EXPECT_DOUBLE_EQ(ing2.getAmount().getValue(), 5.0);
}

// Test assignment operator
TEST_F(IngredientTest, AssignmentOperator) {
  Measurement m(3.0, MeasurementUnit::CUP);
  Ingredient ing1("Spinach", m);
  Ingredient ing2;
  ing2 = ing1;

  EXPECT_EQ(ing2.getName(), "Spinach");
  EXPECT_DOUBLE_EQ(ing2.getAmount().getValue(), 3.0);
}
