#include "config_parser.h"
#include <crow.h>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

Config loadConfig(const std::string &configFilePath) {
  Config config;
  // DB Path configuration
  config.db_path = "meals.db";
  const char *envDbPath = std::getenv("DB_PATH");
  if (envDbPath) {
    config.db_path = envDbPath;
  }

  // Set default port
  config.port = 8080;

  std::ifstream configFile(configFilePath);
  if (!configFile.is_open()) {
    std::cerr << "Warning: Could not open main config file at "
              << configFilePath << ". Using defaults.\n";
  } else {
    std::stringstream buffer;
    buffer << configFile.rdbuf();
    std::string jsonString = buffer.str();

    auto jsonBody = crow::json::load(jsonString);
    if (!jsonBody) {
      std::cerr << "Warning: Failed to parse JSON in " << configFilePath
                << ". Using defaults.\n";
    } else {
      if (jsonBody.has("port")) {
        config.port = jsonBody["port"].i();
      } else {
        std::cerr << "Notice: 'port' not found in config. Using default: "
                  << config.port << "\n";
      }

      if (jsonBody.has("email_recipients")) {
        for (const auto &email : jsonBody["email_recipients"]) {
          config.email_recipients.push_back(email.s());
        }
      } else {
        std::cerr << "Notice: 'email_recipients' not found in config. No "
                     "emails will be sent.\n";
      }

      if (jsonBody.has("email_credentials_file")) {
        std::string credPath = jsonBody["email_credentials_file"].s();

        // Check if the credentials file exists
        std::ifstream credFile(credPath);
        if (!credFile.is_open()) {
          if (!config.email_recipients.empty()) {
            std::cerr
                << "Warning: Could not open email credentials file at "
                << credPath
                << ". Checking /secrets/credentials.json as fallback...\n";
          }

          // Fallback for Secret Manager mount
          credPath = "/secrets/credentials.json";
          credFile.open(credPath);
        }

        if (credFile.is_open()) {
          std::stringstream credBuffer;
          credBuffer << credFile.rdbuf();
          auto credJson = crow::json::load(credBuffer.str());

          if (credJson) {
            if (credJson.has("email")) {
              config.sender_email = credJson["email"].s();
            } else {
              std::cerr << "Warning: 'email' field missing in credentials file "
                        << credPath << "\n";
            }
            if (credJson.has("password")) {
              config.sender_password = credJson["password"].s();
            } else {
              std::cerr
                  << "Warning: 'password' field missing in credentials file "
                  << credPath << "\n";
            }
          } else {
            std::cerr << "Warning: Failed to parse JSON in " << credPath
                      << "\n";
          }
        }
      } else {
        std::cerr << "Notice: 'email_credentials_file' not found in config.\n";
      }

      if (jsonBody.has("gmail_calendar_credentials_file")) {
        std::string credPath = jsonBody["gmail_calendar_credentials_file"].s();
        std::ifstream credFile(credPath);
        
        if (!credFile.is_open()) {
          std::cerr << "Warning: Could not open calendar credentials file at "
                    << credPath << ". Checking /secrets_calendar/calendar_credentials.json as fallback...\n";
          credPath = "/secrets_calendar/calendar_credentials.json";
          credFile.open(credPath);
        }

        if (credFile.is_open()) {
          std::stringstream credBuffer;
          credBuffer << credFile.rdbuf();
          auto credJson = crow::json::load(credBuffer.str());

          if (credJson) {
            auto oauthLayer = credJson;
            if (credJson.has("web")) {
              oauthLayer = credJson["web"];
            } else if (credJson.has("installed")) {
              oauthLayer = credJson["installed"];
            }

            if (oauthLayer.has("client_id")) {
              config.google_client_id = oauthLayer["client_id"].s();
            } else {
              std::cerr << "Warning: 'client_id' field missing in calendar "
                           "credentials file "
                        << credPath << "\n";
            }
            if (oauthLayer.has("client_secret")) {
              config.google_client_secret = oauthLayer["client_secret"].s();
            } else {
              std::cerr << "Warning: 'client_secret' field missing in calendar "
                           "credentials file "
                        << credPath << "\n";
            }

            // Handle redirect_uris (array) or redirect_uri (string)
            if (oauthLayer.has("redirect_uris")) {
              auto uris = oauthLayer["redirect_uris"];
              if (uris.size() > 0) {
                config.google_redirect_uri = uris[0].s();
              } else {
                std::cerr << "Warning: 'redirect_uris' array is empty in "
                          << credPath << "\n";
              }
            } else if (oauthLayer.has("redirect_uri")) {
              config.google_redirect_uri = oauthLayer["redirect_uri"].s();
            } else {
              std::cerr << "Warning: Neither 'redirect_uri' nor "
                           "'redirect_uris' found in "
                        << credPath << "\n";
            }
          } else {
            std::cerr << "Warning: Failed to parse JSON in " << credPath
                      << "\n";
          }
        } else {
          std::cerr << "Warning: Could not open calendar credentials file at "
                    << credPath << "\n";
        }
      } else {
        std::cerr << "Notice: 'gmail_calendar_credentials_file' not found in "
                     "config. Google Calendar integration may not work.\n";
      }

      if (jsonBody.has("google_redirect_uri")) {
        config.google_redirect_uri = jsonBody["google_redirect_uri"].s();
      }

      if (config.google_redirect_uri.empty()) {
        config.google_redirect_uri =
            "http://localhost:8080/auth/google/callback";
      }
    }
  }

  // Cloud Run sets the PORT environment variable - this MUST take precedence
  const char *envPort = std::getenv("PORT");
  if (envPort) {
    try {
      config.port = std::stoi(envPort);
    } catch (...) {
      std::cerr << "Warning: Invalid PORT environment variable: " << envPort
                << ". Using config/default value: " << config.port << ".\n";
    }
  }

  // Allow environment variable override for redirect URI
  const char *envRedirectUri = std::getenv("GOOGLE_REDIRECT_URI");
  if (envRedirectUri) {
    config.google_redirect_uri = envRedirectUri;
  }

  return config;
}
