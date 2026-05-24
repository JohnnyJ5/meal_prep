#include "integrations/google/google_tokens_repository.h"

#include <sqlite3.h>

#include <utility>

#include "integrations/google/token_encryption.h"

GoogleTokensRepository::GoogleTokensRepository(std::shared_ptr<DbConnection> conn)
    : d_conn(std::move(conn)) {}

bool GoogleTokensRepository::saveTokens(const std::string& accessToken,
                                        const std::string& refreshToken, int64_t expiryTime) {
    std::lock_guard<std::recursive_mutex> lock(d_conn->mutex());
    sqlite3* db = d_conn->raw();
    if (!db) return false;
    const std::string query =
        "INSERT OR REPLACE INTO google_tokens (id, access_token, refresh_token, expiry_time) "
        "VALUES (1, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return false;
    std::string encAccessToken = TokenEncryption::encrypt(accessToken);
    std::string encRefreshToken =
        refreshToken.empty() ? "" : TokenEncryption::encrypt(refreshToken);
    sqlite3_bind_text(stmt, 1, encAccessToken.c_str(), -1, SQLITE_TRANSIENT);
    if (encRefreshToken.empty()) {
        sqlite3_bind_null(stmt, 2);
    } else {
        sqlite3_bind_text(stmt, 2, encRefreshToken.c_str(), -1, SQLITE_TRANSIENT);
    }
    sqlite3_bind_int64(stmt, 3, expiryTime);
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

bool GoogleTokensRepository::getTokens(std::string& accessToken, std::string& refreshToken,
                                       int64_t& expiryTime) {
    std::lock_guard<std::recursive_mutex> lock(d_conn->mutex());
    sqlite3* db = d_conn->raw();
    if (!db) return false;
    const std::string query =
        "SELECT access_token, refresh_token, expiry_time FROM google_tokens WHERE id = 1;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return false;
    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* access = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (access) {
            accessToken = TokenEncryption::decrypt(access);
        }
        const char* refresh = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        if (refresh) {
            refreshToken = TokenEncryption::decrypt(refresh);
        } else {
            refreshToken = "";
        }
        expiryTime = sqlite3_column_int64(stmt, 2);
        found = true;
    }
    sqlite3_finalize(stmt);
    return found;
}
