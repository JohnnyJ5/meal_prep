#include <gtest/gtest.h>

#include <sstream>

#include "../src/ingredient.h"
#include "../src/meal.h"
#include "../src/meal_planner.h"

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
    std::vector<Ingredient> ing1 = {Ingredient("Spinach", Measurement(1.0, MeasurementUnit::CUP))};
    std::vector<Ingredient> ing2 = {
        Ingredient("Chicken Breast", Measurement(1.0, MeasurementUnit::POUND))};

    Meal meal1("Meal 1", ing1);
    Meal meal2("Meal 2", ing2);

    std::vector<std::reference_wrapper<Meal>> meals = {meal1, meal2};
    std::map<std::string, Ingredient> allIngredients;

    ConsolidateAllIngredients(allIngredients, meals);

    EXPECT_EQ(allIngredients.size(), 2);
    EXPECT_TRUE(allIngredients.find("Spinach") != allIngredients.end());
    EXPECT_TRUE(allIngredients.find("Chicken Breast") != allIngredients.end());

    EXPECT_DOUBLE_EQ(allIngredients["Spinach"].getAmount().getValue(), 1.0);
    EXPECT_DOUBLE_EQ(allIngredients["Chicken Breast"].getAmount().getValue(), 1.0);
}

// Test multiple meals with overlapping ingredients (same units)
TEST_F(MealPlannerTest, ConsolidateOverlapping) {
    std::vector<Ingredient> ing1 = {Ingredient("Spinach", Measurement(1.0, MeasurementUnit::CUP))};
    std::vector<Ingredient> ing2 = {Ingredient("Spinach", Measurement(2.0, MeasurementUnit::CUP))};

    Meal meal1("Meal 1", ing1);
    Meal meal2("Meal 2", ing2);

    std::vector<std::reference_wrapper<Meal>> meals = {meal1, meal2};
    std::map<std::string, Ingredient> allIngredients;

    ConsolidateAllIngredients(allIngredients, meals);

    EXPECT_EQ(allIngredients.size(), 1);
    EXPECT_TRUE(allIngredients.find("Spinach") != allIngredients.end());

    EXPECT_DOUBLE_EQ(allIngredients["Spinach"].getAmount().getValue(), 3.0);
}

// Test multiple meals with overlapping ingredients (different units requiring
// conversion)
TEST_F(MealPlannerTest, ConsolidateOverlappingDifferentUnits) {
    std::vector<Ingredient> ing1 = {
        Ingredient("Olive Oil", Measurement(1.0, MeasurementUnit::TABLESPOON))};
    std::vector<Ingredient> ing2 = {
        Ingredient("Olive Oil", Measurement(3.0, MeasurementUnit::TEASPOON))};

    Meal meal1("Meal 1", ing1);
    Meal meal2("Meal 2", ing2);

    std::vector<std::reference_wrapper<Meal>> meals = {meal1, meal2};
    std::map<std::string, Ingredient> allIngredients;

    ConsolidateAllIngredients(allIngredients, meals);

    EXPECT_EQ(allIngredients.size(), 1);
    EXPECT_TRUE(allIngredients.find("Olive Oil") != allIngredients.end());

    // 1 tbsp + 3 tsp = 2 tbsp
    EXPECT_NEAR(allIngredients["Olive Oil"].getAmount().getValue(), 2.0, 0.001);
    EXPECT_EQ(allIngredients["Olive Oil"].getAmount().getUnit(), MeasurementUnit::TABLESPOON);
}

// PrintWeeklySchedule with empty schedule shows "(Nothing planned)" for all days
TEST_F(MealPlannerTest, PrintWeeklyScheduleEmpty) {
    std::map<std::string, std::vector<std::string>> schedule;
    std::ostringstream oss;
    PrintWeeklySchedule(oss, schedule);
    std::string output = oss.str();
    EXPECT_NE(output.find("Monday"), std::string::npos);
    EXPECT_NE(output.find("(Nothing planned)"), std::string::npos);
    EXPECT_NE(output.find("Sunday"), std::string::npos);
}

// PrintWeeklySchedule lists meals for days that have them
TEST_F(MealPlannerTest, PrintWeeklyScheduleWithMeals) {
    std::map<std::string, std::vector<std::string>> schedule;
    schedule["Monday"] = {"turkey-burgers", "chicken-stir-fry"};
    schedule["Wednesday"] = {"baked-chicken"};

    std::ostringstream oss;
    PrintWeeklySchedule(oss, schedule);
    std::string output = oss.str();

    EXPECT_NE(output.find("turkey-burgers"), std::string::npos);
    EXPECT_NE(output.find("chicken-stir-fry"), std::string::npos);
    EXPECT_NE(output.find("baked-chicken"), std::string::npos);
    // Days without meals still show the placeholder
    EXPECT_NE(output.find("(Nothing planned)"), std::string::npos);
}
