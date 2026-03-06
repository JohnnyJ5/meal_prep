#ifndef DB_MANAGER_H
#define DB_MANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <sqlite3.h>
#include "meal.h"

class DBManager {
public:
    DBManager(const std::string& dbPath);
    ~DBManager();

    // Disable copy/move for simplicity
    DBManager(const DBManager&) = delete;
    DBManager& operator=(const DBManager&) = delete;

    bool initializeSchema();
    bool seedDefaultMeals();

    // CRUD
    bool addMeal(const Meal& meal);
    bool updateMeal(const Meal& meal);
    bool deleteMeal(const std::string& mealName);
    std::unique_ptr<Meal> getMeal(const std::string& mealName);
    bool getAllMeals(std::vector<std::string>& meals);

    // Provide raw connection if occasionally needed (e.g. testing)
    sqlite3* getConnection() const { return d_db; }

private:
    sqlite3* d_db;
    std::string d_dbPath;

    bool executeQuery(const std::string& query);
    int getMealId(const std::string& mealName);
};

#endif // DB_MANAGER_H
