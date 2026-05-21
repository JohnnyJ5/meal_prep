#pragma once

#include <sqlite3.h>

#include <cstdint>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "meal.h"
#include "workout.h"

/**
 * @brief Manages SQLite database operations for the Meal Prep application.
 *
 * This class handles schema initialization, seeding default data, and basic
 * CRUD operations for meals and their associated ingredients.
 */
class DBManager {
   public:
    explicit DBManager(const std::string &dbPath);
    ~DBManager();

    // Disable copy/move for simplicity
    DBManager(const DBManager &) = delete;
    DBManager &operator=(const DBManager &) = delete;

    /**
     * @brief Initializes the database schema if it does not already exist.
     * @return true if successful, false otherwise.
     */
    bool initializeSchema();

    /**
     * @brief Seeds the database with default meals if empty.
     * @return true if successful, false otherwise.
     */
    bool seedDefaultMeals();

    /**
     * @brief Seeds the database with default available ingredients if empty.
     * @return true if successful, false otherwise.
     */
    bool seedDefaultIngredients();

    // CRUD Operations

    /**
     * @brief Adds a new available ingredient to the database.
     * @param name The name of the ingredient.
     * @param category The category of the ingredient.
     * @return true if added successfully, false otherwise.
     */
    bool addIngredient(const std::string &name, const std::string &category);

    /**
     * @brief Retrieves a list of all available ingredients from the database.
     * @param ingredients Vector to populate with pairs of ingredient names and
     * categories.
     * @return true if successful, false otherwise.
     */
    bool getAllIngredients(std::vector<std::pair<std::string, std::string>> &ingredients);

    /**
     * @brief Adds a new meal to the database.
     * @param meal The meal object to add.
     * @return true if added successfully, false otherwise.
     */
    bool addMeal(const Meal &meal);

    /**
     * @brief Updates an existing meal's ingredients.
     * @param meal The meal object containing updated ingredients.
     * @return true if updated successfully, false otherwise.
     */
    bool updateMeal(const Meal &meal);

    /**
     * @brief Deletes a meal from the database.
     * @param mealName The name of the meal to delete.
     * @return true if deleted successfully, false otherwise.
     */
    bool deleteMeal(const std::string &mealName);

    /**
     * @brief Retrieves a meal from the database by name.
     * @param mealName The name of the meal to retrieve.
     * @return unique_ptr to the Meal, or nullptr if not found.
     */
    std::unique_ptr<Meal> getMeal(const std::string &mealName);

    /**
     * @brief Retrieves a list of all available meal ids, names, and categories
     * from the database.
     * @param meals Vector to populate with tuples of (id, name, category).
     * @return true if successful, false otherwise.
     */
    bool getAllMeals(std::vector<std::tuple<int, std::string, std::string>> &meals);

    /**
     * @brief Returns the set of meal IDs that have at least one optional
     * ingredient. Used so the UI can show an add-on picker only for meals
     * that actually have selectable add-ons.
     */
    std::set<int> getMealIdsWithOptionalIngredients();

    /**
     * @brief Saves Google OAuth2 tokens to the database.
     * @param accessToken The access token.
     * @param refreshToken The refresh token.
     * @param expiryTime The timestamp when the access token expires.
     * @return true if successful, false otherwise.
     */
    bool saveGoogleTokens(const std::string &accessToken, const std::string &refreshToken,
                          int64_t expiryTime);

    /**
     * @brief Retrieves Google OAuth2 tokens from the database.
     * @param accessToken String to populate with the access token.
     * @param refreshToken String to populate with the refresh token.
     * @param expiryTime Pointer to populate with the expiry timestamp.
     * @return true if tokens were found, false otherwise.
     */
    bool getGoogleTokens(std::string &accessToken, std::string &refreshToken, int64_t &expiryTime);

    // --- Workouts ---

    /**
     * @brief Inserts a workout (with its blocks and exercises) atomically.
     * @param workout Input/output. On success, workout.id is set to the new row id.
     * @return true on success, false on failure (transaction rolled back).
     */
    bool addWorkout(Workout &workout);

    /**
     * @brief Replaces an existing workout. Blocks/exercises are wiped and re-inserted.
     * @return true on success, false on failure (transaction rolled back).
     */
    bool updateWorkout(const Workout &workout);

    /**
     * @brief Deletes a workout by id. Cascades to blocks and exercises.
     */
    bool deleteWorkout(int id);

    /**
     * @brief Loads a single workout including its blocks and exercises.
     * @return populated Workout with id != 0 on success; id == 0 if not found.
     */
    Workout getWorkout(int id);

    /**
     * @brief Lists all workouts, most recent first.
     */
    std::vector<WorkoutSummary> listWorkouts();

   private:
    sqlite3 *d_db{nullptr};
    std::string d_dbPath;
    mutable std::recursive_mutex d_mutex;

    bool executeQuery(const std::string &query);
    int getMealId(const std::string &mealName);
};
