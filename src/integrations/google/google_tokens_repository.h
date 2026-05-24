#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "core/db/db_connection.h"

/// SQLite-backed storage for Google OAuth2 tokens. Tokens are encrypted at
/// rest via TokenEncryption.
class GoogleTokensRepository {
   public:
    explicit GoogleTokensRepository(std::shared_ptr<DbConnection> conn);

    bool saveTokens(const std::string& accessToken, const std::string& refreshToken,
                    int64_t expiryTime);

    bool getTokens(std::string& accessToken, std::string& refreshToken, int64_t& expiryTime);

   private:
    std::shared_ptr<DbConnection> d_conn;
};
