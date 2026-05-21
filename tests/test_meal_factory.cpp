#include <gtest/gtest.h>

#include <set>
#include <string>
#include <tuple>

#include "../src/db_manager.h"
#include "../src/meal.h"
#include "../src/meal_factory.h"

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
    auto meal1 = factory->createMeal("Turkey-Burgers");  // Wrong case
    auto meal2 = factory->createMeal("TURKEY-BURGERS");  // Wrong case

    EXPECT_EQ(meal1, nullptr);
    EXPECT_EQ(meal2, nullptr);
}

// Test getAvailableMeals
TEST_F(MealFactoryTest, GetAvailableMeals) {
    std::vector<std::tuple<int, std::string, std::string>> meals;
    factory->getAvailableMeals(meals);

    EXPECT_GT(meals.size(), 0);

    // Check that expected meals are in the list
    bool hasTurkeyBurgers = false;
    bool hasChickenStirFry = false;

    for (const auto &mealTuple : meals) {
        if (std::get<1>(mealTuple) == "turkey-burgers") {
            hasTurkeyBurgers = true;
        }
        if (std::get<1>(mealTuple) == "chicken-stir-fry") {
            hasChickenStirFry = true;
        }
    }

    EXPECT_TRUE(hasTurkeyBurgers);
    EXPECT_TRUE(hasChickenStirFry);
}

// createMeal without an add-on set excludes all optional ingredients
TEST_F(MealFactoryTest, CreateMealExcludesOptionalByDefault) {
    auto dbManager = std::make_shared<DBManager>(":memory:");
    dbManager->initializeSchema();
    std::vector<Ingredient> ings = {
        Ingredient("Eggs", Measurement(2.0, MeasurementUnit::WHOLE), "None", false),
        Ingredient("Cottage Cheese", Measurement(1.0, MeasurementUnit::CUP), "None", true),
        Ingredient("Pumpkin", Measurement(0.25, MeasurementUnit::CUP), "None", true),
    };
    dbManager->addMeal(Meal("pancakes-test", ings, "Breakfast"));
    MealFactory localFactory(dbManager);

    auto meal = localFactory.createMeal("pancakes-test");
    ASSERT_NE(meal, nullptr);
    EXPECT_EQ(meal->getIngredients().size(), 1u);
    EXPECT_EQ(meal->getIngredients()[0].getName(), "Eggs");
}

// createMeal with a set of enabled add-ons includes only the matching optionals
TEST_F(MealFactoryTest, CreateMealIncludesSelectedAddOns) {
    auto dbManager = std::make_shared<DBManager>(":memory:");
    dbManager->initializeSchema();
    std::vector<Ingredient> ings = {
        Ingredient("Eggs", Measurement(2.0, MeasurementUnit::WHOLE), "None", false),
        Ingredient("Cottage Cheese", Measurement(1.0, MeasurementUnit::CUP), "None", true),
        Ingredient("Pumpkin", Measurement(0.25, MeasurementUnit::CUP), "None", true),
        Ingredient("Blueberry", Measurement(0.5, MeasurementUnit::CUP), "None", true),
    };
    dbManager->addMeal(Meal("pancakes-test", ings, "Breakfast"));
    MealFactory localFactory(dbManager);

    auto meal = localFactory.createMeal("pancakes-test", {"Blueberry", "Pumpkin"});
    ASSERT_NE(meal, nullptr);
    EXPECT_EQ(meal->getIngredients().size(), 3u);

    std::set<std::string> names;
    for (const auto &i : meal->getIngredients()) names.insert(i.getName());
    EXPECT_EQ(names.count("Eggs"), 1u);
    EXPECT_EQ(names.count("Blueberry"), 1u);
    EXPECT_EQ(names.count("Pumpkin"), 1u);
    EXPECT_EQ(names.count("Cottage Cheese"), 0u);
}

// Test that all available meals can be creatable
TEST_F(MealFactoryTest, AllAvailableMealsAreCreatable) {
    std::vector<std::tuple<int, std::string, std::string>> meals;
    factory->getAvailableMeals(meals);

    for (const auto &mealTuple : meals) {
        auto meal = factory->createMeal(std::get<1>(mealTuple));
        EXPECT_NE(meal, nullptr) << "Failed to create meal: " << std::get<1>(mealTuple);
        if (meal) {
            EXPECT_FALSE(meal->getName().empty());
            EXPECT_FALSE(meal->getCategory().empty());
            EXPECT_GT(meal->getIngredients().size(), 0);
        }
    }
}
