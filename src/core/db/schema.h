#pragma once

#include "core/db/db_connection.h"

/**
 * @brief Creates every table the app needs and runs in-place column
 * migrations for older databases (adds meals.category, ingredients.is_optional
 * when missing).
 */
bool initializeSchema(DbConnection& conn);
