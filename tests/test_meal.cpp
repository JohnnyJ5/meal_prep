#include <gtest/gtest.h>
#include "../meal.h"
#include "../ingredient_names.h"

class MealTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test Meal base class
TEST_F(MealTest, MealConstructorAndGetters) {
    std::vector<Ingredient> ingredients = {
        Ingredient(IngredientNames::SPINACH, Measurement(2.0, MeasurementUnit::CUP)),
        Ingredient(IngredientNames::SALT, Measurement(1.0, MeasurementUnit::TEASPOON))
    };
    
    Meal meal("Test Meal", ingredients);
    
    EXPECT_EQ(meal.getName(), "Test Meal");
    EXPECT_EQ(meal.getIngredients().size(), 2);
    EXPECT_EQ(meal.getIngredients()[0].getName(), IngredientNames::SPINACH);
    EXPECT_EQ(meal.getIngredients()[1].getName(), IngredientNames::SALT);
}

// Test TurkeyBurgers meal
TEST_F(MealTest, TurkeyBurgersMeal) {
    TurkeyBurgers meal;
    
    EXPECT_EQ(meal.getName(), "Turkey Burgers");
    const auto& ingredients = meal.getIngredients();
    
    // Check that it has ingredients
    EXPECT_GT(ingredients.size(), 0);
    
    // Check for specific ingredients
    bool hasSpinach = false;
    bool hasGroundTurkey = false;
    for (const auto& ing : ingredients) {
        if (ing.getName() == IngredientNames::SPINACH) {
            hasSpinach = true;
        }
        if (ing.getName() == IngredientNames::GROUND_TURKEY) {
            hasGroundTurkey = true;
        }
    }
    
    EXPECT_TRUE(hasSpinach);
    EXPECT_TRUE(hasGroundTurkey);
}

// Test TurkeyMeatballs meal
TEST_F(MealTest, TurkeyMeatballsMeal) {
    TurkeyMeatballs meal;
    
    EXPECT_EQ(meal.getName(), "Turkey Meatballs");
    const auto& ingredients = meal.getIngredients();
    
    EXPECT_GT(ingredients.size(), 0);
    
    bool hasGroundTurkey = false;
    bool hasBreadcrumbs = false;
    for (const auto& ing : ingredients) {
        if (ing.getName() == IngredientNames::GROUND_TURKEY) {
            hasGroundTurkey = true;
        }
        if (ing.getName() == IngredientNames::BREADCRUMBS) {
            hasBreadcrumbs = true;
        }
    }
    
    EXPECT_TRUE(hasGroundTurkey);
    EXPECT_TRUE(hasBreadcrumbs);
}

// Test CreamyGarlicChickenPenneSpinach meal
TEST_F(MealTest, CreamyGarlicChickenPenneSpinachMeal) {
    CreamyGarlicChickenPenneSpinach meal;
    
    EXPECT_EQ(meal.getName(), "Creamy Garlic Chicken Penne Spinach");
    const auto& ingredients = meal.getIngredients();
    
    EXPECT_GT(ingredients.size(), 0);
    
    bool hasChicken = false;
    bool hasPenne = false;
    for (const auto& ing : ingredients) {
        if (ing.getName() == IngredientNames::CHICKEN_BREAST) {
            hasChicken = true;
        }
        if (ing.getName() == IngredientNames::PENNE_PASTA) {
            hasPenne = true;
        }
    }
    
    EXPECT_TRUE(hasChicken);
    EXPECT_TRUE(hasPenne);
}

