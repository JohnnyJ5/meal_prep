#pragma once

#include <sqlite3.h>

#include <mutex>
#include <string>

/**
 * @brief RAII wrapper around a sqlite3 connection.
 *
 * Owns the connection handle, exposes a small mutex so repositories sharing
 * the connection can serialize multi-statement transactions, and provides a
 * generic `executeQuery` helper for one-off statements.
 */
class DbConnection {
   public:
    explicit DbConnection(const std::string& dbPath);
    ~DbConnection();

    DbConnection(const DbConnection&) = delete;
    DbConnection& operator=(const DbConnection&) = delete;

    bool isOpen() const { return d_db != nullptr; }
    sqlite3* raw() const { return d_db; }
    std::recursive_mutex& mutex() { return d_mutex; }

    /// Convenience wrapper around sqlite3_exec; logs failures to stderr.
    bool executeQuery(const std::string& query);

   private:
    sqlite3* d_db{nullptr};
    std::recursive_mutex d_mutex;
};
