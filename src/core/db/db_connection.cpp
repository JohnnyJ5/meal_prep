#include "core/db/db_connection.h"

#include <iostream>

DbConnection::DbConnection(const std::string& dbPath) {
    if (sqlite3_open(dbPath.c_str(), &d_db) != SQLITE_OK) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(d_db) << "\n";
        d_db = nullptr;
    }
}

DbConnection::~DbConnection() {
    if (d_db) {
        sqlite3_close(d_db);
    }
}

bool DbConnection::executeQuery(const std::string& query) {
    if (!d_db) {
        std::cerr << "Database connection is not initialized.\n";
        return false;
    }
    char* errMsg = nullptr;
    if (sqlite3_exec(d_db, query.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << "\n";
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}