// Test CreamyGarlicChicken meal
TEST_F(MealTest, CreamyGarlicChickenMeal) {
    CreamyGarlicChicken meal;
    
    EXPECT_EQ(meal.getName(), "Creamy Garlic Chicken");
    const auto& ingredients = meal.getIngredients();
    
    EXPECT_GT(ingredients.size(), 0);
    
    bool hasChicken = false;
    bool hasHeavyCream = false;
    for (const auto& ing : ingredients) {
        if (ing.getName() == IngredientNames::CHICKEN_BREAST) {
            hasChicken = true;
        }
        if (ing.getName() == IngredientNames::HEAVY_CREAM) {
            hasHeavyCream = true;
        }
    }
    
    EXPECT_TRUE(hasChicken);
    EXPECT_TRUE(hasHeavyCream);
}

// Test BakedChickenBreast meal
TEST_F(MealTest, BakedChickenBreastMeal) {
    BakedChickenBreast meal;
    
    EXPECT_EQ(meal.getName(), "Baked Chicken Breast");
    const auto& ingredients = meal.getIngredients();
    
    EXPECT_GT(ingredients.size(), 0);
    
    bool hasChicken = false;
    for (const auto& ing : ingredients) {
        if (ing.getName() == IngredientNames::CHICKEN_BREAST) {
            hasChicken = true;
            break;
        }
    }
    
    EXPECT_TRUE(hasChicken);
}

// Test CheesyHamburgerPastaSkillet meal
TEST_F(MealTest, CheesyHamburgerPastaSkilletMeal) {
    CheesyHamburgerPastaSkillet meal;
    
    EXPECT_EQ(meal.getName(), "Cheesy Hamburger Pasta Skillet");
    const auto& ingredients = meal.getIngredients();
    
    EXPECT_GT(ingredients.size(), 0);
    
    bool hasGroundBeef = false;
    bool hasPasta = false;
    for (const auto& ing : ingredients) {
        if (ing.getName() == IngredientNames::GROUND_BEEF) {
            hasGroundBeef = true;
        }
        if (ing.getName() == IngredientNames::PASTA) {
            hasPasta = true;
        }
    }
    
    EXPECT_TRUE(hasGroundBeef);
    EXPECT_TRUE(hasPasta);
}

// Test CottageCheesePancakes meal
TEST_F(MealTest, CottageCheesePancakesMeal) {
    CottageCheesePancakes meal;
    
    EXPECT_EQ(meal.getName(), "Cottage Cheese Pancakes");
    const auto& ingredients = meal.getIngredients();
    
    EXPECT_GT(ingredients.size(), 0);
    
    bool hasCottageCheese = false;
    bool hasEggs = false;
    for (const auto& ing : ingredients) {
        if (ing.getName() == IngredientNames::COTTAGE_CHEESE) {
            hasCottageCheese = true;
        }
        if (ing.getName() == IngredientNames::EGGS) {
            hasEggs = true;
        }
    }
    
    EXPECT_TRUE(hasCottageCheese);
    EXPECT_TRUE(hasEggs);
}

// Test ChickenStirFry meal
TEST_F(MealTest, ChickenStirFryMeal) {
    ChickenStirFry meal;
    
    EXPECT_EQ(meal.getName(), "Chicken Stir Fry");
    const auto& ingredients = meal.getIngredients();
    
    EXPECT_GT(ingredients.size(), 0);
    
    bool hasChicken = false;
    bool hasBroccoli = false;
    for (const auto& ing : ingredients) {
        if (ing.getName() == IngredientNames::CHICKEN_BREAST) {
            hasChicken = true;
        }
        if (ing.getName() == IngredientNames::BROCCOLI) {
            hasBroccoli = true;
        }
    }
    
    EXPECT_TRUE(hasChicken);
    EXPECT_TRUE(hasBroccoli);
}

// Test that ingredients are immutable through getter
TEST_F(MealTest, IngredientsImmutability) {
    TurkeyBurgers meal;
    const auto& ingredients = meal.getIngredients();
    
    // Should be able to read but not modify (const reference)
    EXPECT_FALSE(ingredients.empty());
    
    // Verify we can access ingredients
    for (const auto& ing : ingredients) {
        EXPECT_FALSE(ing.getName().empty());
    }
}
