#include <gtest/gtest.h>
#include "../meal_factory.h"
#include "../meal.h"

class MealFactoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        factory = std::make_unique<MealFactory>();
    }
    void TearDown() override {}
    
    std::unique_ptr<MealFactory> factory;
};

// Test creating turkey-burgers
TEST_F(MealFactoryTest, CreateTurkeyBurgers) {
    auto meal = factory->createMeal("turkey-burgers");
    
    ASSERT_NE(meal, nullptr);
    EXPECT_EQ(meal->getName(), "Turkey Burgers");
}

// Test creating turkey-meatballs
TEST_F(MealFactoryTest, CreateTurkeyMeatballs) {
    auto meal = factory->createMeal("turkey-meatballs");
    
    ASSERT_NE(meal, nullptr);
    EXPECT_EQ(meal->getName(), "Turkey Meatballs");
}

// Test creating creamy-garlic-chicken-penne-spinach
TEST_F(MealFactoryTest, CreateCreamyGarlicChickenPenneSpinach) {
    auto meal = factory->createMeal("creamy-garlic-chicken-penne-spinach");
    
    ASSERT_NE(meal, nullptr);
    EXPECT_EQ(meal->getName(), "Creamy Garlic Chicken Penne Spinach");
}

// Test creating creamy-garlic-chicken
TEST_F(MealFactoryTest, CreateCreamyGarlicChicken) {
    auto meal = factory->createMeal("creamy-garlic-chicken");
    
    ASSERT_NE(meal, nullptr);
    EXPECT_EQ(meal->getName(), "Creamy Garlic Chicken");
}

// Test creating baked-chicken-breast
TEST_F(MealFactoryTest, CreateBakedChickenBreast) {
    auto meal = factory->createMeal("baked-chicken-breast");
    
    ASSERT_NE(meal, nullptr);
    EXPECT_EQ(meal->getName(), "Baked Chicken Breast");
}

// Test creating cheesy-hamburger-pasta-skillet
TEST_F(MealFactoryTest, CreateCheesyHamburgerPastaSkillet) {
    auto meal = factory->createMeal("cheesy-hamburger-pasta-skillet");
    
    ASSERT_NE(meal, nullptr);
    EXPECT_EQ(meal->getName(), "Cheesy Hamburger Pasta Skillet");
}

// Test creating cottage-cheese-pancakes
TEST_F(MealFactoryTest, CreateCottageCheesePancakes) {
    auto meal = factory->createMeal("cottage-cheese-pancakes");
    
    ASSERT_NE(meal, nullptr);
    EXPECT_EQ(meal->getName(), "Cottage Cheese Pancakes");
}

// Test creating chicken-stir-fry
TEST_F(MealFactoryTest, CreateChickenStirFry) {
    auto meal = factory->createMeal("chicken-stir-fry");
    
    ASSERT_NE(meal, nullptr);
    EXPECT_EQ(meal->getName(), "Chicken Stir Fry");
}

// Test creating unknown meal
TEST_F(MealFactoryTest, CreateUnknownMeal) {
    auto meal = factory->createMeal("unknown-meal");
    
    EXPECT_EQ(meal, nullptr);
}

// Test case sensitivity
TEST_F(MealFactoryTest, CaseSensitivity) {
    auto meal1 = factory->createMeal("Turkey-Burgers");  // Wrong case
    auto meal2 = factory->createMeal("TURKEY-BURGERS");  // Wrong case
    
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
    
    for (const auto& mealName : meals) {
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
    
    for (const auto& mealName : mealNames) {
        auto meal = factory->createMeal(mealName);
        EXPECT_NE(meal, nullptr) << "Failed to create meal: " << mealName;
        if (meal) {
            EXPECT_FALSE(meal->getName().empty());
            EXPECT_GT(meal->getIngredients().size(), 0);
        }
    }
}
