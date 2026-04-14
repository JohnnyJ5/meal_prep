#include "config_parser.h"

#include <crow.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

static const std::string kCloudRunCredPath = "/secrets_calendar/calendar_credentials.json";

// Attempts to load Google OAuth credentials from a JSON file.
// Returns true if the file was opened and parsed successfully.
static bool loadCredentialsFile(const std::string &credPath, Config &config) {
    std::ifstream credFile(credPath);
    if (!credFile.is_open()) {
        return false;
    }

    std::stringstream credBuffer;
    credBuffer << credFile.rdbuf();
    auto credJson = crow::json::load(credBuffer.str());

    if (!credJson) {
        std::cerr << "Warning: Failed to parse JSON in " << credPath << "\n";
        return false;
    }

    auto oauthLayer = credJson;
    if (credJson.has("web")) {
        oauthLayer = credJson["web"];
    } else if (credJson.has("installed")) {
        oauthLayer = credJson["installed"];
    }

    if (oauthLayer.has("client_id")) {
        config.google_client_id = oauthLayer["client_id"].s();
    } else {
        std::cerr << "Warning: 'client_id' field missing in " << credPath << "\n";
    }

    if (oauthLayer.has("client_secret")) {
        config.google_client_secret = oauthLayer["client_secret"].s();
    } else {
        std::cerr << "Warning: 'client_secret' field missing in " << credPath << "\n";
    }

    if (oauthLayer.has("redirect_uris")) {
        const auto &uris = oauthLayer["redirect_uris"];
        if (uris.size() > 0) {
            config.google_redirect_uri = uris[0].s();
        } else {
            std::cerr << "Warning: 'redirect_uris' array is empty in " << credPath << "\n";
        }
    } else if (oauthLayer.has("redirect_uri")) {
        config.google_redirect_uri = oauthLayer["redirect_uri"].s();
    }

    return true;
}

Config loadConfig(const std::string &configFilePath) {
    Config config;

    config.db_path = "meals.db";
    const char *envDbPath = std::getenv("DB_PATH");
    if (envDbPath) {
        config.db_path = envDbPath;
    }

    config.port = 8080;

    std::ifstream configFile(configFilePath);
    if (!configFile.is_open()) {
        std::cerr << "Warning: Could not open main config file at " << configFilePath
                  << ". Using defaults.\n";
    } else {
        std::stringstream buffer;
        buffer << configFile.rdbuf();
        auto jsonBody = crow::json::load(buffer.str());

        if (!jsonBody) {
            std::cerr << "Warning: Failed to parse JSON in " << configFilePath
                      << ". Using defaults.\n";
        } else {
            if (jsonBody.has("port")) {
                config.port = static_cast<int>(jsonBody["port"].i());
            } else {
                std::cerr << "Notice: 'port' not found in config. Using default: " << config.port
                          << "\n";
            }

            if (jsonBody.has("gmail_calendar_credentials_file")) {
                std::string credPath = jsonBody["gmail_calendar_credentials_file"].s();
                if (!loadCredentialsFile(credPath, config)) {
                    std::cerr << "Warning: Could not open calendar credentials file at " << credPath
                              << ". Trying " << kCloudRunCredPath << " as fallback...\n";
                    loadCredentialsFile(kCloudRunCredPath, config);
                }
            } else {
                std::cerr << "Notice: 'gmail_calendar_credentials_file' not found in config."
                             " Trying "
                          << kCloudRunCredPath << " as fallback...\n";
                loadCredentialsFile(kCloudRunCredPath, config);
            }

            if (jsonBody.has("google_redirect_uri")) {
                config.google_redirect_uri = jsonBody["google_redirect_uri"].s();
            }
        }
    }

    // If the config file was missing or had no credentials, try the Cloud Run secret path.
    if (config.google_client_id.empty()) {
        std::cerr << "Notice: Google credentials not loaded from config. Trying "
                  << kCloudRunCredPath << "...\n";
        loadCredentialsFile(kCloudRunCredPath, config);
    }

    if (config.google_redirect_uri.empty()) {
        config.google_redirect_uri = "http://localhost:8080/auth/google/callback";
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

    std::cout << "Config loaded:\n"
              << "  port:                " << config.port << "\n"
              << "  db_path:             " << config.db_path << "\n"
              << "  google_client_id:    "
              << (config.google_client_id.empty() ? "(not set)" : config.google_client_id) << "\n"
              << "  google_client_secret:"
              << (config.google_client_secret.empty() ? " (not set)" : " (set)") << "\n"
              << "  google_redirect_uri: "
              << (config.google_redirect_uri.empty() ? "(not set)" : config.google_redirect_uri)
              << "\n";

    return config;
}
