#include <gtest/gtest.h>

#include <cstdlib>
#include <tuple>

#include "../src/db_manager.h"
#include "../src/ingredient.h"
#include "../src/meal.h"
#include "../src/measurement.h"

class DBManagerTest : public ::testing::Test {
   protected:
    void SetUp() override {
        unsetenv("MEAL_PREP_TOKEN_KEY");
        db = std::make_unique<DBManager>(":memory:");
        db->initializeSchema();
    }
    void TearDown() override { unsetenv("MEAL_PREP_TOKEN_KEY"); }

    std::unique_ptr<DBManager> db;

    Meal makeMeal(const std::string &name, const std::string &category = "Test") {
        std::vector<Ingredient> ings = {
            Ingredient("Chicken", Measurement(1.0, MeasurementUnit::POUND))};
        return Meal(name, ings, category);
    }
};

// addMeal stores a meal that can be retrieved by name
TEST_F(DBManagerTest, AddAndGetMeal) {
    Meal meal = makeMeal("test-chicken", "Poultry");
    EXPECT_TRUE(db->addMeal(meal));

    auto retrieved = db->getMeal("test-chicken");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getName(), "test-chicken");
    EXPECT_EQ(retrieved->getCategory(), "Poultry");
    ASSERT_EQ(retrieved->getIngredients().size(), 1u);
    EXPECT_EQ(retrieved->getIngredients()[0].getName(), "Chicken");
    EXPECT_DOUBLE_EQ(retrieved->getIngredients()[0].getAmount().getValue(), 1.0);
    EXPECT_EQ(retrieved->getIngredients()[0].getAmount().getUnit(), MeasurementUnit::POUND);
}

// getMeal for a name not in the DB returns nullptr
TEST_F(DBManagerTest, GetNonExistentMealReturnsNull) {
    auto result = db->getMeal("does-not-exist");
    EXPECT_EQ(result, nullptr);
}

// deleteMeal removes a previously added meal
TEST_F(DBManagerTest, DeleteMealRemovesIt) {
    db->addMeal(makeMeal("delete-me"));
    EXPECT_TRUE(db->deleteMeal("delete-me"));
    EXPECT_EQ(db->getMeal("delete-me"), nullptr);
}

// deleteMeal on a non-existent name still returns true (0 rows deleted, SQLITE_DONE)
TEST_F(DBManagerTest, DeleteNonExistentMealReturnsTrue) {
    EXPECT_TRUE(db->deleteMeal("ghost-meal"));
}

// updateMeal replaces ingredients and category
TEST_F(DBManagerTest, UpdateMealReplacesContent) {
    db->addMeal(makeMeal("update-me", "OldCat"));

    std::vector<Ingredient> newIngs = {
        Ingredient("Beef", Measurement(2.0, MeasurementUnit::POUND)),
        Ingredient("Salt", Measurement(1.0, MeasurementUnit::TEASPOON))};
    Meal updated("update-me", newIngs, "Beef");
    EXPECT_TRUE(db->updateMeal(updated));

    auto retrieved = db->getMeal("update-me");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getIngredients().size(), 2u);
    EXPECT_EQ(retrieved->getCategory(), "Beef");
}

// getAllMeals returns all inserted meals ordered by name
TEST_F(DBManagerTest, GetAllMealsReturnsAllEntries) {
    db->addMeal(makeMeal("meal-b", "Cat2"));
    db->addMeal(makeMeal("meal-a", "Cat1"));

    std::vector<std::tuple<int, std::string, std::string>> meals;
    EXPECT_TRUE(db->getAllMeals(meals));
    ASSERT_EQ(meals.size(), 2u);
    EXPECT_EQ(std::get<1>(meals[0]), "meal-a");  // ordered ASC
    EXPECT_EQ(std::get<1>(meals[1]), "meal-b");
}

// getAllMeals on empty DB returns empty vector
TEST_F(DBManagerTest, GetAllMealsEmptyDB) {
    std::vector<std::tuple<int, std::string, std::string>> meals;
    EXPECT_TRUE(db->getAllMeals(meals));
    EXPECT_TRUE(meals.empty());
}

// addIngredient and getAllIngredients round-trip
TEST_F(DBManagerTest, AddAndGetIngredients) {
    EXPECT_TRUE(db->addIngredient("Spinach", "Vegetables"));
    EXPECT_TRUE(db->addIngredient("Chicken Breast", "Proteins"));

    std::vector<std::pair<std::string, std::string>> ingredients;
    EXPECT_TRUE(db->getAllIngredients(ingredients));
    ASSERT_EQ(ingredients.size(), 2u);

    bool hasSpinach = false, hasChicken = false;
    for (const auto &p : ingredients) {
        if (p.first == "Spinach" && p.second == "Vegetables") hasSpinach = true;
        if (p.first == "Chicken Breast" && p.second == "Proteins") hasChicken = true;
    }
    EXPECT_TRUE(hasSpinach);
    EXPECT_TRUE(hasChicken);
}

// getAllIngredients on empty DB returns empty vector
TEST_F(DBManagerTest, GetAllIngredientsEmpty) {
    std::vector<std::pair<std::string, std::string>> ingredients;
    EXPECT_TRUE(db->getAllIngredients(ingredients));
    EXPECT_TRUE(ingredients.empty());
}

