#include "config_parser.h"
#include <crow.h>
#include <fstream>
#include <iostream>
#include <sstream>

Config loadConfig(const std::string &configFilePath) {
  Config config;
  // Set default values in case of failure
  config.port = 8080;

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
    std::ifstream credFile(credPath);
    if (!credFile.is_open()) {
      std::cerr << "Warning: Could not open credentials file at " << credPath
                << "\n";
      return config;
    }

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
  }

  return config;
}
