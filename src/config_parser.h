#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <string>
#include <vector>

/**
 * @brief Application configuration data.
 */
struct Config {
  int port;
  std::vector<std::string> email_recipients;
  std::string sender_email;
  std::string sender_password;
  std::string db_path;
};

/**
 * @brief Loads configuration from the specified file path.
 *
 * It will parse the main config file, then read the credentials file
 * it points to in order to populate the secret fields.
 *
 * @param configFilePath The path to the configuration JSON file.
 * @return Config The parsed configuration object.
 */
Config loadConfig(const std::string &configFilePath);

#endif // CONFIG_PARSER_H
