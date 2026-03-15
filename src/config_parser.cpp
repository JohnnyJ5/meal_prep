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
  const char* envDbPath = std::getenv("DB_PATH");
  if (envDbPath) {
    config.db_path = envDbPath;
  }

  // Set default port
  config.port = 8080;
  
  // Cloud Run sets the PORT environment variable
  const char* envPort = std::getenv("PORT");
  if (envPort) {
    try {
      config.port = std::stoi(envPort);
    } catch (...) {
      std::cerr << "Warning: Invalid PORT environment variable: " << envPort << ". Using default 8080.\n";
    }
  }

  std::ifstream configFile(configFilePath);
  if (!configFile.is_open()) {
    std::cerr << "Warning: Could not open main config file at "
              << configFilePath << ". Using defaults.\n";
    return config;
  }

  std::stringstream buffer;
  buffer << configFile.rdbuf();
  std::string jsonString = buffer.str();

  auto jsonBody = crow::json::load(jsonString);
  if (!jsonBody) {
    std::cerr << "Warning: Failed to parse JSON in " << configFilePath
              << ". Using defaults.\n";
    return config;
  }

  if (jsonBody.has("port")) {
    config.port = jsonBody["port"].i();
  }

  if (jsonBody.has("email_recipients")) {
    for (const auto &email : jsonBody["email_recipients"]) {
      config.email_recipients.push_back(email.s());
    }
  }

  if (jsonBody.has("credentials_file")) {
    std::string credPath = jsonBody["credentials_file"].s();
    
    // Check if the credentials file exists
    std::ifstream credFile(credPath);
    if (!credFile.is_open()) {
      if (!config.email_recipients.empty()) {
        std::cerr << "Warning: Could not open credentials file at " << credPath
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
        }
        if (credJson.has("password")) {
          config.sender_password = credJson["password"].s();
        }
      } else {
        std::cerr << "Warning: Failed to parse JSON in " << credPath << "\n";
      }
    } else if (!config.email_recipients.empty()) {
      std::cerr << "Warning: Could not open credentials file at " << credPath << "\n";
    }
  }

  return config;
}
