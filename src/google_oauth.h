#ifndef GOOGLE_OAUTH_H
#define GOOGLE_OAUTH_H

#include "config_parser.h"
#include "db_manager.h"
#include <memory>
#include <string>

/**
 * @brief Handles Google OAuth2 Authorization Code Flow.
 */
class GoogleOAuth {
public:
  GoogleOAuth(const Config &config, std::shared_ptr<DBManager> dbManager);

  /**
   * @brief Generates the URL for the user to visit to authorize the application.
   * @return The authorization URL.
   */
  std::string getAuthUrl() const;

  /**
   * @brief Exchanges an authorization code for access and refresh tokens.
   * @param code The authorization code received from Google.
   * @return true if successful, false otherwise.
   */
  bool exchangeCodeForTokens(const std::string &code);

  /**
   * @brief Refreshes the access token using the stored refresh token.
   * @return true if successful, false otherwise.
   */
  bool refreshAccessToken();

  /**
   * @brief Gets a valid access token, refreshing it if necessary.
   * @return The access token, or an empty string if not available/refreshable.
   */
  std::string getAccessToken();

private:
  Config d_config;
  std::shared_ptr<DBManager> d_dbManager;

  struct TokenResponse {
    std::string access_token;
    std::string refresh_token;
    int expires_in;
    bool success;
  };

  TokenResponse makeTokenRequest(const std::string &postData);
};

#endif // GOOGLE_OAUTH_H
