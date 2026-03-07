#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <string>
#include <vector>

struct Config {
  int port;
  std::vector<std::string> email_recipients;
  std::string sender_email;
  std::string sender_password;
};

// Loads configuration from the specified file path.
// It will parse the main config file, then read the credentials file
// it points to in order to populate the secret fields.
Config loadConfig(const std::string &configFilePath);

#endif // CONFIG_PARSER_H
