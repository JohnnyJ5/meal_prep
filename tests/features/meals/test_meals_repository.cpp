#include <gtest/gtest.h>

#include <tuple>

#include "core/db/db_connection.h"
#include "core/db/schema.h"
#include "features/meals/ingredient.h"
#include "features/meals/meal.h"
#include "features/meals/measurement.h"
#include "features/meals/meals_repository.h"

class MealsRepositoryTest : public ::testing::Test {
   protected:
    void SetUp() override {
        conn = std::make_shared<DbConnection>(":memory:");
        initializeSchema(*conn);
        repo = std::make_unique<MealsRepository>(conn);
    }

    std::shared_ptr<DbConnection> conn;
    std::unique_ptr<MealsRepository> repo;

    Meal makeMeal(const std::string &name, const std::string &category = "Test") {
        std::vector<Ingredient> ings = {
            Ingredient("Chicken", Measurement(1.0, MeasurementUnit::POUND))};
        return Meal(name, ings, category);
    }
};

TEST_F(MealsRepositoryTest, AddAndGetMeal) {
    Meal meal = makeMeal("test-chicken", "Poultry");
    EXPECT_TRUE(repo->addMeal(meal));

    auto retrieved = repo->getMeal("test-chicken");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getName(), "test-chicken");
    EXPECT_EQ(retrieved->getCategory(), "Poultry");
    ASSERT_EQ(retrieved->getIngredients().size(), 1u);
    EXPECT_EQ(retrieved->getIngredients()[0].getName(), "Chicken");
    EXPECT_DOUBLE_EQ(retrieved->getIngredients()[0].getAmount().getValue(), 1.0);
    EXPECT_EQ(retrieved->getIngredients()[0].getAmount().getUnit(), MeasurementUnit::POUND);
}

TEST_F(MealsRepositoryTest, GetNonExistentMealReturnsNull) {
    auto result = repo->getMeal("does-not-exist");
    EXPECT_EQ(result, nullptr);
}

TEST_F(MealsRepositoryTest, DeleteMealRemovesIt) {
    repo->addMeal(makeMeal("delete-me"));
    EXPECT_TRUE(repo->deleteMeal("delete-me"));
    EXPECT_EQ(repo->getMeal("delete-me"), nullptr);
}

TEST_F(MealsRepositoryTest, DeleteNonExistentMealReturnsTrue) {
    EXPECT_TRUE(repo->deleteMeal("ghost-meal"));
}

TEST_F(MealsRepositoryTest, UpdateMealReplacesContent) {
    repo->addMeal(makeMeal("update-me", "OldCat"));

    std::vector<Ingredient> newIngs = {
        Ingredient("Beef", Measurement(2.0, MeasurementUnit::POUND)),
        Ingredient("Salt", Measurement(1.0, MeasurementUnit::TEASPOON))};
    Meal updated("update-me", newIngs, "Beef");
    EXPECT_TRUE(repo->updateMeal(updated));

    auto retrieved = repo->getMeal("update-me");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getIngredients().size(), 2u);
    EXPECT_EQ(retrieved->getCategory(), "Beef");
}

TEST_F(MealsRepositoryTest, GetAllMealsReturnsAllEntries) {
    repo->addMeal(makeMeal("meal-b", "Cat2"));
    repo->addMeal(makeMeal("meal-a", "Cat1"));

    std::vector<std::tuple<int, std::string, std::string>> meals;
    EXPECT_TRUE(repo->getAllMeals(meals));
    ASSERT_EQ(meals.size(), 2u);
    EXPECT_EQ(std::get<1>(meals[0]), "meal-a");
    EXPECT_EQ(std::get<1>(meals[1]), "meal-b");
}

TEST_F(MealsRepositoryTest, GetAllMealsEmptyDB) {
    std::vector<std::tuple<int, std::string, std::string>> meals;
    EXPECT_TRUE(repo->getAllMeals(meals));
    EXPECT_TRUE(meals.empty());
}

TEST_F(MealsRepositoryTest, AddAndGetIngredients) {
    EXPECT_TRUE(repo->addIngredient("Spinach", "Vegetables"));
    EXPECT_TRUE(repo->addIngredient("Chicken Breast", "Proteins"));

    std::vector<std::pair<std::string, std::string>> ingredients;
    EXPECT_TRUE(repo->getAllIngredients(ingredients));
    ASSERT_EQ(ingredients.size(), 2u);

    bool hasSpinach = false, hasChicken = false;
    for (const auto &p : ingredients) {
        if (p.first == "Spinach" && p.second == "Vegetables") hasSpinach = true;
        if (p.first == "Chicken Breast" && p.second == "Proteins") hasChicken = true;
    }
    EXPECT_TRUE(hasSpinach);
    EXPECT_TRUE(hasChicken);
}

TEST_F(MealsRepositoryTest, GetAllIngredientsEmpty) {
    std::vector<std::pair<std::string, std::string>> ingredients;
    EXPECT_TRUE(repo->getAllIngredients(ingredients));
    EXPECT_TRUE(ingredients.empty());
}

TEST_F(MealsRepositoryTest, SeedDefaultIngredientsPopulatesTable) {
    EXPECT_TRUE(repo->seedDefaultIngredients());

    std::vector<std::pair<std::string, std::string>> ingredients;
    repo->getAllIngredients(ingredients);
    EXPECT_GT(ingredients.size(), 0u);
}

