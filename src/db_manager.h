#ifndef DB_MANAGER_H
#define DB_MANAGER_H

#include "meal.h"
#include <memory>
#include <sqlite3.h>
#include <string>
#include <vector>

/**
 * @brief Manages SQLite database operations for the Meal Prep application.
 *
 * This class handles schema initialization, seeding default data, and basic
 * CRUD operations for meals and their associated ingredients.
 */
class DBManager {
public:
  DBManager(const std::string &dbPath);
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
  bool getAllIngredients(
      std::vector<std::pair<std::string, std::string>> &ingredients);

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
   * @brief Retrieves a list of all available meal names and categories from the
   * database.
   * @param meals Vector to populate with pairs of meal names and categories.
   * @return true if successful, false otherwise.
   */
  bool getAllMeals(std::vector<std::pair<std::string, std::string>> &meals);

  // Provide raw connection if occasionally needed (e.g. testing)
  sqlite3 *getConnection() const { return d_db; }

private:
  sqlite3 *d_db;
  std::string d_dbPath;

  bool executeQuery(const std::string &query);
  int getMealId(const std::string &mealName);
};

#endif // DB_MANAGER_H
