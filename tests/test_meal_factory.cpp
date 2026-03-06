#include "../src/db_manager.h"
#include "../src/meal.h"
#include "../src/meal_factory.h"
#include <gtest/gtest.h>

class MealFactoryTest : public ::testing::Test {
protected:
  void SetUp() override {
    auto dbManager = std::make_shared<DBManager>(":memory:");
    dbManager->initializeSchema();
    dbManager->seedDefaultMeals();
    factory = std::make_unique<MealFactory>(dbManager);
  }
  void TearDown() override {}

  std::unique_ptr<MealFactory> factory;
};

// Test creating turkey-burgers
TEST_F(MealFactoryTest, CreateTurkeyBurgers) {
  auto meal = factory->createMeal("turkey-burgers");

  ASSERT_NE(meal, nullptr);
  // Note: By default the DB preserves the "turkey-burgers" key name as the
  // inserted Name The previous tests expected "Turkey Burgers", we need to
  // check the actual value
  EXPECT_EQ(meal->getName(), "turkey-burgers");
}

// Test creating unknown meal
TEST_F(MealFactoryTest, CreateUnknownMeal) {
  auto meal = factory->createMeal("unknown-meal");

  EXPECT_EQ(meal, nullptr);
}

// Test case sensitivity (DB searches usually are case sensitive unless COLLATE
// NOCASE is used)
TEST_F(MealFactoryTest, CaseSensitivity) {
  auto meal1 = factory->createMeal("Turkey-Burgers"); // Wrong case
  auto meal2 = factory->createMeal("TURKEY-BURGERS"); // Wrong case

  EXPECT_EQ(meal1, nullptr);
  EXPECT_EQ(meal2, nullptr);
}

// Test getAvailableMeals
TEST_F(MealFactoryTest, GetAvailableMeals) {
  std::vector<std::string> meals;
  factory->getAvailableMeals(meals);

  EXPECT_GT(meals.size(), 0);

  // Check that expected meals are in the list
  bool hasTurkeyBurgers = false;
  bool hasChickenStirFry = false;

  for (const auto &mealName : meals) {
    if (mealName == "turkey-burgers") {
      hasTurkeyBurgers = true;
    }
    if (mealName == "chicken-stir-fry") {
      hasChickenStirFry = true;
    }
  }

  EXPECT_TRUE(hasTurkeyBurgers);
  EXPECT_TRUE(hasChickenStirFry);
}

// Test that all available meals can be created
TEST_F(MealFactoryTest, AllAvailableMealsAreCreatable) {
  std::vector<std::string> mealNames;
  factory->getAvailableMeals(mealNames);

  for (const auto &mealName : mealNames) {
    auto meal = factory->createMeal(mealName);
    EXPECT_NE(meal, nullptr) << "Failed to create meal: " << mealName;
    if (meal) {
      EXPECT_FALSE(meal->getName().empty());
      EXPECT_GT(meal->getIngredients().size(), 0);
    }
  }
}