TEST_F(MealsRepositoryTest, SeedDefaultIngredientsIsIdempotent) {
    repo->seedDefaultIngredients();
    std::vector<std::pair<std::string, std::string>> first;
    repo->getAllIngredients(first);

    repo->seedDefaultIngredients();
    std::vector<std::pair<std::string, std::string>> second;
    repo->getAllIngredients(second);

    EXPECT_EQ(first.size(), second.size());
}

TEST_F(MealsRepositoryTest, SeedDefaultMealsPopulatesTable) {
    EXPECT_TRUE(repo->seedDefaultMeals());

    std::vector<std::tuple<int, std::string, std::string>> meals;
    repo->getAllMeals(meals);
    EXPECT_GT(meals.size(), 0u);
}

TEST_F(MealsRepositoryTest, SeedDefaultMealsIsIdempotent) {
    repo->seedDefaultMeals();
    std::vector<std::tuple<int, std::string, std::string>> first;
    repo->getAllMeals(first);

    repo->seedDefaultMeals();
    std::vector<std::tuple<int, std::string, std::string>> second;
    repo->getAllMeals(second);

    EXPECT_EQ(first.size(), second.size());
}

TEST_F(MealsRepositoryTest, AddMealWithVariedIngredients) {
    std::vector<Ingredient> ings = {
        Ingredient("Chicken", Measurement(2.0, MeasurementUnit::WHOLE), "Strips"),
        Ingredient("Salt", Measurement(1.0, MeasurementUnit::TEASPOON)),
        Ingredient("Garlic", Measurement(3.0, MeasurementUnit::CLOVE), "Minced"),
        Ingredient("Oil", Measurement(2.0, MeasurementUnit::TABLESPOON)),
    };
    Meal meal("varied-meal", ings, "Poultry");
    EXPECT_TRUE(repo->addMeal(meal));

    auto retrieved = repo->getMeal("varied-meal");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getIngredients().size(), 4u);
}

TEST_F(MealsRepositoryTest, AddDuplicateMealFails) {
    EXPECT_TRUE(repo->addMeal(makeMeal("unique-meal")));
    EXPECT_FALSE(repo->addMeal(makeMeal("unique-meal")));
}

TEST_F(MealsRepositoryTest, OptionalIngredientFlagRoundTrips) {
    std::vector<Ingredient> ings = {
        Ingredient("All-Purpose Flour", Measurement(1.0, MeasurementUnit::CUP), "None", false),
        Ingredient("Blueberry", Measurement(0.5, MeasurementUnit::CUP), "None", true),
        Ingredient("Cottage Cheese", Measurement(1.0, MeasurementUnit::CUP), "None", true),
    };
    Meal meal("pancakes", ings, "Breakfast");
    EXPECT_TRUE(repo->addMeal(meal));

    auto retrieved = repo->getMeal("pancakes");
    ASSERT_NE(retrieved, nullptr);
    ASSERT_EQ(retrieved->getIngredients().size(), 3u);
    for (const auto &ing : retrieved->getIngredients()) {
        if (ing.getName() == "All-Purpose Flour") {
            EXPECT_FALSE(ing.isOptional());
        } else {
            EXPECT_TRUE(ing.isOptional());
        }
    }
}

TEST_F(MealsRepositoryTest, UpdateMealPreservesOptionalFlag) {
    repo->addMeal(makeMeal("update-pancakes"));
    std::vector<Ingredient> ings = {
        Ingredient("Eggs", Measurement(2.0, MeasurementUnit::WHOLE), "None", false),
        Ingredient("Pumpkin", Measurement(0.25, MeasurementUnit::CUP), "None", true),
    };
    Meal updated("update-pancakes", ings, "Breakfast");
    EXPECT_TRUE(repo->updateMeal(updated));

    auto retrieved = repo->getMeal("update-pancakes");
    ASSERT_NE(retrieved, nullptr);
    ASSERT_EQ(retrieved->getIngredients().size(), 2u);
    for (const auto &ing : retrieved->getIngredients()) {
        if (ing.getName() == "Pumpkin") EXPECT_TRUE(ing.isOptional());
        if (ing.getName() == "Eggs") EXPECT_FALSE(ing.isOptional());
    }
}

TEST_F(MealsRepositoryTest, GetMealIdsWithOptionalIngredients) {
    repo->addMeal(Meal(
        "no-options", {Ingredient("Salt", Measurement(1.0, MeasurementUnit::TEASPOON))}, "Cat"));
    repo->addMeal(Meal(
        "with-options",
        {Ingredient("Eggs", Measurement(2.0, MeasurementUnit::WHOLE)),
         Ingredient("Blueberry", Measurement(0.5, MeasurementUnit::CUP), "None", true)},
        "Cat"));

    auto ids = repo->getMealIdsWithOptionalIngredients();

    std::vector<std::tuple<int, std::string, std::string>> meals;
    repo->getAllMeals(meals);
    int noOptionsId = -1, withOptionsId = -1;
    for (const auto &m : meals) {
        if (std::get<1>(m) == "no-options") noOptionsId = std::get<0>(m);
        if (std::get<1>(m) == "with-options") withOptionsId = std::get<0>(m);
    }
    ASSERT_NE(noOptionsId, -1);
    ASSERT_NE(withOptionsId, -1);

    EXPECT_EQ(ids.count(noOptionsId), 0u);
    EXPECT_EQ(ids.count(withOptionsId), 1u);
}