// seedDefaultIngredients populates available_ingredients
TEST_F(DBManagerTest, SeedDefaultIngredientsPopulatesTable) {
    EXPECT_TRUE(db->seedDefaultIngredients());

    std::vector<std::pair<std::string, std::string>> ingredients;
    db->getAllIngredients(ingredients);
    EXPECT_GT(ingredients.size(), 0u);
}

// seedDefaultIngredients is idempotent — second call does not add duplicates
TEST_F(DBManagerTest, SeedDefaultIngredientsIsIdempotent) {
    db->seedDefaultIngredients();
    std::vector<std::pair<std::string, std::string>> first;
    db->getAllIngredients(first);

    db->seedDefaultIngredients();
    std::vector<std::pair<std::string, std::string>> second;
    db->getAllIngredients(second);

    EXPECT_EQ(first.size(), second.size());
}

// seedDefaultMeals populates meals table
TEST_F(DBManagerTest, SeedDefaultMealsPopulatesTable) {
    EXPECT_TRUE(db->seedDefaultMeals());

    std::vector<std::tuple<int, std::string, std::string>> meals;
    db->getAllMeals(meals);
    EXPECT_GT(meals.size(), 0u);
}

// seedDefaultMeals is idempotent
TEST_F(DBManagerTest, SeedDefaultMealsIsIdempotent) {
    db->seedDefaultMeals();
    std::vector<std::tuple<int, std::string, std::string>> first;
    db->getAllMeals(first);

    db->seedDefaultMeals();
    std::vector<std::tuple<int, std::string, std::string>> second;
    db->getAllMeals(second);

    EXPECT_EQ(first.size(), second.size());
}

// saveGoogleTokens and getGoogleTokens round-trip (plaintext mode)
TEST_F(DBManagerTest, SaveAndGetGoogleTokensPlaintext) {
    EXPECT_TRUE(db->saveGoogleTokens("access-abc", "refresh-xyz", 9999));

    std::string accessToken, refreshToken;
    int64_t expiryTime;
    EXPECT_TRUE(db->getGoogleTokens(accessToken, refreshToken, expiryTime));
    EXPECT_EQ(accessToken, "access-abc");
    EXPECT_EQ(refreshToken, "refresh-xyz");
    EXPECT_EQ(expiryTime, 9999);
}

// getGoogleTokens returns false when no tokens have been saved
TEST_F(DBManagerTest, GetGoogleTokensNotFound) {
    std::string accessToken, refreshToken;
    int64_t expiryTime;
    EXPECT_FALSE(db->getGoogleTokens(accessToken, refreshToken, expiryTime));
}

// saveGoogleTokens with empty refresh token stores NULL, getGoogleTokens returns ""
TEST_F(DBManagerTest, SaveGoogleTokensEmptyRefresh) {
    EXPECT_TRUE(db->saveGoogleTokens("access-only", "", 12345));

    std::string accessToken, refreshToken;
    int64_t expiryTime;
    EXPECT_TRUE(db->getGoogleTokens(accessToken, refreshToken, expiryTime));
    EXPECT_EQ(accessToken, "access-only");
    EXPECT_EQ(refreshToken, "");
    EXPECT_EQ(expiryTime, 12345);
}

// saveGoogleTokens + getGoogleTokens with encryption enabled (key set)
TEST_F(DBManagerTest, SaveAndGetGoogleTokensEncrypted) {
    setenv("MEAL_PREP_TOKEN_KEY",
           "0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20", 1);

    EXPECT_TRUE(db->saveGoogleTokens("secret-access", "secret-refresh", 42));

    std::string accessToken, refreshToken;
    int64_t expiryTime;
    EXPECT_TRUE(db->getGoogleTokens(accessToken, refreshToken, expiryTime));
    EXPECT_EQ(accessToken, "secret-access");
    EXPECT_EQ(refreshToken, "secret-refresh");
    EXPECT_EQ(expiryTime, 42);
}

// saveGoogleTokens replaces existing tokens on second call
TEST_F(DBManagerTest, SaveGoogleTokensOverwritesPrevious) {
    db->saveGoogleTokens("old-access", "old-refresh", 100);
    db->saveGoogleTokens("new-access", "new-refresh", 200);

    std::string accessToken, refreshToken;
    int64_t expiryTime;
    db->getGoogleTokens(accessToken, refreshToken, expiryTime);
    EXPECT_EQ(accessToken, "new-access");
    EXPECT_EQ(expiryTime, 200);
}

// addMeal with multiple ingredient types and preparation strings
TEST_F(DBManagerTest, AddMealWithVariedIngredients) {
    std::vector<Ingredient> ings = {
        Ingredient("Chicken", Measurement(2.0, MeasurementUnit::WHOLE), "Strips"),
        Ingredient("Salt", Measurement(1.0, MeasurementUnit::TEASPOON)),
        Ingredient("Garlic", Measurement(3.0, MeasurementUnit::CLOVE), "Minced"),
        Ingredient("Oil", Measurement(2.0, MeasurementUnit::TABLESPOON)),
    };
    Meal meal("varied-meal", ings, "Poultry");
    EXPECT_TRUE(db->addMeal(meal));

    auto retrieved = db->getMeal("varied-meal");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getIngredients().size(), 4u);
}

// Duplicate meal name is rejected (UNIQUE constraint on meals.name)
TEST_F(DBManagerTest, AddDuplicateMealFails) {
    EXPECT_TRUE(db->addMeal(makeMeal("unique-meal")));
    EXPECT_FALSE(db->addMeal(makeMeal("unique-meal")));
}
