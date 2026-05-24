#include <gtest/gtest.h>

#include <set>
#include <string>
#include <tuple>

#include "core/db/db_connection.h"
#include "core/db/schema.h"
#include "features/meals/meal.h"
#include "features/meals/meal_factory.h"
#include "features/meals/meals_repository.h"

class MealFactoryTest : public ::testing::Test {
   protected:
    void SetUp() override {
        auto conn = std::make_shared<DbConnection>(":memory:");
        initializeSchema(*conn);
        auto repo = std::make_shared<MealsRepository>(conn);
        repo->seedDefaultMeals();
        factory = std::make_unique<MealFactory>(repo);
    }

    std::unique_ptr<MealFactory> factory;
};

TEST_F(MealFactoryTest, CreateTurkeyBurgers) {
    auto meal = factory->createMeal("turkey-burgers");
    ASSERT_NE(meal, nullptr);
    EXPECT_EQ(meal->getName(), "turkey-burgers");
}

TEST_F(MealFactoryTest, CreateUnknownMeal) {
    auto meal = factory->createMeal("unknown-meal");
    EXPECT_EQ(meal, nullptr);
}

TEST_F(MealFactoryTest, CaseSensitivity) {
    EXPECT_EQ(factory->createMeal("Turkey-Burgers"), nullptr);
    EXPECT_EQ(factory->createMeal("TURKEY-BURGERS"), nullptr);
}

TEST_F(MealFactoryTest, GetAvailableMeals) {
    std::vector<std::tuple<int, std::string, std::string>> meals;
    factory->getAvailableMeals(meals);
    EXPECT_GT(meals.size(), 0);

    bool hasTurkeyBurgers = false;
    bool hasChickenStirFry = false;
    for (const auto &mealTuple : meals) {
        if (std::get<1>(mealTuple) == "turkey-burgers") hasTurkeyBurgers = true;
        if (std::get<1>(mealTuple) == "chicken-stir-fry") hasChickenStirFry = true;
    }
    EXPECT_TRUE(hasTurkeyBurgers);
    EXPECT_TRUE(hasChickenStirFry);
}

TEST_F(MealFactoryTest, CreateMealExcludesOptionalByDefault) {
    auto conn = std::make_shared<DbConnection>(":memory:");
    initializeSchema(*conn);
    auto repo = std::make_shared<MealsRepository>(conn);

    std::vector<Ingredient> ings = {
        Ingredient("Eggs", Measurement(2.0, MeasurementUnit::WHOLE), "None", false),
        Ingredient("Cottage Cheese", Measurement(1.0, MeasurementUnit::CUP), "None", true),
        Ingredient("Pumpkin", Measurement(0.25, MeasurementUnit::CUP), "None", true),
    };
    repo->addMeal(Meal("pancakes-test", ings, "Breakfast"));
    MealFactory localFactory(repo);

    auto meal = localFactory.createMeal("pancakes-test");
    ASSERT_NE(meal, nullptr);
    EXPECT_EQ(meal->getIngredients().size(), 1u);
    EXPECT_EQ(meal->getIngredients()[0].getName(), "Eggs");
}

TEST_F(MealFactoryTest, CreateMealIncludesSelectedAddOns) {
    auto conn = std::make_shared<DbConnection>(":memory:");
    initializeSchema(*conn);
    auto repo = std::make_shared<MealsRepository>(conn);

    std::vector<Ingredient> ings = {
        Ingredient("Eggs", Measurement(2.0, MeasurementUnit::WHOLE), "None", false),
        Ingredient("Cottage Cheese", Measurement(1.0, MeasurementUnit::CUP), "None", true),
        Ingredient("Pumpkin", Measurement(0.25, MeasurementUnit::CUP), "None", true),
        Ingredient("Blueberry", Measurement(0.5, MeasurementUnit::CUP), "None", true),
    };
    repo->addMeal(Meal("pancakes-test", ings, "Breakfast"));
    MealFactory localFactory(repo);

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
